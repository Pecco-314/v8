#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
分析优化效果并生成 Markdown 文档
"""

import re
from pathlib import Path
from test_config import TEST_CASES, OUTPUT_CONFIG


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


def load_test_code(file_path, function_name):
    """加载测试代码中的函数"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 简单提取函数定义
        pattern = rf'function\s+{function_name}\s*\([^)]*\)\s*\{{[^}}]*\}}'
        match = re.search(pattern, content, re.DOTALL)
        if match:
            return match.group(0)
        return None
    except FileNotFoundError:
        return None


def load_metadata(file_path, function_name):
    """加载对应函数的 metadata 行"""
    import subprocess
    
    # 计算哈希
    try:
        result = subprocess.run(['sha256sum', file_path], capture_output=True, text=True)
        if result.returncode != 0:
            return None
        hash_value = result.stdout.split()[0]
        
        metadata_dir = Path("test/mjsunit/compiler/type-injector/metadata")
        metadata_file = metadata_dir / f"{hash_value}.metadata"
        
        if not metadata_file.exists():
            return None
        
        # 获取函数位置
        d8_result = subprocess.run(
            ['out.gn/x64.debug/d8', '--allow-natives-syntax', '--print-ast', 
             'test/mjsunit/mjsunit.js', file_path],
            capture_output=True,
            text=True
        )
        
        # 从 AST 输出中找到函数位置
        output = d8_result.stdout + d8_result.stderr
        func_position = None
        
        lines = output.split('\n')
        for i, line in enumerate(lines):
            func_match = re.search(r'FUNC(?: LITERAL)? at (\d+)', line)
            if func_match:
                position = int(func_match.group(1))
                # 查找函数名
                for j in range(i + 1, min(i + 10, len(lines))):
                    name_match = re.search(r'NAME "([^"]+)"', lines[j])
                    if name_match and name_match.group(1) == function_name:
                        func_position = position
                        break
                if func_position:
                    break
        
        if not func_position:
            return None
        
        # 从 metadata 文件中找到对应行
        with open(metadata_file, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if line and line.startswith(str(func_position)):
                    return line
        
    except Exception:
        pass
    
    return None


def generate_function_section(func_config, graph_dir):
    """生成单个函数的文档部分"""
    file_path = func_config['file']
    function_name = func_config['name']
    expected = func_config['expected']
    flags = func_config['flags']
    
    prefix = get_output_prefix(file_path)
    
    without_file = graph_dir / f"{prefix}-{function_name}-without.txt"
    with_file = graph_dir / f"{prefix}-{function_name}-with.txt"
    
    graph_without = load_graph(without_file)
    graph_with = load_graph(with_file)
    
    lines = []
    lines.append(f"#### 函数: `{function_name}`")
    lines.append("")
    
    # 测试代码
    code = load_test_code(file_path, function_name)
    if code:
        lines.append("**测试代码：**")
        lines.append("")
        lines.append("```javascript")
        lines.append(code)
        lines.append("```")
        lines.append("")
    
    # Metadata
    metadata = load_metadata(file_path, function_name)
    if metadata:
        lines.append("**Metadata：**")
        lines.append("")
        lines.append("```")
        lines.append(metadata)
        lines.append("```")
        lines.append("")
    
    # Flags
    if flags:
        lines.append("**优化 Flags：**")
        lines.append("")
        lines.append("```")
        lines.append(' '.join(flags))
        lines.append("```")
        lines.append("")
    
    # 优化效果表格
    lines.append("**优化效果：**")
    lines.append("")
    lines.append("| 节点类型 | 优化前 | 优化后 |")
    lines.append("|---------|-------|-------|")
    
    for pattern, expect in expected.items():
        before = expect.get('before', '-')
        after = expect.get('after', '-')
        lines.append(f"| `{pattern}` | {before} | {after} |")
    
    lines.append("")
    
    # 优化前 Graph
    lines.append("**优化前 Graph (EarlyOptimization)：**")
    lines.append("")
    lines.append("```")
    if graph_without:
        lines.append(graph_without)
    else:
        lines.append("(Graph 文件缺失)")
    lines.append("```")
    lines.append("")
    
    # 优化后 Graph
    lines.append("**优化后 Graph (EarlyOptimization)：**")
    lines.append("")
    lines.append("```")
    if graph_with:
        lines.append(graph_with)
    else:
        lines.append("(Graph 文件缺失)")
    lines.append("```")
    lines.append("")
    
    return '\n'.join(lines)


def generate_case_section(case, case_number, graph_dir):
    """生成单个测试用例的文档"""
    lines = []
    lines.append(f"### {case_number}. {case['name']}")
    lines.append("")
    
    if case.get('description'):
        lines.append(case['description'])
        lines.append("")
    
    for func in case['functions']:
        lines.append(generate_function_section(func, graph_dir))
    
    return '\n'.join(lines)


def main():
    """主函数"""
    print("=" * 60)
    print("Markdown 文档生成工具")
    print("=" * 60)
    print()
    
    graph_dir = Path(OUTPUT_CONFIG["graph_dir"])
    sections_dir = Path(OUTPUT_CONFIG["sections_dir"])
    sections_dir.mkdir(parents=True, exist_ok=True)
    
    # 为每个测试用例生成单独的 markdown 文件
    for idx, case in enumerate(TEST_CASES, 1):
        print(f"生成: {case['name']}")
        
        section_content = generate_case_section(case, idx, graph_dir)
        
        # 生成文件名
        case_name = case['name'].replace(' ', '-').replace('/', '-')
        section_file = sections_dir / f"{idx:02d}-{case_name}.md"
        
        with open(section_file, 'w', encoding='utf-8') as f:
            f.write(section_content)
        
        print(f"  保存: {section_file}")
    
    print()
    print("✅ 所有文档生成完成！")
    print()
    print(f"文档位置: {sections_dir}/")


if __name__ == "__main__":
    main()
