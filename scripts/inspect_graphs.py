#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
检查 Graph 文件，输出关键节点统计
用于确定 expected 配置的正确值
"""

import re
from pathlib import Path
from test_config import TEST_CASES, OUTPUT_CONFIG


# 要统计的节点类型
INTERESTING_NODES = [
    "CheckString",
    "CheckedTaggedToFloat64",
    "ChangeTaggedToFloat64",
    "TruncateTaggedToBit",
    "ChangeTaggedToBit",
    "CheckBounds",
    "CheckedUint32Bounds",
    "StringConcat",
    "NumberConstant",
]


def count_node(graph, pattern):
    """统计节点数量"""
    if not graph:
        return 0
    if 'LoadField' in pattern and 'length' in pattern:
        matches = re.findall(r'#\d+:LoadField\[.*length', graph, re.IGNORECASE)
    else:
        matches = re.findall(rf'#\d+:{re.escape(pattern)}', graph)
    return len(set(matches))


def load_graph(file_path):
    """加载 Graph 文件"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        return None


def get_output_prefix(file_path):
    """从文件路径获取输出前缀"""
    stem = Path(file_path).stem
    return stem.replace('test-', '')


def inspect_all():
    """检查所有 Graph 并输出统计"""
    graph_dir = Path(OUTPUT_CONFIG["graph_dir"])
    
    print("=" * 80)
    print("Graph 节点统计（用于确定 expected 配置）")
    print("=" * 80)
    print()
    
    for case in TEST_CASES:
        print(f"## {case['name']}")
        print()
        
        for func in case['functions']:
            file_path = func['file']
            function_name = func['name']
            prefix = get_output_prefix(file_path)
            
            without_file = graph_dir / f"{prefix}-{function_name}-without.txt"
            with_file = graph_dir / f"{prefix}-{function_name}-with.txt"
            
            graph_without = load_graph(without_file)
            graph_with = load_graph(with_file)
            
            if not graph_without and not graph_with:
                print(f"### {function_name} ({prefix}) - ❌ Graph 文件缺失")
                print()
                continue
            
            print(f"### {function_name} ({prefix})")
            print()
            print("| 节点类型 | 优化前 | 优化后 | 变化 |")
            print("|---------|-------|-------|------|")
            
            expected_parts = []
            for node in INTERESTING_NODES:
                before = count_node(graph_without, node)
                after = count_node(graph_with, node)
                
                if before == 0 and after == 0:
                    continue  # 跳过不相关的节点
                
                change = ""
                if before > after:
                    change = f"↓{before - after}"
                elif after > before:
                    change = f"↑{after - before}"
                else:
                    change = "="
                
                print(f"| {node} | {before} | {after} | {change} |")
                
                # 生成 expected 配置建议
                if before != after:
                    expected_parts.append(f'"{node}": {{"before": {before}, "after": {after}}}')
                elif after > 0:
                    expected_parts.append(f'"{node}": {{"after": {after}}}')
            
            # 检查 LoadField length（特殊处理）
            before_lf = count_node(graph_without, "LoadField.*length")
            after_lf = count_node(graph_with, "LoadField.*length")
            if before_lf > 0 or after_lf > 0:
                change = ""
                if before_lf > after_lf:
                    change = f"↓{before_lf - after_lf}"
                elif after_lf > before_lf:
                    change = f"↑{after_lf - before_lf}"
                else:
                    change = "="
                print(f"| LoadField[length] | {before_lf} | {after_lf} | {change} |")
                if before_lf != after_lf:
                    expected_parts.append(f'"LoadField.*length": {{"before": {before_lf}, "after": {after_lf}}}')
            
            print()
            
            # 输出建议的 expected 配置
            if expected_parts:
                print("**建议配置：**")
                print("```python")
                print('"expected": {')
                print('    ' + ',\n    '.join(expected_parts))
                print('}')
                print("```")
            else:
                print("**无明显变化**")
            print()
        
        print()


if __name__ == "__main__":
    inspect_all()

