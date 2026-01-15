#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
生成并比较 TurboFan 优化后的汇编代码
对比 with metadata 和 without metadata 两个版本的机器码差异
"""

import subprocess
import re
import os
from pathlib import Path
from test_config import TEST_CASES, V8_CONFIG

# 汇编输出目录
ASSEMBLY_DIR = Path("docs/assembly")


def normalize_addresses(asm_text):
    """
    规范化汇编代码中的地址，便于对比
    - 将堆地址替换为 ADDR1, ADDR2, ...
    - 将代码地址替换为 CODE1, CODE2, ...
    """
    if not asm_text:
        return asm_text
    
    heap_map = {}
    code_map = {}
    heap_id = 1
    code_id = 1
    
    def normalize_hex(addr):
        """去掉前导零"""
        hex_part = addr[2:].lstrip('0') or '0'
        return '0x' + hex_part
    
    def replace_heap_addr(match):
        nonlocal heap_id
        addr = normalize_hex(match.group(0))
        if addr not in heap_map:
            heap_map[addr] = f"HEAP_ADDR_{heap_id}"
            heap_id += 1
        return heap_map[addr]
    
    def replace_code_addr(match):
        nonlocal code_id
        addr = normalize_hex(match.group(0))
        if addr not in code_map:
            code_map[addr] = f"CODE_ADDR_{code_id}"
            code_id += 1
        return code_map[addr]
    
    # 先替换代码地址（REX.W movq 指令中的地址）
    asm_text = re.sub(r'0x[0-9a-fA-F]{8,16}(?=\s+;; code:)', replace_code_addr, asm_text)
    # 再替换堆地址
    asm_text = re.sub(r'0x[0-9a-fA-F]{8,16}', replace_heap_addr, asm_text)
    
    return asm_text


def remove_ansi_codes(text):
    """
    移除 ANSI 控制字符（颜色代码等）
    """
    # ANSI 转义序列的正则表达式
    ansi_escape = re.compile(r'\x1b\[[0-9;]*m')
    return ansi_escape.sub('', text)


def extract_assembly(output_text, function_name):
    """
    从 d8 的 --print-opt-code 输出中提取指定函数的汇编代码
    
    输出格式示例：
    --- Optimized code ---
    optimization_id = 0
    source_position = 123
    kind = TURBOFAN_JS
    name = twice_s
    stack_slots = 2
    compiler = turbofan
    address = 0x...
    
    Instructions (size = 123)
    0x...  0   push rbp
    0x...  1   movq rbp,rsp
    ...
    
    --- End code ---
    """
    # 先移除 ANSI 控制字符
    output_text = remove_ansi_codes(output_text)
    
    # 匹配函数的汇编代码块
    pattern = rf"--- Optimized code ---.*?name = {re.escape(function_name)}.*?--- End code ---"
    match = re.search(pattern, output_text, re.DOTALL)
    
    if not match:
        return None
    
    code_block = match.group(0)
    
    # 提取 Instructions 部分
    instructions_match = re.search(r"Instructions \(size = \d+\)(.*?)(?=\n--- End code ---)", 
                                   code_block, re.DOTALL)
    if instructions_match:
        return instructions_match.group(1).strip()
    
    return None


def run_d8_with_assembly(test_file, function_name, extra_flags=None):
    """
    运行 d8 并捕获优化后的汇编代码
    """
    flags = V8_CONFIG["base_flags"].copy()
    flags.extend([
        "--print-opt-code",  # 打印优化后的代码
        "--code-comments",   # 添加注释，便于理解
        "--print-opt-code-filter=" + function_name,  # 只打印指定函数
        "--no-concurrent-recompilation"  # 禁用并发编译，使输出更清晰
    ])
    
    if extra_flags:
        flags.extend(extra_flags)
    
    cmd = [V8_CONFIG["d8_path"]] + flags + [V8_CONFIG["mjsunit_path"], test_file]
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30
        )
        
        # 检查是否有错误
        if result.returncode != 0:
            print(f"❌ d8 执行失败（返回码 {result.returncode}）")
            print(f"stderr: {result.stderr[:500]}")
            return None
        
        # 从 stdout 提取汇编代码
        return extract_assembly(result.stdout, function_name)
    
    except subprocess.TimeoutExpired:
        print(f"❌ d8 执行超时（30秒）")
        return None
    except Exception as e:
        print(f"❌ 执行出错: {e}")
        return None


def compare_assembly(asm_without, asm_with, function_name):
    """
    比较两个版本的汇编代码，生成对比报告
    """
    if not asm_without:
        return "⚠️ 无法获取 without metadata 版本的汇编代码"
    
    if not asm_with:
        return "⚠️ 无法获取 with metadata 版本的汇编代码"
    
    # 规范化地址
    asm_without_norm = normalize_addresses(asm_without)
    asm_with_norm = normalize_addresses(asm_with)
    
    # 统计指令数量
    lines_without = [l for l in asm_without_norm.split('\n') if l.strip()]
    lines_with = [l for l in asm_with_norm.split('\n') if l.strip()]
    
    # 检查是否完全相同
    if asm_without_norm.strip() == asm_with_norm.strip():
        return f"⚠️ **警告**: 两个版本的汇编代码完全相同！这可能说明 metadata 没有生效。"
    
    # 生成对比报告
    report = []
    report.append(f"## {function_name} 汇编代码对比\n")
    report.append(f"- **Without Metadata**: {len(lines_without)} 行指令")
    report.append(f"- **With Metadata**: {len(lines_with)} 行指令")
    report.append(f"- **差异**: {len(lines_with) - len(lines_without):+d} 行\n")
    
    # 找出关键差异（简单的逐行对比）
    report.append("### 关键差异\n")
    
    # 如果指令数量显著不同，说明有优化
    if abs(len(lines_with) - len(lines_without)) > 5:
        report.append(f"✅ 指令数量有显著变化（{len(lines_without)} → {len(lines_with)}），说明优化生效\n")
    
    # 查找特定的优化模式
    keywords_without = extract_keywords(asm_without)
    keywords_with = extract_keywords(asm_with)
    
    removed_keywords = keywords_without - keywords_with
    added_keywords = keywords_with - keywords_without
    
    if removed_keywords:
        report.append("**移除的检查/调用**:")
        for kw in sorted(removed_keywords):
            report.append(f"  - `{kw}`")
        report.append("")
    
    if added_keywords:
        report.append("**新增的操作**:")
        for kw in sorted(added_keywords):
            report.append(f"  - `{kw}`")
        report.append("")
    
    return "\n".join(report)


def extract_keywords(asm_code):
    """
    从汇编代码中提取关键字（函数调用、检查指令等）
    """
    keywords = set()
    
    # 匹配注释中的关键信息
    for match in re.finditer(r';;.*?(Deoptimize|CheckSmi|CheckHeapObject|CheckString|CheckNumber|CheckBounds|Call|JSCall|Builtin)', asm_code):
        keywords.add(match.group(1))
    
    # 匹配具体的 builtin 调用
    for match in re.finditer(r'Call\s+\((\w+Builtin)\)', asm_code):
        keywords.add(match.group(1))
    
    return keywords


def generate_assembly_report(test_case):
    """
    为单个测试用例生成汇编对比报告
    """
    case_name = test_case["name"]
    print(f"\n{'='*60}")
    print(f"处理测试用例: {case_name}")
    print(f"{'='*60}\n")
    
    # 创建输出目录
    case_dir = ASSEMBLY_DIR / case_name.replace(" ", "-")
    case_dir.mkdir(parents=True, exist_ok=True)
    
    all_reports = []
    all_reports.append(f"# {case_name} - 汇编代码对比\n")
    all_reports.append(f"> {test_case['description']}\n")
    
    has_differences = False
    
    for func in test_case["functions"]:
        func_name = func["name"]
        test_file = func["file"]
        
        print(f"  函数: {func_name}")
        
        # 1. 运行 without metadata
        print(f"    - 生成 without metadata 汇编代码...")
        asm_without = run_d8_with_assembly(test_file, func_name)
        
        # 2. 运行 with metadata
        print(f"    - 生成 with metadata 汇编代码...")
        asm_with = run_d8_with_assembly(test_file, func_name, func.get("flags", []))
        
        # 3. 保存原始汇编代码
        if asm_without:
            output_file = case_dir / f"{func_name}_without.asm"
            with open(output_file, "w", encoding="utf-8") as f:
                f.write(asm_without)
            print(f"    ✓ 保存到 {output_file}")
        
        if asm_with:
            output_file = case_dir / f"{func_name}_with.asm"
            with open(output_file, "w", encoding="utf-8") as f:
                f.write(asm_with)
            print(f"    ✓ 保存到 {output_file}")
        
        # 4. 生成对比报告
        comparison = compare_assembly(asm_without, asm_with, func_name)
        all_reports.append(comparison)
        all_reports.append("\n---\n")
        
        # 检查是否有差异
        if asm_without and asm_with:
            asm_without_norm = normalize_addresses(asm_without)
            asm_with_norm = normalize_addresses(asm_with)
            if asm_without_norm.strip() != asm_with_norm.strip():
                has_differences = True
        
        print(f"    ✓ 完成\n")
    
    # 保存总报告
    report_file = case_dir / "comparison.md"
    with open(report_file, "w", encoding="utf-8") as f:
        f.write("\n".join(all_reports))
    
    print(f"✅ 对比报告已保存到: {report_file}\n")
    
    # 警告：如果所有函数都没有差异
    if not has_differences:
        print(f"⚠️  警告: 该测试用例的所有函数都没有汇编差异！")
        print(f"    这可能说明 metadata 没有生效，请检查：")
        print(f"    1. metadata 文件是否正确生成？")
        print(f"    2. d8 是否正确加载了 metadata？")
        print(f"    3. 函数是否真的被 TurboFan 优化了？\n")
    
    return has_differences


def main():
    """
    主函数：为所有测试用例生成汇编对比
    """
    print("\n" + "="*60)
    print("TurboFan 汇编代码生成与对比工具")
    print("="*60)
    
    ASSEMBLY_DIR.mkdir(parents=True, exist_ok=True)
    
    total_cases = len(TEST_CASES)
    cases_with_diff = 0
    
    for idx, test_case in enumerate(TEST_CASES, 1):
        print(f"\n[{idx}/{total_cases}] 处理中...")
        has_diff = generate_assembly_report(test_case)
        if has_diff:
            cases_with_diff += 1
    
    print("\n" + "="*60)
    print("✅ 所有汇编代码生成完成！")
    print(f"   - 总测试用例: {total_cases}")
    print(f"   - 有差异的用例: {cases_with_diff}")
    print(f"   - 无差异的用例: {total_cases - cases_with_diff}")
    print(f"   - 输出目录: {ASSEMBLY_DIR.absolute()}")
    print("="*60 + "\n")
    
    if cases_with_diff < total_cases:
        print("⚠️  部分测试用例没有产生汇编差异，请检查 metadata 是否正确生效。\n")


if __name__ == "__main__":
    main()
