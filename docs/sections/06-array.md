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


