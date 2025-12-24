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


