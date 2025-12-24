### 5. Interface 类型（嵌套）

#### 测试代码

```javascript
function concat_nested(data) { // data: {first: {x: str, y: str}}
    return data.first.x + data.first.y;
}
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


