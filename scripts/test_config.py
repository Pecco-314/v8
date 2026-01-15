#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试配置文件
定义所有测试用例的参数和预期优化效果
"""

# V8 配置
V8_CONFIG = {
    "d8_path": "out.gn/x64.debug/d8",
    "mjsunit_path": "test/mjsunit/mjsunit.js",
    "base_flags": [
        "--allow-natives-syntax",
        "--turbofan",
        "--no-always-turbofan",
        "--trace-turbo-graph"
    ]
}

# 输出配置
OUTPUT_CONFIG = {
    "graph_dir": "docs/graphs/early-optimization",
    "sections_dir": "docs/sections",
    "main_doc": "docs/type-injector-optimization.md"
}

# Metadata flags（简化常用配置）
METADATA_FLAGS = ["--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata"]
BUILTIN_FLAGS = ["--turbo_builtin_type_table"]
NOINLINE_FLAGS = ["--max_inlined_bytecode_size=0"]

# 测试用例配置
# 按功能分组，每个 case 包含一个测试文件和多个测试函数
TEST_CASES = [
    # ============================================
    # 1. 基本类型优化
    # ============================================
    {
        "name": "基本类型优化",
        "description": "String、Number、Boolean、Symbol、BigInt 基本类型的类型注入优化",
        "functions": [
            {
                "file": "test/mjsunit/compiler/type-injector/test-primitive.js",
                "name": "twice_s",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 1, "after": 0},
                    "CheckString": {"before": 1, "after": 0}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-primitive.js",
                "name": "twice_f",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedTaggedToFloat64": {"before": 1, "after": 0},
                    "ChangeTaggedToFloat64": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-primitive.js",
                "name": "twice_smi",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedTaggedToFloat64": {"before": 1, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-primitive.js",
                "name": "twice_mixed",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedTaggedToFloat64": {"before": 1, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-primitive.js",
                "name": "cal",
                "flags": METADATA_FLAGS,
                "expected": {
                    "TruncateTaggedToBit": {"before": 1, "after": 0},
                    "ChangeTaggedToBit": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-primitive.js",
                "name": "toStr",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 1, "after": 0}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-primitive.js",
                "name": "addBigInt",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckBigInt": {"before": 2, "after": 0}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-primitive.js",
                "name": "addLargeBigInt",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckBigInt": {"before": 2, "after": 0}
                }
            }
        ]
    },
    
    # ============================================
    # 2. 对象类型优化
    # ============================================
    {
        "name": "对象类型优化",
        "description": "Obj 和嵌套 Obj 的类型注入优化",
        "functions": [
            {
                "file": "test/mjsunit/compiler/type-injector/test-obj.js",
                "name": "concat",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 1, "after": 0},
                    "CheckString": {"before": 2, "after": 0}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-obj.js",
                "name": "concat_nested",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 1, "after": 0},
                    "CheckString": {"before": 2, "after": 0}
                }
            }
        ]
    },
    
    # ============================================
    # 3. 数组类型优化
    # ============================================
    {
        "name": "数组类型优化",
        "description": "Array 类型的元素类型注入优化",
        "functions": [
            {
                "file": "test/mjsunit/compiler/type-injector/test-array.js",
                "name": "concat_arr",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 3, "after": 0},
                    "CheckString": {"before": 2, "after": 0}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-array.js",
                "name": "concat_holey",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 3, "after": 0},
                    "CheckString": {"before": 2, "after": 2}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-array.js",
                "name": "concat_loop",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 3, "after": 0},
                    "CheckString": {"before": 2, "after": 0}
                }
            }
        ]
    },
    
    # ============================================
    # 4. 元组类型优化
    # ============================================
    {
        "name": "元组类型优化",
        "description": "Tuple的类型注入、边界检查消除、长度常量化",
        "functions": [
            {
                "file": "test/mjsunit/compiler/type-injector/test-tuple.js",
                "name": "concat",
                "flags": METADATA_FLAGS + BUILTIN_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 3, "after": 0},
                    "CheckString": {"before": 2, "after": 0},
                    "CheckedUint32Bounds": {"before": 3, "after": 1},
                    "LoadField.*length": {"before": 1, "after": 0}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-tuple.js",
                "name": "getLength",
                "flags": METADATA_FLAGS + BUILTIN_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 1, "after": 0},
                    "LoadField.*length": {"before": 1, "after": 0}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-tuple.js",
                "name": "process",
                "flags": METADATA_FLAGS + BUILTIN_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 3, "after": 0},
                    "CheckString": {"before": 2, "after": 0},
                    "CheckedUint32Bounds": {"before": 3, "after": 1},
                    "LoadField.*length": {"before": 1, "after": 0},
                }
            }
        ]
    },
    
    # ============================================
    # 5. 函数调用优化
    # ============================================
    {
        "name": "函数调用优化",
        "description": "函数返回值类型注入优化",
        "functions": [
            {
                "file": "test/mjsunit/compiler/type-injector/test-function-call.js",
                "name": "process",
                "flags": METADATA_FLAGS + NOINLINE_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 1, "after": 0},
                    "CheckString": {"before": 1, "after": 0}
                }
            }
        ]
    },
    
    # ============================================
    # 6. 内建函数类型表优化
    # ============================================
    {
        "name": "内建函数类型表优化",
        "description": "基于 Builtin ID 的内建函数返回值类型注入",
        "functions": [
            {
                "file": "test/mjsunit/compiler/type-injector/test-builtin.js",
                "name": "processNumber",
                "flags": BUILTIN_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 1, "after": 0},
                    "CheckString": {"before": 1, "after": 0}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-builtin.js",
                "name": "processAny",
                "flags": BUILTIN_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 2, "after": 1},
                    "CheckString": {"before": 1, "after": 0}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-builtin.js",
                "name": "testToFixed",
                "flags": BUILTIN_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 1, "after": 0},
                    "CheckString": {"before": 1, "after": 0}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-builtin.js",
                "name": "testRepeat",
                "flags": BUILTIN_FLAGS,
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 2, "after": 0},
                    "CheckString": {"before": 2, "after": 0}
                }
            }
        ]
    },
    
    # ============================================
    # 6. RawInt32 类型优化
    # ============================================
    {
        "name": "RawInt32 类型优化",
        "description": "RawInt32 类型的无溢出检查加法、减法、乘法和取模运算（移除溢出检查）",
        "functions": [
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "addRawInt32",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Add": {"before": 1, "after": 0},
                    "Int32Add": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "addRawInt32Overflow",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Add": {"before": 1, "after": 0},
                    "Int32Add": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "addRawInt32Negative",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Add": {"before": 1, "after": 0},
                    "Int32Add": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "addRawInt32Underflow",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Add": {"before": 1, "after": 0},
                    "Int32Add": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "subRawInt32",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Sub": {"before": 1, "after": 0},
                    "Int32Sub": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "subRawInt32Overflow",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Sub": {"before": 1, "after": 0},
                    "Int32Sub": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "subRawInt32Negative",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Sub": {"before": 1, "after": 0},
                    "Int32Sub": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "subRawInt32Underflow",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Sub": {"before": 1, "after": 0},
                    "Int32Sub": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "mulRawInt32",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Mul": {"before": 1, "after": 0},
                    "Int32Mul": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "mulRawInt32Overflow",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Mul": {"before": 1, "after": 0},
                    "Int32Mul": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "mulRawInt32Negative",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Mul": {"before": 1, "after": 0},
                    "Int32Mul": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "mulRawInt32Underflow",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Mul": {"before": 1, "after": 0},
                    "Int32Mul": {"before": 0, "after": 1}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "divRawInt32Exact",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Div": {"before": 1, "after": 0},
                    "Int32Div": {"before": 0, "after": 2}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "divRawInt32Trunc",
                "flags": METADATA_FLAGS,
                "expected": {
                    "Float64Div": {"before": 1, "after": 0},
                    "Int32Div": {"before": 0, "after": 2}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "divRawInt32Overflow",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Div": {"before": 1, "after": 0},
                    "Int32Div": {"before": 0, "after": 2}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "divRawInt32ByZero",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Div": {"before": 1, "after": 0},
                    "Int32Div": {"before": 0, "after": 2}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "modRawInt32",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Mod": {"before": 1, "after": 0},
                    "Int32Mod": {"before": 0, "after": 2}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "modRawInt32Negative",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Mod": {"before": 1, "after": 0},
                    "Int32Mod": {"before": 0, "after": 2}
                }
            },
            {
                "file": "test/mjsunit/compiler/type-injector/test-rawint32.js",
                "name": "modRawInt32ByZero",
                "flags": METADATA_FLAGS,
                "expected": {
                    "CheckedInt32Mod": {"before": 1, "after": 0},
                    "Int32Mod": {"before": 0, "after": 2}
                }
            }
        ]
    }
]
