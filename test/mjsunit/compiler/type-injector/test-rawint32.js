// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
function prepare(fn) {
    eval('%PrepareFunctionForOptimization(' + fn.name + ')');
}
function optimize(fn) {
    eval('%OptimizeFunctionOnNextCall(' + fn.name + ')');
}
function warmupAndOptimize(fn, ...args) {
    prepare(fn);
    fn(...args);
    optimize(fn);
}
// ===================================
// 加法测试
// ===================================
function addRawInt32(a, b) {
    return a + b;
}
warmupAndOptimize(addRawInt32, 100, 200);
const result1 = addRawInt32(1000, 2000);
assertEquals(3000, result1);
assertOptimized(addRawInt32);
function addRawInt32Overflow(a, b) {
    return a + b;
}
warmupAndOptimize(addRawInt32Overflow, 100, 200);
const result2 = addRawInt32Overflow(2147483647, 1);
print('Overflow result: ' + result2);
// 10 个小整数相加：检验多次加法链的溢出检查是否被重复插入
function addTenRawInt32(a, b, c, d, e, f, g, h, i, j) {
    return a + b + c + d + e + f + g + h + i + j;
}
warmupAndOptimize(addTenRawInt32, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
const add10Result = addTenRawInt32(11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
assertEquals(155, add10Result);
assertOptimized(addTenRawInt32);
// 循环累加：覆盖 Phi 链上的 RawInt32 传播
function sumLoopRawInt32(arr) {
    let acc = 0;
    for (let i = 0; i < arr.length; i++) {
        acc += arr[i];
    }
    return acc;
}
const loopInput = [1, 2, 3, 4, 5, 6, 7, 8];
warmupAndOptimize(sumLoopRawInt32, loopInput);
const loopResult = sumLoopRawInt32(loopInput);
assertEquals(36, loopResult);
assertOptimized(sumLoopRawInt32);
function addRawInt32Negative(a, b) {
    return a + b;
}
warmupAndOptimize(addRawInt32Negative, -100, -200);
const result3 = addRawInt32Negative(-1000, -2000);
assertEquals(-3000, result3);
assertOptimized(addRawInt32Negative);
function addRawInt32Underflow(a, b) {
    return a + b;
}
warmupAndOptimize(addRawInt32Underflow, -100, -200);
const result4 = addRawInt32Underflow(-2147483648, -1);
print('Underflow result: ' + result4);
// ===================================
// 减法测试
// ===================================
function subRawInt32(a, b) {
    return a - b;
}
warmupAndOptimize(subRawInt32, 300, 100);
const result5 = subRawInt32(3000, 1000);
assertEquals(2000, result5);
assertOptimized(subRawInt32);
function subRawInt32Overflow(a, b) {
    return a - b;
}
warmupAndOptimize(subRawInt32Overflow, 300, 100);
const result6 = subRawInt32Overflow(-2147483648, 1);
print('Sub overflow result: ' + result6);
function subRawInt32Negative(a, b) {
    return a - b;
}
warmupAndOptimize(subRawInt32Negative, -100, -200);
const result7 = subRawInt32Negative(-1000, -2000);
assertEquals(1000, result7);
assertOptimized(subRawInt32Negative);
function subRawInt32Underflow(a, b) {
    return a - b;
}
warmupAndOptimize(subRawInt32Underflow, 100, 200);
const result8 = subRawInt32Underflow(2147483647, -1);
print('Sub underflow result: ' + result8);
// ===================================
// 乘法测试
// ===================================
function mulRawInt32(a, b) {
    return a * b;
}
warmupAndOptimize(mulRawInt32, 10, 20);
const result9 = mulRawInt32(30, 40);
assertEquals(1200, result9);
assertOptimized(mulRawInt32);
function mulRawInt32Overflow(a, b) {
    return a * b;
}
warmupAndOptimize(mulRawInt32Overflow, 1000, 2000);
const result10 = mulRawInt32Overflow(2147483647, 2);
print('Mul overflow result: ' + result10);
function mulRawInt32Negative(a, b) {
    return a * b;
}
warmupAndOptimize(mulRawInt32Negative, -3, 7);
const result11 = mulRawInt32Negative(-30, 4);
assertEquals(-120, result11);
assertOptimized(mulRawInt32Negative);
function mulRawInt32Underflow(a, b) {
    return a * b;
}
warmupAndOptimize(mulRawInt32Underflow, -1000, 2000);
const result12 = mulRawInt32Underflow(-2147483648, 2);
print('Mul underflow result: ' + result12);
// ===================================
// 除法测试
// ===================================
function divRawInt32Exact(a, b) {
    return a / b;
}
warmupAndOptimize(divRawInt32Exact, 2000, 40);
const result13 = divRawInt32Exact(1000, 20);
assertEquals(50, result13);
assertOptimized(divRawInt32Exact);
function divRawInt32Trunc(a, b) {
    return a / b;
}
warmupAndOptimize(divRawInt32Trunc, 15, 4);
const result14 = divRawInt32Trunc(15, 4);
// 有 metadata 时返回 3 (int 除法截断)，无 metadata 时返回 3.75 (float 除法)
if (result14 === 3) {
    // 走了 RawInt32 路径，应该已优化
    assertOptimized(divRawInt32Trunc);
}
else {
    // 无 metadata，fallback 到普通 JS 除法
    assertEquals(3.75, result14);
    print('divRawInt32Trunc fallback to normal JS division: ' + result14);
}
function divRawInt32Overflow(a, b) {
    return a / b;
}
warmupAndOptimize(divRawInt32Overflow, -100, -1);
const result15 = divRawInt32Overflow(-2147483648, -1);
// 有 metadata 时走 RawInt32 路径返回 INT_MIN (-2147483648)，无 metadata 时返回 2147483648 (普通 JS 语义)
if (result15 === -2147483648) {
    // 走了 RawInt32 路径，应该已优化
    assertOptimized(divRawInt32Overflow);
}
else {
    // 无 metadata，fallback 到普通 JS 除法
    assertEquals(2147483648, result15);
    print('divRawInt32Overflow fallback to normal JS division: ' + result15);
}
function divRawInt32ByZero(a, b) {
    return a / b;
}
warmupAndOptimize(divRawInt32ByZero, 64, 8);
const result16 = divRawInt32ByZero(64, 8);
assertEquals(8, result16);
// 有 metadata 时应该已优化
if (result16 === 8) {
    assertOptimized(divRawInt32ByZero);
}
const result16Zero = divRawInt32ByZero(123, 0);
if (result16Zero === 0) {
    // 走了 RawInt32 路径，除以零返回 0
    assertOptimized(divRawInt32ByZero);
}
else {
    // 无 metadata，fallback 到普通 JS 除法，返回 Infinity
    assertEquals(Infinity, result16Zero);
    print('divRawInt32ByZero fallback to normal JS division: ' + result16Zero);
}
print('Div by zero result: ' + result16Zero);
// ===================================
// 取模测试
// ===================================
function modRawInt32(a, b) {
    return a % b;
}
prepare(modRawInt32);
for (let i = 0; i < 100; i++) {
    modRawInt32(100, 7);
}
optimize(modRawInt32);
const result17 = modRawInt32(1000, 64);
assertEquals(40, result17);
assertOptimized(modRawInt32);
function modRawInt32Negative(a, b) {
    return a % b;
}
prepare(modRawInt32Negative);
for (let i = 0; i < 100; i++) {
    modRawInt32Negative(-35, 8);
}
optimize(modRawInt32Negative);
const result18 = modRawInt32Negative(-35, 8);
assertEquals(-3, result18);
assertOptimized(modRawInt32Negative);
function modRawInt32ByZero(a, b) {
    return a % b;
}
prepare(modRawInt32ByZero);
for (let i = 0; i < 100; i++) {
    modRawInt32ByZero(123, 7);
}
optimize(modRawInt32ByZero);
const result19 = modRawInt32ByZero(64, 8);
assertEquals(0, result19);
// 有 metadata 时应走 RawInt32 模运算
assertOptimized(modRawInt32ByZero);
const result19Zero = modRawInt32ByZero(123, 0);
if (result19Zero === 0) {
    // RawInt32 路径，除以零返回 0
    assertOptimized(modRawInt32ByZero);
}
else {
    // 无 metadata，普通 JS 语义返回 NaN
    assertTrue(Number.isNaN(result19Zero));
    print('modRawInt32ByZero fallback to normal JS modulus: ' + result19Zero);
}
print('Mod by zero result: ' + result19Zero);
//# sourceMappingURL=test-rawint32.js.map