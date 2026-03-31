#!/usr/bin/env python3
import argparse
import hashlib
import json
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
    if path.is_absolute():
        return path
    return root / path


def sha256_file(file_path: Path) -> str:
    return hashlib.sha256(file_path.read_bytes()).hexdigest()


def parse_result_path(stdout: str) -> Path:
    for line in stdout.splitlines():
        if line.startswith("Results saved to:"):
            return Path(line.split(":", 1)[1].strip())
    raise RuntimeError("failed to find result path in run_bench.py output")


def median_of(result_json: dict, event_name: str) -> float | None:
    section = result_json.get("results", {}).get(event_name, {})
    return section.get("stats", {}).get("median")


def cv_of(result_json: dict, event_name: str) -> float | None:
    section = result_json.get("results", {}).get(event_name, {})
    return section.get("stats", {}).get("cv")


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def merge_config(defaults: dict, override: dict | None) -> dict:
    merged = dict(defaults)
    if not override:
        return merged
    for key, value in override.items():
        merged[key] = value
    return merged


def build_run_bench_command(root: Path, bench_file: Path, d8: Path, cfg: dict, with_metadata: bool) -> list[str]:
    run_bench = root / "scripts" / "bench" / "run_bench.py"
    command = [
        sys.executable,
        str(run_bench),
        "--bench",
        str(bench_file),
        "--d8",
        str(d8),
        "--warmup",
        str(cfg.get("warmup", 5000)),
        "--iterations",
        str(cfg.get("iterations", 8000)),
        "--inner-loop",
        str(cfg.get("inner_loop", 80)),
        "--repeats",
        str(cfg.get("repeats", 40)),
        "--drop-first",
        str(cfg.get("drop_first", 8)),
        "--mad-z",
        str(cfg.get("mad_z", 3.0)),
        "--sleep-ms",
        str(cfg.get("sleep_ms", 25)),
        "--nice",
        str(cfg.get("nice", 0)),
        "--stability-cv-threshold",
        str(cfg.get("stability_cv_threshold", 0.03)),
        "--perf-events",
        str(cfg.get("perf_events", "cycles")),
    ]

    if cfg.get("allow_natives_syntax", True):
        command.append("--allow-natives-syntax")
    if cfg.get("trace_opt", True):
        command.append("--trace-opt")
    if cfg.get("print_opt_code", False):
        command.append("--print-opt-code")
    if cfg.get("force_gc_before_measure", True):
        command.append("--force-gc-before-measure")
    if cfg.get("cpu_core") is not None:
        command.extend(["--cpu-core", str(cfg["cpu_core"])])

    for flag in cfg.get("extra_flags", []):
        command.append(f"--extra-flag={flag}")

    if with_metadata:
        metadata_dir = str(cfg["metadata_dir"])
        command.append(f"--extra-flag=--turbo_metadata_path={metadata_dir}")

    return command


def run_mode(root: Path, bench_file: Path, d8: Path, cfg: dict, with_metadata: bool) -> tuple[Path, dict]:
    command = build_run_bench_command(root, bench_file, d8, cfg, with_metadata)
    result = subprocess.run(command, capture_output=True, text=True, cwd=str(root))
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        raise RuntimeError(
            f"benchmark run failed for {bench_file.name} ({'with' if with_metadata else 'without'} metadata), exit={result.returncode}"
        )
    result_path = parse_result_path(result.stdout)
    payload = load_json(result_path)
    return result_path, payload


def append_result_log(log_path: Path, record: dict) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("a", encoding="utf-8") as handle:
        handle.write(json.dumps(record, ensure_ascii=False) + "\n")


def discover_benches(bench_arg: Path | None, bench_dir: Path) -> list[Path]:
    if bench_arg:
        return [bench_arg]
    return sorted([*bench_dir.glob("*.js"), *bench_dir.glob("*.ts")])


def prepare_bench(
    root: Path,
    bench_file: Path,
    metadata_dir: Path,
    tmp_dir: Path,
    use_wrapper_preprocess: bool,
) -> tuple[Path, bool]:
    if bench_file.suffix != ".ts":
        return bench_file, False

    preprocess_script = root / "scripts" / "ts_locals_to_wrappers.js"
    ts_to_metadata = root / "scripts" / "ts_to_metadata.js"
    preprocessed_dir = tmp_dir / "preprocessed"
    tsc_out_dir = tmp_dir / "tsc"
    preprocessed_dir.mkdir(parents=True, exist_ok=True)
    tsc_out_dir.mkdir(parents=True, exist_ok=True)

    source_ts_for_metadata = bench_file
    output_js_stem = bench_file.stem
    if use_wrapper_preprocess:
        preprocessed_ts = preprocessed_dir / f"{bench_file.stem}.locals.ts"
        preprocess_command = [
            "node",
            str(preprocess_script),
            str(bench_file),
            "--out",
            str(preprocessed_ts),
        ]
        preprocess_result = subprocess.run(preprocess_command, capture_output=True, text=True, cwd=str(root))
        if preprocess_result.returncode != 0:
            print(preprocess_result.stdout)
            print(preprocess_result.stderr, file=sys.stderr)
            raise RuntimeError(f"ts_locals_to_wrappers failed for {bench_file.name}")
        source_ts_for_metadata = preprocessed_ts
        output_js_stem = f"{bench_file.stem}.locals"

    command = [
        "node",
        str(ts_to_metadata),
        str(source_ts_for_metadata),
        "--outDir",
        str(tsc_out_dir),
        "--metadataDir",
        str(metadata_dir),
    ]
    result = subprocess.run(command, capture_output=True, text=True, cwd=str(root))
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        raise RuntimeError(f"ts_to_metadata failed for {bench_file.name}")

    js_file = tsc_out_dir / f"{output_js_stem}.js"
    if not js_file.exists():
        raise RuntimeError(f"compiled JS not found: {js_file}")
    return js_file, True


