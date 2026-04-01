#!/usr/bin/env python3
import argparse
import hashlib
import json
import re
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Set, Tuple


@dataclass
class AssemblyRunResult:
    mode: str
    returncode: int
    stdout: str
    stderr: str
    asm: Optional[str]
    code_size: Optional[int]
    normalized_lines: List[str]


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def load_json(path: Path) -> Dict:
    return json.loads(path.read_text(encoding="utf-8"))


def ensure_dict(value: object) -> Dict:
    return value if isinstance(value, dict) else {}


def resolve_path(path_value: Optional[str], root: Path) -> Optional[Path]:
    if path_value is None:
        return None
    path = Path(path_value)
    return path if path.is_absolute() else (root / path)


def sha256_file(file_path: Path) -> str:
    return hashlib.sha256(file_path.read_bytes()).hexdigest()


def remove_ansi_codes(text: str) -> str:
    return re.compile(r"\x1b\[[0-9;]*m").sub("", text)


def normalize_asm_line(line: str) -> str:
    normalized = re.sub(r"0x[0-9a-fA-F]+", "ADDR", line)
    normalized = re.sub(r"\s+", " ", normalized).strip()
    return normalized


def parse_instruction_size(code_block: str) -> Optional[int]:
    match = re.search(r"Instructions \(size = (\d+)\)", code_block)
    if not match:
        return None
    return int(match.group(1))


def extract_code_blocks(output_text: str) -> List[str]:
    clean = remove_ansi_codes(output_text)
    return re.findall(r"--- Optimized code ---.*?--- End code ---", clean, flags=re.DOTALL)


def extract_assembly(output_text: str, function_name: str) -> Tuple[Optional[str], Optional[int]]:
    function_pattern = re.compile(rf"^name\s*=\s*{re.escape(function_name)}\s*$", flags=re.MULTILINE)
    for block in extract_code_blocks(output_text):
        if not function_pattern.search(block):
            continue
        instructions = re.search(
            r"Instructions \(size = \d+\)(.*?)(?=\n--- End code ---)",
            block,
            flags=re.DOTALL,
        )
        if not instructions:
            continue
        asm_text = instructions.group(1).strip()
        return asm_text, parse_instruction_size(block)
    return None, None


def extract_function_name_from_block(code_block: str) -> Optional[str]:
    match = re.search(r"^name\s*=\s*([^\n]+)$", code_block, flags=re.MULTILINE)
    if not match:
        return None
    return match.group(1).strip()


def extract_all_assemblies(
    output_text: str,
    allowed_names: Optional[Set[str]] = None,
) -> Dict[str, Tuple[str, Optional[int]]]:
    assemblies: Dict[str, Tuple[str, Optional[int]]] = {}
    for block in extract_code_blocks(output_text):
        function_name = extract_function_name_from_block(block)
        if not function_name:
            continue
        if allowed_names is not None and function_name not in allowed_names:
            continue
        instructions = re.search(
            r"Instructions \(size = \d+\)(.*?)(?=\n--- End code ---)",
            block,
            flags=re.DOTALL,
        )
        if not instructions:
            continue
        asm_text = instructions.group(1).strip()
        assemblies[function_name] = (asm_text, parse_instruction_size(block))
    return assemblies


def discover_business_functions(bench_js: Path) -> Set[str]:
    source = bench_js.read_text(encoding="utf-8")
    names: Set[str] = set()

    for match in re.finditer(r"\bfunction\s+([A-Za-z_$][\w$]*)\s*\(", source):
        names.add(match.group(1))

    for match in re.finditer(r"\b(class|interface)\s+([A-Za-z_$][\w$]*)\b", source):
        names.add(match.group(2))

    for match in re.finditer(r"(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*(?:function\s*\(|\([^)]*\)\s*=>)", source):
        names.add(match.group(1))

    filtered = {
        name
        for name in names
        if not name.startswith("__ti_local_wrap_")
        and not name.startswith("__")
        and name not in {"exports", "module", "require"}
    }
    return filtered


