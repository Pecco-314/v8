# TypeInjector 编译优化文档

本文档记录了 V8 TurboFan 编译器中基于 metadata 的 TypeInjector 优化效果。

所有测试均基于 **EarlyOptimization Phase** 的最终 Graph（经过 DCE 和节点折叠），更能反映实际的优化效果。

## 测试环境

- **编译命令**: `ninja -C out.gn/x64.debug d8`
- **运行命令**: `out.gn/x64.debug/d8 --allow-natives-syntax --turbofan --no-always-turbofan --turbo_metadata_path=<metadata_dir> --trace-turbo-graph <test.js>`
- **测试目录**: `test/mjsunit/compiler/type-injector/`
- **验证脚本**: `test/mjsunit/compiler/type-injector/verify_optimization.sh`

## Metadata 格式

统一格式：
```
[BytecodeOffset] @params [param1 param2 ...] @ret [return_type]
```

示例：
```
280 @params any str @ret str
308 @params any @ret str
329 @params any tuple[str,num] @ret str
348 @params any @ret str
408 @params any str @ret str
```

**类型语法**:
- 基本类型: `str`, `num`, `bool`, `any`
- 数组类型: `arr<element_type>` (例如: `arr<str>`)
- Tuple 类型: `tuple[type1,type2,...]` (例如: `tuple[str,num]`)
- Interface 类型: `interface{field1:type1,field2:type2}` (例如: `interface{x:num,y:num}`)

---

### 1. String 类型

#### 测试代码

```javascript
function twice_s(arg) {
    return arg + arg;
}
```

**Metadata**:
```
280 @params any str @ret str
```

#### 优化前（无 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#45:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, dense]()
#39:TypedStateValues[kRepTagged|kTypeAny, dense](#2:Parameter)
#4:Parameter[5, debug name: %context](#0:Start)
#23:HeapConstant[0x3370000670cd <JSFunction twice_s (sfi = 0x337000067031)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, 0x337000067031 <SharedFunctionInfo twice_s>](#9:TypedStateValues, #10:TypedStateValues, #39:TypedStateValues, #4:Parameter, #23:HeapConstant, #0:Start)
#46:ExternalConstant[0x5cbfb6fa0020]()
#38:Int64Constant[0]()
#47:Load[kRepWord64](#46:ExternalConstant, #38:Int64Constant, #0:Start, #0:Start)
#48:StackPointerGreaterThan[JSFunctionEntry](#47:Load, #47:Load)
#57:HeapConstant[0x3370001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#54:LoadStackCheckOffset()
#55:ExternalConstant[0x7b6fb29cd520 <StackGuardWithGap.entry>]()
#56:Int32Constant[1]()
#6:HeapConstant[0x33700004b2f1 <NativeContext[304]>]()
#40:TypedStateValues[, sparse:.]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x337000067031 <SharedFunctionInfo twice_s>](#9:TypedStateValues, #10:TypedStateValues, #40:TypedStateValues, #4:Parameter, #23:HeapConstant, #0:Start)
#49:Branch[Unspecified, True](#48:StackPointerGreaterThan, #0:Start)
#51:IfFalse(#49:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#57:HeapConstant, #54:LoadStackCheckOffset, #55:ExternalConstant, #56:Int32Constant, #6:HeapConstant, #13:FrameState, #48:StackPointerGreaterThan, #51:IfFalse)
#50:IfTrue(#49:Branch)
#52:Merge(#50:IfTrue, #8:Call)
#53:EffectPhi(#48:StackPointerGreaterThan, #8:Call, #52:Merge)
#14:Checkpoint(#15:FrameState, #53:EffectPhi, #52:Merge)
#41:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#2:Parameter, #14:Checkpoint, #52:Merge)
#28:CheckString[FeedbackSource(INVALID)](#41:CheckedTaggedToTaggedPointer, #41:CheckedTaggedToTaggedPointer, #52:Merge)
#30:StringLength(#28:CheckString)
#32:Int32Add(#30:StringLength, #30:StringLength)
#43:Int32Constant[536870889]()
#34:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#32:Int32Add, #43:Int32Constant, #28:CheckString, #52:Merge)
#44:ChangeInt31ToTaggedSigned(#34:CheckedUint32Bounds)
#35:StringConcat(#44:ChangeInt31ToTaggedSigned, #28:CheckString, #28:CheckString)
#19:Return(#45:Int32Constant, #35:StringConcat, #34:CheckedUint32Bounds, #52:Merge)
#20:End(#19:Return)
```

**关键节点**:
- `#41:CheckedTaggedToTaggedPointer` - 检查参数是否为指针类型
- `#28:CheckString` - 运行时字符串类型检查
- `#30:StringLength` - 获取字符串长度
- `#35:StringConcat` - 字符串连接操作

#### 优化后（有 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#42:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#28:StringLength(#2:Parameter)
#30:Int32Add(#28:StringLength, #28:StringLength)
#40:Int32Constant[536870889]()
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, dense]()
#38:TypedStateValues[kRepTagged|kTypeAny, dense](#2:Parameter)
#4:Parameter[5, debug name: %context](#0:Start)
#23:HeapConstant[0x2e94000670cd <JSFunction twice_s (sfi = 0x2e9400067031)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, 0x2e9400067031 <SharedFunctionInfo twice_s>](#9:TypedStateValues, #10:TypedStateValues, #38:TypedStateValues, #4:Parameter, #23:HeapConstant, #0:Start)
#43:ExternalConstant[0x56fab8627020]()
#36:Int64Constant[0]()
#44:Load[kRepWord64](#43:ExternalConstant, #36:Int64Constant, #0:Start, #0:Start)
#45:StackPointerGreaterThan[JSFunctionEntry](#44:Load, #44:Load)
#54:HeapConstant[0x2e94001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#51:LoadStackCheckOffset()
#52:ExternalConstant[0x735f6efcd520 <StackGuardWithGap.entry>]()
#53:Int32Constant[1]()
#6:HeapConstant[0x2e940004b2f1 <NativeContext[304]>]()
#39:TypedStateValues[, sparse:.]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x2e9400067031 <SharedFunctionInfo twice_s>](#9:TypedStateValues, #10:TypedStateValues, #39:TypedStateValues, #4:Parameter, #23:HeapConstant, #0:Start)
#46:Branch[Unspecified, True](#45:StackPointerGreaterThan, #0:Start)
#48:IfFalse(#46:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#54:HeapConstant, #51:LoadStackCheckOffset, #52:ExternalConstant, #53:Int32Constant, #6:HeapConstant, #13:FrameState, #45:StackPointerGreaterThan, #48:IfFalse)
#47:IfTrue(#46:Branch)
#49:Merge(#47:IfTrue, #8:Call)
#50:EffectPhi(#45:StackPointerGreaterThan, #8:Call, #49:Merge)
#14:Checkpoint(#15:FrameState, #50:EffectPhi, #49:Merge)
#32:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#30:Int32Add, #40:Int32Constant, #14:Checkpoint, #49:Merge)
#41:ChangeInt31ToTaggedSigned(#32:CheckedUint32Bounds)
#33:StringConcat(#41:ChangeInt31ToTaggedSigned, #2:Parameter, #2:Parameter)
#19:Return(#42:Int32Constant, #33:StringConcat, #32:CheckedUint32Bounds, #49:Merge)
#20:End(#19:Return)
```

**关键节点**:
- `#2:Parameter[1]` - 参数类型已标注为 `String`（不再是通用 `NonInternal`）
- `#28:StringLength` - 直接访问字符串长度（无需先检查）
- `#33:StringConcat` - 字符串连接操作

**优化效果**:
- ✅ **移除了 `#41:CheckedTaggedToTaggedPointer`** - 不再需要检查参数是否为指针
- ✅ **移除了 `#28:CheckString`** - 不再需要运行时字符串类型检查
- ✅ **直接使用 `StringLength`** - 参数类型已知，可以直接访问字符串属性
- ✅ **Graph 简化** - 从 39 行减少到 37 行

### 2. Number 类型

#### 测试代码

```javascript
function twice_f(arg) {
    return arg + arg;
}
```

**Metadata**:
```
280 @params any num @ret num
```

#### 优化前（无 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#34:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, dense]()
#30:TypedStateValues[kRepTagged|kTypeAny, dense](#2:Parameter)
#4:Parameter[5, debug name: %context](#0:Start)
#22:HeapConstant[0x02ba000670d5 <JSFunction twice_f (sfi = 0x2ba00067021)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, 0x02ba00067021 <SharedFunctionInfo twice_f>](#9:TypedStateValues, #10:TypedStateValues, #30:TypedStateValues, #4:Parameter, #22:HeapConstant, #0:Start)
#36:ExternalConstant[0x645a857bd020]()
#29:Int64Constant[0]()
#37:Load[kRepWord64](#36:ExternalConstant, #29:Int64Constant, #0:Start, #0:Start)
#38:StackPointerGreaterThan[JSFunctionEntry](#37:Load, #37:Load)
#47:HeapConstant[0x02ba001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#44:LoadStackCheckOffset()
#45:ExternalConstant[0x7637c7bcd520 <StackGuardWithGap.entry>]()
#46:Int32Constant[1]()
#6:HeapConstant[0x02ba0004b2f1 <NativeContext[304]>]()
#31:TypedStateValues[, sparse:.]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x02ba00067021 <SharedFunctionInfo twice_f>](#9:TypedStateValues, #10:TypedStateValues, #31:TypedStateValues, #4:Parameter, #22:HeapConstant, #0:Start)
#39:Branch[Unspecified, True](#38:StackPointerGreaterThan, #0:Start)
#41:IfFalse(#39:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#47:HeapConstant, #44:LoadStackCheckOffset, #45:ExternalConstant, #46:Int32Constant, #6:HeapConstant, #13:FrameState, #38:StackPointerGreaterThan, #41:IfFalse)
#40:IfTrue(#39:Branch)
#42:Merge(#40:IfTrue, #8:Call)
#43:EffectPhi(#38:StackPointerGreaterThan, #8:Call, #42:Merge)
#14:Checkpoint(#15:FrameState, #43:EffectPhi, #42:Merge)
#32:CheckedTaggedToFloat64[NumberOrOddball, FeedbackSource(INVALID)](#2:Parameter, #14:Checkpoint, #42:Merge)
#16:Float64Add(#32:CheckedTaggedToFloat64, #32:CheckedTaggedToFloat64)
#35:ChangeFloat64ToTagged[check-for-minus-zero](#16:Float64Add)
#18:Return(#34:Int32Constant, #35:ChangeFloat64ToTagged, #32:CheckedTaggedToFloat64, #42:Merge)
#19:End(#18:Return)
```

**关键节点**:
- `#32:CheckedTaggedToFloat64` - 运行时检查并将Tagged值转换为Float64
- `#16:Float64Add` - 浮点数加法操作
- `#35:ChangeFloat64ToTagged` - 将Float64结果转换回Tagged值

