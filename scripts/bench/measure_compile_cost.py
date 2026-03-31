#!/usr/bin/env python3
import argparse
import hashlib
import json
import random
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def resolve_path(path_value: str | None, root: Path) -> Path | None:
    if path_value is None:
        return None
    path = Path(path_value)
    return path if path.is_absolute() else root / path


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def merge_config(defaults: dict, override: dict | None) -> dict:
    merged = dict(defaults)
    if not override:
        return merged
    for key, value in override.items():
        merged[key] = value
    return merged


def sha256_file(file_path: Path) -> str:
    return hashlib.sha256(file_path.read_bytes()).hexdigest()


def discover_benches(bench_dir: Path, bench_arg: Path | None) -> list[Path]:
    if bench_arg:
        return [bench_arg]
    return sorted([*bench_dir.glob("*.js"), *bench_dir.glob("*.ts")])


def build_harness(tmp_dir: Path, bench_js: Path) -> Path:
    harness_path = tmp_dir / f"harness-compile-{bench_js.stem}.js"
    bench_literal = json.dumps(str(bench_js))
    content = f"""
load({bench_literal});

if (typeof bench !== 'function') {{
  throw new Error('bench() function is required');
}}

if (typeof setup === 'function') {{
  setup();
}}

%PrepareFunctionForOptimization(bench);
bench();
bench();
%OptimizeFunctionOnNextCall(bench);
bench();

if (typeof teardown === 'function') {{
  teardown();
}}

print('BENCH_DONE');
"""
    harness_path.write_text(content, encoding="utf-8")
    return harness_path


def build_d8_command(d8: Path, harness: Path, extra_flags: list[str]) -> list[str]:
    return [
        str(d8),
        "--allow-natives-syntax",
        "--no-concurrent-recompilation",
    ] + extra_flags + [str(harness)]


def prepare_ts_bench(root: Path, bench_file: Path, metadata_dir: Path, tmp_dir: Path) -> Path | None:
    if bench_file.suffix != ".ts":
        return None
    ts_to_metadata = root / "scripts" / "ts_to_metadata.js"
    tsc_out_dir = tmp_dir / "tsc"
    tsc_out_dir.mkdir(parents=True, exist_ok=True)
    command = [
        "node",
        str(ts_to_metadata),
        str(bench_file),
        "--outDir",
        str(tsc_out_dir),
        "--metadataDir",
        str(metadata_dir),
    ]
    result = subprocess.run(command, capture_output=True, text=True, cwd=str(root))
    if result.returncode != 0:
        return None
    js_file = tsc_out_dir / f"{bench_file.stem}.js"
    return js_file if js_file.exists() else None


def timed_run(command: list[str], root: Path) -> tuple[float, int, str, str]:
    start = time.perf_counter()
    result = subprocess.run(command, capture_output=True, text=True, cwd=str(root))
    elapsed = time.perf_counter() - start
    return elapsed, result.returncode, result.stdout, result.stderr


def build_exec_command(command: list[str], cpu_core: int | None, nice_level: int | None) -> list[str]:
    wrapped = list(command)
    if cpu_core is not None and shutil.which("taskset"):
        wrapped = ["taskset", "-c", str(cpu_core)] + wrapped
    if nice_level is not None and shutil.which("nice"):
        wrapped = ["nice", "-n", str(nice_level)] + wrapped
    return wrapped


def summarize_runs(runs: list[dict]) -> dict:
    ok_times = [entry["time_sec"] for entry in runs if entry.get("exit_code") == 0]
    median_sec = statistics.median(ok_times) if ok_times else None
    mean_sec = statistics.fmean(ok_times) if ok_times else None
    stdev_sec = statistics.pstdev(ok_times) if len(ok_times) > 1 else (0.0 if len(ok_times) == 1 else None)
    cv = (stdev_sec / mean_sec) if (stdev_sec is not None and mean_sec) else None
    summary = {
        "runs": runs,
        "ok_count": len(ok_times),
        "total_count": len(runs),
        "median_sec": median_sec,
        "mean_sec": mean_sec,
        "stdev_sec": stdev_sec,
        "cv": cv,
        "min_sec": min(ok_times) if ok_times else None,
        "max_sec": max(ok_times) if ok_times else None,
    }
    return summary


