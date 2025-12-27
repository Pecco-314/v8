# Tuple vs Class 在不同数字类型下的 Turbofan 图分析

## 测试目的

观察 `[number, number]` 形式的 Tuple 和 JS Class 在处理 Smi（小整数）和 HeapNumber（堆数字/浮点数）时，Turbofan 生成的 EarlyOptimization 阶段图有何不同，重点观察 Map 的差异。

## 测试代码

### Tuple with Smi

```javascript
function processTupleSmi(data) {
    return data[0] + data[1];
}
// 预热时使用小整数: [0, 1], [1, 2], ...
```

### Tuple with HeapNumber

```javascript
function processTupleHeapNumber(data) {
    return data[0] + data[1];
}
// 预热时使用浮点数: [0.5, 1.5], [1.5, 2.5], ...
```

### Class with Smi

```javascript
class PointSmi {
    constructor(x, y) { this.x = x; this.y = y; }
}
function processClassSmi(point) {
    return point.x + point.y;
}
// 预热时使用小整数
```

### Class with HeapNumber

```javascript
class PointHeapNumber {
    constructor(x, y) { this.x = x; this.y = y; }
}
function processClassHeapNumber(point) {
    return point.x + point.y;
}
// 预热时使用浮点数
```

***

## EarlyOptimization 阶段图分析

### 1. Tuple with Smi

    #33:CheckMaps[None, 0x63987ecab3d0, FeedbackSource(INVALID)]
    #37:LoadElement[tagged base, 8, Signed31, kRepTaggedSigned|kTypeInt32, FullWriteBarrier]
    #42:LoadElement[tagged base, 8, Signed31, kRepTaggedSigned|kTypeInt32, FullWriteBarrier]
    #27:Int32Add

**关键观察：**

*   Map 地址: `0x63987ecab3d0`
*   元素类型: `Signed31`（即 Smi）
*   表示: `kRepTaggedSigned|kTypeInt32`
*   加法运算: `Int32Add`

### 2. Tuple with HeapNumber

    #33:CheckMaps[None, 0x6143dafae3d0, FeedbackSource(INVALID)]
    #37:LoadElement[tagged base, 8, Number, kRepFloat64|kTypeNumber, FullWriteBarrier]
    #42:LoadElement[tagged base, 8, Number, kRepFloat64|kTypeNumber, FullWriteBarrier]
    #47:Float64Add

**关键观察：**

*   Map 地址: `0x6143dafae3d0`（与 Smi 版本**不同**）
*   元素类型: `Number`
*   表示: `kRepFloat64|kTypeNumber`
*   加法运算: `Float64Add`

### 3. Class with Smi

    #32:CheckMaps[None, 0x61acf417c528, FeedbackSource(INVALID)]
    #33:LoadField[BuildLoadDataField, tagged base, 12, #x, Signed31, kRepTaggedSigned|kTypeInt32, ..., field owner: 0x3245000634ed <Map[20]>]
    #35:LoadField[BuildLoadDataField, tagged base, 16, #y, Signed31, kRepTaggedSigned|kTypeInt32, ..., field owner: 0x32450006351d <Map[20]>]
    #25:Int32Add

**关键观察：**

*   Map 地址: `0x61acf417c528`
*   字段 x: offset 12, `Signed31`, `kRepTaggedSigned|kTypeInt32`
*   字段 y: offset 16, `Signed31`, `kRepTaggedSigned|kTypeInt32`
*   **注意**: x 和 y 的 field owner Map 是不同的！
    *   x field owner: `0x3245000634ed`
    *   y field owner: `0x32450006351d`
*   这表示 Class 在添加属性时发生了 **Map transition**
*   加法运算: `Int32Add`

### 4. Class with HeapNumber

    #32:CheckMaps[None, 0x5a4089c5b568, FeedbackSource(INVALID)]
    #33:LoadField[BuildLoadDataField, tagged base, 12, #x, OtherInternal, kRepTaggedPointer|kTypeAny, ..., field owner: 0x0b5c0006352d <Map[20]>]
    #34:LoadField[HeapNumberValue, tagged base, 4, Number, kRepFloat64|kTypeNumber, ...]
    #36:LoadField[BuildLoadDataField, tagged base, 16, #y, OtherInternal, kRepTaggedPointer|kTypeAny, ..., field owner: 0x0b5c0006355d <Map[20]>]
    #37:LoadField[HeapNumberValue, tagged base, 4, Number, kRepFloat64|kTypeNumber, ...]
    #42:Float64Add

**关键观察：**

*   Map 地址: `0x5a4089c5b568`（与 Smi 版本**不同**）
*   字段 x: offset 12, `OtherInternal`（指向 HeapNumber 的指针）
*   字段 y: offset 16, `OtherInternal`
*   需要额外的 `LoadField[HeapNumberValue]` 来读取实际数值
*   **注意**: x 和 y 的 field owner Map 也是不同的！
    *   x field owner: `0x0b5c0006352d`
    *   y field owner: `0x0b5c0006355d`
*   加法运算: `Float64Add`

***

## 核心结论

### Map 差异汇总

| 数据结构          | 数值类型       | Map 地址           |
| ------------- | ---------- | ---------------- |
| Tuple (Array) | Smi        | `0x63987ecab3d0` |
| Tuple (Array) | HeapNumber | `0x6143dafae3d0` |
| Class         | Smi        | `0x61acf417c528` |
| Class         | HeapNumber | `0x5a4089c5b568` |

**结论 1: Tuple 的不同数字类型有不同的 Map**

