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


