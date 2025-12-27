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

// ===================================
// 乘法测试
// ===================================

// 测试 9: 正常乘法（不溢出）
function mulRawInt32(a, b) {
    return a * b;
}

%PrepareFunctionForOptimization(mulRawInt32);

for (var i = 0; i < 100; i++) {
    mulRawInt32(10, 20);
}

%OptimizeFunctionOnNextCall(mulRawInt32);

var result9 = mulRawInt32(30, 40);
assertEquals(1200, result9);
assertOptimized(mulRawInt32);

// 测试 10: 溢出情况（UB - 应该回绕）
function mulRawInt32Overflow(a, b) {
    return a * b;
}

%PrepareFunctionForOptimization(mulRawInt32Overflow);

for (var i = 0; i < 100; i++) {
    mulRawInt32Overflow(1000, 2000);
}

%OptimizeFunctionOnNextCall(mulRawInt32Overflow);

var result10 = mulRawInt32Overflow(2147483647, 2);
print("Mul overflow result: " + result10);

// 测试 11: 负数乘法
function mulRawInt32Negative(a, b) {
    return a * b;
}

%PrepareFunctionForOptimization(mulRawInt32Negative);

for (var i = 0; i < 100; i++) {
    mulRawInt32Negative(-3, 7);
}

%OptimizeFunctionOnNextCall(mulRawInt32Negative);

var result11 = mulRawInt32Negative(-30, 4);
assertEquals(-120, result11);
assertOptimized(mulRawInt32Negative);

// 测试 12: 下溢情况（UB - 应该回绕）
function mulRawInt32Underflow(a, b) {
    return a * b;
}

%PrepareFunctionForOptimization(mulRawInt32Underflow);

for (var i = 0; i < 100; i++) {
    mulRawInt32Underflow(-1000, 2000);
}

%OptimizeFunctionOnNextCall(mulRawInt32Underflow);

var result12 = mulRawInt32Underflow(-2147483648, 2);
print("Mul underflow result: " + result12);

// ===================================
// 除法测试
// ===================================

// 测试 13: 精确整除
function divRawInt32Exact(a, b) {
    return a / b;
}

%PrepareFunctionForOptimization(divRawInt32Exact);

for (var i = 0; i < 100; i++) {
    divRawInt32Exact(2000, 40);
}

%OptimizeFunctionOnNextCall(divRawInt32Exact);

var result13 = divRawInt32Exact(1000, 20);
assertEquals(50, result13);
assertOptimized(divRawInt32Exact);

// 测试 14: 非整除（向零截断）
function divRawInt32Trunc(a, b) {
    return a / b;
}

%PrepareFunctionForOptimization(divRawInt32Trunc);

for (var i = 0; i < 100; i++) {
    divRawInt32Trunc(15, 4);
}

%OptimizeFunctionOnNextCall(divRawInt32Trunc);

var result14 = divRawInt32Trunc(15, 4);
assertEquals(3, result14);
assertOptimized(divRawInt32Trunc);

// 测试 15: 溢出路径（INT_MIN / -1）
function divRawInt32Overflow(a, b) {
    return a / b;
}

%PrepareFunctionForOptimization(divRawInt32Overflow);

for (var i = 0; i < 100; i++) {
    divRawInt32Overflow(-100, -1);
}

%OptimizeFunctionOnNextCall(divRawInt32Overflow);

var result15 = divRawInt32Overflow(-2147483648, -1);
// 硬件 INT32 溢出回绕，当前流水线返回 INT_MIN
assertEquals(-2147483648, result15);
assertOptimized(divRawInt32Overflow);

// 测试 16: 除以 0（兜底行为观察）
function divRawInt32ByZero(a, b) {
    return a / b;
}

%PrepareFunctionForOptimization(divRawInt32ByZero);

for (var i = 0; i < 100; i++) {
    divRawInt32ByZero(64, 8);
}

%OptimizeFunctionOnNextCall(divRawInt32ByZero);

var result16 = divRawInt32ByZero(64, 8);
assertEquals(8, result16);
assertOptimized(divRawInt32ByZero);

var result16Zero = divRawInt32ByZero(123, 0);
print("Div by zero result: " + result16Zero);
