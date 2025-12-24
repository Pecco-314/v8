#!/bin/bash

# 验证 TypeInjector 优化效果的脚本
# 对比有/无 metadata 时的编译图，检查特定节点是否被优化
#
# 用法: ./verify_optimization.sh [d8路径]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
V8_DIR="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
D8="${1:-$V8_DIR/out.gn/x64.debug/d8}"
METADATA_PATH="$SCRIPT_DIR/metadata"
MJSUNIT="$V8_DIR/test/mjsunit/mjsunit.js"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "=========================================="
echo "TypeInjector 优化效果验证"
echo "=========================================="
echo ""

# 对比测试函数
# 参数: 测试名, 测试文件, 目标函数名, 优化前应有的节点, 优化后应有的节点, 优化后应移除的节点
compare_test() {
    local name="$1"
    local file="$2"
    local func="$3"
    local before_has="$4"
    local after_has="$5"
    local after_removed="$6"
    
    echo -e "${CYAN}=== $name ===${NC}"
    echo "文件: $file"
    echo "函数: $func"
    echo ""
    
    # 无 metadata 运行
    echo -e "${YELLOW}[无 metadata]${NC}"
    local output_without=$("$D8" --allow-natives-syntax --turbofan --no-always-turbofan \
        --trace-turbo-graph \
        "$MJSUNIT" "$file" 2>&1 || true)
    
    # 检查优化前应有的节点
    if [ -n "$before_has" ]; then
        IFS=',' read -ra PATTERNS <<< "$before_has"
        for p in "${PATTERNS[@]}"; do
            local cnt=$(echo "$output_without" | grep -c "$p" || true)
            echo "  $p: $cnt 处"
        done
    fi
    
    # 有 metadata 运行
    echo ""
    echo -e "${YELLOW}[有 metadata]${NC}"
    local output_with=$("$D8" --allow-natives-syntax --turbofan --no-always-turbofan \
        --turbo_metadata_path="$METADATA_PATH" \
        --trace-turbo-graph \
        "$MJSUNIT" "$file" 2>&1 || true)
    
    # 检查优化后应有的节点
    if [ -n "$after_has" ]; then
        IFS=',' read -ra PATTERNS <<< "$after_has"
        for p in "${PATTERNS[@]}"; do
            local cnt=$(echo "$output_with" | grep -c "$p" || true)
            if [ "$cnt" -gt 0 ]; then
                echo -e "  ${GREEN}✓${NC} $p: $cnt 处"
            else
                echo -e "  ${RED}✗${NC} $p: 未找到"
            fi
        done
    fi
    
    # 检查优化后应移除的节点
    if [ -n "$after_removed" ]; then
        IFS=',' read -ra PATTERNS <<< "$after_removed"
        for p in "${PATTERNS[@]}"; do
            local cnt_before=$(echo "$output_without" | grep -c "$p" || true)
            local cnt_after=$(echo "$output_with" | grep -c "$p" || true)
            if [ "$cnt_after" -lt "$cnt_before" ]; then
                echo -e "  ${GREEN}✓${NC} $p: $cnt_before → $cnt_after (已减少)"
            elif [ "$cnt_after" -eq 0 ]; then
                echo -e "  ${GREEN}✓${NC} $p: 已完全移除"
            else
                echo -e "  ${YELLOW}!${NC} $p: $cnt_before → $cnt_after"
            fi
        done
    fi
    
    echo ""
}

# 根据 exam.md 中的实验结果进行验证

# String: CheckedTaggedToTaggedPointer 和 CheckString 应被移除
compare_test "String 类型" \
    "$SCRIPT_DIR/test-string.js" \
    "twice_s" \
    "CheckString,CheckedTaggedToTaggedPointer" \
    "StringLength,StringConcat" \
    "CheckString,CheckedTaggedToTaggedPointer"

# Number: CheckedTaggedToFloat64 应变成 ChangeTaggedToFloat64
compare_test "Number 类型" \
    "$SCRIPT_DIR/test-number.js" \
    "twice_f" \
    "CheckedTaggedToFloat64" \
    "ChangeTaggedToFloat64,Float64Add" \
    "CheckedTaggedToFloat64"

# Boolean: TruncateTaggedToBit 应变成 ChangeTaggedToBit
compare_test "Boolean 类型" \
    "$SCRIPT_DIR/test-boolean.js" \
    "cal" \
    "TruncateTaggedToBit" \
    "ChangeTaggedToBit" \
    "TruncateTaggedToBit"

# Interface: CheckString 应被移除，CheckMaps 保留
compare_test "Interface 类型" \
    "$SCRIPT_DIR/test-interface.js" \
    "concat" \
    "CheckString,CheckMaps" \
    "CheckMaps,StringConcat" \
    "CheckString"

# 嵌套 Interface: CheckString 应被移除
compare_test "嵌套 Interface 类型" \
    "$SCRIPT_DIR/test-interface-nested.js" \
    "concat_nested" \
    "CheckString" \
    "StringConcat" \
    "CheckString"

# Array: CheckString 和 CheckedTaggedToTaggedPointer 应被移除
compare_test "Array 类型" \
    "$SCRIPT_DIR/test-array.js" \
    "concat_arr" \
    "CheckString" \
    "StringConcat" \
    "CheckString"

echo "=========================================="
echo "验证完成"
echo "=========================================="