#### 优化后（有 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#34:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#31:ChangeTaggedToFloat64(#2:Parameter)
#27:Float64Add(#31:ChangeTaggedToFloat64, #31:ChangeTaggedToFloat64)
#35:ChangeFloat64ToTagged[check-for-minus-zero](#27:Float64Add)
#36:ExternalConstant[0x56d0277ba020]()
#30:Int64Constant[0]()
#37:Load[kRepWord64](#36:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#38:StackPointerGreaterThan[JSFunctionEntry](#37:Load, #37:Load)
#47:HeapConstant[0x13ea001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#44:LoadStackCheckOffset()
#45:ExternalConstant[0x7c6bc7fcd520 <StackGuardWithGap.entry>]()
#46:Int32Constant[1]()
#6:HeapConstant[0x13ea0004b2f1 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, dense]()
#33:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#22:HeapConstant[0x13ea000670d5 <JSFunction twice_f (sfi = 0x13ea00067021)>]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x13ea00067021 <SharedFunctionInfo twice_f>](#9:TypedStateValues, #10:TypedStateValues, #33:TypedStateValues, #4:Parameter, #22:HeapConstant, #0:Start)
#39:Branch[Unspecified, True](#38:StackPointerGreaterThan, #0:Start)
#41:IfFalse(#39:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#47:HeapConstant, #44:LoadStackCheckOffset, #45:ExternalConstant, #46:Int32Constant, #6:HeapConstant, #13:FrameState, #38:StackPointerGreaterThan, #41:IfFalse)
#40:IfTrue(#39:Branch)
#42:Merge(#40:IfTrue, #8:Call)
#43:EffectPhi(#38:StackPointerGreaterThan, #8:Call, #42:Merge)
#18:Return(#34:Int32Constant, #35:ChangeFloat64ToTagged, #43:EffectPhi, #42:Merge)
#19:End(#18:Return)
```

**关键节点**:
- `#2:Parameter[1]` - 参数类型已标注为 `Number`
- `#31:ChangeTaggedToFloat64` - 无检查的类型转换（直接转换）
- `#27:Float64Add` - 浮点数加法操作

**优化效果**:
- ✅ **移除了 `#32:CheckedTaggedToFloat64`** - 不再需要运行时类型检查
- ✅ **使用 `#31:ChangeTaggedToFloat64`** - 直接转换，无需检查
- ✅ **Checkpoint 不再依赖类型检查** - 简化了效果链
- ✅ **Graph 简化** - 从 34 行减少到 31 行

### 3. Boolean 类型

#### 测试代码

```javascript
function cal(a, b) {
    if (a) {
        return a;
    } else {
        return b;
    }
}
```

**Metadata**:
```
267 @params any bool bool @ret any
```

#### 优化前（无 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#38:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#39:ExternalConstant[0x585ffe1d2020]()
#35:Int64Constant[0]()
#40:Load[kRepWord64](#39:ExternalConstant, #35:Int64Constant, #0:Start, #0:Start)
#41:StackPointerGreaterThan[JSFunctionEntry](#40:Load, #40:Load)
#50:HeapConstant[0x09bf001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#47:LoadStackCheckOffset()
#48:ExternalConstant[0x7c7346dcd520 <StackGuardWithGap.entry>]()
#49:Int32Constant[1]()
#7:HeapConstant[0x09bf0004b2f1 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#36:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#26:HeapConstant[0x09bf000670c9 <JSFunction cal (sfi = 0x9bf00067029)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x09bf00067029 <SharedFunctionInfo cal>](#10:TypedStateValues, #11:TypedStateValues, #36:TypedStateValues, #5:Parameter, #26:HeapConstant, #0:Start)
#42:Branch[Unspecified, True](#41:StackPointerGreaterThan, #0:Start)
#44:IfFalse(#42:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#50:HeapConstant, #47:LoadStackCheckOffset, #48:ExternalConstant, #49:Int32Constant, #7:HeapConstant, #14:FrameState, #41:StackPointerGreaterThan, #44:IfFalse)
#43:IfTrue(#42:Branch)
#45:Merge(#43:IfTrue, #9:Call)
#46:EffectPhi(#41:StackPointerGreaterThan, #9:Call, #45:Merge)
#37:TruncateTaggedToBit(#2:Parameter)
#16:Branch[Machine, None](#37:TruncateTaggedToBit, #45:Merge)
#17:IfTrue(#16:Branch)
#27:Return(#38:Int32Constant, #2:Parameter, #46:EffectPhi, #17:IfTrue)
#19:IfFalse(#16:Branch)
#28:Return(#38:Int32Constant, #3:Parameter, #46:EffectPhi, #19:IfFalse)
#23:End(#27:Return, #28:Return)
```

**关键节点**:
- `#37:TruncateTaggedToBit` - 运行时转换为布尔值（可能需要检查）
- `#16:Branch` - 条件分支

#### 优化后（有 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#38:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#39:ExternalConstant[0x5df7ee6c5020]()
#35:Int64Constant[0]()
#40:Load[kRepWord64](#39:ExternalConstant, #35:Int64Constant, #0:Start, #0:Start)
#41:StackPointerGreaterThan[JSFunctionEntry](#40:Load, #40:Load)
#50:HeapConstant[0x19ab001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#47:LoadStackCheckOffset()
#48:ExternalConstant[0x704711bcd520 <StackGuardWithGap.entry>]()
#49:Int32Constant[1]()
#7:HeapConstant[0x19ab0004b2f1 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#36:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#26:HeapConstant[0x19ab000670c9 <JSFunction cal (sfi = 0x19ab00067029)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x19ab00067029 <SharedFunctionInfo cal>](#10:TypedStateValues, #11:TypedStateValues, #36:TypedStateValues, #5:Parameter, #26:HeapConstant, #0:Start)
#42:Branch[Unspecified, True](#41:StackPointerGreaterThan, #0:Start)
#44:IfFalse(#42:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#50:HeapConstant, #47:LoadStackCheckOffset, #48:ExternalConstant, #49:Int32Constant, #7:HeapConstant, #14:FrameState, #41:StackPointerGreaterThan, #44:IfFalse)
#43:IfTrue(#42:Branch)
#45:Merge(#43:IfTrue, #9:Call)
#46:EffectPhi(#41:StackPointerGreaterThan, #9:Call, #45:Merge)
#37:ChangeTaggedToBit(#2:Parameter)
#16:Branch[Machine, None](#37:ChangeTaggedToBit, #45:Merge)
#17:IfTrue(#16:Branch)
#27:Return(#38:Int32Constant, #2:Parameter, #46:EffectPhi, #17:IfTrue)
#19:IfFalse(#16:Branch)
#28:Return(#38:Int32Constant, #3:Parameter, #46:EffectPhi, #19:IfFalse)
#23:End(#27:Return, #28:Return)
```

**关键节点**:
- `#2:Parameter[1]` - 参数类型已标注为 `Boolean`
- `#37:ChangeTaggedToBit` - 无检查的类型转换
- `#16:Branch` - 条件分支

**优化效果**:
- ✅ **替换了 `#37:TruncateTaggedToBit`** - 从可能需要检查的 `Truncate` 变为无检查的 `Change`
- ✅ **类型已知** - 参数类型已标注为 `Boolean`，编译器可以安全地直接转换
- ✅ **Graph 大小相同** - 虽然节点数相同（34 行），但运行时开销更小


### 4. Interface 类型

#### 测试代码

```javascript
function concat(data) {
    return data.x + data.y;
}
```

**Metadata**:
```
257 @params any interface{x:str,y:str} @ret str
```

#### 优化前（无 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#57:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:.]()
#52:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#32:HeapConstant[0x0fce00067135 <JSFunction concat (sfi = 0xfce00067051)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 0, Ignore, 0x0fce00067051 <SharedFunctionInfo concat>](#9:TypedStateValues, #10:TypedStateValues, #52:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#58:ExternalConstant[0x5e277826b020]()
#51:Int64Constant[0]()
#59:Load[kRepWord64](#58:ExternalConstant, #51:Int64Constant, #0:Start, #0:Start)
#60:StackPointerGreaterThan[JSFunctionEntry](#59:Load, #59:Load)
#69:HeapConstant[0x0fce001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#66:LoadStackCheckOffset()
#67:ExternalConstant[0x76a9783cd520 <StackGuardWithGap.entry>]()
#68:Int32Constant[1]()
#6:HeapConstant[0x0fce0004b2f1 <NativeContext[304]>]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x0fce00067051 <SharedFunctionInfo concat>](#9:TypedStateValues, #10:TypedStateValues, #52:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#61:Branch[Unspecified, True](#60:StackPointerGreaterThan, #0:Start)
#63:IfFalse(#61:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#69:HeapConstant, #66:LoadStackCheckOffset, #67:ExternalConstant, #68:Int32Constant, #6:HeapConstant, #13:FrameState, #60:StackPointerGreaterThan, #63:IfFalse)
#62:IfTrue(#61:Branch)
#64:Merge(#62:IfTrue, #8:Call)
#65:EffectPhi(#60:StackPointerGreaterThan, #8:Call, #64:Merge)
#14:Checkpoint(#15:FrameState, #65:EffectPhi, #64:Merge)
#53:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#2:Parameter, #14:Checkpoint, #64:Merge)
#33:CheckMaps[None, 0x5e2778322410, FeedbackSource(INVALID)](#53:CheckedTaggedToTaggedPointer, #53:CheckedTaggedToTaggedPointer, #64:Merge)
#34:LoadField[BuildLoadDataField, tagged base, 12, 0xfce00003595: [String] in ReadOnlySpace: #x, NonInternal, kRepTaggedPointer|kTypeAny, FullWriteBarrier, const (field owner: 0x0fce00067211 <Map[20](HOLEY_ELEMENTS)>)](#2:Parameter, #33:CheckMaps, #64:Merge)
#36:LoadField[BuildLoadDataField, tagged base, 16, 0xfce000035a5: [String] in ReadOnlySpace: #y, NonInternal, kRepTaggedPointer|kTypeAny, FullWriteBarrier, const (field owner: 0x0fce0006725d <Map[20](HOLEY_ELEMENTS)>)](#2:Parameter, #34:LoadField, #64:Merge)
#41:CheckString[FeedbackSource(INVALID)](#34:LoadField, #36:LoadField, #64:Merge)
#43:StringLength(#41:CheckString)
#42:CheckString[FeedbackSource(INVALID)](#36:LoadField, #41:CheckString, #64:Merge)
#44:StringLength(#42:CheckString)
#45:Int32Add(#43:StringLength, #44:StringLength)
#55:Int32Constant[536870889]()
#47:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#45:Int32Add, #55:Int32Constant, #42:CheckString, #64:Merge)
#56:ChangeInt31ToTaggedSigned(#47:CheckedUint32Bounds)
#48:StringConcat(#56:ChangeInt31ToTaggedSigned, #41:CheckString, #42:CheckString)
#28:Return(#57:Int32Constant, #48:StringConcat, #47:CheckedUint32Bounds, #64:Merge)
#29:End(#28:Return)
```

**关键节点**:
- `#53:CheckedTaggedToTaggedPointer` - 检查参数是否为指针
- `#33:CheckMaps` - 检查对象的Map（确保结构匹配）
- `#34:LoadField[x]`, `#36:LoadField[y]` - 加载字段
- `#41:CheckString`, `#42:CheckString` - 检查字段是否为字符串