def write_harness(
    tmp_dir: Path,
    bench_js: Path,
    function_name: str,
    setup_name: str,
    teardown_name: str,
    invoke_expr: Optional[str],
    prewarm_exprs: Optional[Sequence[str]] = None,
    warmup_calls: int = 2,
) -> Path:
    harness_path = tmp_dir / f"harness-asm-{bench_js.stem}-{function_name}.js"
    bench_literal = json.dumps(str(bench_js))
    fn_literal = json.dumps(function_name)
    setup_literal = json.dumps(setup_name)
    teardown_literal = json.dumps(teardown_name)
    invoke_expr_literal = invoke_expr if invoke_expr else "benchFn()"
    prewarm_exprs = list(prewarm_exprs or [])
    prewarm_block = "\n".join(f"  ({expr});" for expr in prewarm_exprs)
    warmup_calls = max(1, int(warmup_calls))
    harness = f"""
load({bench_literal});

const benchFn = globalThis[{fn_literal}];
const setupFn = globalThis[{setup_literal}];
const teardownFn = globalThis[{teardown_literal}];

if (typeof benchFn !== 'function') {{
  throw new Error('Target function not found: ' + {fn_literal});
}}

if (typeof setupFn === 'function') setupFn();

function __invokeTarget() {{
    return ({invoke_expr_literal});
}}

function __runPrewarm() {{
{prewarm_block}
}}

__runPrewarm();

%PrepareFunctionForOptimization(benchFn);
for (let i = 0; i < {warmup_calls}; i++) {{
    __invokeTarget();
}}
%OptimizeFunctionOnNextCall(benchFn);
__invokeTarget();

if (typeof teardownFn === 'function') teardownFn();

print('BENCH_ASM_DONE');
""".strip()
    harness_path.write_text(harness + "\n", encoding="utf-8")
    return harness_path


