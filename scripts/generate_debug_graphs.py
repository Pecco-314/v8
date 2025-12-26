#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
生成用于调试的 TurboFan Graph 文件
支持生成多个阶段的图：TypeInjector, EarlyOptimization 等
"""

import subprocess
import re
import os
import sys
from pathlib import Path
from test_config import TEST_CASES, V8_CONFIG, OUTPUT_CONFIG


def normalize_addresses(graph_text):
    """将图中的堆地址替换为统一编号"""
    if not graph_text:
        return graph_text
    
    address_map = {}
    next_id = 1
    
    def normalize_hex(addr):
        hex_part = addr[2:].lstrip('0') or '0'
        return '0x' + hex_part
    
    def replace_address(match):
        nonlocal next_id
        addr = match.group(0)
        normalized_addr = normalize_hex(addr)
        if normalized_addr not in address_map:
            address_map[normalized_addr] = f"ADDR{next_id}"
            next_id += 1
        return address_map[normalized_addr]
    
    pattern = r'0x[0-9a-fA-F]{8,16}'
    return re.sub(pattern, replace_address, graph_text)


def run_d8(test_file, flags):
    """运行 d8 并返回输出"""
    cmd = [V8_CONFIG["d8_path"]] + V8_CONFIG["base_flags"] + flags + [
        V8_CONFIG["mjsunit_path"],
        test_file
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.stdout + result.stderr


def extract_phase_graph(output, function_name, phase_name):
    """
    提取特定函数在特定阶段的 Graph
    
    phase_name 可以是：
    - "TypeInjector" -> V8.TFTypeInjector
    - "EarlyOptimization" -> V8.TFEarlyOptimization
    - "SimplifiedLowering" -> V8.TFSimplifiedLowering
    - "TypedLowering" -> V8.TFTypedLowering
    等等
    """
    lines = output.split('\n')
    in_target_function = False
    in_target_phase = False
    graph_lines = []
    
    # 构造阶段标记
    phase_marker = f"----- Graph after V8.TF{phase_name}"
    
    for line in lines:
        # 检测函数编译开始
        if 'Begin compiling method' in line:
            in_target_function = function_name in line
            in_target_phase = False
            graph_lines = []
        
        # 检测目标阶段
        if in_target_function and phase_marker in line:
            in_target_phase = True
            graph_lines.append(line)
            continue
        
        # 检测 Graph 结束
        if in_target_phase and line.startswith('----- '):
            break
        
        # 收集 Graph 内容
        if in_target_phase:
            graph_lines.append(line)
    
    return '\n'.join(graph_lines) if graph_lines else None


def get_output_prefix(file_path):
    """从文件路径获取输出前缀"""
    stem = Path(file_path).stem
    return stem.replace('test-', '')


def generate_debug_graphs(test_name=None, phases=None):
    """
    生成调试用的 Graph 文件
    
    Args:
        test_name: 可选，只生成指定测试的图（例如 "rawint32"）
        phases: 要生成的阶段列表，默认为 ["TypeInjector", "EarlyOptimization"]
    """
    if phases is None:
        phases = ["TypeInjector", "EarlyOptimization"]
    
    graph_dir = Path(OUTPUT_CONFIG["graph_dir"])
    
    # 为调试图创建子目录
    for phase in phases:
        phase_dir = graph_dir / phase.lower()
        phase_dir.mkdir(parents=True, exist_ok=True)
    
    print("=" * 60)
    print(f"TurboFan 调试 Graph 生成工具 (阶段: {', '.join(phases)})")
    print("=" * 60)
    print()
    
    for case in TEST_CASES:
        # 如果指定了 test_name，只处理匹配的测试
        if test_name and test_name not in case['name'].lower():
            continue
            
        print(f"📋 {case['name']}")
        for func in case['functions']:
            file_path = func['file']
            function_name = func['name']
            flags = func['flags']
            prefix = get_output_prefix(func['file'])
            
            # 如果指定了 test_name，再次在函数级别检查
            if test_name and test_name not in prefix.lower():
                continue
            
            # 生成每个阶段的图（无优化和有优化）
            for phase in phases:
                phase_dir = graph_dir / phase.lower()
                
                # 无优化 Graph
                output_without = run_d8(file_path, [])
                graph_without = extract_phase_graph(output_without, function_name, phase)
                
                # 有优化 Graph
                output_with = run_d8(file_path, flags)
                graph_with = extract_phase_graph(output_with, function_name, phase)
                
                without_file = phase_dir / f"{prefix}-{function_name}-without.txt"
                with_file = phase_dir / f"{prefix}-{function_name}-with.txt"
                
                status = "✅" if graph_without and graph_with else "❌"
                
                if graph_without:
                    normalized_graph = normalize_addresses(graph_without)
                    with open(without_file, 'w', encoding='utf-8') as f:
                        f.write(normalized_graph)
                
                if graph_with:
                    normalized_graph = normalize_addresses(graph_with)
                    with open(with_file, 'w', encoding='utf-8') as f:
                        f.write(normalized_graph)
                
                if status == "✅":
                    print(f"  {status} {function_name} ({phase})")
        print()
    
    print("✅ 调试 Graph 生成完成！")
    print(f"\n生成的图保存在:")
    for phase in phases:
        print(f"  - {graph_dir / phase.lower()}/")


if __name__ == "__main__":
    # 支持命令行参数
    # python3 scripts/generate_debug_graphs.py rawint32
    # python3 scripts/generate_debug_graphs.py rawint32 TypeInjector
    
    test_name = sys.argv[1] if len(sys.argv) > 1 else None
    phases = sys.argv[2:] if len(sys.argv) > 2 else None
    
    generate_debug_graphs(test_name, phases)