#### 优化后（有 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#54:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:.]()
#50:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#32:HeapConstant[0x158400067135 <JSFunction concat (sfi = 0x158400067051)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 0, Ignore, 0x158400067051 <SharedFunctionInfo concat>](#9:TypedStateValues, #10:TypedStateValues, #50:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#55:ExternalConstant[0x6056ae4b0020]()
#49:Int64Constant[0]()
#56:Load[kRepWord64](#55:ExternalConstant, #49:Int64Constant, #0:Start, #0:Start)
#57:StackPointerGreaterThan[JSFunctionEntry](#56:Load, #56:Load)
#66:HeapConstant[0x1584001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#63:LoadStackCheckOffset()
#64:ExternalConstant[0x73e4343cd520 <StackGuardWithGap.entry>]()
#65:Int32Constant[1]()
#6:HeapConstant[0x15840004b2f1 <NativeContext[304]>]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x158400067051 <SharedFunctionInfo concat>](#9:TypedStateValues, #10:TypedStateValues, #50:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#58:Branch[Unspecified, True](#57:StackPointerGreaterThan, #0:Start)
#60:IfFalse(#58:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#66:HeapConstant, #63:LoadStackCheckOffset, #64:ExternalConstant, #65:Int32Constant, #6:HeapConstant, #13:FrameState, #57:StackPointerGreaterThan, #60:IfFalse)
#59:IfTrue(#58:Branch)
#61:Merge(#59:IfTrue, #8:Call)
#62:EffectPhi(#57:StackPointerGreaterThan, #8:Call, #61:Merge)
#14:Checkpoint(#15:FrameState, #62:EffectPhi, #61:Merge)
#33:CheckMaps[None, 0x6056ae567440, FeedbackSource(INVALID)](#2:Parameter, #14:Checkpoint, #61:Merge)
#34:LoadField[BuildLoadDataField, tagged base, 12, 0x158400003595: [String] in ReadOnlySpace: #x, NonInternal, kRepTaggedPointer|kTypeAny, FullWriteBarrier, const (field owner: 0x158400067211 <Map[20](HOLEY_ELEMENTS)>)](#2:Parameter, #33:CheckMaps, #61:Merge)
#41:StringLength(#34:LoadField)
#36:LoadField[BuildLoadDataField, tagged base, 16, 0x1584000035a5: [String] in ReadOnlySpace: #y, NonInternal, kRepTaggedPointer|kTypeAny, FullWriteBarrier, const (field owner: 0x15840006725d <Map[20](HOLEY_ELEMENTS)>)](#2:Parameter, #34:LoadField, #61:Merge)
#42:StringLength(#36:LoadField)
#43:Int32Add(#41:StringLength, #42:StringLength)
#52:Int32Constant[536870889]()
#45:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#43:Int32Add, #52:Int32Constant, #36:LoadField, #61:Merge)
#53:ChangeInt31ToTaggedSigned(#45:CheckedUint32Bounds)
#46:StringConcat(#53:ChangeInt31ToTaggedSigned, #34:LoadField, #36:LoadField)
#28:Return(#54:Int32Constant, #46:StringConcat, #45:CheckedUint32Bounds, #61:Merge)
#29:End(#28:Return)
```

**关键节点**:
- `#2:Parameter[1]` - 参数类型已标注为 Interface `{x: str, y: str}`
- `#33:CheckMaps` - 仍然保留（确保对象结构正确）
- `#34:LoadField[x]`, `#36:LoadField[y]` - 直接加载字段
- `#41:StringLength`, `#42:StringLength` - 直接访问字符串长度

**优化效果**:
- ✅ **移除了 `#53:CheckedTaggedToTaggedPointer`** - 参数类型已知为对象
- ✅ **移除了 `#41:CheckString`, `#42:CheckString`** - 字段类型已知为字符串
- ✅ **保留了 `#33:CheckMaps`** - 仍需验证对象的具体结构
- ✅ **Graph 简化** - 从 43 行减少到 40 行


### 5. Interface 类型（嵌套）

#### 测试代码

```javascript
function concat_nested(data) {
    return data.first.x + data.first.y;
}
```

**Metadata**:
```
303 @params any interface{first:interface{x:str,y:str},second:str} @ret str
```

#### 优化前（无 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#71:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, sparse:^^](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:..]()
#66:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#42:HeapConstant[0x3c2f000671cd <JSFunction concat_nested (sfi = 0x3c2f000670a5)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 0, Ignore, 0x3c2f000670a5 <SharedFunctionInfo concat_nested>](#9:TypedStateValues, #10:TypedStateValues, #66:TypedStateValues, #4:Parameter, #42:HeapConstant, #0:Start)
#72:ExternalConstant[0x618ca2b10020]()
#65:Int64Constant[0]()
#73:Load[kRepWord64](#72:ExternalConstant, #65:Int64Constant, #0:Start, #0:Start)
#74:StackPointerGreaterThan[JSFunctionEntry](#73:Load, #73:Load)
#83:HeapConstant[0x3c2f001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#80:LoadStackCheckOffset()
#81:ExternalConstant[0x7cd6535cd520 <StackGuardWithGap.entry>]()
#82:Int32Constant[1]()
#6:HeapConstant[0x3c2f0004b2f1 <NativeContext[304]>]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x3c2f000670a5 <SharedFunctionInfo concat_nested>](#9:TypedStateValues, #10:TypedStateValues, #66:TypedStateValues, #4:Parameter, #42:HeapConstant, #0:Start)
#75:Branch[Unspecified, True](#74:StackPointerGreaterThan, #0:Start)
#77:IfFalse(#75:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#83:HeapConstant, #80:LoadStackCheckOffset, #81:ExternalConstant, #82:Int32Constant, #6:HeapConstant, #13:FrameState, #74:StackPointerGreaterThan, #77:IfFalse)
#76:IfTrue(#75:Branch)
#78:Merge(#76:IfTrue, #8:Call)
#79:EffectPhi(#74:StackPointerGreaterThan, #8:Call, #78:Merge)
#14:Checkpoint(#15:FrameState, #79:EffectPhi, #78:Merge)
#67:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#2:Parameter, #14:Checkpoint, #78:Merge)
#43:CheckMaps[None, 0x618ca2bc7428, FeedbackSource(INVALID)](#67:CheckedTaggedToTaggedPointer, #67:CheckedTaggedToTaggedPointer, #78:Merge)
#44:LoadField[BuildLoadDataField, tagged base, 12, 0x3c2f00002475: [String] in ReadOnlySpace: #first, 0x3c2f00067315 <Map[20](HOLEY_ELEMENTS)>, OtherObject, kRepTaggedPointer|kTypeAny, FullWriteBarrier, const (field owner: 0x3c2f0006733d <Map[20](HOLEY_ELEMENTS)>)](#2:Parameter, #43:CheckMaps, #78:Merge)
#46:LoadField[BuildLoadDataField, tagged base, 12, 0x3c2f00003595: [String] in ReadOnlySpace: #x, NonInternal, kRepTaggedPointer|kTypeAny, FullWriteBarrier, const (field owner: 0x3c2f000672c9 <Map[20](HOLEY_ELEMENTS)>)](#44:LoadField, #44:LoadField, #78:Merge)
#50:LoadField[BuildLoadDataField, tagged base, 16, 0x3c2f000035a5: [String] in ReadOnlySpace: #y, NonInternal, kRepTaggedPointer|kTypeAny, FullWriteBarrier, const (field owner: 0x3c2f00067315 <Map[20](HOLEY_ELEMENTS)>)](#44:LoadField, #46:LoadField, #78:Merge)
#55:CheckString[FeedbackSource(INVALID)](#46:LoadField, #50:LoadField, #78:Merge)
#57:StringLength(#55:CheckString)
#56:CheckString[FeedbackSource(INVALID)](#50:LoadField, #55:CheckString, #78:Merge)
#58:StringLength(#56:CheckString)
#59:Int32Add(#57:StringLength, #58:StringLength)
#69:Int32Constant[536870889]()
#61:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#59:Int32Add, #69:Int32Constant, #56:CheckString, #78:Merge)
#70:ChangeInt31ToTaggedSigned(#61:CheckedUint32Bounds)
#62:StringConcat(#70:ChangeInt31ToTaggedSigned, #55:CheckString, #56:CheckString)
#38:Return(#71:Int32Constant, #62:StringConcat, #61:CheckedUint32Bounds, #78:Merge)
#39:End(#38:Return)
```

**关键节点**:
- `#67:CheckedTaggedToTaggedPointer` - 检查参数是否为指针
- `#43:CheckMaps` - 检查外层对象的Map
- `#44:LoadField[first]` - 加载嵌套对象
- `#46:LoadField[x]`, `#50:LoadField[y]` - 加载嵌套对象的字段
- `#55:CheckString`, `#56:CheckString` - 检查字段是否为字符串