def main() -> int:
    parser = argparse.ArgumentParser(description="Run benchmarks with global defaults + optional per-file overrides")
    parser.add_argument("--bench", help="Single benchmark file path (optional)")
    parser.add_argument("--d8", help="Path to d8 binary (optional, default from config)")
    parser.add_argument("--config", default="scripts/bench/config/defaults.json", help="Global default config JSON")
    parser.add_argument(
        "--overrides",
        default="scripts/bench/config/overrides.json",
        help="Per-file overrides JSON (optional)",
    )
    parser.add_argument(
        "--no-wrapper-preprocess",
        action="store_true",
        help="Disable ts_locals_to_wrappers preprocessing for TS benchmarks",
    )
    args = parser.parse_args()

    root = repo_root()
    config_path = resolve_path(args.config, root)
    if config_path is None or not config_path.exists():
        print(f"config file not found: {config_path}")
        return 2

    defaults = load_json(config_path)
    overrides_path = resolve_path(args.overrides, root)
    overrides = load_json(overrides_path) if overrides_path and overrides_path.exists() else {}

    bench_dir = resolve_path(defaults.get("bench_dir", "scripts/bench/benchmarks"), root)
    metadata_dir = resolve_path(defaults.get("metadata_dir", "scripts/bench/metadata"), root)
    d8 = resolve_path(args.d8 or defaults.get("d8", "out.gn/x64.release/d8"), root)
    bench_arg = resolve_path(args.bench, root)

    for path in [bench_dir, metadata_dir, d8]:
        if path is None or not path.exists():
            print(f"path not found: {path}")
            return 2
    if bench_arg and not bench_arg.exists():
        print(f"bench not found: {bench_arg}")
        return 2

    benches = discover_benches(bench_arg, bench_dir)
    if not benches:
        print("no benchmark files found")
        return 2

    tmp_dir = root / "tmp" / "bench"
    tmp_dir.mkdir(parents=True, exist_ok=True)
    run_id = time.strftime("%Y%m%d-%H%M%S")
    result_log = tmp_dir / "bench-results.jsonl"

    suite_report = {
        "run_id": run_id,
        "config": defaults,
        "results": [],
    }

    for bench_file in benches:
        per_file_override = overrides.get(bench_file.name, {})
        cfg = merge_config(defaults, per_file_override)
        cfg["metadata_dir"] = str(metadata_dir)
        use_wrapper_preprocess = bool(cfg.get("preprocess_ts_locals_wrappers", True)) and (not args.no_wrapper_preprocess)

        bench_js, generated = prepare_bench(root, bench_file, metadata_dir, tmp_dir, use_wrapper_preprocess)
        file_hash = sha256_file(bench_js)
        metadata_file = metadata_dir / f"{file_hash}.metadata"
        require_metadata = bool(cfg.get("require_metadata", True))
        if require_metadata and not metadata_file.exists():
            raise RuntimeError(f"metadata missing for {bench_file.name}: {metadata_file}")

        print(f"\n== Benchmark: {bench_file.name} ==")
        print(f"Metadata: {metadata_file}")

        without_path, without_data = run_mode(root, bench_js, d8, cfg, with_metadata=False)
        with_path, with_data = run_mode(root, bench_js, d8, cfg, with_metadata=True)

        events = [e.strip() for e in str(cfg.get("perf_events", "cycles")).split(",") if e.strip()]
        compare = {}
        print("Comparison (median):")
        for event in events:
            base = median_of(without_data, event)
            opt = median_of(with_data, event)
            base_cv = cv_of(without_data, event)
            opt_cv = cv_of(with_data, event)
            if base is None or opt is None:
                continue
            improvement = ((base - opt) / base * 100.0) if base else 0.0
            compare[event] = {
                "without_metadata": base,
                "with_metadata": opt,
                "without_cv": base_cv,
                "with_cv": opt_cv,
                "improvement_percent": improvement,
            }
            print(
                f"  {event}: without={base} (cv={base_cv}) with={opt} (cv={opt_cv}) improvement={improvement:.2f}%"
            )

        append_result_log(
            result_log,
            {
                "run_id": run_id,
                "bench": str(bench_file),
                "bench_js": str(bench_js),
                "hash": file_hash,
                "metadata": str(metadata_file),
                "comparison": compare,
                "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            },
        )

        suite_report["results"].append(
            {
                "bench": str(bench_file),
                "bench_js": str(bench_js),
                "generated_js": generated,
                "hash": file_hash,
                "metadata": str(metadata_file),
                "without_metadata_result": str(without_path),
                "with_metadata_result": str(with_path),
                "comparison": compare,
                "override": per_file_override,
            }
        )

    report_path = tmp_dir / f"pipeline-report-{run_id}.json"
    report_path.write_text(json.dumps(suite_report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"\nSuite report saved to: {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