def summarize_pair_deltas(interleaved_runs: list[dict]) -> dict:
    pair_bucket: dict[int, dict[str, float]] = {}
    for entry in interleaved_runs:
        if entry.get("exit_code") != 0:
            continue
        pair_id = entry.get("pair_id")
        mode = entry.get("mode")
        if not isinstance(pair_id, int) or mode not in ("with", "without"):
            continue
        if pair_id not in pair_bucket:
            pair_bucket[pair_id] = {}
        pair_bucket[pair_id][mode] = entry["time_sec"]

    deltas = []
    for pair_id in sorted(pair_bucket.keys()):
        pair = pair_bucket[pair_id]
        if "with" in pair and "without" in pair:
            deltas.append(pair["with"] - pair["without"])

    if not deltas:
        return {
            "paired_count": 0,
            "deltas_sec": [],
            "median_sec": None,
            "mean_sec": None,
            "stdev_sec": None,
            "min_sec": None,
            "max_sec": None,
            "sign_consistency": None,
        }

    positive = sum(1 for value in deltas if value > 0)
    negative = sum(1 for value in deltas if value < 0)
    sign_consistency = max(positive, negative) / len(deltas)

    return {
        "paired_count": len(deltas),
        "deltas_sec": deltas,
        "median_sec": statistics.median(deltas),
        "mean_sec": statistics.fmean(deltas),
        "stdev_sec": statistics.pstdev(deltas) if len(deltas) > 1 else 0.0,
        "min_sec": min(deltas),
        "max_sec": max(deltas),
        "sign_consistency": sign_consistency,
    }


def bootstrap_ci(values: list[float], confidence: float = 0.95, iterations: int = 2000) -> dict:
    if not values:
        return {
            "confidence": confidence,
            "iterations": iterations,
            "lower": None,
            "upper": None,
        }

    if len(values) == 1:
        only = values[0]
        return {
            "confidence": confidence,
            "iterations": iterations,
            "lower": only,
            "upper": only,
        }

    samples = []
    n = len(values)
    alpha = (1.0 - confidence) / 2.0
    rng = random.Random(42)
    for _ in range(max(100, iterations)):
        picked = [values[rng.randrange(n)] for _ in range(n)]
        samples.append(statistics.median(picked))
    samples.sort()
    lo_idx = max(0, int(alpha * len(samples)) - 1)
    hi_idx = min(len(samples) - 1, int((1.0 - alpha) * len(samples)) - 1)
    return {
        "confidence": confidence,
        "iterations": iterations,
        "lower": samples[lo_idx],
        "upper": samples[hi_idx],
    }


def classify_delta(ci: dict, tolerance_ratio: float = 0.001, baseline_sec: float | None = None) -> dict:
    lower = ci.get("lower")
    upper = ci.get("upper")
    if lower is None or upper is None:
        return {
            "label": "unknown",
            "reason": "insufficient_data",
        }

    tolerance_sec = 0.0
    if baseline_sec is not None and baseline_sec > 0:
        tolerance_sec = baseline_sec * tolerance_ratio

    if lower > tolerance_sec:
        return {
            "label": "overhead",
            "reason": "ci_above_tolerance",
            "tolerance_sec": tolerance_sec,
        }
    if upper < -tolerance_sec:
        return {
            "label": "speedup",
            "reason": "ci_below_tolerance",
            "tolerance_sec": tolerance_sec,
        }
    return {
        "label": "neutral",
        "reason": "ci_crosses_tolerance",
        "tolerance_sec": tolerance_sec,
    }


