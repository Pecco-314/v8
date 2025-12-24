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

