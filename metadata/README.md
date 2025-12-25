# V8 Metadata 目录

本目录存放 TypeInjector 使用的类型元数据文件。

## builtin.metadata

V8 内建函数的类型签名。

### 格式

```
[BuiltinId] @params [receiver_type param_types...] @ret [return_type]
```

- `BuiltinId`: 内建函数的 ID（整数），对应 `v8::internal::Builtin` 枚举值
- `receiver_type`: 接收者类型（this），如 `num`, `str`, `bool`, `any`
- `param_types`: 其他参数类型（可选）
- `return_type`: 返回值类型

### 类型标注

- `any`: 任意类型
- `num`: Number（包括原始数字和 Number 包装对象）
- `str`: String（包括原始字符串和 String 包装对象）
- `bool`: Boolean（包括原始布尔值和 Boolean 包装对象）
- `arr`: Array
- `tuple[T1,T2,...]`: Tuple（固定长度和类型的数组）
- `interface{field1:T1,field2:T2}`: Interface

### 注释

使用 `#` 开头的行为注释，会被解析器忽略。

### 示例

```
1338 @params num @ret str  # NumberPrototypeToString
1377 @params any @ret str  # ObjectPrototypeToString
```

## 用户自定义 metadata

用户代码的类型 metadata 文件应使用 `--turbo_metadata_path` 指定，格式为：

```
[BytecodeOffset] @params [param_types...] @ret [return_type]
```

例如：`test/mjsunit/compiler/type-injector/metadata/` 目录。