def run_single(command: list[str], root: Path, mode: str, pair_id: int, order_index: int) -> dict:
    start = time.perf_counter()
    result = subprocess.run(command, capture_output=True, text=True, cwd=str(root))
    elapsed = time.perf_counter() - start
    return {
        "mode": mode,
        "pair_id": pair_id,
        "order_index": order_index,
        "time_sec": elapsed,
        "exit_code": result.returncode,
        "stdout_tail": "\n".join(result.stdout.splitlines()[-20:]),
        "stderr_tail": "\n".join(result.stderr.splitlines()[-20:]),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Measure compile overhead with/without metadata")
    parser.add_argument("--bench", help="Single benchmark path")
    parser.add_argument("--d8", help="Path to d8")
    parser.add_argument("--config", default="scripts/bench/config/defaults.json")
    parser.add_argument("--overrides", default="scripts/bench/config/overrides.json")
    parser.add_argument("--repeats", type=int, default=5, help="Number of timing runs per mode")
    parser.add_argument("--warmup-pairs", type=int, default=1, help="Interleaved warmup pairs before recording")
    parser.add_argument("--cpu-core", type=int, help="Pin each run to one CPU core via taskset")
    parser.add_argument("--nice", type=int, help="Run with nice priority")
    parser.add_argument("--sleep-ms", type=int, help="Sleep milliseconds between runs")
    parser.add_argument("--bootstrap-iterations", type=int, default=2000, help="Bootstrap iterations for paired-delta CI")
    parser.add_argument("--neutral-tolerance-ratio", type=float, default=0.001, help="Neutral zone ratio against without median (default 0.1%)")
    parser.add_argument("--out", help="Output JSON path (default: tmp/bench/compile-cost-<timestamp>.json)")
    parser.add_argument("--jsonl", help="Output JSONL path (default: tmp/bench/compile-cost.jsonl)")
    args = parser.parse_args()

    root = repo_root()
    config_path = resolve_path(args.config, root)
    defaults = load_json(config_path) if config_path and config_path.exists() else {}
    overrides_path = resolve_path(args.overrides, root)
    overrides = load_json(overrides_path) if overrides_path and overrides_path.exists() else {}

    bench_dir = resolve_path(defaults.get("bench_dir", "scripts/bench/benchmarks"), root)
    metadata_dir = resolve_path(defaults.get("metadata_dir", "scripts/bench/metadata"), root)
    d8 = resolve_path(args.d8 or defaults.get("d8", "out.gn/x64.release/d8"), root)
    bench_arg = resolve_path(args.bench, root)

    if bench_dir is None or not bench_dir.exists():
        print(f"bench dir not found: {bench_dir}")
        return 2
    if metadata_dir is None or not metadata_dir.exists():
        print(f"metadata dir not found: {metadata_dir}")
        return 2
    if d8 is None or not d8.exists():
        print(f"d8 not found: {d8}")
        return 2
    if bench_arg and not bench_arg.exists():
        print(f"bench not found: {bench_arg}")
        return 2

    benches = discover_benches(bench_dir, bench_arg)
    if not benches:
        print("no benchmark files found")
        return 2

    run_id = time.strftime("%Y%m%d-%H%M%S")
    out_path = Path(args.out) if args.out else root / "tmp" / "bench" / f"compile-cost-{run_id}.json"
    jsonl_path = Path(args.jsonl) if args.jsonl else root / "tmp" / "bench" / "compile-cost.jsonl"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    jsonl_path.parent.mkdir(parents=True, exist_ok=True)

    results = []
    tmp_dir = root / "tmp" / "bench"
    tmp_dir.mkdir(parents=True, exist_ok=True)

    for bench_file in benches:
        per_file_override = overrides.get(bench_file.name, {})
        cfg = merge_config(defaults, per_file_override)
        cfg["metadata_dir"] = str(metadata_dir)

        bench_js = bench_file
        if bench_file.suffix == ".ts":
            compiled = prepare_ts_bench(root, bench_file, metadata_dir, tmp_dir)
            if compiled is None:
                record = {
                    "run_id": run_id,
                    "bench": str(bench_file),
                    "status": "tsc_failed",
                    "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
                }
                results.append(record)
                with jsonl_path.open("a", encoding="utf-8") as handle:
                    handle.write(json.dumps(record, ensure_ascii=False) + "\n")
                continue
            bench_js = compiled

        file_hash = sha256_file(bench_js)
        metadata_file = metadata_dir / f"{file_hash}.metadata"

        harness = build_harness(tmp_dir, bench_js)
        without_cmd_raw = build_d8_command(d8, harness, [])
        with_cmd_raw = build_d8_command(d8, harness, [f"--turbo_metadata_path={metadata_dir}"])

        cpu_core = args.cpu_core if args.cpu_core is not None else cfg.get("cpu_core")
        nice_level = args.nice if args.nice is not None else cfg.get("nice")
        sleep_ms = args.sleep_ms if args.sleep_ms is not None else int(cfg.get("sleep_ms", 0))

        without_cmd = build_exec_command(without_cmd_raw, cpu_core, nice_level)
        with_cmd = build_exec_command(with_cmd_raw, cpu_core, nice_level)

        warmup_pairs = max(0, args.warmup_pairs)
        repeats = max(1, args.repeats)

        for pair_id in range(warmup_pairs):
            order = ("without", "with") if pair_id % 2 == 0 else ("with", "without")
            for mode in order:
                _ = run_single(without_cmd if mode == "without" else with_cmd, root, mode, pair_id, 0)
                if sleep_ms > 0:
                    time.sleep(sleep_ms / 1000.0)

        interleaved_runs: list[dict] = []
        for pair_id in range(repeats):
            order = ("without", "with") if pair_id % 2 == 0 else ("with", "without")
            for order_index, mode in enumerate(order):
                run = run_single(without_cmd if mode == "without" else with_cmd, root, mode, pair_id, order_index)
                interleaved_runs.append(run)
                if sleep_ms > 0:
                    time.sleep(sleep_ms / 1000.0)

        without_runs = [entry for entry in interleaved_runs if entry["mode"] == "without"]
        with_runs = [entry for entry in interleaved_runs if entry["mode"] == "with"]

        without_summary = summarize_runs(without_runs)
        with_summary = summarize_runs(with_runs)
        pair_summary = summarize_pair_deltas(interleaved_runs)
        without_median = without_summary["median_sec"]
        with_median = with_summary["median_sec"]
        has_failure = without_summary["ok_count"] < without_summary["total_count"] or with_summary["ok_count"] < with_summary["total_count"]
        delta_sec = pair_summary["median_sec"]
        if delta_sec is None:
            delta_sec = (with_median - without_median) if (without_median is not None and with_median is not None) else None
        delta_percent = ((delta_sec / without_median) * 100.0) if (delta_sec is not None and without_median) else None

        pair_ci = bootstrap_ci(
            pair_summary.get("deltas_sec", []),
            confidence=0.95,
            iterations=max(100, args.bootstrap_iterations),
        )
        pair_classification = classify_delta(
            pair_ci,
            tolerance_ratio=max(0.0, args.neutral_tolerance_ratio),
            baseline_sec=without_median,
        )

        record = {
            "run_id": run_id,
            "bench": str(bench_file),
            "bench_js": str(bench_js),
            "hash": file_hash,
            "metadata": str(metadata_file),
            "measurement": {
                "repeats_per_mode": repeats,
                "warmup_pairs": warmup_pairs,
                "interleaved": True,
                "cpu_core": cpu_core,
                "nice": nice_level,
                "sleep_ms": sleep_ms,
            },
            "without": without_summary,
            "with": with_summary,
            "paired": pair_summary,
            "paired_ci": pair_ci,
            "classification": pair_classification,
            "delta_sec": delta_sec,
            "delta_percent": delta_percent,
            "status": "run_failed" if has_failure else "ok",
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
        }
        results.append(record)
        with jsonl_path.open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(record, ensure_ascii=False) + "\n")

    out_path.write_text(json.dumps({"run_id": run_id, "results": results}, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"Saved: {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
