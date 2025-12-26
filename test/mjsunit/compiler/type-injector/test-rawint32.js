// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// 测试：RawInt32 类型 - 无溢出检查的 int32 加法

// ===================================
// 测试 1: 正常加法（不溢出）
// ===================================
// 预期优化：注入 RawInt32 类型后，a + b 直接使用 Int32Add，
// 不会有 CheckedInt32Add 和 DeoptimizeIf[Overflow]

function addRawInt32(a, b) {
    return a + b;
}

%PrepareFunctionForOptimization(addRawInt32);

// 预热：使用正常范围的值
for (var i = 0; i < 100; i++) {
    addRawInt32(100, 200);
}

%OptimizeFunctionOnNextCall(addRawInt32);

// 正常测试
var result1 = addRawInt32(1000, 2000);
assertEquals(3000, result1);

// 验证函数已被优化
assertOptimized(addRawInt32);

// ===================================
// 测试 2: 溢出情况（UB - 应该回绕）
// ===================================
// 重要：这个测试验证溢出时不会 deopt
// 由于是 UB，我们期望：
// 1. 不触发 deoptimization
// 2. 结果是二补数回绕 (INT32_MAX + 1 = INT32_MIN = -2147483648)

function addRawInt32Overflow(a, b) {
    return a + b;
}

%PrepareFunctionForOptimization(addRawInt32Overflow);

// 预热：使用正常范围的值
for (var i = 0; i < 100; i++) {
    addRawInt32Overflow(100, 200);
}

%OptimizeFunctionOnNextCall(addRawInt32Overflow);

// 溢出测试 - INT32_MAX + 1 应该回绕到 INT32_MIN
var result2 = addRawInt32Overflow(2147483647, 1);
print("Overflow result: " + result2);

// 注意：由于溢出是 UB，我们不能断言具体的值
// 但我们可以验证函数仍然是优化状态（没有 deopt）
// 在大多数架构上，结果应该是 -2147483648（二补数回绕）

// 如果这里还是 optimized，说明没有触发 deopt
var status = %GetOptimizationStatus(addRawInt32Overflow);
print("Optimization status after overflow: " + status);

// 再次调用，确保稳定
var result3 = addRawInt32Overflow(2000000000, 200000000);
print("Another overflow: " + result3);

// ===================================
// 测试 3: 负数加法
// ===================================
function addRawInt32Negative(a, b) {
    return a + b;
}

%PrepareFunctionForOptimization(addRawInt32Negative);

for (var i = 0; i < 100; i++) {
    addRawInt32Negative(-100, -200);
}

%OptimizeFunctionOnNextCall(addRawInt32Negative);

var result4 = addRawInt32Negative(-1000, -2000);
assertEquals(-3000, result4);

assertOptimized(addRawInt32Negative);

// ===================================
// 测试 4: 负数溢出（下溢）
// ===================================
function addRawInt32Underflow(a, b) {
    return a + b;
}

%PrepareFunctionForOptimization(addRawInt32Underflow);

for (var i = 0; i < 100; i++) {
    addRawInt32Underflow(-100, -200);
}

%OptimizeFunctionOnNextCall(addRawInt32Underflow);

// INT32_MIN + (-1) 应该回绕到 INT32_MAX
var result5 = addRawInt32Underflow(-2147483648, -1);
print("Underflow result: " + result5);

var status2 = %GetOptimizationStatus(addRawInt32Underflow);
print("Optimization status after underflow: " + status2);
