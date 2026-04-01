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


def collect_performance(root: Path, bench: Path, args: argparse.Namespace, run_id: str) -> tuple[dict[str, Any], int]:
    tmp_dir = root / "tmp" / "bench"
    tmp_dir.mkdir(parents=True, exist_ok=True)
    bench_stem = bench_key(bench)

    overrides_path = tmp_dir / f"nightly-overrides-{bench_stem}-{run_id}.json"
    overrides_path.write_text(
        json.dumps(build_perf_override(bench.name, max(1, args.perf_repeats)), indent=2, ensure_ascii=False),
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

    record = {
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
    return record, perf_ret["returncode"]


def collect_compile_latency(root: Path, bench: Path, args: argparse.Namespace, run_id: str) -> tuple[dict[str, Any], int]:
    tmp_dir = root / "tmp" / "bench"
    tmp_dir.mkdir(parents=True, exist_ok=True)
    bench_stem = bench_key(bench)

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

    record = {
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
    return record, compile_ret["returncode"]


def collect_binary_codegen(
    root: Path,
    bench: Path,
    args: argparse.Namespace,
    hotspots: dict[str, Any],
) -> tuple[dict[str, Any], int]:
    bench_stem = bench_key(bench)
    hotspot = hotspots.get(bench_stem, {})

    asm_cmd = [
        sys.executable,
        str(root / "scripts" / "bench" / "generate_bench_assembly.py"),
        "--bench",
        str(bench),
        "--metadata-mode",
        args.codegen_metadata_mode,
        "--stats-scope",
        "all-business",
    ]
    if hotspot.get("function"):
        asm_cmd += ["--function", str(hotspot["function"])]
    if hotspot.get("invoke_expr"):
        asm_cmd += ["--invoke-expr", str(hotspot["invoke_expr"])]
    if args.trace_ir:
        asm_cmd.append("--trace-ir")
    if args.codegen_verify_rawint32_add_reduction:
        asm_cmd.append("--verify-rawint32-add-reduction")

    if args.codegen_compile_coverage:
        asm_cmd.append("--compile-coverage")
        asm_cmd += ["--coverage-warmup-calls", str(args.coverage_warmup_calls)]
        if args.codegen_coverage_all_business:
            asm_cmd.append("--coverage-all-business-functions")
        for coverage_target in args.coverage_target:
            asm_cmd += ["--coverage-target", str(coverage_target)]

    if args.codegen_no_inline:
        asm_cmd.append("--no-inline")
    if args.no_wrapper_preprocess:
        asm_cmd.append("--no-wrapper-preprocess")
    if args.d8:
        asm_cmd += ["--d8", args.d8]

    asm_ret = run_cmd(asm_cmd, root)

    comparison_path = root / "docs" / "assembly" / "benchmarks" / bench_stem / "comparison.json"
    asm_metrics: dict[str, Any] = {}
    if comparison_path.exists():
        cmp_json = load_json(comparison_path)
        diff = cmp_json.get("diff", {})
        scope = cmp_json.get("stats_scope", "target")
        optimized = cmp_json.get("optimized_business_functions", {})
        totals = cmp_json.get("all_business_totals", {}) if isinstance(cmp_json.get("all_business_totals"), dict) else {}
        coverage = cmp_json.get("compile_coverage", {}) if isinstance(cmp_json.get("compile_coverage"), dict) else {}
        coverage_without = coverage.get("without", {}) if isinstance(coverage.get("without"), dict) else {}
        coverage_with = coverage.get("with", {}) if isinstance(coverage.get("with"), dict) else {}
        raw_phase = cmp_json.get("rawint32_strength_reduction", {}) if isinstance(cmp_json.get("rawint32_strength_reduction"), dict) else {}
        raw_without = raw_phase.get("without", {}) if isinstance(raw_phase.get("without"), dict) else {}
        raw_with = raw_phase.get("with", {}) if isinstance(raw_phase.get("with"), dict) else {}
        raw_without_totals = raw_without.get("totals", {}) if isinstance(raw_without.get("totals"), dict) else {}
        raw_with_totals = raw_with.get("totals", {}) if isinstance(raw_with.get("totals"), dict) else {}
        verify = cmp_json.get("verification", {}) if isinstance(cmp_json.get("verification"), dict) else {}
        verify_raw = verify.get("rawint32_add_reduction", {}) if isinstance(verify.get("rawint32_add_reduction"), dict) else {}

        without_instruction_size = totals.get("without_instruction_size")
        with_instruction_size = totals.get("with_instruction_size")
        if without_instruction_size is None:
            without_instruction_size = (cmp_json.get("without_metadata") or {}).get("instruction_size")
        if with_instruction_size is None:
            with_instruction_size = (cmp_json.get("with_metadata") or {}).get("instruction_size")

        asm_metrics = {
            "stats_scope": scope,
            "line_delta": diff.get("line_delta"),
            "instruction_size_delta": diff.get("instruction_size_delta"),
            "changed_lines": diff.get("changed_lines"),
            "without_instruction_size": without_instruction_size,
            "with_instruction_size": with_instruction_size,
            "optimized_without_count": optimized.get("without_count"),
            "optimized_with_count": optimized.get("with_count"),
            "optimized_paired_count": optimized.get("paired_count"),
            "coverage_enabled": coverage.get("enabled"),
            "coverage_target_count": len(coverage.get("targets", [])) if isinstance(coverage.get("targets"), list) else None,
            "coverage_without_compiled_target_count": coverage_without.get("compiled_target_count"),
            "coverage_with_compiled_target_count": coverage_with.get("compiled_target_count"),
            "raw_phase_enabled": raw_phase.get("enabled"),
            "raw_without_checked_int32_add": raw_without_totals.get("CheckedInt32Add"),
            "raw_with_checked_int32_add": raw_with_totals.get("CheckedInt32Add"),
            "raw_without_int32_add": raw_without_totals.get("Int32Add"),
            "raw_with_int32_add": raw_with_totals.get("Int32Add"),
            "verify_rawint32_add_status": verify_raw.get("status"),
            "verify_rawint32_add_regression_count": verify_raw.get("regression_count"),
        }

    record = {
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
    return record, asm_ret["returncode"]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Nightly benchmark orchestrator (performance / compile latency / binary codegen)")
    parser.add_argument("--bench-dir", default="scripts/bench/benchmarks", help="Benchmark directory")
    parser.add_argument("--d8", help="Optional d8 path override")
    parser.add_argument(
        "--dimensions",
        default="performance,compile_latency,binary_codegen",
        help="Comma-separated dimensions: performance,compile_latency,binary_codegen",
    )

    parser.add_argument("--perf-repeats", type=int, default=30, help="Repeats for performance dimension")
    parser.add_argument("--compile-repeats", type=int, default=30, help="Repeats for compile-latency dimension")
    parser.add_argument("--compile-warmup-pairs", type=int, default=2, help="Warmup pairs for compile-latency")
    parser.add_argument("--compile-bootstrap-iterations", type=int, default=3000, help="Bootstrap iterations")

    parser.add_argument("--trace-ir", action="store_true", help="Enable IR tracing in binary_codegen dimension")
    parser.add_argument(
        "--codegen-metadata-mode",
        choices=["strict", "off"],
        default="strict",
        help="Metadata mode for binary_codegen",
    )
    parser.add_argument(
        "--codegen-no-compile-coverage",
        action="store_true",
        help="Disable compile coverage in binary_codegen (default enabled)",
    )
    parser.add_argument(
        "--codegen-no-all-business-coverage",
        action="store_true",
        help="Disable auto coverage for all business functions (default enabled)",
    )
    parser.add_argument(
        "--codegen-no-verify-rawint32-add-reduction",
        action="store_true",
        help="Disable rawint32 CheckedInt32Add->Int32Add regression verification in binary_codegen",
    )
    parser.add_argument(
        "--codegen-allow-inline",
        action="store_true",
        help="Allow inlining in binary_codegen (default is no-inline)",
    )
    parser.add_argument("--coverage-warmup-calls", type=int, default=8, help="Warmup calls for compile coverage")
    parser.add_argument(
        "--coverage-target",
        action="append",
        default=[],
        help="Additional coverage target in format function=invoke_expr",
    )
    parser.add_argument(
        "--no-wrapper-preprocess",
        action="store_true",
        help="Disable ts_locals_to_wrappers preprocessing for TS benchmarks",
    )

    parser.add_argument("--max-benches", type=int, help="Optional max number of benches for quick tests")
    parser.add_argument("--results-jsonl", default="tmp/bench/nightly-results.jsonl", help="Append-only result stream")
    parser.add_argument("--run-log", default="tmp/bench/nightly.log", help="Human-readable run log")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = repo_root()

    requested_dimensions = [item.strip() for item in args.dimensions.split(",") if item.strip()]
    valid_dimensions = {"performance", "compile_latency", "binary_codegen"}
    if any(item not in valid_dimensions for item in requested_dimensions):
        print(f"invalid dimensions: {requested_dimensions}")
        return 2

    args.codegen_compile_coverage = not args.codegen_no_compile_coverage
    args.codegen_coverage_all_business = not args.codegen_no_all_business_coverage
    args.codegen_no_inline = not args.codegen_allow_inline
    args.codegen_verify_rawint32_add_reduction = not args.codegen_no_verify_rawint32_add_reduction

    bench_dir = (root / args.bench_dir).resolve()
    if not bench_dir.exists():
        print(f"bench dir not found: {bench_dir}")
        return 2

    hotspots_path = root / "scripts" / "bench" / "config" / "hotspots.json"
    hotspots = load_json(hotspots_path) if hotspots_path.exists() else {}

    benches = discover_benches(bench_dir)
    if args.max_benches is not None:
        benches = benches[: max(0, args.max_benches)]
    if not benches:
        print("no benchmarks found")
        return 2

    results_jsonl = (root / args.results_jsonl).resolve()
    results_jsonl.parent.mkdir(parents=True, exist_ok=True)
    results_jsonl.write_text("", encoding="utf-8")

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
            "dimensions": requested_dimensions,
            "config": {
                "perf_repeats": args.perf_repeats,
                "compile_repeats": args.compile_repeats,
                "compile_warmup_pairs": args.compile_warmup_pairs,
                "compile_bootstrap_iterations": args.compile_bootstrap_iterations,
                "trace_ir": args.trace_ir,
                "bench_dir": str(bench_dir),
                "codegen_metadata_mode": args.codegen_metadata_mode,
                "codegen_compile_coverage": args.codegen_compile_coverage,
                "codegen_coverage_all_business": args.codegen_coverage_all_business,
                "codegen_no_inline": args.codegen_no_inline,
            },
        },
    )

    with run_log.open("a", encoding="utf-8") as log:
        log.write(
            f"[{now_iso()}] run_start {run_id} benches={len(benches)} dimensions={','.join(requested_dimensions)}\\n"
        )

    total_dimension_records = 0
    total_failures = 0

    for index, bench in enumerate(benches, start=1):
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
            log.write(f"[{now_iso()}] bench_start {index}/{len(benches)} {bench.name}\\n")

        bench_failures = 0
        for dimension in requested_dimensions:
            if dimension == "performance":
                record, returncode = collect_performance(root, bench, args, run_id)
            elif dimension == "compile_latency":
                record, returncode = collect_compile_latency(root, bench, args, run_id)
            else:
                record, returncode = collect_binary_codegen(root, bench, args, hotspots)

            append_jsonl(
                results_jsonl,
                {
                    "type": "dimension_result",
                    "run_id": run_id,
                    "timestamp": now_iso(),
                    "bench": str(bench),
                    **record,
                },
            )
            total_dimension_records += 1
            if returncode != 0:
                bench_failures += 1

        total_failures += bench_failures

        append_jsonl(
            results_jsonl,
            {
                "type": "bench_end",
                "run_id": run_id,
                "timestamp": now_iso(),
                "bench": str(bench),
                "dimension_count": len(requested_dimensions),
                "failed_dimensions": bench_failures,
            },
        )

        with run_log.open("a", encoding="utf-8") as log:
            log.write(
                f"[{now_iso()}] bench_end {bench.name} failed_dimensions={bench_failures}/{len(requested_dimensions)}\\n"
            )

    append_jsonl(
        results_jsonl,
        {
            "type": "run_end",
            "run_id": run_id,
            "timestamp": now_iso(),
            "bench_count": len(benches),
            "dimension_records": total_dimension_records,
            "failed_dimensions": total_failures,
            "results_jsonl": str(results_jsonl),
            "run_log": str(run_log),
        },
    )

    with run_log.open("a", encoding="utf-8") as log:
        log.write(f"[{now_iso()}] run_end {run_id} dimensions={total_dimension_records} failed={total_failures}\\n")

    print(f"Run ID: {run_id}")
    print(f"Dimensions: {','.join(requested_dimensions)}")
    print(f"Results JSONL: {results_jsonl}")
    print(f"Run log: {run_log}")
    return 1 if total_failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
