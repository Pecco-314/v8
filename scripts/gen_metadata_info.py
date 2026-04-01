#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
生成测试文件的 metadata 信息
输出文件哈希和所有函数的位置
"""

import sys
import subprocess
import re
from pathlib import Path


NON_FUNCTION_KEYWORDS = {
    'if', 'else', 'for', 'while', 'switch', 'case', 'default',
    'return', 'throw', 'try', 'catch', 'finally', 'do', 'break', 'continue'
}


def extract_declared_function_names(source_text):
    """提取源码中声明的业务函数名（支持多种写法）"""
    names = set()

    patterns = [
        r'(?:^|\n)\s*function\s+([A-Za-z_$][\w$]*)\s*\(',
        r'(?:^|\n)\s*(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*function\b',
        r'(?:^|\n)\s*(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*(?:async\s+)?\([^\)]*\)\s*=>',
        r'(?:^|\n)\s*(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*(?:async\s+)?[A-Za-z_$][\w$]*\s*=>',
    ]

    for pattern in patterns:
        for match in re.finditer(pattern, source_text, re.MULTILINE):
            name = match.group(1)
            if name:
                names.add(name)

    class_method_pattern = r'^[ \t]*(?:async\s+)?([A-Za-z_$][\w$]*)\s*\([^\)]*\)\s*\{'
    class_block_pattern = r'class\s+[A-Za-z_$][\w$]*\s*\{([\s\S]*?)\n\}'
    for class_block in re.finditer(class_block_pattern, source_text, re.MULTILINE):
        body = class_block.group(1)
        for method in re.finditer(class_method_pattern, body, re.MULTILINE):
            method_name = method.group(1)
            if method_name and method_name not in {'constructor'} and method_name not in NON_FUNCTION_KEYWORDS:
                names.add(method_name)

    return names


def extract_function_positions_from_source(source_text):
    """从源码文本直接提取函数位置，作为 d8 AST 解析失败时的回退。"""
    positions = []

    patterns = [
        r'function\s+([A-Za-z_$][\w$]*)\s*\(',
        r'(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*function\b',
        r'(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*(?:async\s+)?\([^\)]*\)\s*=>',
        r'(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*(?:async\s+)?[A-Za-z_$][\w$]*\s*=>',
    ]

    for pattern in patterns:
        for match in re.finditer(pattern, source_text, re.MULTILINE):
            name = match.group(1)
            if name:
                positions.append((name, match.start(1)))

    class_block_pattern = r'class\s+[A-Za-z_$][\w$]*\s*\{([\s\S]*?)\n\}'
    class_method_pattern = r'^[ \t]*(?:async\s+)?([A-Za-z_$][\w$]*)\s*\([^\)]*\)\s*\{'
    for class_block in re.finditer(class_block_pattern, source_text, re.MULTILINE):
        body = class_block.group(1)
        body_offset = class_block.start(1)
        for method in re.finditer(class_method_pattern, body, re.MULTILINE):
            method_name = method.group(1)
            if method_name and method_name not in {'constructor'} and method_name not in NON_FUNCTION_KEYWORDS:
                positions.append((method_name, body_offset + method.start(1)))

    seen = set()
    deduped = []
    for item in sorted(positions, key=lambda x: x[1]):
        if item not in seen:
            seen.add(item)
            deduped.append(item)
    return deduped


def get_file_hash(file_path):
    """计算文件的 SHA256 哈希"""
    result = subprocess.run(['sha256sum', file_path], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"❌ 无法计算哈希: {file_path}", file=sys.stderr)
        return None
    return result.stdout.split()[0]


def get_function_positions(file_path, d8_path="out.gn/x64.debug/d8"):
    """
    使用 d8 --print-ast 获取所有函数的位置
    返回 [(函数名, start_position), ...]
    通过在 harness 中标记测试文件起始位置来过滤
    """
    import tempfile
    import os
    
    # 创建临时 harness 文件，使用最小化的 stub 函数
    stub_prelude = ';'.join([
        'function assertEquals(){}',
        'function assertOptimized(){}',
        'function assertUnoptimized(){}',
        'function assertTrue(){}',
        'function assertFalse(){}',
    ])
    
    # 创建临时目录和 harness 文件
    with tempfile.TemporaryDirectory(prefix='genmeta-') as tmpdir:
        harness_path = os.path.join(tmpdir, 'harness.js')
        abs_test_path = os.path.abspath(file_path)
        
        # 读取测试文件内容以获取实际函数名列表
        with open(abs_test_path, 'r') as f:
            test_content = f.read()
        
        test_functions = extract_declared_function_names(test_content)
        
        # 构建 harness
        with open(harness_path, 'w') as f:
            f.write(f"{stub_prelude}; load('{abs_test_path}');")
        
        # 运行 d8 --print-ast
        result = subprocess.run(
            [d8_path, '--allow-natives-syntax', '--print-ast', harness_path],
            capture_output=True,
            text=True
        )
    
    if result.returncode != 0:
        print(f"❌ d8 执行失败: {file_path}", file=sys.stderr)
        return []
    
    output = result.stdout + result.stderr
    functions = []
    
    # 解析 AST 输出
    # 格式示例：
    # FUNC at 280
    # . NAME "twice_s"
    # 或者：
    # . FUNC LITERAL at 280
    #   . NAME "twice_s"
    
    lines = output.split('\n')
    i = 0
    while i < len(lines):
        line = lines[i]
        
        # 匹配 "FUNC at <position>" 或 "FUNC LITERAL at <position>"
        func_match = re.search(r'FUNC(?: LITERAL)? at (\d+)', line)
        if func_match:
            position = int(func_match.group(1))
            
            # 查找后续行中的函数名
            # 通常在 FUNC 行之后的几行内
            func_name = None
            for j in range(i + 1, min(i + 10, len(lines))):
                name_match = re.search(r'NAME "([^"]+)"', lines[j])
                if name_match:
                    func_name = name_match.group(1)
                    break
            
            if func_name:  # 忽略空名称
                # 只保留在测试文件中实际声明的函数
                if func_name in test_functions:
                    functions.append((func_name, position))
        
        i += 1
    
    parsed = sorted(set(functions), key=lambda item: item[1])
    if parsed:
        return parsed

    # d8 AST 在不同版本上格式可能差异较大；回退到源码解析避免误报“未找到函数”。
    with open(file_path, 'r') as f:
        source_text = f.read()
    fallback = extract_function_positions_from_source(source_text)
    return fallback


def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("用法: python3 gen_metadata_info.py <test_file.js>")
        sys.exit(1)
    
    file_path = sys.argv[1]
    
    if not Path(file_path).exists():
        print(f"❌ 文件不存在: {file_path}", file=sys.stderr)
        sys.exit(1)
    
    print("=" * 60)
    print(f"文件: {file_path}")
    print("=" * 60)
    
    # 计算哈希
    file_hash = get_file_hash(file_path)
    if not file_hash:
        sys.exit(1)
    
    print(f"哈希: {file_hash}")
    print()
    
    # 获取函数位置
    functions = get_function_positions(file_path)
    
    if not functions:
        print("⚠️  未找到任何函数")
        sys.exit(0)
    
    print("函数列表:")
    for func_name, position in functions:
        print(f"  - {func_name:20s} @ {position}")
    
    print()
    print("=" * 60)
    print("Metadata 模板:")
    print("=" * 60)
    for func_name, position in functions:
        print(f"{position} @params any <类型> @ret <返回类型>")
    print()
    print(f"保存到: test/mjsunit/compiler/type-injector/metadata/{file_hash}.metadata")


if __name__ == "__main__":
    main()

