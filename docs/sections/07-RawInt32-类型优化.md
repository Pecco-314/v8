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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction addRawInt32 (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo addRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo addRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Add(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Add)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction addRawInt32Overflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Add(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Add)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction addRawInt32Negative (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Add(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Add)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction addRawInt32Underflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo addRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Add(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Add)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#70:ExternalConstant[ADDR3]()
#46:Int64Constant[0]()
#71:Load[kRepWord64](#70:ExternalConstant, #46:Int64Constant, #0:Start, #0:Start)
#72:StackPointerGreaterThan[JSFunctionEntry](#71:Load, #71:Load)
#81:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#78:LoadStackCheckOffset()
#79:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#80:Int32Constant[1]()
#15:HeapConstant[ADDR6 <NativeContext[304]>]()
#48:TypedStateValues[, sparse:.]()
#22:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo addTenRawInt32>](#18:TypedStateValues, #19:TypedStateValues, #48:TypedStateValues, #13:Parameter, #39:HeapConstant, #0:Start)
#73:Branch[Unspecified, True](#72:StackPointerGreaterThan, #0:Start)
#75:IfFalse(#73:Branch)
#17:Call[Code:StackGuardWithGap:r1s1i5f1](#81:HeapConstant, #78:LoadStackCheckOffset, #79:ExternalConstant, #80:Int32Constant, #15:HeapConstant, #22:FrameState, #72:StackPointerGreaterThan, #75:IfFalse)
#74:IfTrue(#73:Branch)
#76:Merge(#74:IfTrue, #17:Call)
#77:EffectPhi(#72:StackPointerGreaterThan, #17:Call, #76:Merge)
#23:Checkpoint(#24:FrameState, #77:EffectPhi, #76:Merge)
#49:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #23:Checkpoint, #76:Merge)
#50:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #49:CheckedTaggedSignedToInt32, #76:Merge)
#69:Int32Add(#49:CheckedTaggedSignedToInt32, #50:CheckedTaggedSignedToInt32)
#51:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#4:Parameter, #50:CheckedTaggedSignedToInt32, #76:Merge)
#68:Int32Add(#69:Int32Add, #51:CheckedTaggedSignedToInt32)
#52:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#5:Parameter, #51:CheckedTaggedSignedToInt32, #76:Merge)
#67:Int32Add(#68:Int32Add, #52:CheckedTaggedSignedToInt32)
#53:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#6:Parameter, #52:CheckedTaggedSignedToInt32, #76:Merge)
#66:Int32Add(#67:Int32Add, #53:CheckedTaggedSignedToInt32)
#54:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#7:Parameter, #53:CheckedTaggedSignedToInt32, #76:Merge)
#65:Int32Add(#66:Int32Add, #54:CheckedTaggedSignedToInt32)
#55:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#8:Parameter, #54:CheckedTaggedSignedToInt32, #76:Merge)
#64:Int32Add(#65:Int32Add, #55:CheckedTaggedSignedToInt32)
#56:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#9:Parameter, #55:CheckedTaggedSignedToInt32, #76:Merge)
#63:Int32Add(#64:Int32Add, #56:CheckedTaggedSignedToInt32)
#57:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#10:Parameter, #56:CheckedTaggedSignedToInt32, #76:Merge)
#62:Int32Add(#63:Int32Add, #57:CheckedTaggedSignedToInt32)
#58:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#11:Parameter, #57:CheckedTaggedSignedToInt32, #76:Merge)
#61:Int32Add(#62:Int32Add, #58:CheckedTaggedSignedToInt32)
#60:ChangeInt32ToTagged(#61:Int32Add)
#35:Return(#59:Int32Constant, #60:ChangeInt32ToTagged, #58:CheckedTaggedSignedToInt32, #76:Merge)
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
#97:Int32Constant[1]()
#114:ExternalConstant[ADDR1]()
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
#103:ExternalConstant[ADDR4]()
#104:Load[kRepWord64](#103:ExternalConstant, #89:Int64Constant, #0:Start, #0:Start)
#105:StackPointerGreaterThan[JSFunctionEntry](#104:Load, #104:Load)
#113:HeapConstant[ADDR5 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#111:LoadStackCheckOffset()
#112:ExternalConstant[ADDR6 <StackGuardWithGap.entry>]()
#6:HeapConstant[ADDR7 <NativeContext[304]>]()
#10:TypedStateValues[, sparse:...]()
#13:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #10:TypedStateValues, #91:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#106:Branch[Unspecified, True](#105:StackPointerGreaterThan, #0:Start)
#108:IfFalse(#106:Branch)
#8:Call[Code:StackGuardWithGap:r1s1i5f1](#113:HeapConstant, #111:LoadStackCheckOffset, #112:ExternalConstant, #97:Int32Constant, #6:HeapConstant, #13:FrameState, #105:StackPointerGreaterThan, #108:IfFalse)
#107:IfTrue(#106:Branch)
#109:Merge(#107:IfTrue, #8:Call)
#110:EffectPhi(#105:StackPointerGreaterThan, #8:Call, #109:Merge)
#84:Checkpoint(#82:FrameState, #110:EffectPhi, #109:Merge)
#73:CheckMaps[None, ADDR8, FeedbackSource(INVALID)](#2:Parameter, #84:Checkpoint, #109:Merge)
#72:LoadField[BuildLoadDataField, tagged base, 12, ADDR9: [String] in ReadOnlySpace: #length, Range(0, 67108864), kRepTaggedSigned|kTypeInt32, FullWriteBarrier, mutable](#2:Parameter, #73:CheckMaps, #109:Merge)
#92:Int32Constant[0]()
#93:ChangeTaggedSignedToInt32(#72:LoadField)
#65:Uint32LessThan(#92:Int32Constant, #93:ChangeTaggedSignedToInt32)
#81:Branch[Machine, None](#65:Uint32LessThan, #109:Merge)
#79:IfTrue(#81:Branch)
#70:LoadField[JSObjectElements, tagged base, 8, Internal, kRepTaggedPointer|kTypeAny, PointerWriteBarrier, mutable](#2:Parameter, #72:LoadField, #79:IfTrue)
#68:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#92:Int32Constant, #93:ChangeTaggedSignedToInt32, #70:LoadField, #79:IfTrue)
#95:ChangeUint32ToUint64(#68:CheckedUint32Bounds)
#67:LoadElement[tagged base, 8, Signed31, kRepTaggedSigned|kTypeInt32, FullWriteBarrier](#70:LoadField, #95:ChangeUint32ToUint64, #68:CheckedUint32Bounds, #79:IfTrue)
#115:Load[kRepWord8|kTypeUint32](#114:ExternalConstant, #89:Int64Constant, #67:LoadElement, #79:IfTrue)
#122:ExternalConstant[ADDR10 <HandleNoHeapWritesInterrupts.entry>]()
#96:ChangeTaggedSignedToInt32(#67:LoadElement)
#75:TypedStateValues[kRepWord32|kTypeInt32, kRepWord32|kTypeInt32, sparse:^^.](#96:ChangeTaggedSignedToInt32, #97:Int32Constant)
#74:FrameState[UNOPTIMIZED_FRAME, 30, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #75:TypedStateValues, #91:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#117:Branch[Unspecified, False](#115:Load, #79:IfTrue)
#119:IfTrue(#117:Branch)
#76:Call[Code:HandleNoHeapWritesInterrupts:r1s0i4f1](#113:HeapConstant, #122:ExternalConstant, #92:Int32Constant, #6:HeapConstant, #74:FrameState, #115:Load, #119:IfTrue)
#118:IfFalse(#117:Branch)
#120:Merge(#118:IfFalse, #76:Call)
#121:EffectPhi(#115:Load, #76:Call, #120:Merge)
#66:TypeGuard[Range(0, 67108864)](#97:Int32Constant, #121:EffectPhi, #120:Merge)
#18:Phi[kRepWord32](#96:ChangeTaggedSignedToInt32, #42:CheckedInt32Add, #15:Loop)
#99:ChangeUint32ToUint64(#58:CheckedUint32Bounds)
#63:Uint32LessThan(#19:Phi, #93:ChangeTaggedSignedToInt32)
#29:Branch[Machine, None](#63:Uint32LessThan, #15:Loop)
#36:IfTrue(#29:Branch)
#59:LoadElement[tagged base, 8, Signed31, kRepTaggedSigned|kTypeInt32, FullWriteBarrier](#70:LoadField, #99:ChangeUint32ToUint64, #58:CheckedUint32Bounds, #36:IfTrue)
#100:ChangeTaggedSignedToInt32(#59:LoadElement)
#42:CheckedInt32Add(#18:Phi, #100:ChangeTaggedSignedToInt32, #59:LoadElement, #36:IfTrue)
#123:Load[kRepWord8|kTypeUint32](#114:ExternalConstant, #89:Int64Constant, #42:CheckedInt32Add, #36:IfTrue)
#125:Branch[Unspecified, False](#123:Load, #36:IfTrue)
#126:IfFalse(#125:Branch)
#46:TypedStateValues[kRepWord32|kTypeInt32, kRepWord32|kTypeInt32, sparse:^^.](#42:CheckedInt32Add, #44:Int32Add)
#47:FrameState[UNOPTIMIZED_FRAME, 30, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #46:TypedStateValues, #91:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#127:IfTrue(#125:Branch)
#45:Call[Code:HandleNoHeapWritesInterrupts:r1s0i4f1](#113:HeapConstant, #122:ExternalConstant, #92:Int32Constant, #6:HeapConstant, #47:FrameState, #123:Load, #127:IfTrue)
#128:Merge(#126:IfFalse, #45:Call)
#15:Loop(#120:Merge, #128:Merge)
#19:Phi[kRepWord32](#66:TypeGuard, #60:TypeGuard, #15:Loop)
#22:TypedStateValues[kRepWord32|kTypeInt32, kRepWord32|kTypeInt32, sparse:^^.](#18:Phi, #19:Phi)
#23:FrameState[UNOPTIMIZED_FRAME, 4, Ignore, ADDR3 <SharedFunctionInfo sumLoopRawInt32>](#9:TypedStateValues, #22:TypedStateValues, #91:TypedStateValues, #4:Parameter, #52:HeapConstant, #0:Start)
#21:Checkpoint(#23:FrameState, #16:EffectPhi, #15:Loop)
#58:CheckedUint32Bounds[FeedbackSource(INVALID), 0](#19:Phi, #93:ChangeTaggedSignedToInt32, #21:Checkpoint, #36:IfTrue)
#44:Int32Add(#58:CheckedUint32Bounds, #97:Int32Constant)
#129:EffectPhi(#123:Load, #45:Call, #128:Merge)
#60:TypeGuard[Range(0, 67108864)](#44:Int32Add, #129:EffectPhi, #128:Merge)
#16:EffectPhi(#66:TypeGuard, #60:TypeGuard, #15:Loop)
#20:Terminate(#16:EffectPhi, #15:Loop)
#102:ChangeInt32ToTagged(#18:Phi)
#30:IfFalse(#29:Branch)
#87:Return(#92:Int32Constant, #102:ChangeInt32ToTagged, #16:EffectPhi, #30:IfFalse)
#80:IfFalse(#81:Branch)
#88:Return(#92:Int32Constant, #89:Int64Constant, #72:LoadField, #80:IfFalse)
#49:End(#20:Terminate, #87:Return, #88:Return)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction subRawInt32 (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo subRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo subRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Sub(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Sub)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction subRawInt32Overflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Sub(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Sub)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction subRawInt32Negative (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Sub(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Sub)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction subRawInt32Underflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo subRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Sub(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Sub)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction mulRawInt32 (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Mul(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Mul)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction mulRawInt32Overflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Mul(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Mul)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction mulRawInt32Negative (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Mul(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Mul)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction mulRawInt32Underflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#38:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#39:Load[kRepWord64](#38:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#40:StackPointerGreaterThan[JSFunctionEntry](#39:Load, #39:Load)
#49:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#46:LoadStackCheckOffset()
#47:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#48:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo mulRawInt32Underflow>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:Branch[Unspecified, True](#40:StackPointerGreaterThan, #0:Start)
#43:IfFalse(#41:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#49:HeapConstant, #46:LoadStackCheckOffset, #47:ExternalConstant, #48:Int32Constant, #7:HeapConstant, #14:FrameState, #40:StackPointerGreaterThan, #43:IfFalse)
#42:IfTrue(#41:Branch)
#44:Merge(#42:IfTrue, #9:Call)
#45:EffectPhi(#40:StackPointerGreaterThan, #9:Call, #44:Merge)
#15:Checkpoint(#16:FrameState, #45:EffectPhi, #44:Merge)
#33:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #44:Merge)
#34:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #33:CheckedTaggedSignedToInt32, #44:Merge)
#37:Int32Mul(#33:CheckedTaggedSignedToInt32, #34:CheckedTaggedSignedToInt32)
#36:ChangeInt32ToTagged(#37:Int32Mul)
#19:Return(#35:Int32Constant, #36:ChangeInt32ToTagged, #34:CheckedTaggedSignedToInt32, #44:Merge)
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
#42:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#38:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawInt32Exact (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Exact>](#10:TypedStateValues, #11:TypedStateValues, #38:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#61:ExternalConstant[ADDR3]()
#37:Int64Constant[0]()
#62:Load[kRepWord64](#61:ExternalConstant, #37:Int64Constant, #0:Start, #0:Start)
#63:StackPointerGreaterThan[JSFunctionEntry](#62:Load, #62:Load)
#72:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#69:LoadStackCheckOffset()
#70:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#71:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#39:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Exact>](#10:TypedStateValues, #11:TypedStateValues, #39:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#64:Branch[Unspecified, True](#63:StackPointerGreaterThan, #0:Start)
#66:IfFalse(#64:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#72:HeapConstant, #69:LoadStackCheckOffset, #70:ExternalConstant, #71:Int32Constant, #7:HeapConstant, #14:FrameState, #63:StackPointerGreaterThan, #66:IfFalse)
#65:IfTrue(#64:Branch)
#67:Merge(#65:IfTrue, #9:Call)
#68:EffectPhi(#63:StackPointerGreaterThan, #9:Call, #67:Merge)
#15:Checkpoint(#16:FrameState, #68:EffectPhi, #67:Merge)
#27:Branch[Unspecified, True](#3:Parameter, #67:Merge)
#29:IfTrue(#27:Branch)
#40:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #29:IfTrue)
#55:Int32Sub(#42:Int32Constant, #40:CheckedTaggedSignedToInt32)
#41:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #40:CheckedTaggedSignedToInt32, #29:IfTrue)
#46:Int32Constant[-1]()
#51:Word32Equal(#41:CheckedTaggedSignedToInt32, #46:Int32Constant)
#48:Branch[Unspecified, True](#41:CheckedTaggedSignedToInt32, #29:IfTrue)
#50:IfTrue(#48:Branch)
#52:Branch[Machine, False](#51:Word32Equal, #50:IfTrue)
#54:IfFalse(#52:Branch)
#56:Int32Div(#40:CheckedTaggedSignedToInt32, #41:CheckedTaggedSignedToInt32, #54:IfFalse)
#53:IfTrue(#52:Branch)
#57:Merge(#53:IfTrue, #54:IfFalse)
#58:Phi[kRepWord32](#55:Int32Sub, #56:Int32Div, #57:Merge)
#49:IfFalse(#48:Branch)
#59:Merge(#49:IfFalse, #57:Merge)
#60:Phi[kRepWord32](#42:Int32Constant, #58:Phi, #59:Merge)
#43:ChangeInt32ToTagged(#60:Phi)
#19:Return(#42:Int32Constant, #43:ChangeInt32ToTagged, #41:CheckedTaggedSignedToInt32, #29:IfTrue)
#44:Int64Constant[452]()
#73:ExternalConstant[ADDR7 <ThrowRangeError.entry>]()
#28:IfFalse(#27:Branch)
#31:Call[Code:ThrowRangeError:r1s1i5f1](#72:HeapConstant, #44:Int64Constant, #73:ExternalConstant, #71:Int32Constant, #5:Parameter, #16:FrameState, #15:Checkpoint, #28:IfFalse)
#32:Throw(#31:Call, #31:Call)
#20:End(#19:Return, #32:Throw)
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
#42:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#38:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawInt32Trunc (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Trunc>](#10:TypedStateValues, #11:TypedStateValues, #38:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#45:ExternalConstant[ADDR3]()
#37:Int64Constant[0]()
#46:Load[kRepWord64](#45:ExternalConstant, #37:Int64Constant, #0:Start, #0:Start)
#47:StackPointerGreaterThan[JSFunctionEntry](#46:Load, #46:Load)
#56:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#53:LoadStackCheckOffset()
#54:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#55:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#39:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Trunc>](#10:TypedStateValues, #11:TypedStateValues, #39:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#48:Branch[Unspecified, True](#47:StackPointerGreaterThan, #0:Start)
#50:IfFalse(#48:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#56:HeapConstant, #53:LoadStackCheckOffset, #54:ExternalConstant, #55:Int32Constant, #7:HeapConstant, #14:FrameState, #47:StackPointerGreaterThan, #50:IfFalse)
#49:IfTrue(#48:Branch)
#51:Merge(#49:IfTrue, #9:Call)
#52:EffectPhi(#47:StackPointerGreaterThan, #9:Call, #51:Merge)
#15:Checkpoint(#16:FrameState, #52:EffectPhi, #51:Merge)
#27:Branch[Unspecified, True](#3:Parameter, #51:Merge)
#29:IfTrue(#27:Branch)
#40:CheckedTaggedToFloat64[NumberOrOddball, FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #29:IfTrue)
#41:CheckedTaggedToFloat64[NumberOrOddball, FeedbackSource(INVALID)](#3:Parameter, #40:CheckedTaggedToFloat64, #29:IfTrue)
#17:Float64Div(#40:CheckedTaggedToFloat64, #41:CheckedTaggedToFloat64)
#43:ChangeFloat64ToTagged[check-for-minus-zero](#17:Float64Div)
#19:Return(#42:Int32Constant, #43:ChangeFloat64ToTagged, #41:CheckedTaggedToFloat64, #29:IfTrue)
#44:Int64Constant[452]()
#57:ExternalConstant[ADDR7 <ThrowRangeError.entry>]()
#28:IfFalse(#27:Branch)
#31:Call[Code:ThrowRangeError:r1s1i5f1](#56:HeapConstant, #44:Int64Constant, #57:ExternalConstant, #55:Int32Constant, #5:Parameter, #16:FrameState, #15:Checkpoint, #28:IfFalse)
#32:Throw(#31:Call, #31:Call)
#20:End(#19:Return, #32:Throw)
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
4437 @params any rawint32 rawint32 @ret rawint32  # divRawInt32Overflow
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
#42:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#38:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawInt32Overflow (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #38:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#61:ExternalConstant[ADDR3]()
#37:Int64Constant[0]()
#62:Load[kRepWord64](#61:ExternalConstant, #37:Int64Constant, #0:Start, #0:Start)
#63:StackPointerGreaterThan[JSFunctionEntry](#62:Load, #62:Load)
#72:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#69:LoadStackCheckOffset()
#70:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#71:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#39:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawInt32Overflow>](#10:TypedStateValues, #11:TypedStateValues, #39:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#64:Branch[Unspecified, True](#63:StackPointerGreaterThan, #0:Start)
#66:IfFalse(#64:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#72:HeapConstant, #69:LoadStackCheckOffset, #70:ExternalConstant, #71:Int32Constant, #7:HeapConstant, #14:FrameState, #63:StackPointerGreaterThan, #66:IfFalse)
#65:IfTrue(#64:Branch)
#67:Merge(#65:IfTrue, #9:Call)
#68:EffectPhi(#63:StackPointerGreaterThan, #9:Call, #67:Merge)
#15:Checkpoint(#16:FrameState, #68:EffectPhi, #67:Merge)
#27:Branch[Unspecified, True](#3:Parameter, #67:Merge)
#29:IfTrue(#27:Branch)
#40:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #29:IfTrue)
#55:Int32Sub(#42:Int32Constant, #40:CheckedTaggedSignedToInt32)
#41:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #40:CheckedTaggedSignedToInt32, #29:IfTrue)
#46:Int32Constant[-1]()
#51:Word32Equal(#41:CheckedTaggedSignedToInt32, #46:Int32Constant)
#48:Branch[Unspecified, True](#41:CheckedTaggedSignedToInt32, #29:IfTrue)
#50:IfTrue(#48:Branch)
#52:Branch[Machine, False](#51:Word32Equal, #50:IfTrue)
#54:IfFalse(#52:Branch)
#56:Int32Div(#40:CheckedTaggedSignedToInt32, #41:CheckedTaggedSignedToInt32, #54:IfFalse)
#53:IfTrue(#52:Branch)
#57:Merge(#53:IfTrue, #54:IfFalse)
#58:Phi[kRepWord32](#55:Int32Sub, #56:Int32Div, #57:Merge)
#49:IfFalse(#48:Branch)
#59:Merge(#49:IfFalse, #57:Merge)
#60:Phi[kRepWord32](#42:Int32Constant, #58:Phi, #59:Merge)
#43:ChangeInt32ToTagged(#60:Phi)
#19:Return(#42:Int32Constant, #43:ChangeInt32ToTagged, #41:CheckedTaggedSignedToInt32, #29:IfTrue)
#44:Int64Constant[452]()
#73:ExternalConstant[ADDR7 <ThrowRangeError.entry>]()
#28:IfFalse(#27:Branch)
#31:Call[Code:ThrowRangeError:r1s1i5f1](#72:HeapConstant, #44:Int64Constant, #73:ExternalConstant, #71:Int32Constant, #5:Parameter, #16:FrameState, #15:Checkpoint, #28:IfFalse)
#32:Throw(#31:Call, #31:Call)
#20:End(#19:Return, #32:Throw)
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
4722 @params any rawint32 rawint32 @ret rawint32  # divRawInt32ByZero
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
#42:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#38:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawInt32ByZero (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawInt32ByZero>](#10:TypedStateValues, #11:TypedStateValues, #38:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#61:ExternalConstant[ADDR3]()
#37:Int64Constant[0]()
#62:Load[kRepWord64](#61:ExternalConstant, #37:Int64Constant, #0:Start, #0:Start)
#63:StackPointerGreaterThan[JSFunctionEntry](#62:Load, #62:Load)
#72:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#69:LoadStackCheckOffset()
#70:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#71:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#39:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawInt32ByZero>](#10:TypedStateValues, #11:TypedStateValues, #39:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#64:Branch[Unspecified, True](#63:StackPointerGreaterThan, #0:Start)
#66:IfFalse(#64:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#72:HeapConstant, #69:LoadStackCheckOffset, #70:ExternalConstant, #71:Int32Constant, #7:HeapConstant, #14:FrameState, #63:StackPointerGreaterThan, #66:IfFalse)
#65:IfTrue(#64:Branch)
#67:Merge(#65:IfTrue, #9:Call)
#68:EffectPhi(#63:StackPointerGreaterThan, #9:Call, #67:Merge)
#15:Checkpoint(#16:FrameState, #68:EffectPhi, #67:Merge)
#27:Branch[Unspecified, True](#3:Parameter, #67:Merge)
#29:IfTrue(#27:Branch)
#40:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #29:IfTrue)
#55:Int32Sub(#42:Int32Constant, #40:CheckedTaggedSignedToInt32)
#41:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #40:CheckedTaggedSignedToInt32, #29:IfTrue)
#46:Int32Constant[-1]()
#51:Word32Equal(#41:CheckedTaggedSignedToInt32, #46:Int32Constant)
#48:Branch[Unspecified, True](#41:CheckedTaggedSignedToInt32, #29:IfTrue)
#50:IfTrue(#48:Branch)
#52:Branch[Machine, False](#51:Word32Equal, #50:IfTrue)
#54:IfFalse(#52:Branch)
#56:Int32Div(#40:CheckedTaggedSignedToInt32, #41:CheckedTaggedSignedToInt32, #54:IfFalse)
#53:IfTrue(#52:Branch)
#57:Merge(#53:IfTrue, #54:IfFalse)
#58:Phi[kRepWord32](#55:Int32Sub, #56:Int32Div, #57:Merge)
#49:IfFalse(#48:Branch)
#59:Merge(#49:IfFalse, #57:Merge)
#60:Phi[kRepWord32](#42:Int32Constant, #58:Phi, #59:Merge)
#43:ChangeInt32ToTagged(#60:Phi)
#19:Return(#42:Int32Constant, #43:ChangeInt32ToTagged, #41:CheckedTaggedSignedToInt32, #29:IfTrue)
#44:Int64Constant[452]()
#73:ExternalConstant[ADDR7 <ThrowRangeError.entry>]()
#28:IfFalse(#27:Branch)
#31:Call[Code:ThrowRangeError:r1s1i5f1](#72:HeapConstant, #44:Int64Constant, #73:ExternalConstant, #71:Int32Constant, #5:Parameter, #16:FrameState, #15:Checkpoint, #28:IfFalse)
#32:Throw(#31:Call, #31:Call)
#20:End(#19:Return, #32:Throw)
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
5480 @params any rawint32 rawint32 @ret rawint32  # modRawInt32
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
#42:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#38:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction modRawInt32 (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo modRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #38:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#53:ExternalConstant[ADDR3]()
#37:Int64Constant[0]()
#54:Load[kRepWord64](#53:ExternalConstant, #37:Int64Constant, #0:Start, #0:Start)
#55:StackPointerGreaterThan[JSFunctionEntry](#54:Load, #54:Load)
#64:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#61:LoadStackCheckOffset()
#62:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#63:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#39:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo modRawInt32>](#10:TypedStateValues, #11:TypedStateValues, #39:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#56:Branch[Unspecified, True](#55:StackPointerGreaterThan, #0:Start)
#58:IfFalse(#56:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#64:HeapConstant, #61:LoadStackCheckOffset, #62:ExternalConstant, #63:Int32Constant, #7:HeapConstant, #14:FrameState, #55:StackPointerGreaterThan, #58:IfFalse)
#57:IfTrue(#56:Branch)
#59:Merge(#57:IfTrue, #9:Call)
#60:EffectPhi(#55:StackPointerGreaterThan, #9:Call, #59:Merge)
#15:Checkpoint(#16:FrameState, #60:EffectPhi, #59:Merge)
#27:Branch[Unspecified, True](#3:Parameter, #59:Merge)
#29:IfTrue(#27:Branch)
#40:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #29:IfTrue)
#41:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #40:CheckedTaggedSignedToInt32, #29:IfTrue)
#47:Branch[Unspecified, True](#41:CheckedTaggedSignedToInt32, #29:IfTrue)
#49:IfTrue(#47:Branch)
#50:Int32Mod(#40:CheckedTaggedSignedToInt32, #41:CheckedTaggedSignedToInt32, #49:IfTrue)
#48:IfFalse(#47:Branch)
#51:Merge(#48:IfFalse, #49:IfTrue)
#52:Phi[kRepWord32](#42:Int32Constant, #50:Int32Mod, #51:Merge)
#43:ChangeInt32ToTagged(#52:Phi)
#19:Return(#42:Int32Constant, #43:ChangeInt32ToTagged, #41:CheckedTaggedSignedToInt32, #29:IfTrue)
#44:Int64Constant[452]()
#65:ExternalConstant[ADDR7 <ThrowRangeError.entry>]()
#28:IfFalse(#27:Branch)
#31:Call[Code:ThrowRangeError:r1s1i5f1](#64:HeapConstant, #44:Int64Constant, #65:ExternalConstant, #63:Int32Constant, #5:Parameter, #16:FrameState, #15:Checkpoint, #28:IfFalse)
#32:Throw(#31:Call, #31:Call)
#20:End(#19:Return, #32:Throw)
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
5739 @params any rawint32 rawint32 @ret rawint32  # modRawInt32Negative
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
#42:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#38:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction modRawInt32Negative (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo modRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #38:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#53:ExternalConstant[ADDR3]()
#37:Int64Constant[0]()
#54:Load[kRepWord64](#53:ExternalConstant, #37:Int64Constant, #0:Start, #0:Start)
#55:StackPointerGreaterThan[JSFunctionEntry](#54:Load, #54:Load)
#64:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#61:LoadStackCheckOffset()
#62:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#63:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#39:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo modRawInt32Negative>](#10:TypedStateValues, #11:TypedStateValues, #39:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#56:Branch[Unspecified, True](#55:StackPointerGreaterThan, #0:Start)
#58:IfFalse(#56:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#64:HeapConstant, #61:LoadStackCheckOffset, #62:ExternalConstant, #63:Int32Constant, #7:HeapConstant, #14:FrameState, #55:StackPointerGreaterThan, #58:IfFalse)
#57:IfTrue(#56:Branch)
#59:Merge(#57:IfTrue, #9:Call)
#60:EffectPhi(#55:StackPointerGreaterThan, #9:Call, #59:Merge)
#15:Checkpoint(#16:FrameState, #60:EffectPhi, #59:Merge)
#27:Branch[Unspecified, True](#3:Parameter, #59:Merge)
#29:IfTrue(#27:Branch)
#40:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #29:IfTrue)
#41:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #40:CheckedTaggedSignedToInt32, #29:IfTrue)
#47:Branch[Unspecified, True](#41:CheckedTaggedSignedToInt32, #29:IfTrue)
#49:IfTrue(#47:Branch)
#50:Int32Mod(#40:CheckedTaggedSignedToInt32, #41:CheckedTaggedSignedToInt32, #49:IfTrue)
#48:IfFalse(#47:Branch)
#51:Merge(#48:IfFalse, #49:IfTrue)
#52:Phi[kRepWord32](#42:Int32Constant, #50:Int32Mod, #51:Merge)
#43:ChangeInt32ToTagged(#52:Phi)
#19:Return(#42:Int32Constant, #43:ChangeInt32ToTagged, #41:CheckedTaggedSignedToInt32, #29:IfTrue)
#44:Int64Constant[452]()
#65:ExternalConstant[ADDR7 <ThrowRangeError.entry>]()
#28:IfFalse(#27:Branch)
#31:Call[Code:ThrowRangeError:r1s1i5f1](#64:HeapConstant, #44:Int64Constant, #65:ExternalConstant, #63:Int32Constant, #5:Parameter, #16:FrameState, #15:Checkpoint, #28:IfFalse)
#32:Throw(#31:Call, #31:Call)
#20:End(#19:Return, #32:Throw)
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
6034 @params any rawint32 rawint32 @ret rawint32  # modRawInt32ByZero
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
#42:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#38:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction modRawInt32ByZero (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo modRawInt32ByZero>](#10:TypedStateValues, #11:TypedStateValues, #38:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#53:ExternalConstant[ADDR3]()
#37:Int64Constant[0]()
#54:Load[kRepWord64](#53:ExternalConstant, #37:Int64Constant, #0:Start, #0:Start)
#55:StackPointerGreaterThan[JSFunctionEntry](#54:Load, #54:Load)
#64:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#61:LoadStackCheckOffset()
#62:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#63:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#39:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo modRawInt32ByZero>](#10:TypedStateValues, #11:TypedStateValues, #39:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#56:Branch[Unspecified, True](#55:StackPointerGreaterThan, #0:Start)
#58:IfFalse(#56:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#64:HeapConstant, #61:LoadStackCheckOffset, #62:ExternalConstant, #63:Int32Constant, #7:HeapConstant, #14:FrameState, #55:StackPointerGreaterThan, #58:IfFalse)
#57:IfTrue(#56:Branch)
#59:Merge(#57:IfTrue, #9:Call)
#60:EffectPhi(#55:StackPointerGreaterThan, #9:Call, #59:Merge)
#15:Checkpoint(#16:FrameState, #60:EffectPhi, #59:Merge)
#27:Branch[Unspecified, True](#3:Parameter, #59:Merge)
#29:IfTrue(#27:Branch)
#40:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #29:IfTrue)
#41:CheckedTaggedSignedToInt32[FeedbackSource(INVALID)](#3:Parameter, #40:CheckedTaggedSignedToInt32, #29:IfTrue)
#47:Branch[Unspecified, True](#41:CheckedTaggedSignedToInt32, #29:IfTrue)
#49:IfTrue(#47:Branch)
#50:Int32Mod(#40:CheckedTaggedSignedToInt32, #41:CheckedTaggedSignedToInt32, #49:IfTrue)
#48:IfFalse(#47:Branch)
#51:Merge(#48:IfFalse, #49:IfTrue)
#52:Phi[kRepWord32](#42:Int32Constant, #50:Int32Mod, #51:Merge)
#43:ChangeInt32ToTagged(#52:Phi)
#19:Return(#42:Int32Constant, #43:ChangeInt32ToTagged, #41:CheckedTaggedSignedToInt32, #29:IfTrue)
#44:Int64Constant[452]()
#65:ExternalConstant[ADDR7 <ThrowRangeError.entry>]()
#28:IfFalse(#27:Branch)
#31:Call[Code:ThrowRangeError:r1s1i5f1](#64:HeapConstant, #44:Int64Constant, #65:ExternalConstant, #63:Int32Constant, #5:Parameter, #16:FrameState, #15:Checkpoint, #28:IfFalse)
#32:Throw(#31:Call, #31:Call)
#20:End(#19:Return, #32:Throw)
```

#### 函数: `divRawInt64Const`

**测试代码：**

```javascript
function divRawInt64Const(a, b) {
    return a / b;
}
```

**Metadata：**

```
9131 @params any rawint64 rawint64 @ret rawint64  # divRawInt64Const
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#39:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawInt64Const (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawInt64Const>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#42:Load[kRepWord64](#41:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#43:StackPointerGreaterThan[JSFunctionEntry](#42:Load, #42:Load)
#52:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#49:LoadStackCheckOffset()
#50:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#51:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawInt64Const>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#44:Branch[Unspecified, True](#43:StackPointerGreaterThan, #0:Start)
#46:IfFalse(#44:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#52:HeapConstant, #49:LoadStackCheckOffset, #50:ExternalConstant, #51:Int32Constant, #7:HeapConstant, #14:FrameState, #43:StackPointerGreaterThan, #46:IfFalse)
#45:IfTrue(#44:Branch)
#47:Merge(#45:IfTrue, #9:Call)
#48:EffectPhi(#43:StackPointerGreaterThan, #9:Call, #47:Merge)
#15:Checkpoint(#16:FrameState, #48:EffectPhi, #47:Merge)
#33:CheckBigInt[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #47:Merge)
#34:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#33:CheckBigInt, #33:CheckBigInt, #47:Merge)
#35:TruncateBigIntToWord64(#34:CheckedBigIntToBigInt64)
#36:CheckBigInt[FeedbackSource(INVALID)](#3:Parameter, #34:CheckedBigIntToBigInt64, #47:Merge)
#37:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#36:CheckBigInt, #36:CheckBigInt, #47:Merge)
#38:TruncateBigIntToWord64(#37:CheckedBigIntToBigInt64)
#17:CheckedInt64Div(#35:TruncateBigIntToWord64, #38:TruncateBigIntToWord64, #37:CheckedBigIntToBigInt64, #47:Merge)
#40:ChangeInt64ToBigInt(#17:CheckedInt64Div)
#19:Return(#39:Int32Constant, #40:ChangeInt64ToBigInt, #17:CheckedInt64Div, #47:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#39:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawInt64Const (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawInt64Const>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#42:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#43:Load[kRepWord64](#42:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#44:StackPointerGreaterThan[JSFunctionEntry](#43:Load, #43:Load)
#53:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#50:LoadStackCheckOffset()
#51:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#52:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawInt64Const>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#45:Branch[Unspecified, True](#44:StackPointerGreaterThan, #0:Start)
#47:IfFalse(#45:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#53:HeapConstant, #50:LoadStackCheckOffset, #51:ExternalConstant, #52:Int32Constant, #7:HeapConstant, #14:FrameState, #44:StackPointerGreaterThan, #47:IfFalse)
#46:IfTrue(#45:Branch)
#48:Merge(#46:IfTrue, #9:Call)
#49:EffectPhi(#44:StackPointerGreaterThan, #9:Call, #48:Merge)
#15:Checkpoint(#16:FrameState, #49:EffectPhi, #48:Merge)
#33:CheckBigInt[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #48:Merge)
#34:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#33:CheckBigInt, #33:CheckBigInt, #48:Merge)
#35:TruncateBigIntToWord64(#34:CheckedBigIntToBigInt64)
#36:CheckBigInt[FeedbackSource(INVALID)](#3:Parameter, #34:CheckedBigIntToBigInt64, #48:Merge)
#37:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#36:CheckBigInt, #36:CheckBigInt, #48:Merge)
#38:TruncateBigIntToWord64(#37:CheckedBigIntToBigInt64)
#41:Int64Div(#35:TruncateBigIntToWord64, #38:TruncateBigIntToWord64, #48:Merge)
#40:ChangeInt64ToBigInt(#41:Int64Div)
#19:Return(#39:Int32Constant, #40:ChangeInt64ToBigInt, #37:CheckedBigIntToBigInt64, #48:Merge)
#20:End(#19:Return)
```

#### 函数: `modRawInt64Const`

**测试代码：**

```javascript
function modRawInt64Const(a, b) {
    return a % b;
}
```

**Metadata：**

```
9465 @params any rawint64 rawint64 @ret rawint64  # modRawInt64Const
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#39:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction modRawInt64Const (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo modRawInt64Const>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#42:Load[kRepWord64](#41:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#43:StackPointerGreaterThan[JSFunctionEntry](#42:Load, #42:Load)
#52:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#49:LoadStackCheckOffset()
#50:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#51:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo modRawInt64Const>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#44:Branch[Unspecified, True](#43:StackPointerGreaterThan, #0:Start)
#46:IfFalse(#44:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#52:HeapConstant, #49:LoadStackCheckOffset, #50:ExternalConstant, #51:Int32Constant, #7:HeapConstant, #14:FrameState, #43:StackPointerGreaterThan, #46:IfFalse)
#45:IfTrue(#44:Branch)
#47:Merge(#45:IfTrue, #9:Call)
#48:EffectPhi(#43:StackPointerGreaterThan, #9:Call, #47:Merge)
#15:Checkpoint(#16:FrameState, #48:EffectPhi, #47:Merge)
#33:CheckBigInt[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #47:Merge)
#34:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#33:CheckBigInt, #33:CheckBigInt, #47:Merge)
#35:TruncateBigIntToWord64(#34:CheckedBigIntToBigInt64)
#36:CheckBigInt[FeedbackSource(INVALID)](#3:Parameter, #34:CheckedBigIntToBigInt64, #47:Merge)
#37:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#36:CheckBigInt, #36:CheckBigInt, #47:Merge)
#38:TruncateBigIntToWord64(#37:CheckedBigIntToBigInt64)
#17:CheckedInt64Mod(#35:TruncateBigIntToWord64, #38:TruncateBigIntToWord64, #37:CheckedBigIntToBigInt64, #47:Merge)
#40:ChangeInt64ToBigInt(#17:CheckedInt64Mod)
#19:Return(#39:Int32Constant, #40:ChangeInt64ToBigInt, #17:CheckedInt64Mod, #47:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#39:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction modRawInt64Const (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo modRawInt64Const>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#42:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#43:Load[kRepWord64](#42:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#44:StackPointerGreaterThan[JSFunctionEntry](#43:Load, #43:Load)
#53:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#50:LoadStackCheckOffset()
#51:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#52:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo modRawInt64Const>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#45:Branch[Unspecified, True](#44:StackPointerGreaterThan, #0:Start)
#47:IfFalse(#45:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#53:HeapConstant, #50:LoadStackCheckOffset, #51:ExternalConstant, #52:Int32Constant, #7:HeapConstant, #14:FrameState, #44:StackPointerGreaterThan, #47:IfFalse)
#46:IfTrue(#45:Branch)
#48:Merge(#46:IfTrue, #9:Call)
#49:EffectPhi(#44:StackPointerGreaterThan, #9:Call, #48:Merge)
#15:Checkpoint(#16:FrameState, #49:EffectPhi, #48:Merge)
#33:CheckBigInt[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #48:Merge)
#34:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#33:CheckBigInt, #33:CheckBigInt, #48:Merge)
#35:TruncateBigIntToWord64(#34:CheckedBigIntToBigInt64)
#36:CheckBigInt[FeedbackSource(INVALID)](#3:Parameter, #34:CheckedBigIntToBigInt64, #48:Merge)
#37:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#36:CheckBigInt, #36:CheckBigInt, #48:Merge)
#38:TruncateBigIntToWord64(#37:CheckedBigIntToBigInt64)
#41:Int64Mod(#35:TruncateBigIntToWord64, #38:TruncateBigIntToWord64, #48:Merge)
#40:ChangeInt64ToBigInt(#41:Int64Mod)
#19:Return(#39:Int32Constant, #40:ChangeInt64ToBigInt, #37:CheckedBigIntToBigInt64, #48:Merge)
#20:End(#19:Return)
```

#### 函数: `divRawUint64Const`

**测试代码：**

```javascript
function divRawUint64Const(a, b) {
    return a / b;
}
```

**Metadata：**

```
9642 @params any rawuint64 rawuint64 @ret rawuint64  # divRawUint64Const
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#39:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawUint64Const (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawUint64Const>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#42:Load[kRepWord64](#41:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#43:StackPointerGreaterThan[JSFunctionEntry](#42:Load, #42:Load)
#52:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#49:LoadStackCheckOffset()
#50:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#51:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawUint64Const>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#44:Branch[Unspecified, True](#43:StackPointerGreaterThan, #0:Start)
#46:IfFalse(#44:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#52:HeapConstant, #49:LoadStackCheckOffset, #50:ExternalConstant, #51:Int32Constant, #7:HeapConstant, #14:FrameState, #43:StackPointerGreaterThan, #46:IfFalse)
#45:IfTrue(#44:Branch)
#47:Merge(#45:IfTrue, #9:Call)
#48:EffectPhi(#43:StackPointerGreaterThan, #9:Call, #47:Merge)
#15:Checkpoint(#16:FrameState, #48:EffectPhi, #47:Merge)
#33:CheckBigInt[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #47:Merge)
#34:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#33:CheckBigInt, #33:CheckBigInt, #47:Merge)
#35:TruncateBigIntToWord64(#34:CheckedBigIntToBigInt64)
#36:CheckBigInt[FeedbackSource(INVALID)](#3:Parameter, #34:CheckedBigIntToBigInt64, #47:Merge)
#37:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#36:CheckBigInt, #36:CheckBigInt, #47:Merge)
#38:TruncateBigIntToWord64(#37:CheckedBigIntToBigInt64)
#17:CheckedInt64Div(#35:TruncateBigIntToWord64, #38:TruncateBigIntToWord64, #37:CheckedBigIntToBigInt64, #47:Merge)
#40:ChangeInt64ToBigInt(#17:CheckedInt64Div)
#19:Return(#39:Int32Constant, #40:ChangeInt64ToBigInt, #17:CheckedInt64Div, #47:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#39:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction divRawUint64Const (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo divRawUint64Const>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#42:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#43:Load[kRepWord64](#42:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#44:StackPointerGreaterThan[JSFunctionEntry](#43:Load, #43:Load)
#53:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#50:LoadStackCheckOffset()
#51:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#52:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo divRawUint64Const>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#45:Branch[Unspecified, True](#44:StackPointerGreaterThan, #0:Start)
#47:IfFalse(#45:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#53:HeapConstant, #50:LoadStackCheckOffset, #51:ExternalConstant, #52:Int32Constant, #7:HeapConstant, #14:FrameState, #44:StackPointerGreaterThan, #47:IfFalse)
#46:IfTrue(#45:Branch)
#48:Merge(#46:IfTrue, #9:Call)
#49:EffectPhi(#44:StackPointerGreaterThan, #9:Call, #48:Merge)
#15:Checkpoint(#16:FrameState, #49:EffectPhi, #48:Merge)
#33:CheckBigInt[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #48:Merge)
#34:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#33:CheckBigInt, #33:CheckBigInt, #48:Merge)
#35:TruncateBigIntToWord64(#34:CheckedBigIntToBigInt64)
#36:CheckBigInt[FeedbackSource(INVALID)](#3:Parameter, #34:CheckedBigIntToBigInt64, #48:Merge)
#37:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#36:CheckBigInt, #36:CheckBigInt, #48:Merge)
#38:TruncateBigIntToWord64(#37:CheckedBigIntToBigInt64)
#41:Uint64Div(#35:TruncateBigIntToWord64, #38:TruncateBigIntToWord64, #48:Merge)
#40:ChangeInt64ToBigInt(#41:Uint64Div)
#19:Return(#39:Int32Constant, #40:ChangeInt64ToBigInt, #37:CheckedBigIntToBigInt64, #48:Merge)
#20:End(#19:Return)
```

#### 函数: `modRawUint64Const`

**测试代码：**

```javascript
function modRawUint64Const(a, b) {
    return a % b;
}
```

**Metadata：**

```
9981 @params any rawuint64 rawuint64 @ret rawuint64  # modRawUint64Const
```

**优化 Flags：**

```
--turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
```

**优化效果：**

| 节点类型 | 优化前 | 优化后 |
|---------|-------|-------|

**优化前 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#39:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction modRawUint64Const (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo modRawUint64Const>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#41:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#42:Load[kRepWord64](#41:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#43:StackPointerGreaterThan[JSFunctionEntry](#42:Load, #42:Load)
#52:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#49:LoadStackCheckOffset()
#50:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#51:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo modRawUint64Const>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#44:Branch[Unspecified, True](#43:StackPointerGreaterThan, #0:Start)
#46:IfFalse(#44:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#52:HeapConstant, #49:LoadStackCheckOffset, #50:ExternalConstant, #51:Int32Constant, #7:HeapConstant, #14:FrameState, #43:StackPointerGreaterThan, #46:IfFalse)
#45:IfTrue(#44:Branch)
#47:Merge(#45:IfTrue, #9:Call)
#48:EffectPhi(#43:StackPointerGreaterThan, #9:Call, #47:Merge)
#15:Checkpoint(#16:FrameState, #48:EffectPhi, #47:Merge)
#33:CheckBigInt[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #47:Merge)
#34:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#33:CheckBigInt, #33:CheckBigInt, #47:Merge)
#35:TruncateBigIntToWord64(#34:CheckedBigIntToBigInt64)
#36:CheckBigInt[FeedbackSource(INVALID)](#3:Parameter, #34:CheckedBigIntToBigInt64, #47:Merge)
#37:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#36:CheckBigInt, #36:CheckBigInt, #47:Merge)
#38:TruncateBigIntToWord64(#37:CheckedBigIntToBigInt64)
#17:CheckedInt64Mod(#35:TruncateBigIntToWord64, #38:TruncateBigIntToWord64, #37:CheckedBigIntToBigInt64, #47:Merge)
#40:ChangeInt64ToBigInt(#17:CheckedInt64Mod)
#19:Return(#39:Int32Constant, #40:ChangeInt64ToBigInt, #17:CheckedInt64Mod, #47:Merge)
#20:End(#19:Return)
```

**优化后 Graph (EarlyOptimization)：**

```
----- Graph after V8.TFEarlyOptimization ----- 
#39:Int32Constant[0]()
#0:Start()
#2:Parameter[1](#0:Start)
#1:Parameter[0, debug name: %this](#0:Start)
#3:Parameter[2](#0:Start)
#10:TypedStateValues[kRepTagged|kTypeAny, kRepTagged|kTypeAny, kRepTagged|kTypeAny, dense](#1:Parameter, #2:Parameter, #3:Parameter)
#11:TypedStateValues[, dense]()
#31:TypedStateValues[kRepTagged|kTypeAny, dense](#3:Parameter)
#5:Parameter[6, debug name: %context](#0:Start)
#23:HeapConstant[ADDR1 <JSFunction modRawUint64Const (sfi = ADDR2)>]()
#16:FrameState[UNOPTIMIZED_FRAME, 2, Ignore, ADDR2 <SharedFunctionInfo modRawUint64Const>](#10:TypedStateValues, #11:TypedStateValues, #31:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#42:ExternalConstant[ADDR3]()
#30:Int64Constant[0]()
#43:Load[kRepWord64](#42:ExternalConstant, #30:Int64Constant, #0:Start, #0:Start)
#44:StackPointerGreaterThan[JSFunctionEntry](#43:Load, #43:Load)
#53:HeapConstant[ADDR4 <Code BUILTIN CEntry_Return1_ArgvOnStack_NoBuiltinExit>]()
#50:LoadStackCheckOffset()
#51:ExternalConstant[ADDR5 <StackGuardWithGap.entry>]()
#52:Int32Constant[1]()
#7:HeapConstant[ADDR6 <NativeContext[304]>]()
#32:TypedStateValues[, sparse:.]()
#14:FrameState[UNOPTIMIZED_FRAME, -1, Ignore, ADDR2 <SharedFunctionInfo modRawUint64Const>](#10:TypedStateValues, #11:TypedStateValues, #32:TypedStateValues, #5:Parameter, #23:HeapConstant, #0:Start)
#45:Branch[Unspecified, True](#44:StackPointerGreaterThan, #0:Start)
#47:IfFalse(#45:Branch)
#9:Call[Code:StackGuardWithGap:r1s1i5f1](#53:HeapConstant, #50:LoadStackCheckOffset, #51:ExternalConstant, #52:Int32Constant, #7:HeapConstant, #14:FrameState, #44:StackPointerGreaterThan, #47:IfFalse)
#46:IfTrue(#45:Branch)
#48:Merge(#46:IfTrue, #9:Call)
#49:EffectPhi(#44:StackPointerGreaterThan, #9:Call, #48:Merge)
#15:Checkpoint(#16:FrameState, #49:EffectPhi, #48:Merge)
#33:CheckBigInt[FeedbackSource(INVALID)](#2:Parameter, #15:Checkpoint, #48:Merge)
#34:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#33:CheckBigInt, #33:CheckBigInt, #48:Merge)
#35:TruncateBigIntToWord64(#34:CheckedBigIntToBigInt64)
#36:CheckBigInt[FeedbackSource(INVALID)](#3:Parameter, #34:CheckedBigIntToBigInt64, #48:Merge)
#37:CheckedBigIntToBigInt64[FeedbackSource(INVALID)](#36:CheckBigInt, #36:CheckBigInt, #48:Merge)
#38:TruncateBigIntToWord64(#37:CheckedBigIntToBigInt64)
#41:Uint64Mod(#35:TruncateBigIntToWord64, #38:TruncateBigIntToWord64, #48:Merge)
#40:ChangeInt64ToBigInt(#41:Uint64Mod)
#19:Return(#39:Int32Constant, #40:ChangeInt64ToBigInt, #37:CheckedBigIntToBigInt64, #48:Merge)
#20:End(#19:Return)
```
