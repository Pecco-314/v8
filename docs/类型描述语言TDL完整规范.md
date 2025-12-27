# 类型描述语言 (TDL) 规范

TDL 是连接类型元数据与 V8 TypeInjector 的中间语言，提供统一的类型描述格式。

## 基本语法

### 用户函数签名

```
[BytecodeOffset] @params [param1 param2 ...] @ret [return_type]
```

**示例**：
```
313 @params any rawint32 rawint32 @ret rawint32
369 @params any str @ret str
882 @params any num @ret num
```

### 内建函数签名

```
[BuiltinId] @params [receiver param1 ...] @ret [return_type]
```

**示例**：
```
1338 @params num @ret str      # NumberPrototypeToString
1377 @params any @ret str      # ObjectPrototypeToString
```

## 类型系统

### 原始类型

| 类型 | 说明 | V8 映射 | 用途 |
|------|------|---------|------|
| `any` | 任意类型 | `Type::Any()` | 接收者参数、未知类型 |
| `void` | 空类型 | `Type::Undefined()` | 无返回值函数 |
| `bool` | 布尔值 | `Type::Boolean()` | 逻辑运算 |
| `num` | 数值类型 | `Type::Number()` | 通用数值 |
| `str` | 字符串 | `Type::String()` | 文本处理 |
| `symbol` | Symbol | `Type::Symbol()` | 符号类型 |
| `bigint` | BigInt | `Type::BigInt()` | 大整数 |
| `rawint32` | 32位整数 | `Type::Range(INT32_MIN, INT32_MAX)` | **特殊优化类型** |

### 复合类型

#### 数组类型

```
arr<ElementType>
array<ElementType>
```

**示例**：
```
@params any arr<num> @ret num
@params any array<str> @ret void
```

#### 元组类型

```
tuple<Type1, Type2, ...>
```

**示例**：
```
@params any tuple<num,num> @ret num        # [number, number]
@params any tuple<str,num,bool> @ret void  # [string, number, boolean]
```

**特性**：
- 固定长度
- 每个位置类型确定
- 支持常量索引边界检查消除

#### 对象类型

```
obj{field1:Type1, field2:Type2, ...}
interface{field1:Type1, field2:Type2, ...}
```

**示例**：
```
@params any obj{x:num,y:num} @ret num
@params any interface{id:num,name:str} @ret str
```

**说明**：
- `obj` 和 `interface` 语义相同，不区分
- 字段顺序对应对象属性定义顺序

### 嵌套类型

支持任意嵌套：

```
arr<tuple<num,str>>                        # Array<[number, string]>
tuple<obj{x:num,y:num}, str>               # [{x: number, y: number}, string]
obj{data:arr<num>, meta:obj{id:num}}       # {data: number[], meta: {id: number}}
```

## 解析器实现

### 核心数据结构

```cpp
struct TypeAST {
    enum TypeKind : int {
        // 原始类型 (0-99)
        Any = 0, Void, Bool, Num, Str, Symbol, BigInt, RawInt32,
        // 复合类型 (100+)
        Arr = 100, Tuple, Obj
    } kind;
    
    std::string field_name;        // 对象字段名
    std::vector<TypeAST> children; // 嵌套类型
};
```

### 解析流程

```cpp
// src/compiler/type-storage.h
class TypeParser {
public:
    static std::optional<TypeAST> Parse(const std::string& type_str);
    
private:
    static TypeAST ParsePrimitive(const std::string& token);
    static TypeAST ParseComposite(const std::string& type_str);
    static std::vector<TypeAST> ParseList(const std::string& list_str);
};
```

**关键函数**：
- `Parse()`: 入口，识别原始/复合类型
- `ParsePrimitive()`: 解析基本类型关键字
- `ParseComposite()`: 处理 `<>` 和 `{}`包裹的复合类型
- `ParseList()`: 按 `,` 分割列表

## TDL 到 V8 类型映射

### 原始类型映射

```cpp
Type TypeInjector::ConvertToV8Type(const TypeAST& type_ast) {
    switch (type_ast.kind) {
        case TypeAST::Any:      return Type::Any();
        case TypeAST::Void:     return Type::Undefined();
        case TypeAST::Bool:     return Type::Boolean();
        case TypeAST::Num:      return Type::Number();
        case TypeAST::Str:      return Type::String();
        case TypeAST::Symbol:   return Type::Symbol();
        case TypeAST::BigInt:   return Type::BigInt();
        case TypeAST::RawInt32: return Type::Range(INT32_MIN, INT32_MAX, zone());
    }
}
```

### 复合类型映射

**数组**：映射到 V8 内部的 `JSArray` 类型表示
**元组**：同样映射到 `JSArray`，但利用索引信息进行优化
**对象**：映射到 `JSObject`，字段信息用于 `LoadField` 优化

## RawInt32 特殊语义

### 与 num 的区别

| 特性 | num | rawint32 |
|------|-----|----------|
| 值域 | 所有数值（整数+浮点） | [-2³¹, 2³¹-1] |
| 表示 | TaggedSigned 或 HeapNumber | 保证 TaggedSigned |
| 溢出 | 自动转换为浮点 | **守护分支处理** |
| 优化 | 标准类型检查消除 | **直接使用硬件指令** |

### 使用限制

**安全场景**：
- 已知输入范围在 [-2³¹, 2³¹-1]
- 来自显式的整数常量
- 数组长度、索引等天然整数场景

**不安全场景**：
- 用户输入未验证
- 浮点运算结果
- 可能溢出的累加操作

## 文件格式规范

### 用户函数类型文件

**路径**: `metadata/<file_hash>.tdl`
**格式**: 每行一个函数签名

```
313 @params any rawint32 rawint32 @ret rawint32
369 @params any str @ret str
882 @params any num @ret num
1449 @params any tuple<num,num> @ret num
```

### 内建函数类型文件

**路径**: `metadata/builtin.tdl`
**格式**: 内建函数 ID + 签名

```
1338 @params num @ret str
1377 @params any @ret str
```

## 类型注解示例

### 简单函数

```javascript
// JavaScript
function add(a, b) { return a + b; }

// TDL 注解
// 123 @params any num num @ret num
```

### 复杂数据结构

```javascript
// JavaScript
function processPoint(point) {
    return point[0] + point[1];
}

// TDL 注解  
// 456 @params any tuple<num,num> @ret num
```

### 对象方法

```javascript
// JavaScript
class Point {
    constructor(x, y) { this.x = x; this.y = y; }
}
function getX(point) { return point.x; }

// TDL 注解
// 789 @params any obj{x:num,y:num} @ret num
```

## 局限性

### 当前不支持

1. **联合类型**: `num | str` 暂不支持
2. **可选类型**: `num?` 暂不支持  
3. **泛型**: `arr<T>` 中的 `T` 必须具体化
4. **函数类型**: `(num) => str` 暂不支持
5. **字面量类型**: `"foo" | "bar"` 暂不支持

### 未来扩展方向

- 联合类型支持：需要增强类型推导
- 泛型实例化：基于调用点类型具体化
- 更精细的数值范围：`int8`, `uint32` 等
