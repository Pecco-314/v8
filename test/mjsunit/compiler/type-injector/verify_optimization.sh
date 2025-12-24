#!/bin/bash

# TypeInjector 优化效果验证脚本
# 
# 功能：对比有无 metadata 时 SimplifiedLowering phase 之后的最终 Graph
# 说明：SimplifiedLowering 是优化流程的最后阶段，此时的 Graph 最能反映优化效果

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 路径配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
D8="$SCRIPT_DIR/../../../../out.gn/x64.debug/d8"
MJSUNIT="$SCRIPT_DIR/../../../mjsunit/mjsunit.js"
METADATA_PATH="$SCRIPT_DIR/metadata"

# 测试统计
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 提取 EarlyOptimization phase 之后的最终 Graph
# EarlyOptimization 阶段经过了 DCE 和节点折叠，更能反映优化效果
# 参数：$1 - 完整输出，$2 - 函数名（可选，用于提取特定函数的Graph）
extract_final_graph() {
    local output="$1"
    local func_name="$2"
    
    if [ -z "$func_name" ]; then
        # 提取第一个 EarlyOptimization Graph
        echo "$output" | awk '/----- Graph after.*EarlyOptimization/ {found=1; print; next} found && /^----- schedule/ {exit} found'
    else
        # 提取特定函数的 EarlyOptimization Graph
        echo "$output" | awk -v fname="$func_name" '
            /SharedFunctionInfo/ && $0 ~ fname {in_func=1}
            in_func && /----- Graph after.*EarlyOptimization/ {found=1; print; next}
            found && /^----- schedule/ {exit}
            found
        '
    fi
}

# 统计节点数量
# 参数：$1 - 完整输出，$2 - 节点模式，$3 - 函数名（可选）
count_nodes() {
    local output="$1"
    local pattern="$2"
    local func_name="$3"
    local graph=$(extract_final_graph "$output" "$func_name")
    
    # 对于 LoadField.*length 这种特殊模式
    if [[ "$pattern" == *"LoadField"* && "$pattern" == *"length"* ]]; then
        echo "$graph" | grep -iE "^#[0-9]+:LoadField\[.*length" | grep -oE "^#[0-9]+:" | sort -u | wc -l
    else
        # 一般模式：提取 #数字:节点名 格式，去重后统计
        echo "$graph" | grep -oE "#[0-9]+:$pattern" | sort -u | wc -l
    fi
}

# 单个测试
# 参数：
#   $1 - 测试名称
#   $2 - 测试文件路径
#   $3 - 函数名
#   $4 - 优化前应存在的节点（逗号分隔）
#   $5 - 优化后应存在的节点（逗号分隔）
#   $6 - 优化后应移除的节点（逗号分隔）
run_test() {
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
    
    local test_passed=true
    
    # 检查是否需要禁止内联
    local noinline_flag=""
    if [[ "$file" == *"function-call"* ]]; then
        noinline_flag="--max_inlined_bytecode_size=0"
    fi
    
    # 无 metadata 运行
    local output_without=$("$D8" --allow-natives-syntax --turbofan --no-always-turbofan \
        $noinline_flag \
        --trace-turbo-graph \
        "$MJSUNIT" "$file" 2>&1 || true)
    
    # 有 metadata 运行
    local output_with=$("$D8" --allow-natives-syntax --turbofan --no-always-turbofan \
        --turbo_metadata_path="$METADATA_PATH" \
        $noinline_flag \
        --trace-turbo-graph \
        "$MJSUNIT" "$file" 2>&1 || true)
    
    # 检查优化前应有的节点
    echo -e "${YELLOW}[无 metadata - 最终 Graph]${NC}"
    if [ -n "$before_has" ]; then
        IFS=',' read -r -a PATTERNS <<< "$before_has"
        for p in "${PATTERNS[@]}"; do
            local cnt=$(count_nodes "$output_without" "$p" "$func")
            if [ "$cnt" -gt 0 ]; then
                echo "  $p: $cnt 个"
            fi
        done
    fi
    
    # 检查优化后应有的节点
    echo ""
    echo -e "${YELLOW}[有 metadata - 最终 Graph]${NC}"
    if [ -n "$after_has" ]; then
        IFS=',' read -r -a PATTERNS <<< "$after_has"
        for p in "${PATTERNS[@]}"; do
            local cnt=$(count_nodes "$output_with" "$p" "$func")
            if [ "$cnt" -gt 0 ]; then
                echo -e "  ${GREEN}✓${NC} $p: $cnt 个"
            else
                echo -e "  ${RED}✗${NC} $p: 未找到"
                test_passed=false
            fi
        done
    fi
    
    # 检查优化后应移除的节点
    if [ -n "$after_removed" ]; then
        IFS=',' read -r -a PATTERNS <<< "$after_removed"
        for p in "${PATTERNS[@]}"; do
            local cnt_before=$(count_nodes "$output_without" "$p" "$func")
            local cnt_after=$(count_nodes "$output_with" "$p" "$func")
            if [ "$cnt_after" -eq 0 ] && [ "$cnt_before" -gt 0 ]; then
                echo -e "  ${GREEN}✓${NC} $p: $cnt_before → $cnt_after (已完全移除)"
            elif [ "$cnt_after" -lt "$cnt_before" ]; then
                echo -e "  ${GREEN}✓${NC} $p: $cnt_before → $cnt_after (已减少)"
            elif [ "$cnt_before" -eq 0 ] && [ "$cnt_after" -eq 0 ]; then
                echo -e "  ${GREEN}✓${NC} $p: $cnt_before → $cnt_after (未出现)"
            else
                echo -e "  ${RED}✗${NC} $p: $cnt_before → $cnt_after (未移除)"
                test_passed=false
            fi
        done
    fi
    
    echo ""
    
    # 更新统计
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if [ "$test_passed" = true ]; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
        echo -e "${GREEN}测试通过${NC}"
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
        echo -e "${RED}测试失败${NC}"
    fi
    echo ""
}

