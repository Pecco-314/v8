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


def discover_business_functions(bench_js: Path, excluded_names: Optional[Set[str]] = None) -> Set[str]:
    source = bench_js.read_text(encoding="utf-8")
    names: Set[str] = set()
    excluded_names = set(excluded_names or set())

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
        and not re.fullmatch(r"[A-Z0-9_]+", name)
        and name not in {"exports", "module", "require"}
        and name not in excluded_names
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


def write_coverage_harness(
        tmp_dir: Path,
        bench_js: Path,
        setup_name: str,
        teardown_name: str,
        coverage_targets: Sequence[Dict[str, str]],
        warmup_calls: int,
) -> Path:
        harness_path = tmp_dir / f"harness-coverage-{bench_js.stem}.js"
        bench_literal = json.dumps(str(bench_js))
        setup_literal = json.dumps(setup_name)
        teardown_literal = json.dumps(teardown_name)
        warmup_calls = max(1, int(warmup_calls))

        target_blocks: List[str] = []
        for target in coverage_targets:
                fn_name = target["function"]
                invoke_expr = target["invoke_expr"]
                fn_name_literal = json.dumps(fn_name)
                target_blocks.append(
                        f"""
try {{
    const __fn = globalThis[{fn_name_literal}];
    if (typeof __fn !== 'function') {{
        print('COVERAGE_RESULT|{fn_name}|not_function');
    }} else {{
        %PrepareFunctionForOptimization(__fn);
        for (let i = 0; i < {warmup_calls}; i++) {{
            ({invoke_expr});
        }}
        %OptimizeFunctionOnNextCall(__fn);
        ({invoke_expr});
        print('COVERAGE_RESULT|{fn_name}|invoked');
    }}
}} catch (e) {{
    print('COVERAGE_RESULT|{fn_name}|invoke_failed:' + String(e));
}}
""".strip()
                )

        targets_script = "\n\n".join(target_blocks)
        harness = f"""
load({bench_literal});

const setupFn = globalThis[{setup_literal}];
const teardownFn = globalThis[{teardown_literal}];

if (typeof setupFn === 'function') setupFn();

{targets_script}

if (typeof teardownFn === 'function') teardownFn();

print('BENCH_COVERAGE_DONE');
""".strip()
        harness_path.write_text(harness + "\n", encoding="utf-8")
        return harness_path


def parse_compiled_methods(output_text: str) -> Set[str]:
    compiled: Set[str] = set()

    for name in re.findall(r"Finished compiling method\s+([^\s]+)\s+using TurboFan", output_text):
        normalized = name.strip()
        if normalized:
            compiled.add(normalized)

    for name in re.findall(r"completed optimizing\s+[^\n]*<JSFunction\s+([^\s>]+)", output_text):
        normalized = name.strip()
        if normalized:
            compiled.add(normalized)

    for block_name in extract_all_assemblies(output_text).keys():
        normalized = block_name.strip()
        if normalized:
            compiled.add(normalized)

    return compiled


def parse_coverage_results(output_text: str) -> Dict[str, str]:
        results: Dict[str, str] = {}
        for line in output_text.splitlines():
                if not line.startswith("COVERAGE_RESULT|"):
                        continue
                parts = line.split("|", 2)
                if len(parts) != 3:
                        continue
                _, function_name, status = parts
                results[function_name] = status
        return results


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


def build_d8_flags(
    defaults: Dict,
    function_name: str,
    trace_ir: bool,
    use_filter: bool = True,
    no_inline: bool = False,
) -> List[str]:
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

    if no_inline:
        no_inline_flags = [
            "--max-inlined-bytecode-size=0",
            "--max-inlined-bytecode-size-small=0",
            "--max-inlined-bytecode-size-cumulative=0",
            "--max-inlined-bytecode-size-absolute=0",
        ]
        for flag in no_inline_flags:
            if flag not in flags:
                flags.append(flag)

    if trace_ir:
        trace_flags = ["--trace-turbo", "--trace-turbo-reduction"]
        if use_filter:
            trace_flags.append(f"--trace-turbo-filter={function_name}")
        for flag in trace_flags:
            if flag not in flags:
                flags.append(flag)
    return flags


def run_d8(d8_path: Path, harness: Path, flags: Sequence[str]) -> Tuple[int, str, str]:
    command = [str(d8_path)] + list(flags) + [str(harness)]
    result = subprocess.run(command, capture_output=True, text=True)
    return result.returncode, result.stdout, result.stderr


