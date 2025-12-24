### 1. String 类型

#### 测试代码

```javascript
function twice_s(arg) { // arg: str
    return arg + arg;
}
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

