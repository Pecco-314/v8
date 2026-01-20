### 7. RawInt32 类型优化

RawInt32 类型的无溢出检查加法、减法、乘法和取模运算（移除溢出检查）

#### 函数: `addRawInt32`

**测试代码：**

```javascript
function addRawInt32(a, b) {
    return a + b;
}
```

**Metadata：**

```
530 @params any rawint32 rawint32 @ret rawint32  # addRawInt32
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Add` | 1 | 0 |
| `Int32Add` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction addRawInt32 (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo addRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo addRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Add(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Add)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Add, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Add(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Add)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction addRawInt32 (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo addRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `addRawInt32Overflow`

**测试代码：**

```javascript
function addRawInt32Overflow(a, b) {
    return a + b;
}
```

**Metadata：**

```
729 @params any rawint32 rawint32 @ret rawint32  # addRawInt32Overflow
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Add` | 1 | 0 |
| `Int32Add` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction addRawInt32Overflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Add(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Add)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Add, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Add(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Add)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction addRawInt32Overflow (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo addRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `addRawInt32Negative`

**测试代码：**

```javascript
function addRawInt32Negative(a, b) {
    return a + b;
}
```

**Metadata：**

```
1646 @params any rawint32 rawint32 @ret rawint32  # addRawInt32Negative
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Add` | 1 | 0 |
| `Int32Add` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction addRawInt32Negative (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Add(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Add)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Add, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Add(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Add)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction addRawInt32Negative (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo addRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `addRawInt32Underflow`

**测试代码：**

```javascript
function addRawInt32Underflow(a, b) {
    return a + b;
}
```

**Metadata：**

```
1875 @params any rawint32 rawint32 @ret rawint32  # addRawInt32Underflow
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Add` | 1 | 0 |
| `Int32Add` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction addRawInt32Underflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Add(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Add)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Add, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Add(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Add)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction addRawInt32Underflow (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo addRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `addTenRawInt32`

**测试代码：**

```javascript
function addTenRawInt32(a, b, c, d, e, f, g, h, i, j) {
    return a + b + c + d + e + f + g + h + i + j;
}
```

**Metadata：**

```
954 @params any rawint32 rawint32 rawint32 rawint32 rawint32 rawint32 rawint32 rawint32 rawint32 rawint32 @ret rawint32  # addTenRawInt32
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Add` | 9 | 0 |
| `Int32Add` | 0 | 9 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#59:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#4:Parameter[3](#0:Start)
#5:Parameter[4](#0:Start)
#6:Parameter[5](#0:Start)
#7:Parameter[6](#0:Start)
#8:Parameter[7](#0:Start)
#9:Parameter[8](#0:Start)
#10:Parameter[9](#0:Start)
#11:Parameter[10](#0:Start)
#18:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter, #4:Parameter, #5:Parameter, #6:Parameter, #7:Parameter, #8:Parameter, #9:Parameter, #10:Parameter, #11:Parameter)
#19:TypedStateValues[, sparse:.]()
#47:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#13:Parameter[14, debug name: %context](#0:Start)
#39:HeapConstant[ADDR1 <JSFunction addTenRawInt32 (sfi = ADDR2)>]()
#24:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo addTenRawInt32>](#18:TypedStateValues, #19:TypedStateValues, #47:TypedStateValues, #13:Parameter, #39:HeapConstant, #0:Start)
#61:ExternalConstant[ADDR3]()
#46:Int64Constant[0]()
#62:Load[kRepWord64](#61:ExternalConstant, #46:Int64Constant, #0:Start, #0:Start)
#63:StackPointerGreaterThan[JSFunctionEntry](#62:Load, #62:Load)
#72:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#69:LoadStackCheckOffset()
#70:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#71:Int32Constant[1]()
#15:HeapConstant[ADDR6 <NativeContext[304]>]()
#48:TypedStateValues[, sparse:.]()
#22:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo addTenRawInt32>](#18:TypedStateValues, #19:TypedStateValues, #48:TypedStateValues, #13:Parameter, #39:HeapConstant, #0:Start)
#64:Branch[Unspecified, True](#63:StackPointerGreaterThan, #0:Start)
#66:IfFalse(#64:Branch)
#17:Call[Code:StackGuardWithGap:r1s1i5f1](#72:HeapConstant, #69:LoadStackCheckOffset, #70:ExternalConstant, #71:Int32Constant, #15:HeapConstant, #22:FrameState, #63:StackPointerGreaterThan, #66:IfFalse)
#65:IfTrue(#64:Branch)
#67:Merge(#65:IfTrue, #17:Call)
#68:EffectPhi(#63:StackPointerGreaterThan, #17:Call, #67:Merge)
#23:Checkpoint(#24:FrameState, #68:EffectPhi, #67:Merge)
#49:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #23:Checkpoint, #67:Merge)
#50:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #49:CheckedTaggedSignedToInt32, #67:Merge)
#25:CheckedInt32Add(#49:CheckedTaggedSignedToInt32, #50:CheckedTaggedSignedToInt32, #50:CheckedTaggedSignedToInt32, #67:Merge)
#51:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#4:Parameter, #25:CheckedInt32Add, #67:Merge)
#26:CheckedInt32Add(#25:CheckedInt32Add, #51:CheckedTaggedSignedToInt32, #51:CheckedTaggedSignedToInt32, #67:Merge)
#52:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#5:Parameter, #26:CheckedInt32Add, #67:Merge)
#27:CheckedInt32Add(#26:CheckedInt32Add, #52:CheckedTaggedSignedToInt32, #52:CheckedTaggedSignedToInt32, #67:Merge)
#53:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#6:Parameter, #27:CheckedInt32Add, #67:Merge)
#28:CheckedInt32Add(#27:CheckedInt32Add, #53:CheckedTaggedSignedToInt32, #53:CheckedTaggedSignedToInt32, #67:Merge)
#54:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#7:Parameter, #28:CheckedInt32Add, #67:Merge)
#29:CheckedInt32Add(#28:CheckedInt32Add, #54:CheckedTaggedSignedToInt32, #54:CheckedTaggedSignedToInt32, #67:Merge)
#55:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#8:Parameter, #29:CheckedInt32Add, #67:Merge)
#30:CheckedInt32Add(#29:CheckedInt32Add, #55:CheckedTaggedSignedToInt32, #55:CheckedTaggedSignedToInt32, #67:Merge)
#56:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#9:Parameter, #30:CheckedInt32Add, #67:Merge)
#31:CheckedInt32Add(#30:CheckedInt32Add, #56:CheckedTaggedSignedToInt32, #56:CheckedTaggedSignedToInt32, #67:Merge)
#57:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#10:Parameter, #31:CheckedInt32Add, #67:Merge)
#32:CheckedInt32Add(#31:CheckedInt32Add, #57:CheckedTaggedSignedToInt32, #57:CheckedTaggedSignedToInt32, #67:Merge)
#58:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#11:Parameter, #32:CheckedInt32Add, #67:Merge)
#33:CheckedInt32Add(#32:CheckedInt32Add, #58:CheckedTaggedSignedToInt32, #58:CheckedTaggedSignedToInt32, #67:Merge)
#60:ChangeInt32ToTagged(#33:CheckedInt32Add)
#35:Return(#59:Int32Constant, #60:ChangeInt32ToTagged, #33:CheckedInt32Add, #67:Merge)
#36:End(#35:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#67:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#56:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#57:ChangeTaggedToInt32(#3:Parameter)
#42:Int32Add(#56:ChangeTaggedToInt32, #57:ChangeTaggedToInt32)
#4:Parameter[3](#0:Start)
#58:ChangeTaggedToInt32(#4:Parameter)
#43:Int32Add(#42:Int32Add, #58:ChangeTaggedToInt32)
#5:Parameter[4](#0:Start)
#59:ChangeTaggedToInt32(#5:Parameter)
#44:Int32Add(#43:Int32Add, #59:ChangeTaggedToInt32)
#6:Parameter[5](#0:Start)
#60:ChangeTaggedToInt32(#6:Parameter)
#45:Int32Add(#44:Int32Add, #60:ChangeTaggedToInt32)
#7:Parameter[6](#0:Start)
#61:ChangeTaggedToInt32(#7:Parameter)
#46:Int32Add(#45:Int32Add, #61:ChangeTaggedToInt32)
#8:Parameter[7](#0:Start)
#62:ChangeTaggedToInt32(#8:Parameter)
#47:Int32Add(#46:Int32Add, #62:ChangeTaggedToInt32)
#9:Parameter[8](#0:Start)
#63:ChangeTaggedToInt32(#9:Parameter)
#48:Int32Add(#47:Int32Add, #63:ChangeTaggedToInt32)
#10:Parameter[9](#0:Start)
#64:ChangeTaggedToInt32(#10:Parameter)
#49:Int32Add(#48:Int32Add, #64:ChangeTaggedToInt32)
#11:Parameter[10](#0:Start)
#65:ChangeTaggedToInt32(#11:Parameter)
#50:Int32Add(#49:Int32Add, #65:ChangeTaggedToInt32)
#68:ChangeInt32ToTagged(#50:Int32Add)
#69:ExternalConstant[ADDR1]()
#55:Int64Constant[0]()
#70:Load[kRepWord64](#69:ExternalConstant, #55:Int64Constant, #0:Start, #0:Start)
#71:StackPointerGreaterThan[JSFunctionEntry](#70:Load, #70:Load)
#80:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#77:LoadStackCheckOffset()
#78:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#79:Int32Constant[1]()
#15:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#18:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter, #4:Parameter, #5:Parameter, #6:Parameter, #7:Parameter, #8:Parameter, #9:Parameter, #10:Parameter, #11:Parameter)
#19:TypedStateValues[, sparse:.]()
#66:TypedStateValues[, sparse:.]()
#13:Parameter[14, debug name: %context](#0:Start)
#39:HeapConstant[ADDR5 <JSFunction addTenRawInt32 (sfi = ADDR6)>]()
#22:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo addTenRawInt32>](#18:TypedStateValues, #19:TypedStateValues, #66:TypedStateValues, #13:Parameter, #39:HeapConstant, #0:Start)
#72:Branch[Unspecified, True](#71:StackPointerGreaterThan, #0:Start)
#74:IfFalse(#72:Branch)
#17:Call[Code:StackGuardWithGap:r1s1i5f1](#80:HeapConstant, #77:LoadStackCheckOffset, #78:ExternalConstant, #79:Int32Constant, #15:HeapConstant, #22:FrameState, #71:StackPointerGreaterThan, #74:IfFalse)
#73:IfTrue(#72:Branch)
#75:Merge(#73:IfTrue, #17:Call)
#76:EffectPhi(#71:StackPointerGreaterThan, #17:Call, #75:Merge)
#35:Return(#67:Int32Constant, #68:ChangeInt32ToTagged, #76:EffectPhi, #75:Merge)
#36:End(#35:Return)
```

#### 函数: `sumLoopRawInt32`

**测试代码：**

```javascript
function sumLoopRawInt32(arr) {
    let acc = 0;
    for (let i = 0; i < arr.length; i++) {
        acc += arr[i];
    }
```

**Metadata：**

```
1301 @params any arr<rawint32> @ret rawint32  # sumLoopRawInt32
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Add` | 1 | 0 |
| `Int32Add` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#98:Int32Constant[1]()
#115:ExternalConstant[ADDR1]()
#89:Int64Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#83:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, sparse:^^.](#89:Int64Constant, #89:Int64Constant)
#91:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#52:HeapConstant[ADDR2 <JSFunction sumLoopRawInt32 (sfi = ADDR3)>]()
#82:FrameState[UNOPTIMIZED_FRAME, 4, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #83:TypedStateValues, #91:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#104:ExternalConstant[ADDR4]()
#105:Load[kRepWord64](#104:ExternalConstant, #89:Int64Constant, #0:Start, #0:Start)
#106:StackPointerGreaterThan[JSFunctionEntry](#105:Load, #105:Load)
#114:HeapConstant[ADDR5 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#112:LoadStackCheckOffset()
#113:ExternalConstant[ADDR6 <StackGuardWithGap.entry>]()
#6:HeapConstant[ADDR7 <NativeContext[304]>]()
#10:TypedStateValues[, sparse:...]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #10:TypedStateValues, #91:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#107:Branch[Unspecified, True](#106:StackPointerGreaterThan, #0:Start)
#109:IfFalse(#107:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#114:HeapConstant, #112:LoadStackCheckOffset, #113:ExternalConstant, #98:Int32Constant, #6:HeapConstant, #13:FrameState, #106:StackPointerGreaterThan, #109:IfFalse)
#108:IfTrue(#107:Branch)
#110:Merge(#108:IfTrue, #8:Call)
#111:EffectPhi(#106:StackPointerGreaterThan, #8:Call, #110:Merge)
#84:Checkpoint(#82:FrameState, #111:EffectPhi, #110:Merge)
#92:CheckedTaggedToTaggedPointer[FeedbackSource(INVALID)](#2:Parameter, #84:Checkpoint, #110:Merge)
#73:CheckMaps[None, ADDR8, FeedbackSource(INVALID)](#92:CheckedTaggedToTaggedPointer, #92:CheckedTaggedToTaggedPointer, #110:Merge)
#72:LoadField[BuildLoadDataField, tagged base, 12, ADDR9: [String] in ReadOnlySpace: #length, Range(0, 67108864), kRepTaggedSigned|kTypeInt32, FullWriteBarrier, mutable](#2:Parameter, #73:CheckMaps, #110:Merge)
#93:Int32Constant[0]()
#94:ChangeTaggedSignedToInt32(#72:LoadField)
#65:Uint32LessThan(#93:Int32Constant, #94:ChangeTaggedSignedToInt32)
#81:Branch[Machine, None](#65:Uint32LessThan, #110:Merge)
#79:IfTrue(#81:Branch)
#70:LoadField[JSObjectElements, tagged base, 8, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#2:Parameter, #72:LoadField, #79:IfTrue)
#68:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#93:Int32Constant, #94:ChangeTaggedSignedToInt32, #70:LoadField, #79:IfTrue)
#96:ChangeUint32ToUint64(#68:CheckedUint32Bounds)
#67:LoadElement[tagged base, 8, Signed31, kRepTaggedSigned|kTypeInt32, FullWriteBarrier](#70:LoadField, #96:ChangeUint32ToUint64, #68:CheckedUint32Bounds, #79:IfTrue)
#116:Load[kRepWord8|kTypeUint32](#115:ExternalConstant, #89:Int64Constant, #67:LoadElement, #79:IfTrue)
#123:ExternalConstant[ADDR10 <HandleNoHeapWritesInterrupts.entry>]()
#97:ChangeTaggedSignedToInt32(#67:LoadElement)
#75:TypedStateValues[kRepWord32|kTypeInt32, kRepWord32|kTypeInt32, sparse:^^.](#97:ChangeTaggedSignedToInt32, #98:Int32Constant)
#74:FrameState[UNOPTIMIZED_FRAME, 30, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #75:TypedStateValues, #91:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#118:Branch[Unspecified, False](#116:Load, #79:IfTrue)
#120:IfTrue(#118:Branch)
#76:Call[Code:HandleNoHeapWritesInterrupts:r1s0i4f1](#114:HeapConstant, #123:ExternalConstant, #93:Int32Constant, #6:HeapConstant, #74:FrameState, #116:Load, #120:IfTrue)
#119:IfFalse(#118:Branch)
#121:Merge(#119:IfFalse, #76:Call)
#122:EffectPhi(#116:Load, #76:Call, #121:Merge)
#66:TypeGuard[Range(0, 67108864)](#98:Int32Constant, #122:EffectPhi, #121:Merge)
#18:Phi[kRepWord32](#97:ChangeTaggedSignedToInt32, #42:CheckedInt32Add, #15:Loop)
#100:ChangeUint32ToUint64(#58:CheckedUint32Bounds)
#63:Uint32LessThan(#19:Phi, #94:ChangeTaggedSignedToInt32)
#29:Branch[Machine, None](#63:Uint32LessThan, #15:Loop)
#36:IfTrue(#29:Branch)
#59:LoadElement[tagged base, 8, Signed31, kRepTaggedSigned|kTypeInt32, FullWriteBarrier](#70:LoadField, #100:ChangeUint32ToUint64, #58:CheckedUint32Bounds, #36:IfTrue)
#101:ChangeTaggedSignedToInt32(#59:LoadElement)
#42:CheckedInt32Add(#18:Phi, #101:ChangeTaggedSignedToInt32, #59:LoadElement, #36:IfTrue)
#124:Load[kRepWord8|kTypeUint32](#115:ExternalConstant, #89:Int64Constant, #42:CheckedInt32Add, #36:IfTrue)
#126:Branch[Unspecified, False](#124:Load, #36:IfTrue)
#127:IfFalse(#126:Branch)
#46:TypedStateValues[kRepWord32|kTypeInt32, kRepWord32|kTypeInt32, sparse:^^.](#42:CheckedInt32Add, #44:Int32Add)
#47:FrameState[UNOPTIMIZED_FRAME, 30, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #46:TypedStateValues, #91:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#128:IfTrue(#126:Branch)
#45:Call[Code:HandleNoHeapWritesInterrupts:r1s0i4f1](#114:HeapConstant, #123:ExternalConstant, #93:Int32Constant, #6:HeapConstant, #47:FrameState, #124:Load, #128:IfTrue)
#129:Merge(#127:IfFalse, #45:Call)
#15:Loop(#121:Merge, #129:Merge)
#19:Phi[kRepWord32](#66:TypeGuard, #60:TypeGuard, #15:Loop)
#22:TypedStateValues[kRepWord32|kTypeInt32, kRepWord32|kTypeInt32, sparse:^^.](#18:Phi, #19:Phi)
#23:FrameState[UNOPTIMIZED_FRAME, 4, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #22:TypedStateValues, #91:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#21:Checkpoint(#23:FrameState, #16:EffectPhi, #15:Loop)
#58:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#19:Phi, #94:ChangeTaggedSignedToInt32, #21:Checkpoint, #36:IfTrue)
#44:Int32Add(#58:CheckedUint32Bounds, #98:Int32Constant)
#130:EffectPhi(#124:Load, #45:Call, #129:Merge)
#60:TypeGuard[Range(0, 67108864)](#44:Int32Add, #130:EffectPhi, #129:Merge)
#16:EffectPhi(#66:TypeGuard, #60:TypeGuard, #15:Loop)
#20:Terminate(#16:EffectPhi, #15:Loop)
#103:ChangeInt32ToTagged(#18:Phi)
#30:IfFalse(#29:Branch)
#87:Return(#93:Int32Constant, #103:ChangeInt32ToTagged, #16:EffectPhi, #30:IfFalse)
#80:IfFalse(#81:Branch)
#88:Return(#93:Int32Constant, #89:Int64Constant, #72:LoadField, #80:IfFalse)
#49:End(#20:Terminate, #87:Return, #88:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#98:Int32Constant[1]()
#115:ExternalConstant[ADDR1]()
#91:Int64Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#9:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter)
#84:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, sparse:^^.](#91:Int64Constant, #91:Int64Constant)
#92:TypedStateValues[, sparse:.]()
#4:Parameter[5, debug name: %context](#0:Start)
#52:HeapConstant[ADDR2 <JSFunction sumLoopRawInt32 (sfi = ADDR3)>]()
#83:FrameState[UNOPTIMIZED_FRAME, 4, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #84:TypedStateValues, #92:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#104:ExternalConstant[ADDR4]()
#105:Load[kRepWord64](#104:ExternalConstant, #91:Int64Constant, #0:Start, #0:Start)
#106:StackPointerGreaterThan[JSFunctionEntry](#105:Load, #105:Load)
#114:HeapConstant[ADDR5 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#112:LoadStackCheckOffset()
#113:ExternalConstant[ADDR6 <StackGuardWithGap.entry>]()
#6:HeapConstant[ADDR7 <NativeContext[304]>]()
#10:TypedStateValues[, sparse:...]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #10:TypedStateValues, #92:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#107:Branch[Unspecified, True](#106:StackPointerGreaterThan, #0:Start)
#109:IfFalse(#107:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#114:HeapConstant, #112:LoadStackCheckOffset, #113:ExternalConstant, #98:Int32Constant, #6:HeapConstant, #13:FrameState, #106:StackPointerGreaterThan, #109:IfFalse)
#108:IfTrue(#107:Branch)
#110:Merge(#108:IfTrue, #8:Call)
#111:EffectPhi(#106:StackPointerGreaterThan, #8:Call, #110:Merge)
#85:Checkpoint(#83:FrameState, #111:EffectPhi, #110:Merge)
#75:CheckMaps[None, ADDR8, FeedbackSource(INVALID)](#2:Parameter, #85:Checkpoint, #110:Merge)
#74:LoadField[BuildLoadDataField, tagged base, 12, ADDR9: [String] in ReadOnlySpace: #length, Range(0, 67108864), kRepTaggedSigned|kTypeInt32, FullWriteBarrier, mutable](#2:Parameter, #75:CheckMaps, #110:Merge)
#93:Int32Constant[0]()
#94:ChangeTaggedSignedToInt32(#74:LoadField)
#66:Uint32LessThan(#93:Int32Constant, #94:ChangeTaggedSignedToInt32)
#82:Branch[Machine, None](#66:Uint32LessThan, #110:Merge)
#80:IfTrue(#82:Branch)
#72:LoadField[JSObjectElements, tagged base, 8, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#2:Parameter, #74:LoadField, #80:IfTrue)
#70:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#93:Int32Constant, #94:ChangeTaggedSignedToInt32, #72:LoadField, #80:IfTrue)
#96:ChangeUint32ToUint64(#70:CheckedUint32Bounds)
#69:LoadElement[tagged base, 8, Signed31, kRepTaggedSigned|kTypeInt32, FullWriteBarrier](#72:LoadField, #96:ChangeUint32ToUint64, #70:CheckedUint32Bounds, #80:IfTrue)
#116:Load[kRepWord8|kTypeUint32](#115:ExternalConstant, #91:Int64Constant, #69:LoadElement, #80:IfTrue)
#123:ExternalConstant[ADDR10 <HandleNoHeapWritesInterrupts.entry>]()
#97:ChangeTaggedSignedToInt32(#69:LoadElement)
#90:Int64Constant[2]()
#77:TypedStateValues[kRepWord32|kTypeInt32, kRepTagged|kTypeAny, sparse:^^.](#97:ChangeTaggedSignedToInt32, #90:Int64Constant)
#76:FrameState[UNOPTIMIZED_FRAME, 30, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #77:TypedStateValues, #92:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#118:Branch[Unspecified, False](#116:Load, #80:IfTrue)
#120:IfTrue(#118:Branch)
#78:Call[Code:HandleNoHeapWritesInterrupts:r1s0i4f1](#114:HeapConstant, #123:ExternalConstant, #93:Int32Constant, #6:HeapConstant, #76:FrameState, #116:Load, #120:IfTrue)
#119:IfFalse(#118:Branch)
#121:Merge(#119:IfFalse, #78:Call)
#122:EffectPhi(#116:Load, #78:Call, #121:Merge)
#68:TypeGuard[Range(0, 67108864)](#98:Int32Constant, #122:EffectPhi, #121:Merge)
#18:Phi[kRepWord32](#97:ChangeTaggedSignedToInt32, #42:CheckedInt32Add, #15:Loop)
#22:TypedStateValues[kRepWord32|kTypeInt32, kRepWord32|kTypeInt32, sparse:^^.](#18:Phi, #19:Phi)
#23:FrameState[UNOPTIMIZED_FRAME, 4, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #22:TypedStateValues, #92:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#21:Checkpoint(#23:FrameState, #16:EffectPhi, #15:Loop)
#64:Uint32LessThan(#19:Phi, #94:ChangeTaggedSignedToInt32)
#29:Branch[Machine, None](#64:Uint32LessThan, #15:Loop)
#36:IfTrue(#29:Branch)
#58:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#19:Phi, #94:ChangeTaggedSignedToInt32, #21:Checkpoint, #36:IfTrue)
#101:ChangeUint32ToUint64(#58:CheckedUint32Bounds)
#59:LoadElement[tagged base, 8, Signed31, kRepTaggedSigned|kTypeInt32, FullWriteBarrier](#72:LoadField, #101:ChangeUint32ToUint64, #58:CheckedUint32Bounds, #36:IfTrue)
#102:ChangeTaggedToInt32(#59:LoadElement)
#42:CheckedInt32Add(#18:Phi, #102:ChangeTaggedToInt32, #59:LoadElement, #36:IfTrue)
#124:Load[kRepWord8|kTypeUint32](#115:ExternalConstant, #91:Int64Constant, #42:CheckedInt32Add, #36:IfTrue)
#126:Branch[Unspecified, False](#124:Load, #36:IfTrue)
#127:IfFalse(#126:Branch)
#46:TypedStateValues[kRepWord32|kTypeInt32, kRepWord32|kTypeInt32, sparse:^^.](#42:CheckedInt32Add, #61:Int32Add)
#47:FrameState[UNOPTIMIZED_FRAME, 30, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #46:TypedStateValues, #92:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#128:IfTrue(#126:Branch)
#45:Call[Code:HandleNoHeapWritesInterrupts:r1s0i4f1](#114:HeapConstant, #123:ExternalConstant, #93:Int32Constant, #6:HeapConstant, #47:FrameState, #124:Load, #128:IfTrue)
#129:Merge(#127:IfFalse, #45:Call)
#15:Loop(#121:Merge, #129:Merge)
#19:Phi[kRepWord32](#68:TypeGuard, #60:TypeGuard, #15:Loop)
#61:Int32Add(#19:Phi, #98:Int32Constant)
#130:EffectPhi(#124:Load, #45:Call, #129:Merge)
#60:TypeGuard[Range(0, 67108864)](#61:Int32Add, #130:EffectPhi, #129:Merge)
#16:EffectPhi(#68:TypeGuard, #60:TypeGuard, #15:Loop)
#20:Terminate(#16:EffectPhi, #15:Loop)
#103:ChangeInt32ToTagged(#18:Phi)
#30:IfFalse(#29:Branch)
#88:Return(#93:Int32Constant, #103:ChangeInt32ToTagged, #16:EffectPhi, #30:IfFalse)
#81:IfFalse(#82:Branch)
#89:Return(#93:Int32Constant, #91:Int64Constant, #74:LoadField, #81:IfFalse)
#49:End(#20:Terminate, #88:Return, #89:Return)
```

#### 函数: `subRawInt32`

**测试代码：**

```javascript
function subRawInt32(a, b) {
    return a - b;
}
```

**Metadata：**

```
2157 @params any rawint32 rawint32 @ret rawint32  # subRawInt32
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Sub` | 1 | 0 |
| `Int32Sub` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction subRawInt32 (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo subRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo subRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Sub(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Sub)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Sub, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Sub(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Sub)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction subRawInt32 (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo subRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `subRawInt32Overflow`

**测试代码：**

```javascript
function subRawInt32Overflow(a, b) {
    return a - b;
}
```

**Metadata：**

```
2356 @params any rawint32 rawint32 @ret rawint32  # subRawInt32Overflow
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Sub` | 1 | 0 |
| `Int32Sub` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction subRawInt32Overflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Sub(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Sub)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Sub, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Sub(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Sub)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction subRawInt32Overflow (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo subRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `subRawInt32Negative`

**测试代码：**

```javascript
function subRawInt32Negative(a, b) {
    return a - b;
}
```

**Metadata：**

```
2558 @params any rawint32 rawint32 @ret rawint32  # subRawInt32Negative
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Sub` | 1 | 0 |
| `Int32Sub` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction subRawInt32Negative (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Sub(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Sub)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Sub, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Sub(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Sub)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction subRawInt32Negative (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo subRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `subRawInt32Underflow`

**测试代码：**

```javascript
function subRawInt32Underflow(a, b) {
    return a - b;
}
```

**Metadata：**

```
2786 @params any rawint32 rawint32 @ret rawint32  # subRawInt32Underflow
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Sub` | 1 | 0 |
| `Int32Sub` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction subRawInt32Underflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Sub(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Sub)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Sub, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Sub(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Sub)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction subRawInt32Underflow (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo subRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `mulRawInt32`

**测试代码：**

```javascript
function mulRawInt32(a, b) {
    return a * b;
}
```

**Metadata：**

```
3069 @params any rawint32 rawint32 @ret rawint32  # mulRawInt32
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Mul` | 1 | 0 |
| `Int32Mul` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction mulRawInt32 (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Mul[check-for-minus-zero](#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Mul)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Mul, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Mul(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Mul)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction mulRawInt32 (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo mulRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `mulRawInt32Overflow`

**测试代码：**

```javascript
function mulRawInt32Overflow(a, b) {
    return a * b;
}
```

**Metadata：**

```
3262 @params any rawint32 rawint32 @ret rawint32  # mulRawInt32Overflow
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Mul` | 1 | 0 |
| `Int32Mul` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction mulRawInt32Overflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Mul[check-for-minus-zero](#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Mul)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Mul, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Mul(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Mul)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction mulRawInt32Overflow (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo mulRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `mulRawInt32Negative`

**测试代码：**

```javascript
function mulRawInt32Negative(a, b) {
    return a * b;
}
```

**Metadata：**

```
3467 @params any rawint32 rawint32 @ret rawint32  # mulRawInt32Negative
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Mul` | 1 | 0 |
| `Int32Mul` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction mulRawInt32Negative (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Mul[check-for-minus-zero](#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Mul)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Mul, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Mul(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Mul)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction mulRawInt32Negative (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo mulRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `mulRawInt32Underflow`

**测试代码：**

```javascript
function mulRawInt32Underflow(a, b) {
    return a * b;
}
```

**Metadata：**

```
3686 @params any rawint32 rawint32 @ret rawint32  # mulRawInt32Underflow
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Mul` | 1 | 0 |
| `Int32Mul` | 0 | 1 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction mulRawInt32Underflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Mul[check-for-minus-zero](#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Mul)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Mul, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#26:Int32Mul(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32)
#36:ChangeInt32ToTagged(#26:Int32Mul)
#37:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#34:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction mulRawInt32Underflow (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo mulRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #34:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #44:EffectPhi, #43:Merge)
#20:End(#19:Return)
```

#### 函数: `divRawInt32Exact`

**测试代码：**

```javascript
function divRawInt32Exact(a, b) {
    return a / b;
}
```

**Metadata：**

```
3979 @params any rawint32 rawint32 @ret rawint32  # divRawInt32Exact
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Div` | 1 | 0 |
| `Int32Div` | 0 | 2 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawInt32Exact (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Exact>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Exact>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Div(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Div)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Div, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#34:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#36:Int32LessThan(#34:Int32Constant, #33:ChangeTaggedToInt32)
#37:Branch[Machine, True](#36:Int32LessThan, #0:Start)
#38:IfTrue(#37:Branch)
#39:Int32Div(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #38:IfTrue)
#35:Int32Constant[-1]()
#41:Int32LessThan(#33:ChangeTaggedToInt32, #35:Int32Constant)
#40:IfFalse(#37:Branch)
#42:Branch[Machine, None](#41:Int32LessThan, #40:IfFalse)
#43:IfTrue(#42:Branch)
#44:Int32Div(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #43:IfTrue)
#50:Int32Sub(#34:Int32Constant, #32:ChangeTaggedToInt32)
#45:IfFalse(#42:Branch)
#47:Branch[Unspecified, None](#33:ChangeTaggedToInt32, #45:IfFalse)
#48:IfFalse(#47:Branch)
#49:IfTrue(#47:Branch)
#51:Merge(#48:IfFalse, #49:IfTrue)
#52:Phi[kRepWord32](#34:Int32Constant, #50:Int32Sub, #51:Merge)
#53:Merge(#43:IfTrue, #51:Merge)
#54:Phi[kRepWord32](#44:Int32Div, #52:Phi, #53:Merge)
#55:Merge(#38:IfTrue, #53:Merge)
#56:Phi[kRepWord32](#39:Int32Div, #54:Phi, #55:Merge)
#58:ChangeInt32ToTagged(#56:Phi)
#59:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#60:Load[kRepWord64](#59:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#61:StackPointerGreaterThan[JSFunctionEntry](#60:Load, #60:Load)
#70:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#67:LoadStackCheckOffset()
#68:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#69:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#57:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction divRawInt32Exact (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo divRawInt32Exact>](#10:TypedStateValues, #11:TypedStateValues, #57:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#62:Branch[Unspecified, True](#61:StackPointerGreaterThan, #0:Start)
#64:IfFalse(#62:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#70:HeapConstant, #67:LoadStackCheckOffset, #68:ExternalConstant, #69:Int32Constant, #7:HeapConstant, #14:FrameState, #61:StackPointerGreaterThan, #64:IfFalse)
#63:IfTrue(#62:Branch)
#65:Merge(#63:IfTrue, #9:Call)
#66:EffectPhi(#61:StackPointerGreaterThan, #9:Call, #65:Merge)
#19:Return(#34:Int32Constant, #58:ChangeInt32ToTagged, #66:EffectPhi, #65:Merge)
#20:End(#19:Return)
```

#### 函数: `divRawInt32Trunc`

**测试代码：**

```javascript
function divRawInt32Trunc(a, b) {
    return a / b;
}
```

**Metadata：**

```
4188 @params any rawint32 rawint32 @ret rawint32  # divRawInt32Trunc
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `Float64Div` | 1 | 0 |
| `Int32Div` | 0 | 2 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawInt32Trunc (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Trunc>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Trunc>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedToFloat64[NumberOrOddball, FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedToFloat64[NumberOrOddball, FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedToFloat64, #43:Merge)
#17:Float64Div(#33:CheckedTaggedToFloat64, #34:CheckedTaggedToFloat64)
#36:ChangeFloat64ToTagged[check-for-minus-zero](#17:Float64Div)
#19:Return(#35:Int32Constant, #36:ChangeFloat64ToTagged, #34:CheckedTaggedToFloat64, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#34:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#36:Int32LessThan(#34:Int32Constant, #33:ChangeTaggedToInt32)
#37:Branch[Machine, True](#36:Int32LessThan, #0:Start)
#38:IfTrue(#37:Branch)
#39:Int32Div(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #38:IfTrue)
#35:Int32Constant[-1]()
#41:Int32LessThan(#33:ChangeTaggedToInt32, #35:Int32Constant)
#40:IfFalse(#37:Branch)
#42:Branch[Machine, None](#41:Int32LessThan, #40:IfFalse)
#43:IfTrue(#42:Branch)
#44:Int32Div(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #43:IfTrue)
#50:Int32Sub(#34:Int32Constant, #32:ChangeTaggedToInt32)
#45:IfFalse(#42:Branch)
#47:Branch[Unspecified, None](#33:ChangeTaggedToInt32, #45:IfFalse)
#48:IfFalse(#47:Branch)
#49:IfTrue(#47:Branch)
#51:Merge(#48:IfFalse, #49:IfTrue)
#52:Phi[kRepWord32](#34:Int32Constant, #50:Int32Sub, #51:Merge)
#53:Merge(#43:IfTrue, #51:Merge)
#54:Phi[kRepWord32](#44:Int32Div, #52:Phi, #53:Merge)
#55:Merge(#38:IfTrue, #53:Merge)
#56:Phi[kRepWord32](#39:Int32Div, #54:Phi, #55:Merge)
#58:ChangeInt32ToTagged(#56:Phi)
#59:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#60:Load[kRepWord64](#59:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#61:StackPointerGreaterThan[JSFunctionEntry](#60:Load, #60:Load)
#70:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#67:LoadStackCheckOffset()
#68:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#69:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#57:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction divRawInt32Trunc (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo divRawInt32Trunc>](#10:TypedStateValues, #11:TypedStateValues, #57:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#62:Branch[Unspecified, True](#61:StackPointerGreaterThan, #0:Start)
#64:IfFalse(#62:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#70:HeapConstant, #67:LoadStackCheckOffset, #68:ExternalConstant, #69:Int32Constant, #7:HeapConstant, #14:FrameState, #61:StackPointerGreaterThan, #64:IfFalse)
#63:IfTrue(#62:Branch)
#65:Merge(#63:IfTrue, #9:Call)
#66:EffectPhi(#61:StackPointerGreaterThan, #9:Call, #65:Merge)
#19:Return(#34:Int32Constant, #58:ChangeInt32ToTagged, #66:EffectPhi, #65:Merge)
#20:End(#19:Return)
```

#### 函数: `divRawInt32Overflow`

**测试代码：**

```javascript
function divRawInt32Overflow(a, b) {
    return a / b;
}
```

**Metadata：**

```
4639 @params any rawint32 rawint32 @ret rawint32  # divRawInt32Overflow
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Div` | 1 | 0 |
| `Int32Div` | 0 | 2 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawInt32Overflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Div(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Div)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Div, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#34:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#36:Int32LessThan(#34:Int32Constant, #33:ChangeTaggedToInt32)
#37:Branch[Machine, True](#36:Int32LessThan, #0:Start)
#38:IfTrue(#37:Branch)
#39:Int32Div(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #38:IfTrue)
#35:Int32Constant[-1]()
#41:Int32LessThan(#33:ChangeTaggedToInt32, #35:Int32Constant)
#40:IfFalse(#37:Branch)
#42:Branch[Machine, None](#41:Int32LessThan, #40:IfFalse)
#43:IfTrue(#42:Branch)
#44:Int32Div(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #43:IfTrue)
#50:Int32Sub(#34:Int32Constant, #32:ChangeTaggedToInt32)
#45:IfFalse(#42:Branch)
#47:Branch[Unspecified, None](#33:ChangeTaggedToInt32, #45:IfFalse)
#48:IfFalse(#47:Branch)
#49:IfTrue(#47:Branch)
#51:Merge(#48:IfFalse, #49:IfTrue)
#52:Phi[kRepWord32](#34:Int32Constant, #50:Int32Sub, #51:Merge)
#53:Merge(#43:IfTrue, #51:Merge)
#54:Phi[kRepWord32](#44:Int32Div, #52:Phi, #53:Merge)
#55:Merge(#38:IfTrue, #53:Merge)
#56:Phi[kRepWord32](#39:Int32Div, #54:Phi, #55:Merge)
#58:ChangeInt32ToTagged(#56:Phi)
#59:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#60:Load[kRepWord64](#59:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#61:StackPointerGreaterThan[JSFunctionEntry](#60:Load, #60:Load)
#70:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#67:LoadStackCheckOffset()
#68:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#69:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#57:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction divRawInt32Overflow (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo divRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #57:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#62:Branch[Unspecified, True](#61:StackPointerGreaterThan, #0:Start)
#64:IfFalse(#62:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#70:HeapConstant, #67:LoadStackCheckOffset, #68:ExternalConstant, #69:Int32Constant, #7:HeapConstant, #14:FrameState, #61:StackPointerGreaterThan, #64:IfFalse)
#63:IfTrue(#62:Branch)
#65:Merge(#63:IfTrue, #9:Call)
#66:EffectPhi(#61:StackPointerGreaterThan, #9:Call, #65:Merge)
#19:Return(#34:Int32Constant, #58:ChangeInt32ToTagged, #66:EffectPhi, #65:Merge)
#20:End(#19:Return)
```

#### 函数: `divRawInt32ByZero`

**测试代码：**

```javascript
function divRawInt32ByZero(a, b) {
    return a / b;
}
```

**Metadata：**

```
5157 @params any rawint32 rawint32 @ret rawint32  # divRawInt32ByZero
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Div` | 1 | 0 |
| `Int32Div` | 0 | 2 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawInt32ByZero (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawInt32ByZero>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawInt32ByZero>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Div(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Div)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Div, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#34:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#36:Int32LessThan(#34:Int32Constant, #33:ChangeTaggedToInt32)
#37:Branch[Machine, True](#36:Int32LessThan, #0:Start)
#38:IfTrue(#37:Branch)
#39:Int32Div(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #38:IfTrue)
#35:Int32Constant[-1]()
#41:Int32LessThan(#33:ChangeTaggedToInt32, #35:Int32Constant)
#40:IfFalse(#37:Branch)
#42:Branch[Machine, None](#41:Int32LessThan, #40:IfFalse)
#43:IfTrue(#42:Branch)
#44:Int32Div(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #43:IfTrue)
#50:Int32Sub(#34:Int32Constant, #32:ChangeTaggedToInt32)
#45:IfFalse(#42:Branch)
#47:Branch[Unspecified, None](#33:ChangeTaggedToInt32, #45:IfFalse)
#48:IfFalse(#47:Branch)
#49:IfTrue(#47:Branch)
#51:Merge(#48:IfFalse, #49:IfTrue)
#52:Phi[kRepWord32](#34:Int32Constant, #50:Int32Sub, #51:Merge)
#53:Merge(#43:IfTrue, #51:Merge)
#54:Phi[kRepWord32](#44:Int32Div, #52:Phi, #53:Merge)
#55:Merge(#38:IfTrue, #53:Merge)
#56:Phi[kRepWord32](#39:Int32Div, #54:Phi, #55:Merge)
#58:ChangeInt32ToTagged(#56:Phi)
#59:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#60:Load[kRepWord64](#59:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#61:StackPointerGreaterThan[JSFunctionEntry](#60:Load, #60:Load)
#70:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#67:LoadStackCheckOffset()
#68:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#69:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#57:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction divRawInt32ByZero (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo divRawInt32ByZero>](#10:TypedStateValues, #11:TypedStateValues, #57:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#62:Branch[Unspecified, True](#61:StackPointerGreaterThan, #0:Start)
#64:IfFalse(#62:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#70:HeapConstant, #67:LoadStackCheckOffset, #68:ExternalConstant, #69:Int32Constant, #7:HeapConstant, #14:FrameState, #61:StackPointerGreaterThan, #64:IfFalse)
#63:IfTrue(#62:Branch)
#65:Merge(#63:IfTrue, #9:Call)
#66:EffectPhi(#61:StackPointerGreaterThan, #9:Call, #65:Merge)
#19:Return(#34:Int32Constant, #58:ChangeInt32ToTagged, #66:EffectPhi, #65:Merge)
#20:End(#19:Return)
```

#### 函数: `modRawInt32`

**测试代码：**

```javascript
function modRawInt32(a, b) {
    return a % b;
}
```

**Metadata：**

```
5864 @params any rawint32 rawint32 @ret rawint32  # modRawInt32
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Mod` | 1 | 0 |
| `Int32Mod` | 0 | 2 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction modRawInt32 (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo modRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo modRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Mod(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Mod)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Mod, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#34:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#35:Int32Constant[-1]()
#39:Int32Add(#33:ChangeTaggedToInt32, #35:Int32Constant)
#40:Word32And(#33:ChangeTaggedToInt32, #39:Int32Add)
#36:Int32LessThan(#34:Int32Constant, #33:ChangeTaggedToInt32)
#37:Branch[Machine, True](#36:Int32LessThan, #0:Start)
#38:IfTrue(#37:Branch)
#41:Branch[Machine, None](#40:Word32And, #38:IfTrue)
#42:IfTrue(#41:Branch)
#43:Int32Mod(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #42:IfTrue)
#48:Int32Sub(#34:Int32Constant, #32:ChangeTaggedToInt32)
#49:Word32And(#48:Int32Sub, #39:Int32Add)
#50:Int32Sub(#34:Int32Constant, #49:Word32And)
#52:Word32And(#32:ChangeTaggedToInt32, #39:Int32Add)
#45:Int32LessThan(#32:ChangeTaggedToInt32, #34:Int32Constant)
#44:IfFalse(#41:Branch)
#46:Branch[Machine, False](#45:Int32LessThan, #44:IfFalse)
#47:IfTrue(#46:Branch)
#51:IfFalse(#46:Branch)
#53:Merge(#47:IfTrue, #51:IfFalse)
#54:Phi[kRepWord32](#50:Int32Sub, #52:Word32And, #53:Merge)
#55:Merge(#42:IfTrue, #53:Merge)
#56:Phi[kRepWord32](#43:Int32Mod, #54:Phi, #55:Merge)
#58:Int32LessThan(#33:ChangeTaggedToInt32, #35:Int32Constant)
#57:IfFalse(#37:Branch)
#59:Branch[Machine, True](#58:Int32LessThan, #57:IfFalse)
#60:IfTrue(#59:Branch)
#61:Int32Mod(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #60:IfTrue)
#62:IfFalse(#59:Branch)
#63:Merge(#60:IfTrue, #62:IfFalse)
#64:Phi[kRepWord32](#61:Int32Mod, #34:Int32Constant, #63:Merge)
#65:Merge(#55:Merge, #63:Merge)
#66:Phi[kRepWord32](#56:Phi, #64:Phi, #65:Merge)
#68:ChangeInt32ToTagged(#66:Phi)
#69:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#70:Load[kRepWord64](#69:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#71:StackPointerGreaterThan[JSFunctionEntry](#70:Load, #70:Load)
#80:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#77:LoadStackCheckOffset()
#78:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#79:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#67:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction modRawInt32 (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo modRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #67:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#72:Branch[Unspecified, True](#71:StackPointerGreaterThan, #0:Start)
#74:IfFalse(#72:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#80:HeapConstant, #77:LoadStackCheckOffset, #78:ExternalConstant, #79:Int32Constant, #7:HeapConstant, #14:FrameState, #71:StackPointerGreaterThan, #74:IfFalse)
#73:IfTrue(#72:Branch)
#75:Merge(#73:IfTrue, #9:Call)
#76:EffectPhi(#71:StackPointerGreaterThan, #9:Call, #75:Merge)
#19:Return(#34:Int32Constant, #68:ChangeInt32ToTagged, #76:EffectPhi, #75:Merge)
#20:End(#19:Return)
```

#### 函数: `modRawInt32Negative`

**测试代码：**

```javascript
function modRawInt32Negative(a, b) {
    return a % b;
}
```

**Metadata：**

```
6123 @params any rawint32 rawint32 @ret rawint32  # modRawInt32Negative
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Mod` | 1 | 0 |
| `Int32Mod` | 0 | 2 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction modRawInt32Negative (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo modRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo modRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Mod(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Mod)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Mod, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#34:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#35:Int32Constant[-1]()
#39:Int32Add(#33:ChangeTaggedToInt32, #35:Int32Constant)
#40:Word32And(#33:ChangeTaggedToInt32, #39:Int32Add)
#36:Int32LessThan(#34:Int32Constant, #33:ChangeTaggedToInt32)
#37:Branch[Machine, True](#36:Int32LessThan, #0:Start)
#38:IfTrue(#37:Branch)
#41:Branch[Machine, None](#40:Word32And, #38:IfTrue)
#42:IfTrue(#41:Branch)
#43:Int32Mod(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #42:IfTrue)
#48:Int32Sub(#34:Int32Constant, #32:ChangeTaggedToInt32)
#49:Word32And(#48:Int32Sub, #39:Int32Add)
#50:Int32Sub(#34:Int32Constant, #49:Word32And)
#52:Word32And(#32:ChangeTaggedToInt32, #39:Int32Add)
#45:Int32LessThan(#32:ChangeTaggedToInt32, #34:Int32Constant)
#44:IfFalse(#41:Branch)
#46:Branch[Machine, False](#45:Int32LessThan, #44:IfFalse)
#47:IfTrue(#46:Branch)
#51:IfFalse(#46:Branch)
#53:Merge(#47:IfTrue, #51:IfFalse)
#54:Phi[kRepWord32](#50:Int32Sub, #52:Word32And, #53:Merge)
#55:Merge(#42:IfTrue, #53:Merge)
#56:Phi[kRepWord32](#43:Int32Mod, #54:Phi, #55:Merge)
#58:Int32LessThan(#33:ChangeTaggedToInt32, #35:Int32Constant)
#57:IfFalse(#37:Branch)
#59:Branch[Machine, True](#58:Int32LessThan, #57:IfFalse)
#60:IfTrue(#59:Branch)
#61:Int32Mod(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #60:IfTrue)
#62:IfFalse(#59:Branch)
#63:Merge(#60:IfTrue, #62:IfFalse)
#64:Phi[kRepWord32](#61:Int32Mod, #34:Int32Constant, #63:Merge)
#65:Merge(#55:Merge, #63:Merge)
#66:Phi[kRepWord32](#56:Phi, #64:Phi, #65:Merge)
#68:ChangeInt32ToTagged(#66:Phi)
#69:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#70:Load[kRepWord64](#69:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#71:StackPointerGreaterThan[JSFunctionEntry](#70:Load, #70:Load)
#80:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#77:LoadStackCheckOffset()
#78:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#79:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#67:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction modRawInt32Negative (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo modRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #67:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#72:Branch[Unspecified, True](#71:StackPointerGreaterThan, #0:Start)
#74:IfFalse(#72:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#80:HeapConstant, #77:LoadStackCheckOffset, #78:ExternalConstant, #79:Int32Constant, #7:HeapConstant, #14:FrameState, #71:StackPointerGreaterThan, #74:IfFalse)
#73:IfTrue(#72:Branch)
#75:Merge(#73:IfTrue, #9:Call)
#76:EffectPhi(#71:StackPointerGreaterThan, #9:Call, #75:Merge)
#19:Return(#34:Int32Constant, #68:ChangeInt32ToTagged, #76:EffectPhi, #75:Merge)
#20:End(#19:Return)
```

#### 函数: `modRawInt32ByZero`

**测试代码：**

```javascript
function modRawInt32ByZero(a, b) {
    return a % b;
}
```

**Metadata：**

```
6418 @params any rawint32 rawint32 @ret rawint32  # modRawInt32ByZero
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|
| `CheckedInt32Mod` | 1 | 0 |
| `Int32Mod` | 0 | 2 |

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#35:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction modRawInt32ByZero (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo modRawInt32ByZero>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#37:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#38:Load[kRepWord64](#37:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#39:StackPointerGreaterThan[JSFunctionEntry](#38:Load, #38:Load)
#48:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#45:LoadStackCheckOffset()
#46:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#47:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo modRawInt32ByZero>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#40:Branch[Unspecified, True](#39:StackPointerGreaterThan, #0:Start)
#42:IfFalse(#40:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#48:HeapConstant, #45:LoadStackCheckOffset, #46:ExternalConstant, #47:Int32Constant, #7:HeapConstant, #14:FrameState, #39:StackPointerGreaterThan, #42:IfFalse)
#41:IfTrue(#40:Branch)
#43:Merge(#41:IfTrue, #9:Call)
#44:EffectPhi(#39:StackPointerGreaterThan, #9:Call, #43:Merge)
#15:Checkpoint(#16:FrameState, #44:EffectPhi, #43:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #43:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #43:Merge)
#17:CheckedInt32Mod(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32, #43:Merge)
#36:ChangeInt32ToTagged(#17:CheckedInt32Mod)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #17:CheckedInt32Mod, #43:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#34:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#32:ChangeTaggedToInt32(#2:Parameter)
#3:Parameter[2](#0:Start)
#33:ChangeTaggedToInt32(#3:Parameter)
#35:Int32Constant[-1]()
#39:Int32Add(#33:ChangeTaggedToInt32, #35:Int32Constant)
#40:Word32And(#33:ChangeTaggedToInt32, #39:Int32Add)
#36:Int32LessThan(#34:Int32Constant, #33:ChangeTaggedToInt32)
#37:Branch[Machine, True](#36:Int32LessThan, #0:Start)
#38:IfTrue(#37:Branch)
#41:Branch[Machine, None](#40:Word32And, #38:IfTrue)
#42:IfTrue(#41:Branch)
#43:Int32Mod(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #42:IfTrue)
#48:Int32Sub(#34:Int32Constant, #32:ChangeTaggedToInt32)
#49:Word32And(#48:Int32Sub, #39:Int32Add)
#50:Int32Sub(#34:Int32Constant, #49:Word32And)
#52:Word32And(#32:ChangeTaggedToInt32, #39:Int32Add)
#45:Int32LessThan(#32:ChangeTaggedToInt32, #34:Int32Constant)
#44:IfFalse(#41:Branch)
#46:Branch[Machine, False](#45:Int32LessThan, #44:IfFalse)
#47:IfTrue(#46:Branch)
#51:IfFalse(#46:Branch)
#53:Merge(#47:IfTrue, #51:IfFalse)
#54:Phi[kRepWord32](#50:Int32Sub, #52:Word32And, #53:Merge)
#55:Merge(#42:IfTrue, #53:Merge)
#56:Phi[kRepWord32](#43:Int32Mod, #54:Phi, #55:Merge)
#58:Int32LessThan(#33:ChangeTaggedToInt32, #35:Int32Constant)
#57:IfFalse(#37:Branch)
#59:Branch[Machine, True](#58:Int32LessThan, #57:IfFalse)
#60:IfTrue(#59:Branch)
#61:Int32Mod(#32:ChangeTaggedToInt32, #33:ChangeTaggedToInt32, #60:IfTrue)
#62:IfFalse(#59:Branch)
#63:Merge(#60:IfTrue, #62:IfFalse)
#64:Phi[kRepWord32](#61:Int32Mod, #34:Int32Constant, #63:Merge)
#65:Merge(#55:Merge, #63:Merge)
#66:Phi[kRepWord32](#56:Phi, #64:Phi, #65:Merge)
#68:ChangeInt32ToTagged(#66:Phi)
#69:ExternalConstant[ADDR1]()
#31:Int64Constant[0]()
#70:Load[kRepWord64](#69:ExternalConstant, #31:Int64Constant, #0:Start, #0:Start)
#71:StackPointerGreaterThan[JSFunctionEntry](#70:Load, #70:Load)
#80:HeapConstant[ADDR2 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#77:LoadStackCheckOffset()
#78:ExternalConstant[ADDR3 <StackGuardWithGap.entry>]()
#79:Int32Constant[1]()
#7:HeapConstant[ADDR4 <NativeContext[304]>]()
#1:Parameter[0, debug name: %this](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#67:TypedStateValues[, sparse:.]()
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR5 <JSFunction modRawInt32ByZero (sfi = ADDR6)>]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR6 <SharedFunctionInfo modRawInt32ByZero>](#10:TypedStateValues, #11:TypedStateValues, #67:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#72:Branch[Unspecified, True](#71:StackPointerGreaterThan, #0:Start)
#74:IfFalse(#72:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#80:HeapConstant, #77:LoadStackCheckOffset, #78:ExternalConstant, #79:Int32Constant, #7:HeapConstant, #14:FrameState, #71:StackPointerGreaterThan, #74:IfFalse)
#73:IfTrue(#72:Branch)
#75:Merge(#73:IfTrue, #9:Call)
#76:EffectPhi(#71:StackPointerGreaterThan, #9:Call, #75:Merge)
#19:Return(#34:Int32Constant, #68:ChangeInt32ToTagged, #76:EffectPhi, #75:Merge)
#20:End(#19:Return)
```
