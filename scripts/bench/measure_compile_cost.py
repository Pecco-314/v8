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
    content = f"""
load({bench_js!r});

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


def timed_run(command: list[str], root: Path) -> tuple[float, int]:
    start = time.perf_counter()
    result = subprocess.run(command, capture_output=True, text=True, cwd=str(root))
    elapsed = time.perf_counter() - start
    return elapsed, result.returncode


def main() -> int:
    parser = argparse.ArgumentParser(description="Measure compile overhead with/without metadata")
    parser.add_argument("--bench", help="Single benchmark path")
    parser.add_argument("--d8", help="Path to d8")
    parser.add_argument("--config", default="scripts/bench/config/defaults.json")
    parser.add_argument("--overrides", default="scripts/bench/config/overrides.json")
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
        without_cmd = build_d8_command(d8, harness, [])
        with_cmd = build_d8_command(d8, harness, [f"--turbo_metadata_path={metadata_dir}"])

        without_time, without_code = timed_run(without_cmd, root)
        with_time, with_code = timed_run(with_cmd, root)

        record = {
            "run_id": run_id,
            "bench": str(bench_file),
            "bench_js": str(bench_js),
            "hash": file_hash,
            "metadata": str(metadata_file),
            "without_time_sec": without_time,
            "with_time_sec": with_time,
            "delta_sec": with_time - without_time,
            "delta_percent": ((with_time - without_time) / without_time * 100.0) if without_time else None,
            "without_exit": without_code,
            "with_exit": with_code,
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
