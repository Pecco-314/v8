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