#### 优化后（有 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#68:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, sparse:^^](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:..]()
#64:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#42:HeapConstant[0x3d1a000671cd <JSFunction concat_nested (sfi = 0x3d1a000670a5)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 0, Ignore, 0x3d1a000670a5 <SharedFunctionInfo concat_nested>](#9:TypedStateValues, #10:TypedStateValues, #64:TypedStateValues, #4:Parameter, #42:HeapConstant, #0:Start)
#69:ExternalConstant[0x5cab07dfc020]()
#63:Int64Constant[0]()
#70:Load[kRepWord64](#69:ExternalConstant, #63:Int64Constant, #0:Start, #0:Start)
#71:StackPointerGreaterThan[JSFunctionEntry](#70:Load, #70:Load)
#80:HeapConstant[0x3d1a001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#77:LoadStackCheckOffset()
#78:ExternalConstant[0x779292bcd520 <StackGuardWithGap.entry>]()
#79:Int32Constant[1]()
#6:HeapConstant[0x3d1a0004b2f1 <NativeContext[304]>]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x3d1a000670a5 <SharedFunctionInfo concat_nested>](#9:TypedStateValues, #10:TypedStateValues, #64:TypedStateValues, #4:Parameter, #42:HeapConstant, #0:Start)
#72:Branch[Unspecified, True](#71:StackPointerGreaterThan, #0:Start)
#74:IfFalse(#72:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#80:HeapConstant, #77:LoadStackCheckOffset, #78:ExternalConstant, #79:Int32Constant, #6:HeapConstant, #13:FrameState, #71:StackPointerGreaterThan, #74:IfFalse)
#73:IfTrue(#72:Branch)
#75:Merge(#73:IfTrue, #8:Call)
#76:EffectPhi(#71:StackPointerGreaterThan, #8:Call, #75:Merge)
#14:Checkpoint(#15:FrameState, #76:EffectPhi, #75:Merge)
#43:CheckMaps[None, 0x5cab07eb3458, FeedbackSource(INVALID)](#2:Parameter, #14:Checkpoint, #75:Merge)
#44:LoadField[BuildLoadDataField, tagged base, 12, 0x3d1a00002475: [String] in ReadOnlySpace: #first, 0x3d1a00067315 <Map[20](HOLEY_ELEMENTS)>, OtherObject, kRepTaggedPointer|kTypeAny, FullWriteBarrier, const (field owner: 0x3d1a0006733d <Map[20](HOLEY_ELEMENTS)>)](#2:Parameter, #43:CheckMaps, #75:Merge)
#46:LoadField[BuildLoadDataField, tagged base, 12, 0x3d1a00003595: [String] in ReadOnlySpace: #x, NonInternal, kRepTaggedPointer|kTypeAny, FullWriteBarrier, const (field owner: 0x3d1a000672c9 <Map[20](HOLEY_ELEMENTS)>)](#44:LoadField, #44:LoadField, #75:Merge)
#55:StringLength(#46:LoadField)
#50:LoadField[BuildLoadDataField, tagged base, 16, 0x3d1a000035a5: [String] in ReadOnlySpace: #y, NonInternal, kRepTaggedPointer|kTypeAny, FullWriteBarrier, const (field owner: 0x3d1a00067315 <Map[20](HOLEY_ELEMENTS)>)](#44:LoadField, #46:LoadField, #75:Merge)
#56:StringLength(#50:LoadField)
#57:Int32Add(#55:StringLength, #56:StringLength)
#66:Int32Constant[536870889]()
#59:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#57:Int32Add, #66:Int32Constant, #50:LoadField, #75:Merge)
#67:ChangeInt31ToTaggedSigned(#59:CheckedUint32Bounds)
#60:StringConcat(#67:ChangeInt31ToTaggedSigned, #46:LoadField, #50:LoadField)
#38:Return(#68:Int32Constant, #60:StringConcat, #59:CheckedUint32Bounds, #75:Merge)
#39:End(#38:Return)
```

**关键节点**:
- `#2:Parameter[1]` - 参数类型已标注为嵌套 Interface
- `#43:CheckMaps` - 仍然保留（确保对象结构正确）
- `#44:LoadField[first]`, `#46:LoadField[x]`, `#50:LoadField[y]` - 直接加载字段
- `#55:StringLength`, `#56:StringLength` - 直接访问字符串长度

**优化效果**:
- ✅ **移除了 `#67:CheckedTaggedToTaggedPointer`** - 参数类型已知为对象
- ✅ **移除了 `#55:CheckString`, `#56:CheckString`** - 嵌套字段类型已知为字符串
- ✅ **保留了 `#43:CheckMaps`** - 仍需验证对象的具体结构
- ✅ **Graph 简化** - 从 44 行减少到 41 行


### 6. Array 类型

#### 测试代码

```javascript
function concat_arr(data) {
    return data[0] + data[1];
}
```

**Metadata**:
```
260 @params any arr<str> @ret str
```

#### 优化前（无 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#62:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:.]()
#58:Int64Constant[0]()
#59:TypedStateValues[kRepTagged|kTypeAny, dense](#58:Int64Constant)
#4:Parameter[5, debug name: %context](#0:Start)
#33:HeapConstant[0x002900067161 <JSFunction concat_arr (sfi = 0x290006707d)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 1, Ignore, 0x00290006707d <SharedFunctionInfo concat_arr>](#9:TypedStateValues, #10:TypedStateValues, #59:TypedStateValues, #4:Parameter, #33:HeapConstant, #0:Start)
#74:ExternalConstant[0x5bc383529020]()
#75:Load[kRepWord64](#74:ExternalConstant, #58:Int64Constant, #0:Start, #0:Start)
#76:StackPointerGreaterThan[JSFunctionEntry](#75:Load, #75:Load)
#84:HeapConstant[0x0029001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#82:LoadStackCheckOffset()
#83:ExternalConstant[0x73fbf4bcd520 <StackGuardWithGap.entry>]()
#66:Int32Constant[1]()
#6:HeapConstant[0x00290004b2f1 <NativeContext[304]>]()
#60:TypedStateValues[, sparse:.]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x00290006707d <SharedFunctionInfo concat_arr>](#9:TypedStateValues, #10:TypedStateValues, #60:TypedStateValues, #4:Parameter, #33:HeapConstant, #0:Start)
#77:Branch[Unspecified, True](#76:StackPointerGreaterThan, #0:Start)
#79:IfFalse(#77:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#84:HeapConstant, #82:LoadStackCheckOffset, #83:ExternalConstant, #66:Int32Constant, #6:HeapConstant, #13:FrameState, #76:StackPointerGreaterThan, #79:IfFalse)
#78:IfTrue(#77:Branch)
#80:Merge(#78:IfTrue, #8:Call)
#81:EffectPhi(#76:StackPointerGreaterThan, #8:Call, #80:Merge)
#15:Checkpoint(#16:FrameState, #81:EffectPhi, #80:Merge)
#61:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #80:Merge)
#34:CheckMaps[None, 0x5bc3835e0420, FeedbackSource(INVALID)](#61:CheckedTaggedToTaggedPointer, #61:CheckedTaggedToTaggedPointer, #80:Merge)
#35:LoadField[JSObjectElements, tagged base, 8, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#2:Parameter, #34:CheckMaps, #80:Merge)
#36:LoadField[JSArrayLength, tagged base, 12, Range(0, 67108864), kRepTaggedSigned|kTypeInt32, NoWriteBarrier, mutable](#2:Parameter, #35:LoadField, #80:Merge)
#63:ChangeTaggedSignedToInt32(#36:LoadField)
#37:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#62:Int32Constant, #63:ChangeTaggedSignedToInt32, #36:LoadField, #80:Merge)
#64:ChangeUint32ToUint64(#37:CheckedUint32Bounds)
#38:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#35:LoadField, #64:ChangeUint32ToUint64, #37:CheckedUint32Bounds, #80:Merge)
#42:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#66:Int32Constant, #63:ChangeTaggedSignedToInt32, #38:LoadElement, #80:Merge)
#68:ChangeUint32ToUint64(#42:CheckedUint32Bounds)
#43:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#35:LoadField, #68:ChangeUint32ToUint64, #42:CheckedUint32Bounds, #80:Merge)
#69:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#38:LoadElement, #43:LoadElement, #80:Merge)
#48:CheckString[FeedbackSource(INVALID)](#69:CheckedTaggedToTaggedPointer, #69:CheckedTaggedToTaggedPointer, #80:Merge)
#50:StringLength(#48:CheckString)
#70:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#43:LoadElement, #48:CheckString, #80:Merge)
#49:CheckString[FeedbackSource(INVALID)](#70:CheckedTaggedToTaggedPointer, #70:CheckedTaggedToTaggedPointer, #80:Merge)
#51:StringLength(#49:CheckString)
#52:Int32Add(#50:StringLength, #51:StringLength)
#72:Int32Constant[536870889]()
#54:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#52:Int32Add, #72:Int32Constant, #49:CheckString, #80:Merge)
#73:ChangeInt31ToTaggedSigned(#54:CheckedUint32Bounds)
#55:StringConcat(#73:ChangeInt31ToTaggedSigned, #48:CheckString, #49:CheckString)
#29:Return(#62:Int32Constant, #55:StringConcat, #54:CheckedUint32Bounds, #80:Merge)
#30:End(#29:Return)
```

**关键节点**:
- `#61:CheckedTaggedToTaggedPointer` - 检查参数是否为指针
- `#34:CheckMaps` - 检查数组的Map
- `#37:CheckedUint32Bounds`, `#42:CheckedUint32Bounds` - 边界检查（索引 0 和 1）
- `#38:LoadElement`, `#43:LoadElement` - 加载数组元素
- `#69:CheckedTaggedToTaggedPointer`, `#70:CheckedTaggedToTaggedPointer` - 检查元素是否为指针
- `#48:CheckString`, `#49:CheckString` - 检查元素是否为字符串