def prepare_ts_bench(root: Path, bench_file: Path, metadata_dir: Path, tmp_dir: Path, use_wrapper_preprocess: bool) -> Path:
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
            raise RuntimeError(
                "ts_locals_to_wrappers failed\n"
                f"command: {' '.join(preprocess_command)}\n"
                f"stdout:\n{preprocess_result.stdout}\n"
                f"stderr:\n{preprocess_result.stderr}"
            )
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
        raise RuntimeError(
            "ts_to_metadata failed\n"
            f"command: {' '.join(command)}\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )

    js_file = tsc_out_dir / f"{output_js_stem}.js"
    if not js_file.exists():
        raise RuntimeError(f"compiled JS not found: {js_file}")
    return js_file


def build_d8_flags(defaults: Dict, function_name: str, trace_ir: bool, use_filter: bool = True) -> List[str]:
    flags: List[str] = ["--turbofan", "--print-opt-code"]
    if use_filter:
        flags.append(f"--print-opt-code-filter={function_name}")

    if defaults.get("allow_natives_syntax", True):
        flags.append("--allow-natives-syntax")
    if defaults.get("trace_opt", True):
        flags.append("--trace-opt")

    extra_flags = defaults.get("extra_flags", [])
    for flag in extra_flags:
        if flag not in flags:
            flags.append(flag)

    required_stable_flags = ["--no-concurrent-recompilation", "--no-maglev"]
    for flag in required_stable_flags:
        if flag not in flags:
            flags.append(flag)

    if trace_ir:
        trace_flags = ["--trace-turbo", "--trace-turbo-reduction", f"--trace-turbo-filter={function_name}"]
        for flag in trace_flags:
            if flag not in flags:
                flags.append(flag)
    return flags


def run_d8(d8_path: Path, harness: Path, flags: Sequence[str]) -> Tuple[int, str, str]:
    command = [str(d8_path)] + list(flags) + [str(harness)]
    result = subprocess.run(command, capture_output=True, text=True)
    return result.returncode, result.stdout, result.stderr


def normalize_lines(asm: Optional[str]) -> List[str]:
    if not asm:
        return []
    return [normalize_asm_line(line) for line in asm.splitlines() if line.strip()]


def diff_line_count(left: List[str], right: List[str]) -> int:
    min_len = min(len(left), len(right))
    mismatch = sum(1 for i in range(min_len) if left[i] != right[i])
    return mismatch + abs(len(left) - len(right))


def write_failure_log(log_path: Path, command: Sequence[str], returncode: int, stdout: str, stderr: str) -> None:
    payload = [
        f"command: {' '.join(command)}",
        f"exit_code: {returncode}",
        "",
        "[stdout]",
        stdout,
        "",
        "[stderr]",
        stderr,
    ]
    log_path.write_text("\n".join(payload), encoding="utf-8")


def generate_markdown_report(
    bench_name: str,
    source_file: Path,
    source_hash: str,
    metadata_file: Optional[Path],
    without_result: AssemblyRunResult,
    with_result: AssemblyRunResult,
) -> str:
    lines = [f"# {bench_name} 二进制代码对比", ""]
    lines.append(f"- Source: `{source_file}`")
    lines.append(f"- SHA256: `{source_hash}`")
    lines.append(f"- Metadata: `{metadata_file}`" if metadata_file else "- Metadata: `disabled`")
    lines.append("")

    lines.append("## 结果")
    lines.append("")
    lines.append(f"- without metadata: exit={without_result.returncode}, asm={'yes' if without_result.asm else 'no'}")
    lines.append(f"- with metadata: exit={with_result.returncode}, asm={'yes' if with_result.asm else 'no'}")

    if without_result.asm and with_result.asm:
        line_delta = len(with_result.normalized_lines) - len(without_result.normalized_lines)
        changed_lines = diff_line_count(without_result.normalized_lines, with_result.normalized_lines)
        size_without = without_result.code_size
        size_with = with_result.code_size
        lines.append(f"- 指令行数: {len(without_result.normalized_lines)} -> {len(with_result.normalized_lines)} ({line_delta:+d})")
        if size_without is not None and size_with is not None:
            lines.append(f"- 指令字节: {size_without} -> {size_with} ({size_with - size_without:+d})")
        lines.append(f"- 归一化差异行数: {changed_lines}")
    else:
        lines.append("- ⚠️ 未能同时提取两个模式的汇编输出")

    return "\n".join(lines) + "\n"


def generate_json_report(
    bench_name: str,
    source_file: Path,
    source_hash: str,
    metadata_file: Optional[Path],
    without_result: AssemblyRunResult,
    with_result: AssemblyRunResult,
) -> Dict:
    report = {
        "benchmark": bench_name,
        "source": str(source_file),
        "sha256": source_hash,
        "metadata": str(metadata_file) if metadata_file else None,
        "without_metadata": {
            "exit_code": without_result.returncode,
            "asm_extracted": bool(without_result.asm),
            "instruction_lines": len(without_result.normalized_lines),
            "instruction_size": without_result.code_size,
        },
        "with_metadata": {
            "exit_code": with_result.returncode,
            "asm_extracted": bool(with_result.asm),
            "instruction_lines": len(with_result.normalized_lines),
            "instruction_size": with_result.code_size,
        },
    }

    if without_result.asm and with_result.asm:
        report["diff"] = {
            "line_delta": len(with_result.normalized_lines) - len(without_result.normalized_lines),
            "changed_lines": diff_line_count(without_result.normalized_lines, with_result.normalized_lines),
            "instruction_size_delta": (
                (with_result.code_size - without_result.code_size)
                if with_result.code_size is not None and without_result.code_size is not None
                else None
            ),
        }
    return report


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate benchmark binary assembly with/without metadata")
    parser.add_argument("--bench", help="Single benchmark file path (.ts or .js)")
    parser.add_argument("--d8", help="Path to d8 binary")
    parser.add_argument("--config", default="scripts/bench/config/defaults.json", help="Path to defaults config")
    parser.add_argument(
        "--targets-config",
        default="scripts/bench/config/hotspots.json",
        help="Per-benchmark target function config (function/invoke_expr/setup/teardown/prewarm_exprs/warmup_calls)",
    )
    parser.add_argument("--out-dir", default="docs/assembly/benchmarks", help="Assembly output root")
    parser.add_argument(
        "--metadata-mode",
        choices=["strict", "off"],
        default="strict",
        help="strict: require metadata and compare both modes; off: only generate without metadata",
    )
    parser.add_argument("--function", default=None, help="Override target function name for all benchmarks")
    parser.add_argument(
        "--invoke-expr",
        default=None,
        help="Custom JS invocation expression used for warmup/optimize calls (e.g. matmul(left,right,result))",
    )
    parser.add_argument("--setup", default=None, help="Override setup function name for all benchmarks")
    parser.add_argument("--teardown", default=None, help="Override teardown function name for all benchmarks")
    parser.add_argument(
        "--warmup-calls",
        type=int,
        default=None,
        help="Override warmup invoke count before OptimizeFunctionOnNextCall",
    )
    parser.add_argument("--trace-ir", action="store_true", help="Enable TurboFan IR trace and save with/without logs")
    parser.add_argument(
        "--no-wrapper-preprocess",
        action="store_true",
        help="Disable ts_locals_to_wrappers preprocessing for TS benchmarks",
    )
    parser.add_argument(
        "--stats-scope",
        choices=["target", "all-business"],
        default="target",
        help="target: 只统计目标函数；all-business: 统计所有已优化编译的业务函数（排除库函数）",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = repo_root()

    config_path = resolve_path(args.config, root)
    defaults = load_json(config_path) if config_path and config_path.exists() else {}
    targets_config_path = resolve_path(args.targets_config, root)
    targets_cfg = load_json(targets_config_path) if targets_config_path and targets_config_path.exists() else {}
    use_wrapper_preprocess = bool(defaults.get("preprocess_ts_locals_wrappers", True)) and (not args.no_wrapper_preprocess)

    bench_dir = resolve_path(defaults.get("bench_dir", "scripts/bench/benchmarks"), root)
    metadata_dir = resolve_path(defaults.get("metadata_dir", "scripts/bench/metadata"), root)
    d8_path = resolve_path(args.d8 or defaults.get("d8", "out.gn/x64.release/d8"), root)
    bench_arg = resolve_path(args.bench, root)
    out_dir = resolve_path(args.out_dir, root)

    if bench_dir is None or not bench_dir.exists():
        print(f"bench dir not found: {bench_dir}")
        return 2
    if d8_path is None or not d8_path.exists():
        print(f"d8 not found: {d8_path}")
        return 2
    if out_dir is None:
        print("out dir is invalid")
        return 2
    if args.metadata_mode == "strict":
        if metadata_dir is None:
            print("metadata dir is invalid")
            return 2
        metadata_dir.mkdir(parents=True, exist_ok=True)

    if bench_arg and not bench_arg.exists():
        print(f"bench not found: {bench_arg}")
        return 2

    benches = [bench_arg] if bench_arg else sorted([*bench_dir.glob("*.js"), *bench_dir.glob("*.ts")])
    if not benches:
        print("no benchmark files found")
        return 2

    out_dir.mkdir(parents=True, exist_ok=True)
    tmp_dir = root / "tmp" / "bench"
    tmp_dir.mkdir(parents=True, exist_ok=True)
    run_id = time.strftime("%Y%m%d-%H%M%S")
    log_dir = tmp_dir / "bench-asm-logs" / run_id
    log_dir.mkdir(parents=True, exist_ok=True)
    failures = 0

    for bench_file in benches:
        print(f"\n== Benchmark: {bench_file.name} ==")
        bench_js = bench_file
        if bench_file.suffix == ".ts":
            try:
                bench_js = prepare_ts_bench(root, bench_file, metadata_dir, tmp_dir, use_wrapper_preprocess)
            except Exception as exc:
                failures += 1
                print(f"ts_to_metadata failed: {bench_file}\\n{exc}")
                continue

        source_hash = sha256_file(bench_js)
        metadata_file = (metadata_dir / f"{source_hash}.metadata") if metadata_dir else None

        if args.metadata_mode == "strict":
            if metadata_file is None or not metadata_file.exists():
                failures += 1
                print(f"metadata missing: {metadata_file}")
                continue

        case_dir = out_dir / bench_file.stem
        case_dir.mkdir(parents=True, exist_ok=True)
        target = ensure_dict(targets_cfg.get(bench_file.stem, {}))
        target_function = args.function or target.get("function") or "bench"
        target_setup = args.setup or target.get("setup") or "setup"
        target_teardown = args.teardown or target.get("teardown") or "teardown"
        target_invoke_expr = args.invoke_expr or target.get("invoke_expr") or f"{target_function}()"
        target_warmup_calls = args.warmup_calls if args.warmup_calls is not None else int(target.get("warmup_calls", 2))
        raw_prewarm_exprs = target.get("prewarm_exprs", [])
        if isinstance(raw_prewarm_exprs, str):
            target_prewarm_exprs = [raw_prewarm_exprs]
        elif isinstance(raw_prewarm_exprs, list):
            target_prewarm_exprs = [expr for expr in raw_prewarm_exprs if isinstance(expr, str)]
        else:
            target_prewarm_exprs = []
        use_filter = args.stats_scope != "all-business"
        base_flags = build_d8_flags(defaults, target_function, trace_ir=args.trace_ir, use_filter=use_filter)
        target_extra_flags = target.get("extra_flags", [])
        if isinstance(target_extra_flags, list):
            for flag in target_extra_flags:
                if isinstance(flag, str) and flag and flag not in base_flags:
                    base_flags.append(flag)

        print(f"target function: {target_function}")
        print(f"invoke expr: {target_invoke_expr}")
        print(f"warmup calls: {target_warmup_calls}")
        if target_prewarm_exprs:
            print(f"prewarm expr count: {len(target_prewarm_exprs)}")
        if isinstance(target_extra_flags, list) and target_extra_flags:
            print(f"target extra flags: {target_extra_flags}")

        harness = write_harness(
            tmp_dir=tmp_dir,
            bench_js=bench_js,
            function_name=target_function,
            setup_name=target_setup,
            teardown_name=target_teardown,
            invoke_expr=target_invoke_expr,
            prewarm_exprs=target_prewarm_exprs,
            warmup_calls=target_warmup_calls,
        )

        business_functions = discover_business_functions(bench_js)

        cmd_without = [str(d8_path)] + base_flags + [str(harness)]
        rc_without, stdout_without, stderr_without = run_d8(d8_path, harness, base_flags)
        asm_without_map = extract_all_assemblies(
            stdout_without or "",
            allowed_names=business_functions if args.stats_scope == "all-business" else None,
        )
        asm_without, size_without = extract_assembly(stdout_without or "", target_function)
        if args.stats_scope == "all-business" and target_function in asm_without_map:
            asm_without, size_without = asm_without_map[target_function]
        without_result = AssemblyRunResult(
            mode="without",
            returncode=rc_without,
            stdout=stdout_without,
            stderr=stderr_without,
            asm=asm_without,
            code_size=size_without,
            normalized_lines=normalize_lines(asm_without),
        )

        if args.metadata_mode == "strict":
            metadata_flags: List[str] = []
            if metadata_dir:
                metadata_flags.append(f"--turbo_metadata_path={metadata_dir}")
            cmd_with = [str(d8_path)] + (base_flags + metadata_flags) + [str(harness)]
            rc_with, stdout_with, stderr_with = run_d8(d8_path, harness, base_flags + metadata_flags)
            asm_with_map = extract_all_assemblies(
                stdout_with or "",
                allowed_names=business_functions if args.stats_scope == "all-business" else None,
            )
            asm_with, size_with = extract_assembly(stdout_with or "", target_function)
            if args.stats_scope == "all-business" and target_function in asm_with_map:
                asm_with, size_with = asm_with_map[target_function]
            with_result = AssemblyRunResult(
                mode="with",
                returncode=rc_with,
                stdout=stdout_with,
                stderr=stderr_with,
                asm=asm_with,
                code_size=size_with,
                normalized_lines=normalize_lines(asm_with),
            )
        else:
            cmd_with = []
            asm_with_map = {}
            with_result = AssemblyRunResult(
                mode="with",
                returncode=0,
                stdout="",
                stderr="",
                asm=None,
                code_size=None,
                normalized_lines=[],
            )

        if args.stats_scope == "all-business":
            for function_name, (asm_text, _) in sorted(asm_without_map.items()):
                (case_dir / f"{function_name}_without.asm").write_text(asm_text + "\n", encoding="utf-8")
            for function_name, (asm_text, _) in sorted(asm_with_map.items()):
                (case_dir / f"{function_name}_with.asm").write_text(asm_text + "\n", encoding="utf-8")
        else:
            if without_result.asm:
                (case_dir / f"{target_function}_without.asm").write_text(without_result.asm + "\n", encoding="utf-8")
            if with_result.asm:
                (case_dir / f"{target_function}_with.asm").write_text(with_result.asm + "\n", encoding="utf-8")

        report_md = generate_markdown_report(
            bench_name=bench_file.stem,
            source_file=bench_js,
            source_hash=source_hash,
            metadata_file=metadata_file if args.metadata_mode == "strict" else None,
            without_result=without_result,
            with_result=with_result,
        )
        (case_dir / "comparison.md").write_text(report_md, encoding="utf-8")

        report_json = generate_json_report(
            bench_name=bench_file.stem,
            source_file=bench_js,
            source_hash=source_hash,
            metadata_file=metadata_file if args.metadata_mode == "strict" else None,
            without_result=without_result,
            with_result=with_result,
        )

        if args.stats_scope == "all-business":
            paired_functions = sorted(set(asm_without_map.keys()) & set(asm_with_map.keys()))
            only_without = sorted(set(asm_without_map.keys()) - set(asm_with_map.keys()))
            only_with = sorted(set(asm_with_map.keys()) - set(asm_without_map.keys()))
            aggregate_line_delta = 0
            aggregate_changed_lines = 0
            aggregate_instruction_delta = 0
            per_function = []

            for function_name in paired_functions:
                asm_wout, size_wout = asm_without_map[function_name]
                asm_w, size_w = asm_with_map[function_name]
                lines_wout = normalize_lines(asm_wout)
                lines_w = normalize_lines(asm_w)
                line_delta = len(lines_w) - len(lines_wout)
                changed_lines = diff_line_count(lines_wout, lines_w)
                if size_wout is not None and size_w is not None:
                    instruction_delta = size_w - size_wout
                else:
                    instruction_delta = 0

                aggregate_line_delta += line_delta
                aggregate_changed_lines += changed_lines
                aggregate_instruction_delta += instruction_delta
                per_function.append(
                    {
                        "function": function_name,
                        "without_instruction_lines": len(lines_wout),
                        "with_instruction_lines": len(lines_w),
                        "line_delta": line_delta,
                        "changed_lines": changed_lines,
                        "without_instruction_size": size_wout,
                        "with_instruction_size": size_w,
                        "instruction_size_delta": instruction_delta,
                    }
                )

            report_json["stats_scope"] = "all-business"
            report_json["optimized_business_functions"] = {
                "without_count": len(asm_without_map),
                "with_count": len(asm_with_map),
                "paired_count": len(paired_functions),
                "only_without": only_without,
                "only_with": only_with,
                "paired": per_function,
            }
            report_json["diff"] = {
                "line_delta": aggregate_line_delta,
                "changed_lines": aggregate_changed_lines,
                "instruction_size_delta": aggregate_instruction_delta,
            }
        (case_dir / "comparison.json").write_text(
            json.dumps(report_json, ensure_ascii=False, indent=2),
            encoding="utf-8",
        )

        if rc_without != 0 or not asm_without:
            write_failure_log(
                log_dir / f"{bench_file.stem}-without.log",
                command=cmd_without,
                returncode=rc_without,
                stdout=stdout_without,
                stderr=stderr_without,
            )

        if args.metadata_mode == "strict" and (with_result.returncode != 0 or not with_result.asm):
            write_failure_log(
                log_dir / f"{bench_file.stem}-with.log",
                command=cmd_with,
                returncode=with_result.returncode,
                stdout=with_result.stdout,
                stderr=with_result.stderr,
            )

        if args.trace_ir:
            (case_dir / f"{target_function}_without.ir.log").write_text(without_result.stdout + "\n" + without_result.stderr, encoding="utf-8")
            if args.metadata_mode == "strict":
                (case_dir / f"{target_function}_with.ir.log").write_text(with_result.stdout + "\n" + with_result.stderr, encoding="utf-8")

        print(f"output: {case_dir}")
        print(f"asm without: {'ok' if asm_without else 'missing'}")
        print(f"asm with: {'ok' if with_result.asm else 'missing'}")

        if args.metadata_mode == "strict":
            if without_result.returncode != 0 or with_result.returncode != 0 or not without_result.asm or not with_result.asm:
                failures += 1
        else:
            if without_result.returncode != 0 or not without_result.asm:
                failures += 1

    print(f"\nDone. Output: {out_dir}")
    print(f"Logs: {log_dir}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
