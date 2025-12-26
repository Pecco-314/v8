// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// 测试：RawInt32 类型 - 无溢出检查的 int32 运算（加法和减法）

// ===================================
// 加法测试
// ===================================

// 测试 1: 正常加法（不溢出）
function addRawInt32(a, b) {
    return a + b;
}

%PrepareFunctionForOptimization(addRawInt32);

for (var i = 0; i < 100; i++) {
    addRawInt32(100, 200);
}

%OptimizeFunctionOnNextCall(addRawInt32);

var result1 = addRawInt32(1000, 2000);
assertEquals(3000, result1);
assertOptimized(addRawInt32);

// 测试 2: 溢出情况（UB - 应该回绕）
function addRawInt32Overflow(a, b) {
    return a + b;
}

%PrepareFunctionForOptimization(addRawInt32Overflow);

for (var i = 0; i < 100; i++) {
    addRawInt32Overflow(100, 200);
}

%OptimizeFunctionOnNextCall(addRawInt32Overflow);

var result2 = addRawInt32Overflow(2147483647, 1);
print("Overflow result: " + result2);

// 测试 3: 负数加法
function addRawInt32Negative(a, b) {
    return a + b;
}

%PrepareFunctionForOptimization(addRawInt32Negative);

for (var i = 0; i < 100; i++) {
    addRawInt32Negative(-100, -200);
}

%OptimizeFunctionOnNextCall(addRawInt32Negative);

var result3 = addRawInt32Negative(-1000, -2000);
assertEquals(-3000, result3);
assertOptimized(addRawInt32Negative);

// 测试 4: 负数溢出（下溢）
function addRawInt32Underflow(a, b) {
    return a + b;
}

%PrepareFunctionForOptimization(addRawInt32Underflow);

for (var i = 0; i < 100; i++) {
    addRawInt32Underflow(-100, -200);
}

%OptimizeFunctionOnNextCall(addRawInt32Underflow);

var result4 = addRawInt32Underflow(-2147483648, -1);
print("Underflow result: " + result4);

// ===================================
// 减法测试
// ===================================

// 测试 5: 正常减法（不溢出）
function subRawInt32(a, b) {
    return a - b;
}

%PrepareFunctionForOptimization(subRawInt32);

for (var i = 0; i < 100; i++) {
    subRawInt32(300, 100);
}

%OptimizeFunctionOnNextCall(subRawInt32);

var result5 = subRawInt32(3000, 1000);
assertEquals(2000, result5);
assertOptimized(subRawInt32);

// 测试 6: 溢出情况（UB - 应该回绕）
function subRawInt32Overflow(a, b) {
    return a - b;
}

%PrepareFunctionForOptimization(subRawInt32Overflow);

for (var i = 0; i < 100; i++) {
    subRawInt32Overflow(300, 100);
}

%OptimizeFunctionOnNextCall(subRawInt32Overflow);

var result6 = subRawInt32Overflow(-2147483648, 1);
print("Sub overflow result: " + result6);

// 测试 7: 负数减法
function subRawInt32Negative(a, b) {
    return a - b;
}

%PrepareFunctionForOptimization(subRawInt32Negative);

for (var i = 0; i < 100; i++) {
    subRawInt32Negative(-100, -200);
}

%OptimizeFunctionOnNextCall(subRawInt32Negative);

var result7 = subRawInt32Negative(-1000, -2000);
assertEquals(1000, result7);
assertOptimized(subRawInt32Negative);

// 测试 8: 正数溢出（上溢）
function subRawInt32Underflow(a, b) {
    return a - b;
}

%PrepareFunctionForOptimization(subRawInt32Underflow);

for (var i = 0; i < 100; i++) {
    subRawInt32Underflow(100, 200);
}

%OptimizeFunctionOnNextCall(subRawInt32Underflow);

var result8 = subRawInt32Underflow(2147483647, -1);
print("Sub underflow result: " + result8);