#### 优化后（有 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#59:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:.]()
#56:Int64Constant[0]()
#57:TypedStateValues[kRepTagged|kTypeAny, dense](#56:Int64Constant)
#4:Parameter[5, debug name: %context](#0:Start)
#33:HeapConstant[0x117600067161 <JSFunction concat_arr (sfi = 0x11760006707d)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 1, Ignore, 0x11760006707d <SharedFunctionInfo concat_arr>](#9:TypedStateValues, #10:TypedStateValues, #57:TypedStateValues, #4:Parameter, #33:HeapConstant, #0:Start)
#69:ExternalConstant[0x6543aac32020]()
#70:Load[kRepWord64](#69:ExternalConstant, #56:Int64Constant, #0:Start, #0:Start)
#71:StackPointerGreaterThan[JSFunctionEntry](#70:Load, #70:Load)
#79:HeapConstant[0x1176001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#77:LoadStackCheckOffset()
#78:ExternalConstant[0x73ee761cd520 <StackGuardWithGap.entry>]()
#63:Int32Constant[1]()
#6:HeapConstant[0x11760004b2f1 <NativeContext[304]>]()
#58:TypedStateValues[, sparse:.]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x11760006707d <SharedFunctionInfo concat_arr>](#9:TypedStateValues, #10:TypedStateValues, #58:TypedStateValues, #4:Parameter, #33:HeapConstant, #0:Start)
#72:Branch[Unspecified, True](#71:StackPointerGreaterThan, #0:Start)
#74:IfFalse(#72:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#79:HeapConstant, #77:LoadStackCheckOffset, #78:ExternalConstant, #63:Int32Constant, #6:HeapConstant, #13:FrameState, #71:StackPointerGreaterThan, #74:IfFalse)
#73:IfTrue(#72:Branch)
#75:Merge(#73:IfTrue, #8:Call)
#76:EffectPhi(#71:StackPointerGreaterThan, #8:Call, #75:Merge)
#15:Checkpoint(#16:FrameState, #76:EffectPhi, #75:Merge)
#34:CheckMaps[None, 0x6543aace9410, FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #75:Merge)
#35:LoadField[JSObjectElements, tagged base, 8, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#2:Parameter, #34:CheckMaps, #75:Merge)
#36:LoadField[JSArrayLength, tagged base, 12, Range(0, 67108864), kRepTaggedSigned|kTypeInt32, NoWriteBarrier, mutable](#2:Parameter, #35:LoadField, #75:Merge)
#60:ChangeTaggedSignedToInt32(#36:LoadField)
#37:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#59:Int32Constant, #60:ChangeTaggedSignedToInt32, #36:LoadField, #75:Merge)
#61:ChangeUint32ToUint64(#37:CheckedUint32Bounds)
#38:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#35:LoadField, #61:ChangeUint32ToUint64, #37:CheckedUint32Bounds, #75:Merge)
#48:StringLength(#38:LoadElement)
#42:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#63:Int32Constant, #60:ChangeTaggedSignedToInt32, #38:LoadElement, #75:Merge)
#65:ChangeUint32ToUint64(#42:CheckedUint32Bounds)
#43:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#35:LoadField, #65:ChangeUint32ToUint64, #42:CheckedUint32Bounds, #75:Merge)
#49:StringLength(#43:LoadElement)
#50:Int32Add(#48:StringLength, #49:StringLength)
#67:Int32Constant[536870889]()
#52:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#50:Int32Add, #67:Int32Constant, #43:LoadElement, #75:Merge)
#68:ChangeInt31ToTaggedSigned(#52:CheckedUint32Bounds)
#53:StringConcat(#68:ChangeInt31ToTaggedSigned, #38:LoadElement, #43:LoadElement)
#29:Return(#59:Int32Constant, #53:StringConcat, #52:CheckedUint32Bounds, #75:Merge)
#30:End(#29:Return)
```

**关键节点**:
- `#2:Parameter[1]` - 参数类型已标注为 `arr[str]`
- `#34:CheckMaps` - 仍然保留（确保数组结构正确）
- `#37:CheckedUint32Bounds`, `#42:CheckedUint32Bounds` - 边界检查（保留）
- `#38:LoadElement`, `#43:LoadElement` - 加载数组元素
- `#48:StringLength`, `#49:StringLength` - 直接访问字符串长度

**优化效果**:
- ✅ **移除了 `#61:CheckedTaggedToTaggedPointer`** - 参数类型已知为数组
- ✅ **移除了 `#69:CheckedTaggedToTaggedPointer`, `#70:CheckedTaggedToTaggedPointer`** - 元素类型已知
- ✅ **移除了 `#48:CheckString`, `#49:CheckString`** - 数组元素类型已知为字符串
- ✅ **保留了边界检查** - 数组访问仍需验证索引在范围内
- ✅ **Graph 简化** - 从 53 行减少到 48 行


### 7. Tuple 类型

#### 测试代码

```javascript
function concat(data) {
    return data[0] + data[1];
}
```

**Metadata**:
```
285 @params any tuple[str,str] @ret str
```

#### 优化前（无 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#62:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:.]()
#58:Int64Constant[0]()
#59:TypedStateValues[kRepTagged|kTypeAny, dense](#58:Int64Constant)
#4:Parameter[5, debug name: %context](#0:Start)
#33:HeapConstant[0x2c6300067135 <JSFunction concat (sfi = 0x2c6300067059)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 1, Ignore, 0x2c6300067059 <SharedFunctionInfo concat>](#9:TypedStateValues, #10:TypedStateValues, #59:TypedStateValues, #4:Parameter, #33:HeapConstant, #0:Start)
#74:ExternalConstant[0x637d8bb5f020]()
#75:Load[kRepWord64](#74:ExternalConstant, #58:Int64Constant, #0:Start, #0:Start)
#76:StackPointerGreaterThan[JSFunctionEntry](#75:Load, #75:Load)
#84:HeapConstant[0x2c63001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#82:LoadStackCheckOffset()
#83:ExternalConstant[0x7865477cd520 <StackGuardWithGap.entry>]()
#66:Int32Constant[1]()
#6:HeapConstant[0x2c630004b2f1 <NativeContext[304]>]()
#60:TypedStateValues[, sparse:.]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x2c6300067059 <SharedFunctionInfo concat>](#9:TypedStateValues, #10:TypedStateValues, #60:TypedStateValues, #4:Parameter, #33:HeapConstant, #0:Start)
#77:Branch[Unspecified, True](#76:StackPointerGreaterThan, #0:Start)
#79:IfFalse(#77:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#84:HeapConstant, #82:LoadStackCheckOffset, #83:ExternalConstant, #66:Int32Constant, #6:HeapConstant, #13:FrameState, #76:StackPointerGreaterThan, #79:IfFalse)
#78:IfTrue(#77:Branch)
#80:Merge(#78:IfTrue, #8:Call)
#81:EffectPhi(#76:StackPointerGreaterThan, #8:Call, #80:Merge)
#15:Checkpoint(#16:FrameState, #81:EffectPhi, #80:Merge)
#61:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #80:Merge)
#34:CheckMaps[None, 0x637d8bc163e0, FeedbackSource(INVALID)](#61:CheckedTaggedToTaggedPointer, #61:CheckedTaggedToTaggedPointer, #80:Merge)
#35:LoadField[JSObjectElements, tagged base, 8, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#2:Parameter, #34:CheckMaps, #80:Merge)
#36:LoadField[JSArrayLength, tagged base, 12, Range(0, 67108864), kRepTaggedSigned|kTypeInt32, NoWriteBarrier, mutable](#2:Parameter, #35:LoadField, #80:Merge)
#63:ChangeTaggedSignedToInt32(#36:LoadField)
#37:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#62:Int32Constant, #63:ChangeTaggedSignedToInt32, #36:LoadField, #80:Merge)
#64:ChangeUint32ToUint64(#37:CheckedUint32Bounds)
#38:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#35:LoadField, #64:ChangeUint32ToUint64, #37:CheckedUint32Bounds, #80:Merge)
#42:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#66:Int32Constant, #63:ChangeTaggedSignedToInt32, #38:LoadElement, #80:Merge)
#68:ChangeUint32ToUint64(#42:CheckedUint32Bounds)
#43:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#35:LoadField, #68:ChangeUint32ToUint64, #42:CheckedUint32Bounds, #80:Merge)
#69:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#38:LoadElement, #43:LoadElement, #80:Merge)
#48:CheckString[FeedbackSource(INVALID)](#69:CheckedTaggedToTaggedPointer, #69:CheckedTaggedToTaggedPointer, #80:Merge)
#50:StringLength(#48:CheckString)
#70:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#43:LoadElement, #48:CheckString, #80:Merge)
#49:CheckString[FeedbackSource(INVALID)](#70:CheckedTaggedToTaggedPointer, #70:CheckedTaggedToTaggedPointer, #80:Merge)
#51:StringLength(#49:CheckString)
#52:Int32Add(#50:StringLength, #51:StringLength)
#72:Int32Constant[536870889]()
#54:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#52:Int32Add, #72:Int32Constant, #49:CheckString, #80:Merge)
#73:ChangeInt31ToTaggedSigned(#54:CheckedUint32Bounds)
#55:StringConcat(#73:ChangeInt31ToTaggedSigned, #48:CheckString, #49:CheckString)
#29:Return(#62:Int32Constant, #55:StringConcat, #54:CheckedUint32Bounds, #80:Merge)
#30:End(#29:Return)
```

**关键节点**:
- `#61:CheckedTaggedToTaggedPointer` - 检查参数是否为指针
- `#34:CheckMaps` - 检查Tuple的Map
- `#37:CheckedUint32Bounds`, `#42:CheckedUint32Bounds` - 边界检查（索引 0 和 1）
- `#38:LoadElement`, `#43:LoadElement` - 加载Tuple元素
- `#69:CheckedTaggedToTaggedPointer`, `#70:CheckedTaggedToTaggedPointer` - 检查元素是否为指针
- `#48:CheckString`, `#49:CheckString` - 检查元素是否为字符串