*   Smi 数组和 HeapNumber 数组的 Map 不同
*   这意味着如果函数同时处理这两种情况，会发生去优化

**结论 2: Class 的不同数字类型有不同的 Map**

*   存储 Smi 的 Class 和存储 HeapNumber 的 Class Map 不同
*   字段存储方式也完全不同：
    *   Smi: 直接 inline 存储在对象中
    *   HeapNumber: 存储指针，需要额外间接访问

**结论 3: Class 有 Map Transition 链**

*   每添加一个属性，Map 会转换
*   x 和 y 字段的 field owner Map 不同，说明存在 transition 链

### 数据访问路径对比

    Tuple Smi:
      CheckMaps → LoadElement(Signed31) → Int32Add

    Tuple HeapNumber:
      CheckMaps → LoadElement(Float64) → Float64Add

    Class Smi:
      CheckMaps → LoadField(#x, Signed31) → LoadField(#y, Signed31) → Int32Add

    Class HeapNumber:
      CheckMaps → LoadField(#x, Pointer) → LoadField(HeapNumberValue) 
               → LoadField(#y, Pointer) → LoadField(HeapNumberValue) → Float64Add

### 性能影响

1.  **Tuple 更简洁**: 直接通过索引访问，只需一次 LoadElement
2.  **Class HeapNumber 开销更大**: 需要两次 LoadField（一次取指针，一次取值）
3.  **Class Smi 较高效**: 值直接 inline 在对象中
4.  **类型混合会导致去优化**: 因为 Map 不同，混合使用会触发 deopt

***

## 附录：完整的 EarlyOptimization 图

### Tuple Smi - processTupleSmi

    #33:CheckMaps[None, 0x63987ecab3d0, FeedbackSource(INVALID)](#52:CheckedTaggedToTaggedPointer, ...)
    #34:LoadField[JSObjectElements, tagged base, 8, Internal, ...](#2:Parameter, #33:CheckMaps, ...)
    #35:LoadField[JSArrayLength, tagged base, 12, Range(0, 67108864), ...](#2:Parameter, ...)
    #36:CheckedUint32Bounds[...](#53:Int32Constant, #54:ChangeTaggedSignedToInt32, ...)
    #37:LoadElement[tagged base, 8, Signed31, kRepTaggedSigned|kTypeInt32, FullWriteBarrier](#34:LoadField, ...)
    #41:CheckedUint32Bounds[...](#57:Int32Constant, #54:ChangeTaggedSignedToInt32, ...)
    #42:LoadElement[tagged base, 8, Signed31, kRepTaggedSigned|kTypeInt32, FullWriteBarrier](#34:LoadField, ...)
    #27:Int32Add(#60:ChangeTaggedSignedToInt32, #61:ChangeTaggedSignedToInt32)

### Tuple HeapNumber - processTupleHeapNumber

    #33:CheckMaps[None, 0x6143dafae3d0, FeedbackSource(INVALID)](#53:CheckedTaggedToTaggedPointer, ...)
    #34:LoadField[JSObjectElements, tagged base, 8, Internal, ...](#2:Parameter, ...)
    #35:LoadField[JSArrayLength, tagged base, 12, Range(0, 67108864), ...](#2:Parameter, ...)
    #36:CheckedUint32Bounds[...](#54:Int32Constant, #55:ChangeTaggedSignedToInt32, ...)
    #37:LoadElement[tagged base, 8, Number, kRepFloat64|kTypeNumber, FullWriteBarrier](#34:LoadField, ...)
    #41:CheckedUint32Bounds[...](#58:Int32Constant, #55:ChangeTaggedSignedToInt32, ...)
    #42:LoadElement[tagged base, 8, Number, kRepFloat64|kTypeNumber, FullWriteBarrier](#34:LoadField, ...)
    #47:Float64Add(#37:LoadElement, #42:LoadElement)

### Class Smi - processClassSmi

    #32:CheckMaps[None, 0x61acf417c528, FeedbackSource(INVALID)](#44:CheckedTaggedToTaggedPointer, ...)
    #33:LoadField[BuildLoadDataField, tagged base, 12, #x, Signed31, kRepTaggedSigned|kTypeInt32, 
                  field owner: 0x3245000634ed <Map[20]>](#2:Parameter, ...)
    #35:LoadField[BuildLoadDataField, tagged base, 16, #y, Signed31, kRepTaggedSigned|kTypeInt32,
                  field owner: 0x32450006351d <Map[20]>](#2:Parameter, ...)
    #25:Int32Add(#45:ChangeTaggedSignedToInt32, #46:ChangeTaggedSignedToInt32)

### Class HeapNumber - processClassHeapNumber

    #32:CheckMaps[None, 0x5a4089c5b568, FeedbackSource(INVALID)](#47:CheckedTaggedToTaggedPointer, ...)
    #33:LoadField[BuildLoadDataField, tagged base, 12, #x, OtherInternal, kRepTaggedPointer|kTypeAny,
                  field owner: 0x0b5c0006352d <Map[20]>](#2:Parameter, ...)
    #34:LoadField[HeapNumberValue, tagged base, 4, Number, kRepFloat64|kTypeNumber, ...]
    #36:LoadField[BuildLoadDataField, tagged base, 16, #y, OtherInternal, kRepTaggedPointer|kTypeAny,
                  field owner: 0x0b5c0006355d <Map[20]>](#2:Parameter, ...)
    #37:LoadField[HeapNumberValue, tagged base, 4, Number, kRepFloat64|kTypeNumber, ...]
    #42:Float64Add(#34:LoadField, #37:LoadField)