def split_ir_logs_by_function(ir_text: str, allowed_names: Optional[Set[str]] = None) -> Dict[str, str]:
    chunks: Dict[str, List[str]] = {}
    current_name: Optional[str] = None

    begin_compile = re.compile(r"Begin compiling method\s+([^\s]+)\s+using TurboFan")

    for line in ir_text.splitlines():
        begin_match = begin_compile.search(line)
        if begin_match:
            current_name = begin_match.group(1).strip()
            if allowed_names is not None and current_name not in allowed_names:
                current_name = None
                continue
            chunks.setdefault(current_name, []).append(line)
            continue

        if current_name is not None:
            chunks[current_name].append(line)

    return {name: "\n".join(lines).strip() + "\n" for name, lines in chunks.items() if lines}


def clean_directory(path: Path) -> None:
    if not path.exists():
        return
    for child in path.iterdir():
        try:
            if child.is_file() or child.is_symlink():
                child.unlink()
            elif child.is_dir():
                clean_directory(child)
                child.rmdir()
        except OSError:
            continue


def parse_turbo_graph_function_name(graph: Dict) -> Optional[str]:
    function_info = graph.get("function") if isinstance(graph, dict) else None
    if not isinstance(function_info, dict):
        return None
    function_name = function_info.get("functionName")
    if isinstance(function_name, str) and function_name.strip():
        return function_name.strip()
    return None


def find_early_optimization_phase(graph: Dict) -> Optional[Dict]:
    return find_phase_by_name(graph, "V8.TFEarlyOptimization")


def find_phase_by_name(graph: Dict, phase_name: str) -> Optional[Dict]:
    phases = graph.get("phases") if isinstance(graph, dict) else None
    if not isinstance(phases, list):
        return None
    for phase in phases:
        if not isinstance(phase, dict):
            continue
        if phase.get("name") == phase_name:
            return phase
    return None


def extract_phase_opcode_counts(
    trace_dir: Optional[Path],
    phase_name: str,
    opcodes: Sequence[str],
    allowed_names: Optional[Set[str]] = None,
) -> Dict[str, object]:
    if trace_dir is None or not trace_dir.exists():
        return {
            "trace_dir": str(trace_dir) if trace_dir else None,
            "phase_name": phase_name,
            "raw_graph_file_count": 0,
            "analyzed_functions": [],
            "per_function": [],
            "totals": {opcode: 0 for opcode in opcodes},
        }

    raw_files = sorted(trace_dir.glob("turbo-*.json"))
    latest_by_function: Dict[str, Tuple[int, Path]] = {}

    for file_path in raw_files:
        match = re.match(r"^turbo-(.+)-(\d+)\.json$", file_path.name)
        if not match:
            continue
        fallback_name = match.group(1)
        index = int(match.group(2))

        try:
            graph = json.loads(file_path.read_text(encoding="utf-8"))
        except Exception:
            continue

        function_name = parse_turbo_graph_function_name(graph) or fallback_name
        if allowed_names is not None and function_name not in allowed_names:
            continue

        if function_name not in latest_by_function or index > latest_by_function[function_name][0]:
            latest_by_function[function_name] = (index, file_path)

    opcode_set = set(opcodes)
    per_function: List[Dict[str, object]] = []
    totals = {opcode: 0 for opcode in opcodes}
    analyzed: List[str] = []

    for function_name, (_, file_path) in sorted(latest_by_function.items()):
        try:
            graph = json.loads(file_path.read_text(encoding="utf-8"))
        except Exception:
            continue

        phase = find_phase_by_name(graph, phase_name)
        if not isinstance(phase, dict):
            continue
        data = phase.get("data") if isinstance(phase.get("data"), dict) else {}
        nodes = data.get("nodes") if isinstance(data.get("nodes"), list) else []
        counts = {opcode: 0 for opcode in opcodes}

        for node in nodes:
            if not isinstance(node, dict):
                continue
            if not node.get("live", True):
                continue
            opcode = node.get("opcode")
            if opcode in opcode_set:
                counts[opcode] += 1

        for opcode in opcodes:
            totals[opcode] += counts[opcode]

        per_function.append(
            {
                "function": function_name,
                "counts": counts,
            }
        )
        analyzed.append(function_name)

    return {
        "trace_dir": str(trace_dir),
        "phase_name": phase_name,
        "raw_graph_file_count": len(raw_files),
        "analyzed_functions": sorted(analyzed),
        "per_function": per_function,
        "totals": totals,
    }