#### 优化后（有 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#66:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:.]()
#58:Int64Constant[0]()
#59:TypedStateValues[kRepTagged|kTypeAny, dense](#58:Int64Constant)
#4:Parameter[5, debug name: %context](#0:Start)
#33:HeapConstant[0x3ed900067135 <JSFunction concat (sfi = 0x3ed900067059)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 1, Ignore, 0x3ed900067059 <SharedFunctionInfo concat>](#9:TypedStateValues, #10:TypedStateValues, #59:TypedStateValues, #4:Parameter, #33:HeapConstant, #0:Start)
#67:ExternalConstant[0x5d5412bf7020]()
#68:Load[kRepWord64](#67:ExternalConstant, #58:Int64Constant, #0:Start, #0:Start)
#69:StackPointerGreaterThan[JSFunctionEntry](#68:Load, #68:Load)
#78:HeapConstant[0x3ed9001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#75:LoadStackCheckOffset()
#76:ExternalConstant[0x73ac041cd520 <StackGuardWithGap.entry>]()
#77:Int32Constant[1]()
#6:HeapConstant[0x3ed90004b2f1 <NativeContext[304]>]()
#60:TypedStateValues[, sparse:.]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x3ed900067059 <SharedFunctionInfo concat>](#9:TypedStateValues, #10:TypedStateValues, #60:TypedStateValues, #4:Parameter, #33:HeapConstant, #0:Start)
#70:Branch[Unspecified, True](#69:StackPointerGreaterThan, #0:Start)
#72:IfFalse(#70:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#78:HeapConstant, #75:LoadStackCheckOffset, #76:ExternalConstant, #77:Int32Constant, #6:HeapConstant, #13:FrameState, #69:StackPointerGreaterThan, #72:IfFalse)
#71:IfTrue(#70:Branch)
#73:Merge(#71:IfTrue, #8:Call)
#74:EffectPhi(#69:StackPointerGreaterThan, #8:Call, #73:Merge)
#15:Checkpoint(#16:FrameState, #74:EffectPhi, #73:Merge)
#34:CheckMaps[None, 0x5d5412cae410, FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #73:Merge)
#35:LoadField[JSObjectElements, tagged base, 8, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#2:Parameter, #34:CheckMaps, #73:Merge)
#38:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#35:LoadField, #58:Int64Constant, #35:LoadField, #73:Merge)
#50:StringLength(#38:LoadElement)
#62:Int64Constant[1]()
#43:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#35:LoadField, #62:Int64Constant, #38:LoadElement, #73:Merge)
#51:StringLength(#43:LoadElement)
#52:Int32Add(#50:StringLength, #51:StringLength)
#64:Int32Constant[536870889]()
#54:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#52:Int32Add, #64:Int32Constant, #43:LoadElement, #73:Merge)
#65:ChangeInt31ToTaggedSigned(#54:CheckedUint32Bounds)
#55:StringConcat(#65:ChangeInt31ToTaggedSigned, #38:LoadElement, #43:LoadElement)
#29:Return(#66:Int32Constant, #55:StringConcat, #54:CheckedUint32Bounds, #73:Merge)
#30:End(#29:Return)
```

**关键节点**:
- `#2:Parameter[1]` - 参数类型已标注为 `tuple[str, str]`
- `#34:CheckMaps` - 仍然保留（确保Tuple结构正确）
- `#38:LoadElement`, `#43:LoadElement` - 直接加载Tuple元素（使用常量索引 `#58:Int64Constant[0]` 和 `#62:Int64Constant[1]`）
- `#50:StringLength`, `#51:StringLength` - 直接访问字符串长度

**优化效果**:
- ✅ **移除了 `#61:CheckedTaggedToTaggedPointer`** - 参数类型已知为Tuple（数组）
- ✅ **移除了 `#37:CheckedUint32Bounds`, `#42:CheckedUint32Bounds`** - 索引已知为常量且在Tuple长度范围内，边界检查被优化
- ✅ **移除了 `#69:CheckedTaggedToTaggedPointer`, `#70:CheckedTaggedToTaggedPointer`** - 元素类型已知
- ✅ **移除了 `#48:CheckString`, `#49:CheckString`** - Tuple元素类型已知为字符串
- ✅ **保留了 `#34:CheckMaps`** - Tuple的Hidden Class有两种（根据是否包含SMI），需要验证
- ✅ **Graph 显著简化** - 从 53 行减少到 43 行


### 8. Tuple 类型（混合类型元素）

#### 测试代码

```javascript
function process(data) {
    return data[0] + data[1].toString();
}
```

**Metadata**:
```
329 @params any tuple[str,num] @ret str
```

#### 优化前（无 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#76:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:...]()
#72:Int64Constant[0]()
#73:TypedStateValues[kRepTagged|kTypeAny, dense](#72:Int64Constant)
#4:Parameter[5, debug name: %context](#0:Start)
#43:HeapConstant[0x1b4600067125 <JSFunction process (sfi = 0x1b4600067051)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 1, Ignore, 0x1b4600067051 <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #73:TypedStateValues, #4:Parameter, #43:HeapConstant, #0:Start)
#89:ExternalConstant[0x64c70d788020]()
#90:Load[kRepWord64](#89:ExternalConstant, #72:Int64Constant, #0:Start, #0:Start)
#91:StackPointerGreaterThan[JSFunctionEntry](#90:Load, #90:Load)
#99:HeapConstant[0x1b46001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#97:LoadStackCheckOffset()
#98:ExternalConstant[0x755aeefcd520 <StackGuardWithGap.entry>]()
#80:Int32Constant[1]()
#6:HeapConstant[0x1b460004b2f1 <NativeContext[304]>]()
#74:TypedStateValues[, sparse:.]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x1b4600067051 <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #74:TypedStateValues, #4:Parameter, #43:HeapConstant, #0:Start)
#92:Branch[Unspecified, True](#91:StackPointerGreaterThan, #0:Start)
#94:IfFalse(#92:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#99:HeapConstant, #97:LoadStackCheckOffset, #98:ExternalConstant, #80:Int32Constant, #6:HeapConstant, #13:FrameState, #91:StackPointerGreaterThan, #94:IfFalse)
#93:IfTrue(#92:Branch)
#95:Merge(#93:IfTrue, #8:Call)
#96:EffectPhi(#91:StackPointerGreaterThan, #8:Call, #95:Merge)
#15:Checkpoint(#16:FrameState, #96:EffectPhi, #95:Merge)
#75:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #95:Merge)
#44:CheckMaps[None, 0x64c70d83f558, FeedbackSource(INVALID)](#75:CheckedTaggedToTaggedPointer, #75:CheckedTaggedToTaggedPointer, #95:Merge)
#45:LoadField[JSObjectElements, tagged base, 8, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#2:Parameter, #44:CheckMaps, #95:Merge)
#46:LoadField[JSArrayLength, tagged base, 12, Range(0, 67108864), kRepTaggedSigned|kTypeInt32, NoWriteBarrier, mutable](#2:Parameter, #45:LoadField, #95:Merge)
#77:ChangeTaggedSignedToInt32(#46:LoadField)
#47:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#76:Int32Constant, #77:ChangeTaggedSignedToInt32, #46:LoadField, #95:Merge)
#78:ChangeUint32ToUint64(#47:CheckedUint32Bounds)
#48:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#45:LoadField, #78:ChangeUint32ToUint64, #47:CheckedUint32Bounds, #95:Merge)
#21:TypedStateValues[kRepTagged|kTypeAny, sparse:^..](#48:LoadElement)
#61:HeapConstant[0x1b46001d7c59 <Code BUILTIN NumberPrototypeToString>]()
#55:HeapConstant[0x1b460004ca15 <JSFunction toString (sfi = 0x1b460017a869)>]()
#3:HeapConstant[0x1b4600000011 <undefined>]()
#52:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#80:Int32Constant, #77:ChangeTaggedSignedToInt32, #48:LoadElement, #95:Merge)
#82:ChangeUint32ToUint64(#52:CheckedUint32Bounds)
#53:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#45:LoadField, #82:ChangeUint32ToUint64, #52:CheckedUint32Bounds, #95:Merge)
#54:CheckNumber[FeedbackSource(INVALID)](#53:LoadElement, #53:LoadElement, #95:Merge)
#60:LoadField[JSFunctionContext, tagged base, 20, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#55:HeapConstant, #54:CheckNumber, #95:Merge)
#34:FrameState[UNOPTIMIZED_FRAME, 16, PokeAt(0), 0x1b4600067051 <SharedFunctionInfo process>](#9:TypedStateValues, #21:TypedStateValues, #74:TypedStateValues, #4:Parameter, #43:HeapConstant, #0:Start)
#33:Call[Code:JSTrampoline Descriptor:r1s1i7f1](#61:HeapConstant, #55:HeapConstant, #3:HeapConstant, #80:Int32Constant, #76:Int32Constant, #53:LoadElement, #60:LoadField, #34:FrameState, #60:LoadField, #95:Merge)
#83:TypedStateValues[kRepTagged|kTypeAny, dense](#33:Call)
#36:FrameState[UNOPTIMIZED_FRAME, 20, Ignore, 0x1b4600067051 <SharedFunctionInfo process>](#9:TypedStateValues, #21:TypedStateValues, #83:TypedStateValues, #4:Parameter, #43:HeapConstant, #0:Start)
#35:Checkpoint(#36:FrameState, #33:Call, #33:Call)
#84:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#48:LoadElement, #35:Checkpoint, #33:Call)
#62:CheckString[FeedbackSource(INVALID)](#84:CheckedTaggedToTaggedPointer, #84:CheckedTaggedToTaggedPointer, #33:Call)
#64:StringLength(#62:CheckString)
#85:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#33:Call, #62:CheckString, #33:Call)
#63:CheckString[FeedbackSource(INVALID)](#85:CheckedTaggedToTaggedPointer, #85:CheckedTaggedToTaggedPointer, #33:Call)
#65:StringLength(#63:CheckString)
#66:Int32Add(#64:StringLength, #65:StringLength)
#87:Int32Constant[536870889]()
#68:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#66:Int32Add, #87:Int32Constant, #63:CheckString, #33:Call)
#88:ChangeInt31ToTaggedSigned(#68:CheckedUint32Bounds)
#69:StringConcat(#88:ChangeInt31ToTaggedSigned, #62:CheckString, #63:CheckString)
#39:Return(#76:Int32Constant, #69:StringConcat, #68:CheckedUint32Bounds, #33:Call)
#40:End(#39:Return)
```

**关键节点**:
- `#75:CheckedTaggedToTaggedPointer` - 检查参数是否为指针
- `#44:CheckMaps` - 检查Tuple的Map
- `#47:CheckedUint32Bounds`, `#52:CheckedUint32Bounds` - 边界检查（索引 0 和 1）
- `#48:LoadElement`, `#53:LoadElement` - 加载Tuple元素
- `#54:CheckNumber` - 检查第二个元素是否为数字
- `#33:Call` - 调用 `toString()` 方法
- `#84:CheckedTaggedToTaggedPointer`, `#85:CheckedTaggedToTaggedPointer` - 检查是否为指针
- `#62:CheckString`, `#63:CheckString` - 检查是否为字符串

