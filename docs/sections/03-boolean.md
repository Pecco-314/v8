### 3. Boolean 类型

#### 测试代码

```javascript
function cal(a, b) { // a: bool
    if (a) {
        return a;
    } else {
        return b;
    }
}
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