def extract_early_graphs(
    trace_dir: Optional[Path],
    output_dir: Path,
    mode: str,
    allowed_names: Optional[Set[str]] = None,
) -> Dict[str, object]:
    if trace_dir is None or not trace_dir.exists():
        return {
            "trace_dir": str(trace_dir) if trace_dir else None,
            "raw_graph_file_count": 0,
            "early_graph_files": [],
            "analyzed_functions": [],
        }

    raw_files = sorted(trace_dir.glob("turbo-*.json"))
    latest_by_function: Dict[str, Tuple[int, Path]] = {}

    for file_path in raw_files:
        match = re.match(r"^turbo-(.+)-(\d+)\.json$", file_path.name)
        if not match:
            continue
        fallback_name = match.group(1)
        index = int(match.group(2))

        try:
            graph = json.loads(file_path.read_text(encoding="utf-8"))
        except Exception:
            continue

        function_name = parse_turbo_graph_function_name(graph) or fallback_name
        if allowed_names is not None and function_name not in allowed_names:
            continue

        if function_name not in latest_by_function or index > latest_by_function[function_name][0]:
            latest_by_function[function_name] = (index, file_path)

    analyzed: List[str] = []
    written_files: List[str] = []

    for function_name, (_, file_path) in sorted(latest_by_function.items()):
        try:
            graph = json.loads(file_path.read_text(encoding="utf-8"))
        except Exception:
            continue
        early_phase = find_early_optimization_phase(graph)
        if not isinstance(early_phase, dict):
            continue

        payload = {
            "function": graph.get("function"),
            "phase": early_phase,
        }
        out_file = output_dir / f"{function_name}_{mode}.early-graph.json"
        out_file.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
        analyzed.append(function_name)
        written_files.append(str(out_file))

    return {
        "trace_dir": str(trace_dir),
        "raw_graph_file_count": len(raw_files),
        "early_graph_files": written_files,
        "analyzed_functions": sorted(analyzed),
    }


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
    stats_scope: str = "target",
    optimized_business_functions: Optional[Dict] = None,
    aggregate_diff: Optional[Dict] = None,
    rawint32_strength_reduction: Optional[Dict] = None,
    verification: Optional[Dict] = None,
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

    if stats_scope == "all-business" and isinstance(optimized_business_functions, dict):
        paired = optimized_business_functions.get("paired", [])
        lines.append("")
        lines.append("## all-business 汇总")
        lines.append("")
        lines.append(
            f"- 业务函数数量: without={optimized_business_functions.get('without_count')} with={optimized_business_functions.get('with_count')} paired={optimized_business_functions.get('paired_count')}"
        )
        if isinstance(aggregate_diff, dict):
            lines.append(
                f"- 汇总指令行数变化: {aggregate_diff.get('line_delta')}"
            )
            lines.append(
                f"- 汇总指令字节变化: {aggregate_diff.get('instruction_size_delta')}"
            )
            lines.append(
                f"- 汇总归一化差异行数: {aggregate_diff.get('changed_lines')}"
            )
        if isinstance(paired, list) and paired:
            lines.append("")
            lines.append("- 逐函数统计:")
            for item in paired:
                if not isinstance(item, dict):
                    continue
                fn = item.get("function")
                wout = item.get("without_instruction_size")
                w = item.get("with_instruction_size")
                d = item.get("instruction_size_delta")
                lines.append(f"  - {fn}: {wout} -> {w} ({d:+d})")

    early_graph = optimized_business_functions.get("early_optimization_graph") if isinstance(optimized_business_functions, dict) else None
    if isinstance(early_graph, dict) and early_graph.get("enabled"):
        lines.append("")
        lines.append("## EarlyOptimization 图覆盖")
        lines.append("")
        expected = early_graph.get("expected_functions", [])
        without_info = ensure_dict(early_graph.get("without"))
        with_info = ensure_dict(early_graph.get("with"))
        lines.append(f"- 期望函数数: {len(expected)}")
        lines.append(
            f"- without: 分析到 {len(without_info.get('analyzed_functions', []))}，缺失 {len(without_info.get('missing_functions', []))}"
        )
        if isinstance(early_graph.get("with"), dict):
            lines.append(
                f"- with: 分析到 {len(with_info.get('analyzed_functions', []))}，缺失 {len(with_info.get('missing_functions', []))}"
            )

    if isinstance(rawint32_strength_reduction, dict) and rawint32_strength_reduction.get("enabled"):
        lines.append("")
        lines.append("## RawInt32StrengthReduction")
        lines.append("")
        totals_without = ensure_dict(rawint32_strength_reduction.get("without", {})).get("totals", {})
        totals_with = ensure_dict(rawint32_strength_reduction.get("with", {})).get("totals", {})
        wout_checked = totals_without.get("CheckedInt32Add", 0)
        with_checked = totals_with.get("CheckedInt32Add", 0)
        wout_int32 = totals_without.get("Int32Add", 0)
        with_int32 = totals_with.get("Int32Add", 0)
        lines.append(f"- CheckedInt32Add: {wout_checked} -> {with_checked} ({with_checked - wout_checked:+d})")
        lines.append(f"- Int32Add: {wout_int32} -> {with_int32} ({with_int32 - wout_int32:+d})")

    if isinstance(verification, dict):
        raw_verify = ensure_dict(verification.get("rawint32_add_reduction"))
        if raw_verify.get("enabled"):
            lines.append("")
            lines.append("## 校验")
            lines.append("")
            lines.append(f"- rawint32 add 替换: {raw_verify.get('status')}")
            if raw_verify.get("status") == "failed":
                lines.append(f"- 失败条目数: {raw_verify.get('regression_count', 0)}")

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
    parser.add_argument(
        "--compile-coverage",
        action="store_true",
        help="额外执行多目标编译覆盖统计，输出每个目标函数是否被编译。",
    )
    parser.add_argument(
        "--coverage-warmup-calls",
        type=int,
        default=8,
        help="compile-coverage 模式下每个目标函数的热身调用次数。",
    )
    parser.add_argument(
        "--coverage-target",
        action="append",
        default=[],
        help="追加覆盖目标，格式: function=invoke_expr，例如 parseJson=parseJson(jsonText)",
    )
    parser.add_argument(
        "--coverage-all-business-functions",
        action="store_true",
        help="compile-coverage 时将所有业务函数都加入覆盖目标（默认调用表达式为 fn()）。",
    )
    parser.add_argument(
        "--no-inline",
        action="store_true",
        help="禁用 TurboFan inlining，减少干扰项。",
    )
    parser.add_argument(
        "--verify-rawint32-add-reduction",
        action="store_true",
        help="校验 `V8.TFRawInt32StrengthReduction` 中 CheckedInt32Add 在 with metadata 下是否减少。",
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
        for stale_file in list(case_dir.glob("*.asm")) + list(case_dir.glob("*.ir.log")) + list(case_dir.glob("*.early-graph.json")):
            try:
                stale_file.unlink()
            except OSError:
                pass
        trace_graph_root = case_dir / "trace-graphs"
        trace_graph_without_dir = trace_graph_root / "without"
        trace_graph_with_dir = trace_graph_root / "with"
        if args.trace_ir:
            trace_graph_root.mkdir(parents=True, exist_ok=True)
            for trace_dir in [trace_graph_without_dir, trace_graph_with_dir]:
                trace_dir.mkdir(parents=True, exist_ok=True)
                clean_directory(trace_dir)
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
        base_flags = build_d8_flags(
            defaults,
            target_function,
            trace_ir=args.trace_ir,
            use_filter=use_filter,
            no_inline=args.no_inline,
        )
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

        lifecycle_names = {"setup", "teardown", target_setup, target_teardown}

        coverage_targets: List[Dict[str, str]] = []
        raw_coverage_targets = target.get("coverage_targets", [])
        if isinstance(raw_coverage_targets, list):
            for entry in raw_coverage_targets:
                if not isinstance(entry, dict):
                    continue
                fn_name = entry.get("function")
                invoke_expr = entry.get("invoke_expr")
                if (
                    isinstance(fn_name, str)
                    and fn_name
                    and fn_name not in lifecycle_names
                    and isinstance(invoke_expr, str)
                    and invoke_expr
                ):
                    coverage_targets.append({"function": fn_name, "invoke_expr": invoke_expr})

        for raw in args.coverage_target:
            if not isinstance(raw, str):
                continue
            if "=" not in raw:
                continue
            fn_name, invoke_expr = raw.split("=", 1)
            fn_name = fn_name.strip()
            invoke_expr = invoke_expr.strip()
            if fn_name and fn_name not in lifecycle_names and invoke_expr:
                coverage_targets.append({"function": fn_name, "invoke_expr": invoke_expr})

        business_functions = discover_business_functions(bench_js, excluded_names=lifecycle_names)

        if args.compile_coverage and args.coverage_all_business_functions:
            explicit_names = {item["function"] for item in coverage_targets}
            for fn_name in sorted(business_functions):
                if fn_name in explicit_names:
                    continue
                safe_invoke_expr = (
                    f"(() => {{ const __fn = globalThis[{json.dumps(fn_name)}]; "
                    "if (typeof __fn !== 'function') return 0; "
                    "const __argc = Math.max(0, (__fn.length | 0)); "
                    "if (__argc === 0) return __fn(); "
                    "const __args = new Array(__argc); "
                    "const __pref = ['inputData','data','arr','array','buffer','bytes','input','source']; "
                    "let __pool = []; "
                    "for (let __i = 0; __i < __pref.length; __i++) { "
                    "  const __k = __pref[__i]; "
                    "  try { "
                    "    const __v = globalThis[__k]; "
                    "    if (Array.isArray(__v) || ArrayBuffer.isView(__v)) __pool.push(__v); "
                    "  } catch (e) {} "
                    "} "
                    "for (const __k in globalThis) { "
                    "  try { "
                    "    const __v = globalThis[__k]; "
                    "    if (Array.isArray(__v) || ArrayBuffer.isView(__v)) __pool.push(__v); "
                    "  } catch (e) {} "
                    "} "
                    "for (let __i = 0; __i < __argc; __i++) { "
                    "  __args[__i] = (__i < __pool.length) ? __pool[__i] : 0; "
                    "} "
                    "return __fn.apply(undefined, __args); })()"
                )
                coverage_targets.append({"function": fn_name, "invoke_expr": safe_invoke_expr})

        if args.compile_coverage and not coverage_targets and target_function not in lifecycle_names:
            coverage_targets.append({"function": target_function, "invoke_expr": target_invoke_expr})

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

        run_without_flags = list(base_flags)
        if args.trace_ir:
            run_without_flags.extend([
                "--trace-turbo-graph",
                f"--trace-turbo-path={trace_graph_without_dir}",
            ])

        cmd_without = [str(d8_path)] + run_without_flags + [str(harness)]
        rc_without, stdout_without, stderr_without = run_d8(d8_path, harness, run_without_flags)
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
            run_with_flags = list(base_flags + metadata_flags)
            if args.trace_ir:
                run_with_flags.extend([
                    "--trace-turbo-graph",
                    f"--trace-turbo-path={trace_graph_with_dir}",
                ])
            cmd_with = [str(d8_path)] + run_with_flags + [str(harness)]
            rc_with, stdout_with, stderr_with = run_d8(d8_path, harness, run_with_flags)
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

        report_json = generate_json_report(
            bench_name=bench_file.stem,
            source_file=bench_js,
            source_hash=source_hash,
            metadata_file=metadata_file if args.metadata_mode == "strict" else None,
            without_result=without_result,
            with_result=with_result,
        )

        out_cov_without = ""
        err_cov_without = ""
        out_cov_with = ""
        err_cov_with = ""

        if args.compile_coverage:
            coverage_harness = write_coverage_harness(
                tmp_dir=tmp_dir,
                bench_js=bench_js,
                setup_name=target_setup,
                teardown_name=target_teardown,
                coverage_targets=coverage_targets,
                warmup_calls=args.coverage_warmup_calls,
            )

            coverage_without_flags = list(base_flags)
            if args.trace_ir:
                coverage_without_flags.extend([
                    "--trace-turbo-graph",
                    f"--trace-turbo-path={trace_graph_without_dir}",
                ])
            rc_cov_without, out_cov_without, err_cov_without = run_d8(d8_path, coverage_harness, coverage_without_flags)
            cov_without_text = (out_cov_without or "") + "\n" + (err_cov_without or "")
            compiled_without = parse_compiled_methods(cov_without_text)
            invoke_without = parse_coverage_results(cov_without_text)

            compiled_with: Set[str] = set()
            invoke_with: Dict[str, str] = {}
            rc_cov_with = 0
            if args.metadata_mode == "strict":
                coverage_with_flags = list(base_flags)
                if metadata_dir:
                    coverage_with_flags.append(f"--turbo_metadata_path={metadata_dir}")
                if args.trace_ir:
                    coverage_with_flags.extend([
                        "--trace-turbo-graph",
                        f"--trace-turbo-path={trace_graph_with_dir}",
                    ])
                rc_cov_with, out_cov_with, err_cov_with = run_d8(d8_path, coverage_harness, coverage_with_flags)
                cov_with_text = (out_cov_with or "") + "\n" + (err_cov_with or "")
                compiled_with = parse_compiled_methods(cov_with_text)
                invoke_with = parse_coverage_results(cov_with_text)

            if args.stats_scope == "all-business":
                cov_without_asm_map = extract_all_assemblies(
                    out_cov_without or "",
                    allowed_names=business_functions,
                )
                asm_without_map.update(cov_without_asm_map)

                if args.metadata_mode == "strict":
                    cov_with_asm_map = extract_all_assemblies(
                        out_cov_with or "",
                        allowed_names=business_functions,
                    )
                    asm_with_map.update(cov_with_asm_map)

                if target_function in asm_without_map:
                    without_result.asm, without_result.code_size = asm_without_map[target_function]
                    without_result.normalized_lines = normalize_lines(without_result.asm)
                if args.metadata_mode == "strict" and target_function in asm_with_map:
                    with_result.asm, with_result.code_size = asm_with_map[target_function]
                    with_result.normalized_lines = normalize_lines(with_result.asm)

                for function_name, (asm_text, _) in sorted(asm_without_map.items()):
                    (case_dir / f"{function_name}_without.asm").write_text(asm_text + "\n", encoding="utf-8")
                for function_name, (asm_text, _) in sorted(asm_with_map.items()):
                    (case_dir / f"{function_name}_with.asm").write_text(asm_text + "\n", encoding="utf-8")

            target_names = [item["function"] for item in coverage_targets]
            per_target = []
            for fn_name in target_names:
                per_target.append(
                    {
                        "function": fn_name,
                        "without_invocation": invoke_without.get(fn_name, "missing"),
                        "without_compiled": fn_name in compiled_without,
                        "with_invocation": invoke_with.get(fn_name, "missing") if args.metadata_mode == "strict" else None,
                        "with_compiled": (fn_name in compiled_with) if args.metadata_mode == "strict" else None,
                    }
                )

            report_json["compile_coverage"] = {
                "enabled": True,
                "warmup_calls": args.coverage_warmup_calls,
                "targets": target_names,
                "without": {
                    "returncode": rc_cov_without,
                    "compiled_methods": sorted(compiled_without),
                    "invocations": invoke_without,
                    "compiled_target_count": sum(1 for fn_name in target_names if fn_name in compiled_without),
                },
                "with": {
                    "returncode": rc_cov_with,
                    "compiled_methods": sorted(compiled_with),
                    "invocations": invoke_with,
                    "compiled_target_count": sum(1 for fn_name in target_names if fn_name in compiled_with),
                }
                if args.metadata_mode == "strict"
                else None,
                "per_target": per_target,
            }

        expected_early_functions: Set[str] = set()
        if args.trace_ir:
            if args.compile_coverage and args.coverage_all_business_functions:
                expected_early_functions.update(business_functions)
            elif args.compile_coverage:
                expected_early_functions.update(item["function"] for item in coverage_targets)
            elif args.stats_scope == "all-business":
                expected_early_functions.update(business_functions)
            else:
                expected_early_functions.add(target_function)

            allowed_early_names = expected_early_functions if expected_early_functions else None
            early_without = extract_early_graphs(
                trace_dir=trace_graph_without_dir,
                output_dir=case_dir,
                mode="without",
                allowed_names=allowed_early_names,
            )
            early_with = extract_early_graphs(
                trace_dir=trace_graph_with_dir,
                output_dir=case_dir,
                mode="with",
                allowed_names=allowed_early_names,
            ) if args.metadata_mode == "strict" else None

            expected_sorted = sorted(expected_early_functions)
            without_analyzed = set(early_without.get("analyzed_functions", []))
            without_missing = sorted(set(expected_sorted) - without_analyzed)
            early_without["expected_functions"] = expected_sorted
            early_without["missing_functions"] = without_missing

            if isinstance(early_with, dict):
                with_analyzed = set(early_with.get("analyzed_functions", []))
                with_missing = sorted(set(expected_sorted) - with_analyzed)
                early_with["expected_functions"] = expected_sorted
                early_with["missing_functions"] = with_missing

            report_json["early_optimization_graph"] = {
                "enabled": True,
                "expected_functions": expected_sorted,
                "without": early_without,
                "with": early_with,
            }

            raw_without = extract_phase_opcode_counts(
                trace_dir=trace_graph_without_dir,
                phase_name="V8.TFRawInt32StrengthReduction",
                opcodes=["CheckedInt32Add", "Int32Add"],
                allowed_names=allowed_early_names,
            )
            raw_with = extract_phase_opcode_counts(
                trace_dir=trace_graph_with_dir,
                phase_name="V8.TFRawInt32StrengthReduction",
                opcodes=["CheckedInt32Add", "Int32Add"],
                allowed_names=allowed_early_names,
            ) if args.metadata_mode == "strict" else None

            without_analyzed = set(raw_without.get("analyzed_functions", []))
            raw_without["expected_functions"] = expected_sorted
            raw_without["missing_functions"] = sorted(set(expected_sorted) - without_analyzed)

            if isinstance(raw_with, dict):
                with_analyzed = set(raw_with.get("analyzed_functions", []))
                raw_with["expected_functions"] = expected_sorted
                raw_with["missing_functions"] = sorted(set(expected_sorted) - with_analyzed)

            per_function_compare = []
            without_map = {
                item.get("function"): item
                for item in raw_without.get("per_function", [])
                if isinstance(item, dict) and isinstance(item.get("function"), str)
            }
            with_map = {
                item.get("function"): item
                for item in (raw_with or {}).get("per_function", [])
                if isinstance(item, dict) and isinstance(item.get("function"), str)
            }

            for fn_name in sorted(set(without_map.keys()) | set(with_map.keys())):
                wout_counts = ensure_dict(without_map.get(fn_name, {})).get("counts", {})
                with_counts = ensure_dict(with_map.get(fn_name, {})).get("counts", {})
                per_function_compare.append(
                    {
                        "function": fn_name,
                        "without": {
                            "CheckedInt32Add": ensure_dict(wout_counts).get("CheckedInt32Add", 0),
                            "Int32Add": ensure_dict(wout_counts).get("Int32Add", 0),
                        },
                        "with": {
                            "CheckedInt32Add": ensure_dict(with_counts).get("CheckedInt32Add", 0),
                            "Int32Add": ensure_dict(with_counts).get("Int32Add", 0),
                        },
                    }
                )

            report_json["rawint32_strength_reduction"] = {
                "enabled": True,
                "phase_name": "V8.TFRawInt32StrengthReduction",
                "without": raw_without,
                "with": raw_with,
                "per_function_compare": per_function_compare,
            }

            if args.verify_rawint32_add_reduction:
                verification_status = "passed"
                regression_items = []
                comparable_count = 0

                if not isinstance(raw_with, dict):
                    verification_status = "failed"
                    regression_items.append({
                        "function": "<global>",
                        "reason": "with metadata 的 raw phase 统计缺失",
                    })
                else:
                    for item in per_function_compare:
                        fn_name = item["function"]
                        wout_checked = ensure_dict(item.get("without", {})).get("CheckedInt32Add", 0)
                        with_checked = ensure_dict(item.get("with", {})).get("CheckedInt32Add", 0)
                        wout_int32 = ensure_dict(item.get("without", {})).get("Int32Add", 0)
                        with_int32 = ensure_dict(item.get("with", {})).get("Int32Add", 0)

                        if wout_checked <= 0:
                            continue
                        comparable_count += 1
                        if with_checked >= wout_checked:
                            regression_items.append(
                                {
                                    "function": fn_name,
                                    "reason": "CheckedInt32Add 未减少",
                                    "without_checked": wout_checked,
                                    "with_checked": with_checked,
                                }
                            )
                        if with_int32 < wout_int32:
                            regression_items.append(
                                {
                                    "function": fn_name,
                                    "reason": "Int32Add 计数回退",
                                    "without_int32": wout_int32,
                                    "with_int32": with_int32,
                                }
                            )

                if regression_items:
                    verification_status = "failed"

                verification_payload = {
                    "enabled": True,
                    "status": verification_status,
                    "comparable_function_count": comparable_count,
                    "regression_count": len(regression_items),
                    "regressions": regression_items,
                }
                report_json.setdefault("verification", {})["rawint32_add_reduction"] = verification_payload

                if verification_status == "failed":
                    print("rawint32 add reduction verification failed")
                    failures += 1

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

            total_without_instruction_lines = sum(
                len(normalize_lines(asm_text)) for asm_text, _ in asm_without_map.values()
            )
            total_with_instruction_lines = sum(
                len(normalize_lines(asm_text)) for asm_text, _ in asm_with_map.values()
            )

            without_sizes = [size for _, size in asm_without_map.values() if isinstance(size, int)]
            with_sizes = [size for _, size in asm_with_map.values() if isinstance(size, int)]
            total_without_instruction_size = sum(without_sizes) if without_sizes else None
            total_with_instruction_size = sum(with_sizes) if with_sizes else None
            total_instruction_size_delta = None
            if total_without_instruction_size is not None and total_with_instruction_size is not None:
                total_instruction_size_delta = total_with_instruction_size - total_without_instruction_size

            report_json["stats_scope"] = "all-business"
            report_json["optimized_business_functions"] = {
                "without_count": len(asm_without_map),
                "with_count": len(asm_with_map),
                "paired_count": len(paired_functions),
                "only_without": only_without,
                "only_with": only_with,
                "paired": per_function,
            }
            report_json["all_business_totals"] = {
                "without_instruction_lines": total_without_instruction_lines,
                "with_instruction_lines": total_with_instruction_lines,
                "line_delta": total_with_instruction_lines - total_without_instruction_lines,
                "without_instruction_size": total_without_instruction_size,
                "with_instruction_size": total_with_instruction_size,
                "instruction_size_delta": total_instruction_size_delta,
            }
            report_json["diff"] = {
                "line_delta": total_with_instruction_lines - total_without_instruction_lines,
                "changed_lines": aggregate_changed_lines,
                "instruction_size_delta": total_instruction_size_delta,
                "paired_line_delta": aggregate_line_delta,
                "paired_instruction_size_delta": aggregate_instruction_delta,
            }

        report_md = generate_markdown_report(
            bench_name=bench_file.stem,
            source_file=bench_js,
            source_hash=source_hash,
            metadata_file=metadata_file if args.metadata_mode == "strict" else None,
            without_result=without_result,
            with_result=with_result,
            stats_scope=str(report_json.get("stats_scope", "target")),
            optimized_business_functions={
                **(report_json.get("optimized_business_functions") or {}),
                "early_optimization_graph": report_json.get("early_optimization_graph"),
            },
            aggregate_diff=report_json.get("diff"),
            rawint32_strength_reduction=report_json.get("rawint32_strength_reduction"),
            verification=report_json.get("verification"),
        )
        (case_dir / "comparison.md").write_text(report_md, encoding="utf-8")

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
            without_ir_text = without_result.stdout + "\n" + without_result.stderr
            with_ir_text = with_result.stdout + "\n" + with_result.stderr
            if args.compile_coverage:
                without_ir_text += "\n" + out_cov_without + "\n" + err_cov_without
                if args.metadata_mode == "strict":
                    with_ir_text += "\n" + out_cov_with + "\n" + err_cov_with

            allowed_ir_names = business_functions if args.stats_scope == "all-business" else {target_function}
            without_ir_by_fn = split_ir_logs_by_function(without_ir_text, allowed_names=allowed_ir_names)
            with_ir_by_fn = split_ir_logs_by_function(with_ir_text, allowed_names=allowed_ir_names)

            if without_ir_by_fn:
                for fn_name, text in sorted(without_ir_by_fn.items()):
                    (case_dir / f"{fn_name}_without.ir.log").write_text(text, encoding="utf-8")
            else:
                (case_dir / f"{target_function}_without.ir.log").write_text(without_ir_text, encoding="utf-8")

            if args.metadata_mode == "strict":
                if with_ir_by_fn:
                    for fn_name, text in sorted(with_ir_by_fn.items()):
                        (case_dir / f"{fn_name}_with.ir.log").write_text(text, encoding="utf-8")
                else:
                    (case_dir / f"{target_function}_with.ir.log").write_text(with_ir_text, encoding="utf-8")

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