#### 优化后（有 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#79:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:...]()
#73:Int64Constant[0]()
#74:TypedStateValues[kRepTagged|kTypeAny, dense](#73:Int64Constant)
#4:Parameter[5, debug name: %context](#0:Start)
#43:HeapConstant[0x283400067125 <JSFunction process (sfi = 0x283400067051)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 1, Ignore, 0x283400067051 <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #74:TypedStateValues, #4:Parameter, #43:HeapConstant, #0:Start)
#85:ExternalConstant[0x5c7ff7f4c020]()
#86:Load[kRepWord64](#85:ExternalConstant, #73:Int64Constant, #0:Start, #0:Start)
#87:StackPointerGreaterThan[JSFunctionEntry](#86:Load, #86:Load)
#95:HeapConstant[0x2834001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#93:LoadStackCheckOffset()
#94:ExternalConstant[0x7a1735dcd520 <StackGuardWithGap.entry>]()
#78:Int32Constant[1]()
#6:HeapConstant[0x28340004b2f1 <NativeContext[304]>]()
#75:TypedStateValues[, sparse:.]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x283400067051 <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #75:TypedStateValues, #4:Parameter, #43:HeapConstant, #0:Start)
#88:Branch[Unspecified, True](#87:StackPointerGreaterThan, #0:Start)
#90:IfFalse(#88:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#95:HeapConstant, #93:LoadStackCheckOffset, #94:ExternalConstant, #78:Int32Constant, #6:HeapConstant, #13:FrameState, #87:StackPointerGreaterThan, #90:IfFalse)
#89:IfTrue(#88:Branch)
#91:Merge(#89:IfTrue, #8:Call)
#92:EffectPhi(#87:StackPointerGreaterThan, #8:Call, #91:Merge)
#15:Checkpoint(#16:FrameState, #92:EffectPhi, #91:Merge)
#44:CheckMaps[None, 0x5c7ff8003548, FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #91:Merge)
#45:LoadField[JSObjectElements, tagged base, 8, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#2:Parameter, #44:CheckMaps, #91:Merge)
#48:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#45:LoadField, #73:Int64Constant, #45:LoadField, #91:Merge)
#65:StringLength(#48:LoadElement)
#63:HeapConstant[0x2834001d7c59 <Code BUILTIN NumberPrototypeToString>]()
#55:HeapConstant[0x28340004ca15 <JSFunction toString (sfi = 0x28340017a869)>]()
#3:HeapConstant[0x283400000011 <undefined>]()
#77:Int64Constant[1]()
#53:LoadElement[tagged base, 8, NonInternal, kRepTagged|kTypeAny, FullWriteBarrier](#45:LoadField, #77:Int64Constant, #48:LoadElement, #91:Merge)
#62:LoadField[JSFunctionContext, tagged base, 20, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#55:HeapConstant, #53:LoadElement, #91:Merge)
#21:TypedStateValues[kRepTagged|kTypeAny, sparse:^..](#48:LoadElement)
#34:FrameState[UNOPTIMIZED_FRAME, 16, PokeAt(0), 0x283400067051 <SharedFunctionInfo process>](#9:TypedStateValues, #21:TypedStateValues, #75:TypedStateValues, #4:Parameter, #43:HeapConstant, #0:Start)
#33:Call[Code:JSTrampoline Descriptor:r1s1i7f1](#63:HeapConstant, #55:HeapConstant, #3:HeapConstant, #78:Int32Constant, #79:Int32Constant, #53:LoadElement, #62:LoadField, #34:FrameState, #62:LoadField, #91:Merge)
#80:TypedStateValues[kRepTagged|kTypeAny, dense](#33:Call)
#36:FrameState[UNOPTIMIZED_FRAME, 20, Ignore, 0x283400067051 <SharedFunctionInfo process>](#9:TypedStateValues, #21:TypedStateValues, #80:TypedStateValues, #4:Parameter, #43:HeapConstant, #0:Start)
#35:Checkpoint(#36:FrameState, #33:Call, #33:Call)
#81:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#33:Call, #35:Checkpoint, #33:Call)
#64:CheckString[FeedbackSource(INVALID)](#81:CheckedTaggedToTaggedPointer, #81:CheckedTaggedToTaggedPointer, #33:Call)
#66:StringLength(#64:CheckString)
#67:Int32Add(#65:StringLength, #66:StringLength)
#83:Int32Constant[536870889]()
#69:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#67:Int32Add, #83:Int32Constant, #64:CheckString, #33:Call)
#84:ChangeInt31ToTaggedSigned(#69:CheckedUint32Bounds)
#70:StringConcat(#84:ChangeInt31ToTaggedSigned, #48:LoadElement, #64:CheckString)
#39:Return(#79:Int32Constant, #70:StringConcat, #69:CheckedUint32Bounds, #33:Call)
#40:End(#39:Return)
```

**关键节点**:
- `#2:Parameter[1]` - 参数类型已标注为 `tuple[str, num]`
- `#44:CheckMaps` - 仍然保留（确保Tuple结构正确）
- `#48:LoadElement`, `#53:LoadElement` - 直接加载Tuple元素（使用常量索引）
- `#65:StringLength` - 直接访问第一个元素（字符串）的长度
- `#33:Call` - 调用 `toString()` 方法
- `#64:CheckString` - 检查 `toString()` 返回值是否为字符串

**优化效果**:
- ✅ **移除了 `#75:CheckedTaggedToTaggedPointer`** - 参数类型已知为Tuple
- ✅ **移除了 `#47:CheckedUint32Bounds`, `#52:CheckedUint32Bounds`** - 索引已知为常量且在范围内，边界检查被优化
- ✅ **移除了 `#54:CheckNumber`** - 第二个元素类型已知为数字
- ✅ **移除了 `#84:CheckedTaggedToTaggedPointer`** - 第一个元素类型已知为字符串
- ✅ **移除了 `#62:CheckString`** - 第一个元素类型已知为字符串
- ✅ **保留了 `#44:CheckMaps`** - Tuple的Hidden Class需要验证
- ✅ **保留了 `#81:CheckedTaggedToTaggedPointer`, `#64:CheckString`** - `toString()` 返回值仍需验证
- ✅ **Graph 简化** - 从 64 行减少到 55 行


### 9. Tuple Length 优化

#### 测试代码

```javascript
function getLength(tuple) {
    return tuple.length;
}
```

**Metadata**:
```
288 @params any tuple[str,str] @ret any
```

#### 优化前（无 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, dense]()
#33:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#23:HeapConstant[0x089e00067129 <JSFunction getLength (sfi = 0x89e00067055)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 0, Ignore, 0x089e00067055 <SharedFunctionInfo getLength>](#9:TypedStateValues, #10:TypedStateValues, #33:TypedStateValues, #4:Parameter, #23:HeapConstant, #0:Start)
#36:ExternalConstant[0x62032efde020]()
#32:Int64Constant[0]()
#37:Load[kRepWord64](#36:ExternalConstant, #32:Int64Constant, #0:Start, #0:Start)
#38:StackPointerGreaterThan[JSFunctionEntry](#37:Load, #37:Load)
#47:HeapConstant[0x089e001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#44:LoadStackCheckOffset()
#45:ExternalConstant[0x72d05f3cd520 <StackGuardWithGap.entry>]()
#46:Int32Constant[1]()
#6:HeapConstant[0x089e0004b2f1 <NativeContext[304]>]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x089e00067055 <SharedFunctionInfo getLength>](#9:TypedStateValues, #10:TypedStateValues, #33:TypedStateValues, #4:Parameter, #23:HeapConstant, #0:Start)
#39:Branch[Unspecified, True](#38:StackPointerGreaterThan, #0:Start)
#41:IfFalse(#39:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#47:HeapConstant, #44:LoadStackCheckOffset, #45:ExternalConstant, #46:Int32Constant, #6:HeapConstant, #13:FrameState, #38:StackPointerGreaterThan, #41:IfFalse)
#40:IfTrue(#39:Branch)
#42:Merge(#40:IfTrue, #8:Call)
#43:EffectPhi(#38:StackPointerGreaterThan, #8:Call, #42:Merge)
#14:Checkpoint(#15:FrameState, #43:EffectPhi, #42:Merge)
#34:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#2:Parameter, #14:Checkpoint, #42:Merge)
#24:CheckMaps[None, 0x62032f0953b8, FeedbackSource(INVALID)](#34:CheckedTaggedToTaggedPointer, #34:CheckedTaggedToTaggedPointer, #42:Merge)
#25:LoadField[BuildLoadDataField, tagged base, 12, 0x89e00005e11: [String] in ReadOnlySpace: #length, Range(0, 67108864), kRepTaggedSigned|kTypeInt32, FullWriteBarrier, mutable](#2:Parameter, #24:CheckMaps, #42:Merge)
#19:Return(#35:Int32Constant, #25:LoadField, #25:LoadField, #42:Merge)
#20:End(#19:Return)
```

**关键节点**:
- `#34:CheckedTaggedToTaggedPointer` - 检查参数是否为指针
- `#24:CheckMaps` - 检查Tuple的Map
- `#25:LoadField[#length]` - 运行时加载 `length` 字段

#### 优化后（有 metadata）- 最终 Graph

```
----- Graph after V8.TFEarlyOptimization ----- 
#36:Int32Constant[0]()
#34:Int64Constant[4]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, dense]()
#35:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#23:HeapConstant[0x0ecb00067129 <JSFunction getLength (sfi = 0xecb00067055)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 0, Ignore, 0x0ecb00067055 <SharedFunctionInfo getLength>](#9:TypedStateValues, #10:TypedStateValues, #35:TypedStateValues, #4:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[0x5d2e70cea020]()
#33:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #33:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[0x0ecb001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[0x73faad9cd520 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#6:HeapConstant[0x0ecb0004b2f1 <NativeContext[304]>]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x0ecb00067055 <SharedFunctionInfo getLength>](#9:TypedStateValues, #10:TypedStateValues, #35:TypedStateValues, #4:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #6:HeapConstant, #13:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #8:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #8:Call, #43:Merge)
#14:Checkpoint(#15:FrameState, #44:EffectPhi, #43:Merge)
#24:CheckMaps[None, 0x5d2e70da13e8, FeedbackSource(INVALID)](#2:Parameter, #14:Checkpoint, #43:Merge)
#19:Return(#36:Int32Constant, #34:Int64Constant, #24:CheckMaps, #43:Merge)
#20:End(#19:Return)
```

