### 9. Tuple Length 优化

#### 测试代码

```javascript
function getLength(tuple) { // tuple: tuple[str, str]
    return tuple.length;
}
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

