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
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "stdout_tail": "\n".join(proc.stdout.splitlines()[-40:]),
        "stderr_tail": "\n".join(proc.stderr.splitlines()[-40:]),
    }


def parse_saved_path(stdout: str) -> Path | None:
    for line in stdout.splitlines():
        if "Saved:" in line:
            value = line.split("Saved:", 1)[1].strip()
            if value:
                return Path(value)
        if "Suite report saved to:" in line:
            value = line.split("Suite report saved to:", 1)[1].strip()
            if value:
                return Path(value)
    return None


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def discover_benches(bench_dir: Path) -> list[Path]:
    return sorted([*bench_dir.glob("*.ts"), *bench_dir.glob("*.js")])


def build_perf_override(bench_name: str, repeats: int) -> dict[str, Any]:
    drop_first = max(1, repeats // 4)
    if drop_first >= repeats:
        drop_first = max(0, repeats - 1)
    return {
        bench_name: {
            "repeats": repeats,
            "drop_first": drop_first,
        }
    }


def bench_key(path: Path) -> str:
    return path.stem


def main() -> int:
    parser = argparse.ArgumentParser(description="Nightly full benchmark runner for 3 dimensions")
    parser.add_argument("--bench-dir", default="scripts/bench/benchmarks", help="Benchmark directory")
    parser.add_argument("--d8", help="Optional d8 path override")
    parser.add_argument("--perf-repeats", type=int, default=30, help="Repeats for performance dimension")
    parser.add_argument("--compile-repeats", type=int, default=30, help="Repeats for compile-latency dimension")
    parser.add_argument("--compile-warmup-pairs", type=int, default=2, help="Warmup pairs for compile-latency")
    parser.add_argument("--compile-bootstrap-iterations", type=int, default=3000, help="Bootstrap iterations")
    parser.add_argument("--trace-ir", action="store_true", help="Enable IR tracing in assembly dimension")
    parser.add_argument("--max-benches", type=int, help="Optional max number of benches for quick tests")
    parser.add_argument("--results-jsonl", default="tmp/bench/nightly-full-results.jsonl", help="Append-only result stream")
    parser.add_argument("--run-log", default="tmp/bench/nightly-full.log", help="Human-readable log file")
    args = parser.parse_args()

    root = repo_root()
    bench_dir = (root / args.bench_dir).resolve()
    if not bench_dir.exists():
        print(f"bench dir not found: {bench_dir}")
        return 2

    results_jsonl = (root / args.results_jsonl).resolve()
    run_log = (root / args.run_log).resolve()
    run_log.parent.mkdir(parents=True, exist_ok=True)
    run_id = time.strftime("%Y%m%d-%H%M%S")

    hotspots_path = root / "scripts" / "bench" / "config" / "hotspots.json"
    hotspots = load_json(hotspots_path) if hotspots_path.exists() else {}

    benches = discover_benches(bench_dir)
    if args.max_benches is not None:
        benches = benches[: max(0, args.max_benches)]
    if not benches:
        print("no benchmarks found")
        return 2

    header = {
        "type": "run_start",
        "run_id": run_id,
        "timestamp": now_iso(),
        "bench_count": len(benches),
        "config": {
            "perf_repeats": args.perf_repeats,
            "compile_repeats": args.compile_repeats,
            "compile_warmup_pairs": args.compile_warmup_pairs,
            "compile_bootstrap_iterations": args.compile_bootstrap_iterations,
            "trace_ir": args.trace_ir,
            "bench_dir": str(bench_dir),
        },
    }
    append_jsonl(results_jsonl, header)

    with run_log.open("a", encoding="utf-8") as log:
        log.write(f"[{now_iso()}] run_start {run_id} benches={len(benches)}\n")

    total_records = 0
    for index, bench in enumerate(benches, start=1):
        bench_name = bench.name
        bench_stem = bench_key(bench)
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

        with run_log.open("a", encoding="utf-8") as log:
            log.write(f"[{now_iso()}] bench_start {index}/{len(benches)} {bench_name}\n")

        tmp_dir = root / "tmp" / "bench"
        tmp_dir.mkdir(parents=True, exist_ok=True)

        overrides_path = tmp_dir / f"nightly-overrides-{bench_stem}-{run_id}.json"
        overrides_path.write_text(
            json.dumps(build_perf_override(bench_name, max(1, args.perf_repeats)), indent=2, ensure_ascii=False),
            encoding="utf-8",
        )

        perf_cmd = [
            sys.executable,
            str(root / "scripts" / "bench" / "run_pipeline.py"),
            "--bench",
            str(bench),
            "--overrides",
            str(overrides_path),
        ]
        if args.d8:
            perf_cmd += ["--d8", args.d8]
        perf_ret = run_cmd(perf_cmd, root)
        perf_report_path = parse_saved_path(perf_ret["stdout"])
        perf_metrics: dict[str, Any] = {}
        if perf_report_path and perf_report_path.exists():
            report = load_json(perf_report_path)
            if report.get("results"):
                perf_metrics = report["results"][0].get("comparison", {})

        perf_record = {
            "type": "dimension_result",
            "run_id": run_id,
            "timestamp": now_iso(),
            "bench": str(bench),
            "dimension": "performance",
            "status": "ok" if perf_ret["returncode"] == 0 else "failed",
            "metrics": perf_metrics,
            "report_path": str(perf_report_path) if perf_report_path else None,
            "exec": {
                "duration_sec": perf_ret["duration_sec"],
                "returncode": perf_ret["returncode"],
                "command": perf_ret["command"],
                "stdout_tail": perf_ret["stdout_tail"],
                "stderr_tail": perf_ret["stderr_tail"],
            },
        }
        append_jsonl(results_jsonl, perf_record)
        total_records += 1

        compile_out = tmp_dir / f"compile-cost-nightly-{bench_stem}-{run_id}.json"
        compile_jsonl = tmp_dir / f"compile-cost-nightly-{run_id}.jsonl"
        compile_cmd = [
            sys.executable,
            str(root / "scripts" / "bench" / "measure_compile_cost.py"),
            "--bench",
            str(bench),
            "--repeats",
            str(max(1, args.compile_repeats)),
            "--warmup-pairs",
            str(max(0, args.compile_warmup_pairs)),
            "--bootstrap-iterations",
            str(max(100, args.compile_bootstrap_iterations)),
            "--out",
            str(compile_out),
            "--jsonl",
            str(compile_jsonl),
        ]
        if args.d8:
            compile_cmd += ["--d8", args.d8]
        compile_ret = run_cmd(compile_cmd, root)
        compile_metrics: dict[str, Any] = {}
        if compile_out.exists():
            compile_report = load_json(compile_out)
            if compile_report.get("results"):
                row = compile_report["results"][0]
                compile_metrics = {
                    "classification": row.get("classification"),
                    "delta_percent": row.get("delta_percent"),
                    "delta_sec": row.get("delta_sec"),
                    "paired_ci": row.get("paired_ci"),
                    "paired": {
                        "paired_count": (row.get("paired") or {}).get("paired_count"),
                        "sign_consistency": (row.get("paired") or {}).get("sign_consistency"),
                    },
                    "without_cv": (row.get("without") or {}).get("cv"),
                    "with_cv": (row.get("with") or {}).get("cv"),
                }

        compile_record = {
            "type": "dimension_result",
            "run_id": run_id,
            "timestamp": now_iso(),
            "bench": str(bench),
            "dimension": "compile_latency",
            "status": "ok" if compile_ret["returncode"] == 0 else "failed",
            "metrics": compile_metrics,
            "report_path": str(compile_out),
            "exec": {
                "duration_sec": compile_ret["duration_sec"],
                "returncode": compile_ret["returncode"],
                "command": compile_ret["command"],
                "stdout_tail": compile_ret["stdout_tail"],
                "stderr_tail": compile_ret["stderr_tail"],
            },
        }
        append_jsonl(results_jsonl, compile_record)
        total_records += 1

        asm_cmd = [
            sys.executable,
            str(root / "scripts" / "bench" / "generate_bench_assembly.py"),
            "--bench",
            str(bench),
            "--stats-scope",
            "all-business",
        ]
        hotspot = hotspots.get(bench_stem, {})
        if hotspot.get("function"):
            asm_cmd += ["--function", str(hotspot["function"])]
        if hotspot.get("invoke_expr"):
            asm_cmd += ["--invoke-expr", str(hotspot["invoke_expr"])]
        if args.trace_ir:
            asm_cmd.append("--trace-ir")
        if args.d8:
            asm_cmd += ["--d8", args.d8]
        asm_ret = run_cmd(asm_cmd, root)

        comparison_path = root / "docs" / "assembly" / "benchmarks" / bench_stem / "comparison.json"
        asm_metrics: dict[str, Any] = {}
        if comparison_path.exists():
            cmp_json = load_json(comparison_path)
            diff = cmp_json.get("diff", {})
            optimized = cmp_json.get("optimized_business_functions", {})
            asm_metrics = {
                "line_delta": diff.get("line_delta"),
                "instruction_size_delta": diff.get("instruction_size_delta"),
                "changed_lines": diff.get("changed_lines"),
                "without_instruction_size": ((cmp_json.get("without_metadata") or {}).get("instruction_size")),
                "with_instruction_size": ((cmp_json.get("with_metadata") or {}).get("instruction_size")),
                "optimized_without_count": optimized.get("without_count"),
                "optimized_with_count": optimized.get("with_count"),
                "optimized_paired_count": optimized.get("paired_count"),
            }

        asm_record = {
            "type": "dimension_result",
            "run_id": run_id,
            "timestamp": now_iso(),
            "bench": str(bench),
            "dimension": "binary_codegen",
            "status": "ok" if asm_ret["returncode"] == 0 else "failed",
            "metrics": asm_metrics,
            "report_path": str(comparison_path),
            "exec": {
                "duration_sec": asm_ret["duration_sec"],
                "returncode": asm_ret["returncode"],
                "command": asm_ret["command"],
                "stdout_tail": asm_ret["stdout_tail"],
                "stderr_tail": asm_ret["stderr_tail"],
            },
        }
        append_jsonl(results_jsonl, asm_record)
        total_records += 1

        append_jsonl(
            results_jsonl,
            {
                "type": "bench_end",
                "run_id": run_id,
                "timestamp": now_iso(),
                "bench": str(bench),
                "dimension_count": 3,
            },
        )

        with run_log.open("a", encoding="utf-8") as log:
            log.write(
                f"[{now_iso()}] bench_end {bench_name} perf={perf_record['status']} compile={compile_record['status']} asm={asm_record['status']}\n"
            )

    trailer = {
        "type": "run_end",
        "run_id": run_id,
        "timestamp": now_iso(),
        "bench_count": len(benches),
        "dimension_records": total_records,
        "results_jsonl": str(results_jsonl),
        "run_log": str(run_log),
    }
    append_jsonl(results_jsonl, trailer)
    with run_log.open("a", encoding="utf-8") as log:
        log.write(f"[{now_iso()}] run_end {run_id} dimensions={total_records}\n")

    print(f"Run ID: {run_id}")
    print(f"Results JSONL: {results_jsonl}")
    print(f"Run log: {run_log}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
