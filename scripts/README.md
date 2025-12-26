# TypeInjector 测试与分析工具集

本目录包含用于测试和分析 V8 TypeInjector 优化效果的工具脚本。

## 工作流程

### 1. 准备阶段

#### 1.1 编写测试文件
在 `test/mjsunit/compiler/type-injector/` 目录下创建测试文件，如 `test-primitive.js`：

```javascript
function twice_s(x) {
    return x + x;
}

%PrepareFunctionForOptimization(twice_s);
twice_s("hello");
%OptimizeFunctionOnNextCall(twice_s);
twice_s("world");
```

#### 1.2 生成 Metadata 信息
使用 `gen_metadata_info.py` 获取函数在文件中的字节位置：

```bash
python3 scripts/gen_metadata_info.py test/mjsunit/compiler/type-injector/test-primitive.js
```

输出示例：
```
文件: test/mjsunit/compiler/type-injector/test-primitive.js
哈希: 101c0d42ae5e0e4d8b5e38a4b5c5e9c5a5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e
函数位置:
  twice_s: 280
  twice_f: 369
  cal: 458
```

#### 1.3 创建 Metadata 文件
在 `test/mjsunit/compiler/type-injector/metadata/` 目录下创建以哈希命名的 metadata 文件：

```
280 @params any str @ret str
369 @params any num @ret num
458 @params any bool @ret bool
```

格式：`位置 @params 参数类型... @ret 返回类型`

类型支持：`any`, `void`, `bool`, `num`, `str`, `sym`, `interface{...}`, `tuple<...>`, `array<...>`

### 2. 配置测试

编辑 `scripts/test_config.py`，添加测试用例：

```python
TEST_CASES = [
    {
        "name": "基本类型优化",
        "description": "String、Number、Boolean、Symbol 基本类型的类型注入优化",
        "functions": [
            {
                "file": "test/mjsunit/compiler/type-injector/test-primitive.js",
                "name": "twice_s",
                "flags": METADATA_FLAGS,  # --turbo_metadata_path=...
                "expected": {
                    "CheckedTaggedToTaggedPointer": {"before": 1, "after": 0},
                    "CheckString": {"before": 1, "after": 0}
                }
            }
        ]
    }
]
```

### 3. 生成图和文档

#### 3.1 生成 TurboFan Graph
使用 `generate_graphs.py` 生成优化前后的图：

```bash
python3 scripts/generate_graphs.py
```

- 输出目录：`docs/graphs/early-optimization/`
- 生成文件：`{prefix}-{function}-without.txt` 和 `{prefix}-{function}-with.txt`
- 自动对堆地址进行规范化（ADDR1, ADDR2...），确保生成稳定

#### 3.2 生成优化分析文档
使用 `analyze_optimization.py` 生成 Markdown 文档：

```bash
python3 scripts/analyze_optimization.py
```

- 输出目录：`docs/sections/`
- 自动统计节点变化
- 生成完整的优化前后对比

### 4. 验证测试结果

使用 `verify_tests.py` 验证所有测试是否符合预期：

```bash
python3 scripts/verify_tests.py
```

输出示例：
```
✅ twice_s (primitive): 所有节点符合预期
   CheckedTaggedToTaggedPointer: 1→0 ✓
   CheckString: 1→0 ✓
```

### 5. 调试工具

#### 5.1 查看单个函数的图
```bash
./out.gn/x64.debug/d8 \
  --trace-turbo-graph \
  --allow-natives-syntax \
  test/mjsunit/mjsunit.js \
  test/mjsunit/compiler/type-injector/test-primitive.js \
  2>&1 | grep -A 100 "twice_s"
```

#### 5.2 查看 TypeInjector 调试信息
在 `src/compiler/type-injector.cc` 中设置 `V8_COMPILER_TYPE_INJECTOR_DEBUG` 为 `true`，重新编译后运行：

```bash
./out.gn/x64.debug/d8 \
  --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata \
  --allow-natives-syntax \
  test/mjsunit/mjsunit.js \
  test/mjsunit/compiler/type-injector/test-primitive.js
```

## 脚本详细说明

### generate_graphs.py
**功能**：从 d8 输出中提取 EarlyOptimization 阶段的 Graph

**核心功能**：
- `normalize_addresses()`：规范化堆地址，处理 V8 前导零不一致问题
- `run_d8()`：运行 d8 并捕获输出
- `extract_function_graph()`：提取指定函数的图
- `generate_graphs()`：批量生成所有测试的图

**使用**：
```bash
python3 scripts/generate_graphs.py
```

### analyze_optimization.py
**功能**：分析优化效果并生成 Markdown 文档

**核心功能**：
- `count_nodes()`：统计图中节点数量
- `load_metadata()`：读取 metadata 信息
- `generate_function_section()`：生成单个函数的文档
- `generate_section()`：生成完整章节

**使用**：
```bash
python3 scripts/analyze_optimization.py
```

### verify_tests.py
**功能**：验证测试结果是否符合预期

**核心功能**：
- `count_node()`：统计特定节点数量
- `verify_function()`：验证单个函数
- `verify_all()`：验证所有测试

**使用**：
```bash
python3 scripts/verify_tests.py
```

### gen_metadata_info.py
**功能**：生成测试文件的 metadata 信息

**核心功能**：
- `get_file_hash()`：计算文件 SHA256 哈希
- `get_function_positions()`：使用 d8 --print-ast 获取函数位置

**使用**：
```bash
python3 scripts/gen_metadata_info.py <test-file>
```

### test_config.py
**功能**：测试配置文件

**配置项**：
- `V8_CONFIG`：V8 路径和基础参数
- `OUTPUT_CONFIG`：输出目录配置
- `TEST_CASES`：所有测试用例定义
- `METADATA_FLAGS`, `BUILTIN_FLAGS`, `NOINLINE_FLAGS`：常用 flag 组合

## 添加新测试的完整流程

1. **编写测试文件**：`test/mjsunit/compiler/type-injector/test-bigint.js`
2. **获取位置信息**：`python3 scripts/gen_metadata_info.py test/.../test-bigint.js`
3. **创建 metadata 文件**：`test/.../metadata/{hash}`
4. **配置测试**：编辑 `scripts/test_config.py`
5. **生成图**：`python3 scripts/generate_graphs.py`
6. **生成文档**：`python3 scripts/analyze_optimization.py`
7. **验证结果**：`python3 scripts/verify_tests.py`

## 常见问题

### Q: 如何查看某个节点的定义？
A: 搜索 `src/compiler/opcodes.h` 和 `src/compiler/simplified-operator.h`

### Q: 为什么地址会变化？
A: V8 每次运行的堆地址不同，`normalize_addresses()` 会将其规范化为 ADDR1, ADDR2...

### Q: 如何调试 TypeInjector？
A: 在 `src/compiler/type-injector.cc` 中设置 `V8_COMPILER_TYPE_INJECTOR_DEBUG = true`

### Q: 测试失败怎么办？
A: 
1. 检查 metadata 位置是否正确（使用 `gen_metadata_info.py`）
2. 查看生成的图文件（`docs/graphs/early-optimization/`）
3. 对比优化前后的节点差异

## 相关文件

- 测试文件：`test/mjsunit/compiler/type-injector/`
- Metadata：`test/mjsunit/compiler/type-injector/metadata/`
- 输出图：`docs/graphs/early-optimization/`
- 文档：`docs/sections/`
- 源码：`src/compiler/type-injector.{h,cc}`
