### 7. RawInt32 类型优化

RawInt32 类型的无溢出检查加法运算（移除溢出检查）

#### 函数: `addRawInt32`

**测试代码：**

```javascript
function addRawInt32(a, b) {
    return a + b;
}
```

**Metadata：**

```
391 @params any rawint32 rawint32 @ret rawint32
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
962 @params any rawint32 rawint32 @ret rawint32
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
1784 @params any rawint32 rawint32 @ret rawint32
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
2236 @params any rawint32 rawint32 @ret rawint32
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