**关键节点**:
- `#2:Parameter[1]` - 参数类型已标注为 `tuple[str, str]`
- `#24:CheckMaps` - 仍然保留（确保Tuple结构正确）
- `#34:Int64Constant[4]` - Tuple长度常量（值为2，Smi编码为4）
- `#19:Return` - 直接返回常量，无需加载字段

**优化效果**:
- ✅ **移除了 `#34:CheckedTaggedToTaggedPointer`** - 参数类型已知为Tuple
- ✅ **移除了 `#25:LoadField[#length]`** - Tuple长度已知，直接使用常量替换
- ✅ **保留了 `#24:CheckMaps`** - Tuple的Hidden Class需要验证
- ✅ **Graph 简化** - 从 33 行减少到 32 行，移除了运行时字段访问

### 10. 函数调用返回值类型

#### 测试代码

```javascript
function getStr() { // returns: str
    return "hello";
}

function process(data) { // data: str, returns: str
    var result = getStr();
    return data + result;
}
```


**Metadata**:

```
348 @params any @ret str
408 @params any str @ret str
```

#### 优化前（无 metadata）- 最终 Graph (process 函数)

```
----- Graph after V8.TFEarlyOptimization ----- 
#56:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, sparse:^^](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:..]()
#33:HeapConstant[0x000a000671d1 <JSFunction getStr (sfi = 0xa000670ed)>]()
#38:HeapConstant[0x000a0004b291 <JSGlobalProxy>]()
#3:HeapConstant[0x000a00000011 <undefined>]()
#55:Int32Constant[1]()
#54:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#32:HeapConstant[0x000a00067205 <JSFunction process (sfi = 0xa0006711d)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 0, Ignore, 0x000a0006711d <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #54:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#63:ExternalConstant[0x5fb2244f9020]()
#52:Int64Constant[0]()
#64:Load[kRepWord64](#63:ExternalConstant, #52:Int64Constant, #0:Start, #0:Start)
#65:StackPointerGreaterThan[JSFunctionEntry](#64:Load, #64:Load)
#73:HeapConstant[0x000a001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#71:LoadStackCheckOffset()
#72:ExternalConstant[0x76698dbcd520 <StackGuardWithGap.entry>]()
#6:HeapConstant[0x000a0004b2f1 <NativeContext[304]>]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x000a0006711d <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #54:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#66:Branch[Unspecified, True](#65:StackPointerGreaterThan, #0:Start)
#68:IfFalse(#66:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#73:HeapConstant, #71:LoadStackCheckOffset, #72:ExternalConstant, #55:Int32Constant, #6:HeapConstant, #13:FrameState, #65:StackPointerGreaterThan, #68:IfFalse)
#67:IfTrue(#66:Branch)
#69:Merge(#67:IfTrue, #8:Call)
#70:EffectPhi(#65:StackPointerGreaterThan, #8:Call, #69:Merge)
#14:Checkpoint(#15:FrameState, #70:EffectPhi, #69:Merge)
#40:LoadField[JSFunctionContext, tagged base, 20, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#33:HeapConstant, #14:Checkpoint, #69:Merge)
#22:FrameState[UNOPTIMIZED_FRAME, 4, PokeAt(0), 0x000a0006711d <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #54:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#21:Call[JS:js-call:r1s1i6f1](#33:HeapConstant, #38:HeapConstant, #3:HeapConstant, #55:Int32Constant, #56:Int32Constant, #40:LoadField, #22:FrameState, #40:LoadField, #69:Merge)
#57:TypedStateValues[kRepTagged|kTypeAny, dense](#21:Call)
#24:FrameState[UNOPTIMIZED_FRAME, 10, Ignore, 0x000a0006711d <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #57:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#23:Checkpoint(#24:FrameState, #21:Call, #21:Call)
#58:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#2:Parameter, #23:Checkpoint, #21:Call)
#42:CheckString[FeedbackSource(INVALID)](#58:CheckedTaggedToTaggedPointer, #58:CheckedTaggedToTaggedPointer, #21:Call)
#44:StringLength(#42:CheckString)
#59:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#21:Call, #42:CheckString, #21:Call)
#43:CheckString[FeedbackSource(INVALID)](#59:CheckedTaggedToTaggedPointer, #59:CheckedTaggedToTaggedPointer, #21:Call)
#45:StringLength(#43:CheckString)
#46:Int32Add(#44:StringLength, #45:StringLength)
#61:Int32Constant[536870889]()
#48:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#46:Int32Add, #61:Int32Constant, #43:CheckString, #21:Call)
#62:ChangeInt31ToTaggedSigned(#48:CheckedUint32Bounds)
#49:StringConcat(#62:ChangeInt31ToTaggedSigned, #42:CheckString, #43:CheckString)
#28:Return(#56:Int32Constant, #49:StringConcat, #48:CheckedUint32Bounds, #21:Call)
#29:End(#28:Return)
```

**关键节点**:
- `#21:Call[JS:js-call]` - 调用 `getStr()` 函数
- `#58:CheckedTaggedToTaggedPointer` - 检查参数 `data` 是否为指针
- `#42:CheckString` - 检查参数 `data` 是否为字符串
- `#59:CheckedTaggedToTaggedPointer` - 检查 `getStr()` 返回值是否为指针
- `#43:CheckString` - 检查 `getStr()` 返回值是否为字符串
- `#49:StringConcat` - 字符串连接

#### 优化后（有 metadata）- 最终 Graph (process 函数)

```
----- Graph after V8.TFEarlyOptimization ----- 
#54:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#42:StringLength(#2:Parameter)
#33:HeapConstant[0x2aa4000671d1 <JSFunction getStr (sfi = 0x2aa4000670ed)>]()
#38:HeapConstant[0x2aa40004b291 <JSGlobalProxy>]()
#3:HeapConstant[0x2aa400000011 <undefined>]()
#53:Int32Constant[1]()
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, sparse:^^](#1:Parameter, #2:Parameter)
#10:TypedStateValues[, sparse:..]()
#52:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#32:HeapConstant[0x2aa400067205 <JSFunction process (sfi = 0x2aa40006711d)>]()
#15:FrameState[UNOPTIMIZED_FRAME, 0, Ignore, 0x2aa40006711d <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #52:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#59:ExternalConstant[0x560620209020]()
#50:Int64Constant[0]()
#60:Load[kRepWord64](#59:ExternalConstant, #50:Int64Constant, #0:Start, #0:Start)
#61:StackPointerGreaterThan[JSFunctionEntry](#60:Load, #60:Load)
#69:HeapConstant[0x2aa4001ce315 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#67:LoadStackCheckOffset()
#68:ExternalConstant[0x720a3f5cd520 <StackGuardWithGap.entry>]()
#6:HeapConstant[0x2aa40004b2f1 <NativeContext[304]>]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, 0x2aa40006711d <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #52:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#62:Branch[Unspecified, True](#61:StackPointerGreaterThan, #0:Start)
#64:IfFalse(#62:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#69:HeapConstant, #67:LoadStackCheckOffset, #68:ExternalConstant, #53:Int32Constant, #6:HeapConstant, #13:FrameState, #61:StackPointerGreaterThan, #64:IfFalse)
#63:IfTrue(#62:Branch)
#65:Merge(#63:IfTrue, #8:Call)
#66:EffectPhi(#61:StackPointerGreaterThan, #8:Call, #65:Merge)
#14:Checkpoint(#15:FrameState, #66:EffectPhi, #65:Merge)
#40:LoadField[JSFunctionContext, tagged base, 20, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#33:HeapConstant, #14:Checkpoint, #65:Merge)
#22:FrameState[UNOPTIMIZED_FRAME, 4, PokeAt(0), 0x2aa40006711d <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #52:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#21:Call[JS:js-call:r1s1i6f1](#33:HeapConstant, #38:HeapConstant, #3:HeapConstant, #53:Int32Constant, #54:Int32Constant, #40:LoadField, #22:FrameState, #40:LoadField, #65:Merge)
#43:StringLength(#21:Call)
#44:Int32Add(#42:StringLength, #43:StringLength)
#57:Int32Constant[536870889]()
#56:TypedStateValues[kRepTagged|kTypeAny, dense](#21:Call)
#24:FrameState[UNOPTIMIZED_FRAME, 10, Ignore, 0x2aa40006711d <SharedFunctionInfo process>](#9:TypedStateValues, #10:TypedStateValues, #56:TypedStateValues, #4:Parameter, #32:HeapConstant, #0:Start)
#23:Checkpoint(#24:FrameState, #21:Call, #21:Call)
#46:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#44:Int32Add, #57:Int32Constant, #23:Checkpoint, #21:Call)
#58:ChangeInt31ToTaggedSigned(#46:CheckedUint32Bounds)
#47:StringConcat(#58:ChangeInt31ToTaggedSigned, #2:Parameter, #21:Call)
#28:Return(#54:Int32Constant, #47:StringConcat, #46:CheckedUint32Bounds, #21:Call)
#29:End(#28:Return)
```

**关键节点**:
- `#21:Call[JS:js-call]` - 调用 `getStr()` 函数
- `#2:Parameter[1]` - 参数 `data`，类型已知为 `String`
- `#42:StringLength` - 直接访问 `data` 的长度（无需检查）
- `#43:StringLength` - 直接访问 `getStr()` 返回值的长度（无需检查）
- `#47:StringConcat` - 字符串连接

**优化效果**:
- ✅ **移除了 `#58:CheckedTaggedToTaggedPointer`** - 参数 `data` 类型已知为字符串
- ✅ **移除了 `#42:CheckString`** - 参数 `data` 类型已知，无需运行时检查
- ✅ **移除了 `#59:CheckedTaggedToTaggedPointer`** - `getStr()` 返回值类型已知为字符串
- ✅ **移除了 `#43:CheckString`** - `getStr()` 返回值类型已知，无需运行时检查
- ✅ **Graph 简化** - 从 50 行减少到 46 行

**说明**:
- 本测试成功验证了函数返回值类型注入功能
- 通过禁止内联（`--max_inlined_bytecode_size=0`），`getStr()` 保留为 `JSCall` 节点
- `TypeInjector` 成功将 `getStr()` 的返回值类型注入为 `String`
- 后续的 `CheckString` 和 `CheckedTaggedToTaggedPointer` 检查被完全消除
- 这为实现内建函数类型表（如 `toString()` 返回 `string`）奠定了基础