# 主程序
main() {
    echo "=========================================="
    echo "TypeInjector 优化效果验证"
    echo "（基于 EarlyOptimization Phase 最终 Graph）"
    echo "=========================================="
    echo ""
    
    # String 类型
    run_test "String 类型" \
        "$SCRIPT_DIR/test-string.js" \
        "twice_s" \
        "CheckString" \
        "StringConcat" \
        "CheckString"
    
    # Number 类型
    run_test "Number 类型" \
        "$SCRIPT_DIR/test-number.js" \
        "twice_f" \
        "" \
        "ChangeTaggedToFloat64,Float64Add" \
        ""
    
    # Boolean 类型
    run_test "Boolean 类型" \
        "$SCRIPT_DIR/test-boolean.js" \
        "cal" \
        "" \
        "ChangeTaggedToBit" \
        ""
    
    # Interface 类型
    run_test "Interface 类型" \
        "$SCRIPT_DIR/test-interface.js" \
        "concat" \
        "CheckString,CheckMaps" \
        "CheckMaps,StringConcat" \
        "CheckString"
    
    # 嵌套 Interface 类型
    run_test "嵌套 Interface 类型" \
        "$SCRIPT_DIR/test-interface-nested.js" \
        "concat_nested" \
        "CheckString" \
        "StringConcat" \
        "CheckString"
    
    # Array 类型
    run_test "Array 类型" \
        "$SCRIPT_DIR/test-array.js" \
        "concat_arr" \
        "CheckString" \
        "StringConcat" \
        "CheckString"
    
    # Tuple 类型（同类型元素）
    run_test "Tuple 类型 (同类型元素)" \
        "$SCRIPT_DIR/test-tuple.js" \
        "concat" \
        "CheckString" \
        "StringConcat" \
        "CheckString"
    
    # Tuple 类型（混合类型元素）
    run_test "Tuple 类型 (混合类型元素)" \
        "$SCRIPT_DIR/test-tuple-mixed.js" \
        "process" \
        "CheckString" \
        "StringConcat" \
        "CheckString"
    
    # Tuple Length 优化
    run_test "Tuple Length 优化" \
        "$SCRIPT_DIR/test-tuple-length.js" \
        "getLength" \
        "LoadField\[.*length" \
        "" \
        "LoadField\[.*length"
    
    # 函数调用返回值类型（禁止内联）
    echo ""
    run_test "函数调用返回值类型" \
        "$SCRIPT_DIR/test-function-call.js" \
        "process" \
        "CheckString,CheckedTaggedToTaggedPointer" \
        "StringConcat" \
        "CheckString,CheckedTaggedToTaggedPointer"
    
    # 输出汇总
    echo "=========================================="
    echo "测试汇总"
    echo "=========================================="
    echo -e "总测试数: ${CYAN}$TOTAL_TESTS${NC}"
    echo -e "通过: ${GREEN}$PASSED_TESTS${NC}"
    echo -e "失败: ${RED}$FAILED_TESTS${NC}"
    echo "=========================================="
    
    # 返回失败数作为退出码
    exit $FAILED_TESTS
}

main "$@"
