#!/usr/bin/env python3
import argparse
import json
import re
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def ensure_tmp_dir() -> Path:
    tmp_dir = repo_root() / "tmp" / "bench"
    tmp_dir.mkdir(parents=True, exist_ok=True)
    return tmp_dir


def build_harness(
    bench_path: Path,
    warmup: int,
    iterations: int,
    inner_loop: int,
    use_js_timer: bool,
    force_gc_before_measure: bool,
) -> str:
    bench_literal = json.dumps(str(bench_path))
    timing_code = ""
    if use_js_timer:
        timing_code = """
var __t0 = Date.now();
"""
    timing_code_end = ""
    if use_js_timer:
        timing_code_end = """
var __t1 = Date.now();
print('JS_TIME_MS:' + (__t1 - __t0));
"""

    gc_code = ""
    if force_gc_before_measure:
        gc_code = """
if (typeof gc === 'function') {
  gc();
}
"""

    return f"""
load({bench_literal});

if (typeof bench !== 'function') {{
  throw new Error('bench() function is required');
}}

if (typeof setup === 'function') {{
  setup();
}}

function __runLoop(count, inner) {{
  for (let i = 0; i < count; i++) {{
    for (let j = 0; j < inner; j++) {{
      bench();
    }}
  }}
}}

__runLoop({warmup}, {inner_loop});

{gc_code}

{timing_code}
__runLoop({iterations}, {inner_loop});
{timing_code_end}

if (typeof teardown === 'function') {{
  teardown();
}}

print('BENCH_ITERS:' + ({iterations} * {inner_loop}));
print('BENCH_DONE');
"""


def parse_perf_output(stderr: str, events: set[str]) -> dict:
    results: dict[str, float] = {}
    for line in stderr.splitlines():
        parts = [p.strip() for p in line.split(",")]
        if len(parts) < 3:
            continue
        value, _, event = parts[0], parts[1], parts[2]
        if event not in events:
            continue
        if value in ("<not supported>", "<not counted>"):
            continue
        value = value.replace(" ", "").replace(",", "")
        if not value:
            continue
        try:
            results[event] = float(value)
        except ValueError:
            continue
    return results


def filter_outliers(values: list[float], z: float = 3.5) -> tuple[list[float], list[float]]:
    if len(values) < 5:
        return values, []
    median = statistics.median(values)
    abs_dev = [abs(x - median) for x in values]
    mad = statistics.median(abs_dev)
    if mad == 0:
        return values, []
    scaled = [0.6745 * (x - median) / mad for x in values]
    kept = [x for x, s in zip(values, scaled) if abs(s) <= z]
    dropped = [x for x, s in zip(values, scaled) if abs(s) > z]
    return kept, dropped


def compute_stats(values: list[float]) -> dict:
    if not values:
        return {
            "count": 0,
            "mean": None,
            "median": None,
            "stdev": None,
            "min": None,
            "max": None,
        }
    return {
        "count": len(values),
        "mean": statistics.fmean(values),
        "median": statistics.median(values),
        "stdev": statistics.pstdev(values),
        "cv": (statistics.pstdev(values) / statistics.fmean(values)) if statistics.fmean(values) else None,
        "min": min(values),
        "max": max(values),
    }


def resolve_d8_path(path: str | None) -> str:
    if path:
        return path
    for candidate in ["out.gn/x64.release/d8", "out.gn/x64.debug/d8", "out/x64.release/d8", "out/x64.debug/d8"]:
        candidate_path = repo_root() / candidate
        if candidate_path.exists():
            return str(candidate_path)
    return "d8"


def build_exec_cmd(cmd: list[str], cpu_core: int | None, nice_level: int | None) -> list[str]:
    wrapped = cmd
    if cpu_core is not None:
        wrapped = ["taskset", "-c", str(cpu_core)] + wrapped
    if nice_level is not None:
        wrapped = ["nice", "-n", str(nice_level)] + wrapped
    return wrapped


