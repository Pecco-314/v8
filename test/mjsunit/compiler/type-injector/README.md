# Type Injector 单元测试

本目录包含 V8 TurboFan 编译器中 TypeInjector 优化 Pass 的单元测试。

## 概述

TypeInjector 利用静态类型元数据（来自 TypeScript 等）向 TurboFan 图中注入类型信息。当参数类型静态已知时，编译器可以跳过类型检查，从而生成更优化的代码。

## 测试文件

| 文件 | 测试内容 | 预期优化效果 |
|-----|---------|------------|
| `test-string.js` | String 类型注入 | 移除 `CheckString` 和 `CheckedTaggedToTaggedPointer` |
| `test-number.js` | Number 类型注入 | `CheckedTaggedToFloat64` → `ChangeTaggedToFloat64` |
| `test-boolean.js` | Boolean 类型注入 | `TruncateTaggedToBit` → `ChangeTaggedToBit` |
| `test-interface.js` | Interface/对象类型注入 | 移除字段访问时的 `CheckString` |
| `test-interface-nested.js` | 嵌套 Interface 类型注入 | 移除嵌套字段访问时的 `CheckString` |
| `test-array.js` | 数组元素类型注入 (`arr<str>`) | 移除数组元素访问时的 `CheckString` |

## 辅助脚本

### gen_metadata_info.sh - 生成 metadata 所需信息

当修改测试文件后，需要重新计算哈希和函数位置：

```bash
# 查看单个文件的信息
./gen_metadata_info.sh test-string.js

# 批量查看多个文件
./gen_metadata_info.sh test-*.js
```

输出示例：
```
==========================================
文件: test-string.js
==========================================
哈希: a4c3557f0bd894bc392b84c508d99c02fd2ef9b2f2f0abb09d4007666511a806

函数列表:
  - twice_s @ 280

metadata 文件名: a4c3557f0bd894bc392b84c508d99c02fd2ef9b2f2f0abb09d4007666511a806.metadata
metadata 格式: [起始位置] [参数数量] [类型1] [类型2] ...
```

然后手动创建 metadata 文件：
```bash
echo "280 2 any str" > metadata/a4c3557f...metadata
```

### verify_optimization.sh - 验证优化效果

对比有/无 metadata 时的编译图，验证优化是否生效：

```bash
./verify_optimization.sh
```

## Metadata 文件格式

元数据文件存放在 `metadata/` 子目录中。每个文件以源文件的 SHA256 哈希值命名，扩展名为 `.metadata`。

格式：`[起始位置] [参数数量] [类型1] [类型2] ...`

- `起始位置`：函数在源代码中的起始位置（由 `gen_metadata_info.sh` 生成）
- `参数数量`：参数个数，包含 `this`（索引 0）
- `类型N`：类型字符串

### 类型语法

- 基本类型：`any`, `void`, `bool`, `num`, `str`, `i32`, `u32`, `i64`, `u64`, `f32`, `f64`
- 数组：`arr<元素类型>`，如 `arr<str>`
- 接口：`interface{字段1:类型1,字段2:类型2}`，如 `interface{x:str,y:str}`
- 嵌套接口：`interface{first:interface{x:str,y:str},second:str}`

## 运行测试

从 V8 根目录执行：

```bash
# 验证所有测试的优化效果
./test/mjsunit/compiler/type-injector/verify_optimization.sh

# 运行单个测试
out.gn/x64.debug/d8 --allow-natives-syntax --turbofan --no-always-turbofan \
    --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata \
    test/mjsunit/mjsunit.js \
    test/mjsunit/compiler/type-injector/test-string.js

# 查看优化图（调试用）
out.gn/x64.debug/d8 --allow-natives-syntax --turbofan --no-always-turbofan \
    --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata \
    --trace-turbo-graph \
    test/mjsunit/mjsunit.js \
    test/mjsunit/compiler/type-injector/test-string.js
```

## V8 命令行参数

`--turbo_metadata_path` 指定元数据文件所在目录：

```bash
d8 --turbo_metadata_path=/path/to/metadata script.js
```

注意：flag 使用下划线 `_` 而非连字符 `-`。

如果不指定此参数，TypeInjector 不会加载任何元数据，优化不会生效。
