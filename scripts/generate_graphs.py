#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
生成 TurboFan Graph 文件
从 d8 输出中提取 EarlyOptimization 阶段的 Graph
"""

import subprocess
import re
import os
from pathlib import Path
from test_config import TEST_CASES, V8_CONFIG, OUTPUT_CONFIG


def run_d8(test_file, flags):
    """运行 d8 并返回输出"""
    cmd = [V8_CONFIG["d8_path"]] + V8_CONFIG["base_flags"] + flags + [
        V8_CONFIG["mjsunit_path"],
        test_file
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.stdout + result.stderr


def extract_function_graph(output, function_name):
    """提取特定函数的 EarlyOptimization Graph"""
    lines = output.split('\n')
    in_target_function = False
    in_early_optimization = False
    graph_lines = []
    
    for line in lines:
        # 检测函数编译开始
        if 'Begin compiling method' in line:
            in_target_function = function_name in line
            in_early_optimization = False
            graph_lines = []
        
        # 检测 EarlyOptimization 阶段
        if in_target_function and '----- Graph after V8.TFEarlyOptimization' in line:
            in_early_optimization = True
            graph_lines.append(line)
            continue
        
        # 检测 Graph 结束
        if in_early_optimization and line.startswith('----- '):
            break
        
        # 收集 Graph 内容
        if in_early_optimization:
            graph_lines.append(line)
    
    return '\n'.join(graph_lines) if graph_lines else None


def get_output_prefix(file_path):
    """从文件路径获取输出前缀"""
    stem = Path(file_path).stem  # test-string -> test-string
    return stem.replace('test-', '')  # -> string


def generate_graphs():
    """生成所有测试的 Graph 文件"""
    graph_dir = Path(OUTPUT_CONFIG["graph_dir"])
    graph_dir.mkdir(parents=True, exist_ok=True)
    
    print("=" * 60)
    print("TurboFan Graph 生成工具")
    print("=" * 60)
    print()
    
    for case in TEST_CASES:
        print(f"📋 {case['name']}")
        
        for func in case['functions']:
            file_path = func['file']
            function_name = func['name']
            flags = func['flags']
            
            prefix = get_output_prefix(file_path)
            
            # 无优化 Graph
            output_without = run_d8(file_path, [])
            graph_without = extract_function_graph(output_without, function_name)
            
            # 有优化 Graph
            output_with = run_d8(file_path, flags)
            graph_with = extract_function_graph(output_with, function_name)
            
            # 保存文件
            without_file = graph_dir / f"{prefix}-{function_name}-without.txt"
            with_file = graph_dir / f"{prefix}-{function_name}-with.txt"
            
            status = "✅" if graph_without and graph_with else "❌"
            
            if graph_without:
                with open(without_file, 'w', encoding='utf-8') as f:
                    f.write(graph_without)
            
            if graph_with:
                with open(with_file, 'w', encoding='utf-8') as f:
                    f.write(graph_with)
            
            print(f"  {status} {function_name} ({prefix})")
        
        print()
    
    print("✅ Graph 生成完成！")


if __name__ == "__main__":
    generate_graphs()