def run_once(
    cmd: list[str],
    use_perf: bool,
    perf_events: list[str],
    verbose: bool,
    cpu_core: int | None,
    nice_level: int | None,
) -> tuple[int, str, str]:
    if use_perf:
        perf_cmd = [
            "perf",
            "stat",
            "-x",
            ",",
            "-e",
            ",".join(perf_events),
            "--",
        ] + build_exec_cmd(cmd, cpu_core, nice_level)
        result = subprocess.run(perf_cmd, capture_output=True, text=True)
        if verbose and result.stdout:
            print(result.stdout)
        if verbose and result.stderr:
            print(result.stderr, file=sys.stderr)
        return result.returncode, result.stdout, result.stderr

    plain_cmd = build_exec_cmd(cmd, cpu_core, nice_level)
    result = subprocess.run(plain_cmd, capture_output=True, text=True)
    if verbose and result.stdout:
        print(result.stdout)
    if verbose and result.stderr:
        print(result.stderr, file=sys.stderr)
    return result.returncode, result.stdout, result.stderr


def main() -> int:
    parser = argparse.ArgumentParser(description="V8 micro-benchmark runner with warmup and statistics")
    parser.add_argument("--bench", required=True, help="Benchmark JS file (must define bench())")
    parser.add_argument("--d8", help="Path to d8 binary")
    parser.add_argument("--warmup", type=int, default=1000, help="Warmup loop count")
    parser.add_argument("--iterations", type=int, default=5000, help="Measurement loop count")
    parser.add_argument("--inner-loop", type=int, default=50, help="Inner loop calls per iteration")
    parser.add_argument("--repeats", type=int, default=30, help="Number of repeated runs")
    parser.add_argument("--drop-first", type=int, default=2, help="Drop first N runs before outlier filtering")
    parser.add_argument("--mad-z", type=float, default=3.5, help="MAD z-score threshold for outlier filtering")
    parser.add_argument("--sleep-ms", type=int, default=20, help="Sleep milliseconds between repeats")
    parser.add_argument("--cpu-core", type=int, help="Pin benchmark process to a CPU core via taskset")
    parser.add_argument("--nice", type=int, default=0, help="Run with Unix nice level (higher is lower priority)")
    parser.add_argument("--stability-cv-threshold", type=float, default=0.03, help="Warn if CV (stdev/mean) is above threshold")
    parser.add_argument("--force-gc-before-measure", action="store_true", help="Call gc() before measurement loop (needs --expose-gc)")
    parser.add_argument("--use-js-timer", action="store_true", help="Use JS timer instead of perf (not recommended)")
    parser.add_argument("--require-perf", action="store_true", help="Fail if perf is not available")
    parser.add_argument("--perf-events", default="cycles,instructions", help="perf events list")
    parser.add_argument("--trace-opt", action="store_true", help="Enable --trace-opt and --trace-deopt")
    parser.add_argument("--print-opt-code", action="store_true", help="Enable --print-opt-code --code-comments")
    parser.add_argument("--allow-natives-syntax", action="store_true", default=True, help="Enable --allow-natives-syntax")
    parser.add_argument("--extra-flag", action="append", default=[], help="Extra flag for d8 (repeatable)")
    parser.add_argument("--verbose", action="store_true", help="Print stdout/stderr from each run")
    args = parser.parse_args()

    bench_path = Path(args.bench).resolve()
    if not bench_path.exists():
        print(f"Bench file not found: {bench_path}")
        return 2

    d8_path = resolve_d8_path(args.d8)
    if shutil.which(d8_path) is None and not Path(d8_path).exists():
        print(f"d8 not found: {d8_path}")
        return 2

    tmp_dir = ensure_tmp_dir()
    run_id = time.strftime("%Y%m%d-%H%M%S")
    harness_path = tmp_dir / f"harness-{run_id}.js"
    harness_path.write_text(
        build_harness(
            bench_path,
            args.warmup,
            args.iterations,
            args.inner_loop,
            args.use_js_timer,
            args.force_gc_before_measure,
        ),
        encoding="utf-8",
    )

    d8_flags = []
    if args.allow_natives_syntax:
        d8_flags.append("--allow-natives-syntax")
    if args.trace_opt:
        d8_flags.extend(["--trace-opt", "--trace-deopt"])
    if args.print_opt_code:
        d8_flags.extend(["--print-opt-code", "--code-comments"])
    if args.extra_flag:
        d8_flags.extend(args.extra_flag)

    cmd = [d8_path] + d8_flags + [str(harness_path)]

    use_perf = not args.use_js_timer and shutil.which("perf") is not None
    if args.require_perf and not use_perf:
        print("perf not found in PATH, but --require-perf was set")
        return 2

    if args.cpu_core is not None and shutil.which("taskset") is None:
        print("taskset not found, but --cpu-core was set")
        return 2

    perf_events = [e.strip() for e in args.perf_events.split(",") if e.strip()]
    event_set = set(perf_events)

    results: dict[str, list[float]] = {event: [] for event in perf_events}
    js_times: list[float] = []
    per_run: list[dict] = []

    for i in range(args.repeats):
        code, stdout, stderr = run_once(
            cmd,
            use_perf,
            perf_events,
            args.verbose,
            args.cpu_core,
            args.nice,
        )
        if code != 0:
            print(f"Run #{i} failed with exit code {code}")
            if not args.verbose:
                print(stdout)
                print(stderr, file=sys.stderr)
            return 1

        if use_perf:
            parsed = parse_perf_output(stderr, event_set)
            run_item = {"index": i, "metrics": parsed}
            for event, value in parsed.items():
                results[event].append(value)
            per_run.append(run_item)
        else:
            for line in stdout.splitlines():
                if line.startswith("JS_TIME_MS:"):
                    value = float(line.split(":", 1)[1])
                    js_times.append(value)
                    per_run.append({"index": i, "metrics": {"js_time_ms": value}})

        if args.sleep_ms > 0 and i + 1 < args.repeats:
            time.sleep(args.sleep_ms / 1000.0)

    effective_start = min(max(args.drop_first, 0), args.repeats)

    summary = {
        "bench": str(bench_path),
        "d8": d8_path,
        "warmup": args.warmup,
        "iterations": args.iterations,
        "inner_loop": args.inner_loop,
        "repeats": args.repeats,
        "drop_first": args.drop_first,
        "effective_repeats": args.repeats - effective_start,
        "perf_events": perf_events,
        "use_perf": use_perf,
        "cpu_core": args.cpu_core,
        "nice": args.nice,
        "sleep_ms": args.sleep_ms,
        "mad_z": args.mad_z,
        "stability_cv_threshold": args.stability_cv_threshold,
        "force_gc_before_measure": args.force_gc_before_measure,
        "per_run": per_run,
        "results": {},
        "js_times": js_times,
    }

    if use_perf:
        for event, values in results.items():
            values_after_drop = values[effective_start:]
            filtered, dropped = filter_outliers(values_after_drop, z=args.mad_z)
            stats = compute_stats(filtered)
            summary["results"][event] = {
                "raw": values,
                "after_drop_first": values_after_drop,
                "filtered": filtered,
                "dropped": dropped,
                "stats": stats,
                "stability_ok": (stats["cv"] is None) or (stats["cv"] <= args.stability_cv_threshold),
            }
    else:
        values_after_drop = js_times[effective_start:]
        filtered, dropped = filter_outliers(values_after_drop, z=args.mad_z)
        stats = compute_stats(filtered)
        summary["results"]["js_time_ms"] = {
            "raw": js_times,
            "after_drop_first": values_after_drop,
            "filtered": filtered,
            "dropped": dropped,
            "stats": stats,
            "stability_ok": (stats["cv"] is None) or (stats["cv"] <= args.stability_cv_threshold),
        }

    result_path = tmp_dir / f"results-{run_id}.json"
    result_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")

    print("Benchmark summary:")
    if use_perf:
        for event in perf_events:
            stats = summary["results"][event]["stats"]
            stability_ok = summary["results"][event]["stability_ok"]
            stability_tag = "OK" if stability_ok else "NOISY"
            print(
                f"  {event}: median={stats['median']} mean={stats['mean']} stdev={stats['stdev']} cv={stats['cv']} (n={stats['count']}) [{stability_tag}]"
            )
    else:
        stats = summary["results"]["js_time_ms"]["stats"]
        stability_ok = summary["results"]["js_time_ms"]["stability_ok"]
        stability_tag = "OK" if stability_ok else "NOISY"
        print(
            f"  js_time_ms: median={stats['median']} mean={stats['mean']} stdev={stats['stdev']} cv={stats['cv']} (n={stats['count']}) [{stability_tag}]"
        )

    print(f"Results saved to: {result_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
