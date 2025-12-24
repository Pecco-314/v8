### 10. 函数调用返回值类型

#### 测试代码

```javascript
function getStr() {
    return "hello";
}

function process(data) {
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

