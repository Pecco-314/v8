#!/usr/bin/env python3
import argparse
import json
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Any


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def now_iso() -> str:
    return datetime.now().isoformat(timespec="seconds")


def append_jsonl(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as handle:
        handle.write(json.dumps(payload, ensure_ascii=False) + "\n")


def run_cmd(command: list[str], cwd: Path) -> dict[str, Any]:
    started = time.perf_counter()
    proc = subprocess.run(command, capture_output=True, text=True, cwd=str(cwd))
    elapsed = time.perf_counter() - started
    return {
        "command": command,
        "returncode": proc.returncode,
        "duration_sec": elapsed,
        "stdout_tail": "\n".join(proc.stdout.splitlines()[-40:]),
        "stderr_tail": "\n".join(proc.stderr.splitlines()[-40:]),
    }


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def discover_benches(bench_dir: Path) -> list[Path]:
    return sorted([*bench_dir.glob("*.ts"), *bench_dir.glob("*.js")])


def bench_key(path: Path) -> str:
    return path.stem


def main() -> int:
    parser = argparse.ArgumentParser(description="Nightly codegen-only benchmark runner")
    parser.add_argument("--bench-dir", default="scripts/bench/benchmarks", help="Benchmark directory")
    parser.add_argument("--d8", help="Optional d8 path override")
    parser.add_argument("--trace-ir", action="store_true", help="Enable IR tracing")
    parser.add_argument("--compile-coverage", action="store_true", help="Enable multi-target compile coverage collection")
    parser.add_argument(
        "--coverage-all-business-functions",
        action="store_true",
        help="When compile coverage is enabled, auto-cover all business functions with default fn() invocation.",
    )
    parser.add_argument("--no-inline", action="store_true", help="Disable TurboFan inlining during codegen run")
    parser.add_argument("--coverage-warmup-calls", type=int, default=8, help="Warmup calls per coverage target")
    parser.add_argument(
        "--coverage-target",
        action="append",
        default=[],
        help="Additional coverage target in format function=invoke_expr",
    )
    parser.add_argument("--max-benches", type=int, help="Optional max benchmark count")
    parser.add_argument("--results-jsonl", default="tmp/bench/nightly-codegen-results.jsonl", help="Append-only result stream")
    parser.add_argument("--run-log", default="tmp/bench/nightly-codegen.log", help="Human-readable run log")
    args = parser.parse_args()

    root = repo_root()
    bench_dir = (root / args.bench_dir).resolve()
    if not bench_dir.exists():
        print(f"bench dir not found: {bench_dir}")
        return 2

    hotspots_path = root / "scripts" / "bench" / "config" / "hotspots.json"
    hotspots = load_json(hotspots_path) if hotspots_path.exists() else {}

    merge = hotspots.get("merge-sort", {})
    if merge.get("function") != "mergeSort" or merge.get("invoke_expr") != "mergeSort(data.slice())":
        print("hotspots.json 校验失败: merge-sort 必须指向 mergeSort(data.slice())")
        return 2

    benches = discover_benches(bench_dir)
    if args.max_benches is not None:
        benches = benches[: max(0, args.max_benches)]
    if not benches:
        print("no benchmarks found")
        return 2

    results_jsonl = (root / args.results_jsonl).resolve()
    run_log = (root / args.run_log).resolve()
    run_log.parent.mkdir(parents=True, exist_ok=True)
    run_id = time.strftime("%Y%m%d-%H%M%S")

    results_jsonl.parent.mkdir(parents=True, exist_ok=True)
    results_jsonl.write_text("", encoding="utf-8")

    append_jsonl(
        results_jsonl,
        {
            "type": "run_start",
            "run_id": run_id,
            "timestamp": now_iso(),
            "bench_count": len(benches),
            "config": {
                "trace_ir": args.trace_ir,
                "bench_dir": str(bench_dir),
            },
        },
    )

    with run_log.open("a", encoding="utf-8") as log:
        log.write(f"[{now_iso()}] run_start {run_id} benches={len(benches)}\n")

    result_count = 0
    for index, bench in enumerate(benches, start=1):
        bench_stem = bench_key(bench)
        hotspot = hotspots.get(bench_stem, {})
        metadata_mode = "strict"

        append_jsonl(
            results_jsonl,
            {
                "type": "bench_start",
                "run_id": run_id,
                "timestamp": now_iso(),
                "bench": str(bench),
                "index": index,
                "total": len(benches),
            },
        )

        cmd = [
            sys.executable,
            str(root / "scripts" / "bench" / "generate_bench_assembly.py"),
            "--bench",
            str(bench),
            "--metadata-mode",
            metadata_mode,
            "--stats-scope",
            "all-business",
        ]
        if hotspot.get("function"):
            cmd += ["--function", str(hotspot["function"])]
        if hotspot.get("invoke_expr"):
            cmd += ["--invoke-expr", str(hotspot["invoke_expr"])]
        if args.trace_ir:
            cmd.append("--trace-ir")
        if args.compile_coverage:
            cmd.append("--compile-coverage")
            cmd += ["--coverage-warmup-calls", str(args.coverage_warmup_calls)]
            if args.coverage_all_business_functions:
                cmd.append("--coverage-all-business-functions")
            for coverage_target in args.coverage_target:
                cmd += ["--coverage-target", str(coverage_target)]
        if args.no_inline:
            cmd.append("--no-inline")
        if args.d8:
            cmd += ["--d8", args.d8]

        ret = run_cmd(cmd, root)
        comparison_path = root / "docs" / "assembly" / "benchmarks" / bench_stem / "comparison.json"
        metrics: dict[str, Any] = {}
        if comparison_path.exists():
            cmp_json = load_json(comparison_path)
            diff = cmp_json.get("diff", {})
            scope = cmp_json.get("stats_scope", "target")
            optimized = cmp_json.get("optimized_business_functions", {})
            paired = optimized.get("paired", []) if isinstance(optimized, dict) else []
            target_without_size = (cmp_json.get("without_metadata") or {}).get("instruction_size")
            target_with_size = (cmp_json.get("with_metadata") or {}).get("instruction_size")
            aggregate_without_size = None
            aggregate_with_size = None
            if scope == "all-business" and isinstance(paired, list):
                without_sizes = [item.get("without_instruction_size") for item in paired if isinstance(item, dict)]
                with_sizes = [item.get("with_instruction_size") for item in paired if isinstance(item, dict)]
                if without_sizes and all(isinstance(v, int) for v in without_sizes):
                    aggregate_without_size = sum(without_sizes)
                if with_sizes and all(isinstance(v, int) for v in with_sizes):
                    aggregate_with_size = sum(with_sizes)

            coverage = cmp_json.get("compile_coverage", {}) if isinstance(cmp_json.get("compile_coverage"), dict) else {}
            coverage_without = coverage.get("without", {}) if isinstance(coverage.get("without"), dict) else {}
            coverage_with = coverage.get("with", {}) if isinstance(coverage.get("with"), dict) else {}
            metrics = {
                "stats_scope": scope,
                "line_delta": diff.get("line_delta"),
                "instruction_size_delta": diff.get("instruction_size_delta"),
                "changed_lines": diff.get("changed_lines"),
                "without_instruction_size": aggregate_without_size if aggregate_without_size is not None else target_without_size,
                "with_instruction_size": aggregate_with_size if aggregate_with_size is not None else target_with_size,
                "optimized_without_count": optimized.get("without_count"),
                "optimized_with_count": optimized.get("with_count"),
                "optimized_paired_count": optimized.get("paired_count"),
                "coverage_enabled": coverage.get("enabled"),
                "coverage_target_count": len(coverage.get("targets", [])) if isinstance(coverage.get("targets"), list) else None,
                "coverage_without_compiled_target_count": coverage_without.get("compiled_target_count"),
                "coverage_with_compiled_target_count": coverage_with.get("compiled_target_count"),
            }

        append_jsonl(
            results_jsonl,
            {
                "type": "dimension_result",
                "run_id": run_id,
                "timestamp": now_iso(),
                "bench": str(bench),
                "dimension": "binary_codegen",
                "status": "ok" if ret["returncode"] == 0 else "failed",
                "metrics": metrics,
                "report_path": str(comparison_path),
                "exec": {
                    "duration_sec": ret["duration_sec"],
                    "returncode": ret["returncode"],
                    "command": ret["command"],
                    "stdout_tail": ret["stdout_tail"],
                    "stderr_tail": ret["stderr_tail"],
                },
            },
        )
        result_count += 1

        append_jsonl(
            results_jsonl,
            {
                "type": "bench_end",
                "run_id": run_id,
                "timestamp": now_iso(),
                "bench": str(bench),
                "dimension_count": 1,
            },
        )

        with run_log.open("a", encoding="utf-8") as log:
            log.write(
                f"[{now_iso()}] bench_end {bench.name} codegen={'ok' if ret['returncode'] == 0 else 'failed'}\n"
            )

    append_jsonl(
        results_jsonl,
        {
            "type": "run_end",
            "run_id": run_id,
            "timestamp": now_iso(),
            "bench_count": len(benches),
            "dimension_records": result_count,
            "results_jsonl": str(results_jsonl),
            "run_log": str(run_log),
        },
    )

    with run_log.open("a", encoding="utf-8") as log:
        log.write(f"[{now_iso()}] run_end {run_id} dimensions={result_count}\n")

    print(f"Run ID: {run_id}")
    print(f"Results JSONL: {results_jsonl}")
    print(f"Run log: {run_log}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
