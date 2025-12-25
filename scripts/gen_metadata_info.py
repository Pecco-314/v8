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


def get_file_hash(file_path):
    """计算文件的 SHA256 哈希"""
    result = subprocess.run(['sha256sum', file_path], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"❌ 无法计算哈希: {file_path}", file=sys.stderr)
        return None
    return result.stdout.split()[0]


def get_function_positions(file_path, d8_path="out.gn/x64.debug/d8", mjsunit_path="test/mjsunit/mjsunit.js"):
    """
    使用 d8 --print-ast 获取所有函数的位置
    返回 [(函数名, start_position), ...]
    """
    # 运行 d8 --print-ast（需要加载 mjsunit.js）
    result = subprocess.run(
        [d8_path, '--allow-natives-syntax', '--print-ast', mjsunit_path, file_path],
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
            
            if func_name:
                functions.append((func_name, position))
        
        i += 1
    
    return functions


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

