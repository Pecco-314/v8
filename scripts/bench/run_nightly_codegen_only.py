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

    radix = hotspots.get("radix-sort", {})
    if radix.get("function") != "radixSort" or radix.get("invoke_expr") != "radixSort(data.slice())":
        print("hotspots.json 校验失败: radix-sort 必须指向 radixSort(data.slice())")
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
        ]
        if hotspot.get("function"):
            cmd += ["--function", str(hotspot["function"])]
        if hotspot.get("invoke_expr"):
            cmd += ["--invoke-expr", str(hotspot["invoke_expr"])]
        if args.trace_ir:
            cmd.append("--trace-ir")
        if args.d8:
            cmd += ["--d8", args.d8]

        ret = run_cmd(cmd, root)
        comparison_path = root / "docs" / "assembly" / "benchmarks" / bench_stem / "comparison.json"
        metrics: dict[str, Any] = {}
        if comparison_path.exists():
            cmp_json = load_json(comparison_path)
            diff = cmp_json.get("diff", {})
            metrics = {
                "line_delta": diff.get("line_delta"),
                "instruction_size_delta": diff.get("instruction_size_delta"),
                "changed_lines": diff.get("changed_lines"),
                "without_instruction_size": (cmp_json.get("without_metadata") or {}).get("instruction_size"),
                "with_instruction_size": (cmp_json.get("with_metadata") or {}).get("instruction_size"),
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
