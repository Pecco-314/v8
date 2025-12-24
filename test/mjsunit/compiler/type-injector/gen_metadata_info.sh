#!/bin/bash

# 生成 metadata 所需信息的脚本
# 用法: ./gen_metadata_info.sh <js文件>
# 输出: 文件哈希、函数名及起始位置

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
V8_DIR="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
D8="${D8:-$V8_DIR/out.gn/x64.debug/d8}"

if [ $# -eq 0 ]; then
    echo "用法: $0 <js文件> [js文件2] ..."
    echo "示例: $0 test-string.js"
    echo ""
    echo "环境变量:"
    echo "  D8 - 指定 d8 路径 (默认: $D8)"
    exit 1
fi

for file in "$@"; do
    if [ ! -f "$file" ]; then
        echo "错误: 文件不存在: $file"
        continue
    fi
    
    echo "=========================================="
    echo "文件: $file"
    echo "=========================================="
    
    # 计算 SHA256 哈希
    hash=$(sha256sum "$file" | cut -d' ' -f1)
    echo "哈希: $hash"
    echo ""
    
    # 获取函数名和位置
    echo "函数列表:"
    "$D8" --allow-natives-syntax --print-ast "$file" 2>&1 | \
        awk '
        /FUNC at/ { pos = $3 }
        /^\. NAME "/ {
            # 提取函数名（去掉 ". NAME " 和引号）
            name = $0
            gsub(/^\. NAME "/, "", name)
            gsub(/"$/, "", name)
            if (name != "") {
                printf "  - %s @ %s\n", name, pos
            }
        }
        '
done

