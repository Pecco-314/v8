#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
验证测试结果
检查所有测试是否符合预期
"""

import re
import sys
from pathlib import Path
from test_config import TEST_CASES, OUTPUT_CONFIG


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


def verify_function(func_config):
    """验证单个函数的优化效果"""
    file_path = func_config['file']
    function_name = func_config['name']
    expected = func_config['expected']
    
    graph_dir = Path(OUTPUT_CONFIG["graph_dir"])
    prefix = get_output_prefix(file_path)
    
    without_file = graph_dir / f"{prefix}-{function_name}-without.txt"
    with_file = graph_dir / f"{prefix}-{function_name}-with.txt"
    
    graph_without = load_graph(without_file)
    graph_with = load_graph(with_file)
    
    if not graph_without or not graph_with:
        return False, "Graph 文件缺失", []
    
    results = []
    all_passed = True
    
    for pattern, expect in expected.items():
        before = count_node(graph_without, pattern) if 'before' in expect else None
        after = count_node(graph_with, pattern) if 'after' in expect else None
        
        passed = True
        if 'before' in expect and before != expect['before']:
            passed = False
        if 'after' in expect and after != expect['after']:
            passed = False
        
        if not passed:
            all_passed = False
        
        results.append({
            'pattern': pattern,
            'before': before,
            'after': after,
            'expected_before': expect.get('before'),
            'expected_after': expect.get('after'),
            'passed': passed
        })
    
    return all_passed, None, results


def main():
    """主函数"""
    print("=" * 60)
    print("测试验证工具")
    print("=" * 60)
    print()
    
    total = 0
    passed = 0
    failed = 0
    failed_tests = []
    
    for case in TEST_CASES:
        print(f"📋 {case['name']}")
        
        for func in case['functions']:
            total += 1
            all_passed, error, results = verify_function(func)
            
            function_name = func['name']
            prefix = get_output_prefix(func['file'])
            
            if error:
                print(f"  ❌ {function_name} ({prefix}): {error}")
                failed += 1
                failed_tests.append(f"{case['name']} / {function_name}")
                continue
            
            if all_passed:
                print(f"  ✅ {function_name} ({prefix})")
                passed += 1
            else:
                print(f"  ❌ {function_name} ({prefix}):")
                failed += 1
                failed_tests.append(f"{case['name']} / {function_name}")
                
                for r in results:
                    if not r['passed']:
                        before_str = str(r['before']) if r['before'] is not None else '-'
                        after_str = str(r['after']) if r['after'] is not None else '-'
                        exp_before = r['expected_before'] if r['expected_before'] is not None else '-'
                        exp_after = r['expected_after'] if r['expected_after'] is not None else '-'
                        print(f"      {r['pattern']}: 实际({before_str}→{after_str}) vs 预期(前{exp_before}, 后{exp_after})")
        
        print()
    
    # 汇总
    print("=" * 60)
    print("测试汇总")
    print("=" * 60)
    print(f"总测试数: {total}")
    print(f"通过: {passed} ✅")
    print(f"失败: {failed} ❌")
    
    if failed > 0:
        print()
        print("失败的测试:")
        for test in failed_tests:
            print(f"  - {test}")
        sys.exit(1)
    else:
        print()
        print("🎉 所有测试通过！")
        sys.exit(0)


if __name__ == "__main__":
    main()
