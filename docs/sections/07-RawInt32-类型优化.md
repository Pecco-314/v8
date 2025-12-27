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
926 @params any rawint32 rawint32 @ret rawint32  # addRawInt32Negative
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
1155 @params any rawint32 rawint32 @ret rawint32  # addRawInt32Underflow
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

#### 函数: `subRawInt32`

**测试代码：**

```javascript
function subRawInt32(a, b) {
    return a - b;
}
```

**Metadata：**

```
1437 @params any rawint32 rawint32 @ret rawint32  # subRawInt32
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
1636 @params any rawint32 rawint32 @ret rawint32  # subRawInt32Overflow
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
1838 @params any rawint32 rawint32 @ret rawint32  # subRawInt32Negative
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
2066 @params any rawint32 rawint32 @ret rawint32  # subRawInt32Underflow
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
2349 @params any rawint32 rawint32 @ret rawint32  # mulRawInt32
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
2542 @params any rawint32 rawint32 @ret rawint32  # mulRawInt32Overflow
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
2747 @params any rawint32 rawint32 @ret rawint32  # mulRawInt32Negative
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
2966 @params any rawint32 rawint32 @ret rawint32  # mulRawInt32Underflow
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
3259 @params any rawint32 rawint32 @ret rawint32  # divRawInt32Exact
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
3468 @params any rawint32 rawint32 @ret rawint32  # divRawInt32Trunc
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
3919 @params any rawint32 rawint32 @ret rawint32  # divRawInt32Overflow
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
4437 @params any rawint32 rawint32 @ret rawint32  # divRawInt32ByZero
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
5144 @params any rawint32 rawint32 @ret rawint32  # modRawInt32
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
5403 @params any rawint32 rawint32 @ret rawint32  # modRawInt32Negative
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
5698 @params any rawint32 rawint32 @ret rawint32  # modRawInt32ByZero
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
