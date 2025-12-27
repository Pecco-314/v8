// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_builtin_type_table
// 测试：内建函数类型表优化
// 通过 Builtin ID 识别内建函数的参数和返回值类型
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
// 测试 1: Number.prototype.toString()
// ===================================
// 预期优化：
// - 识别 num.toString() 返回 string
// - 消除后续的 CheckString
function processNumber(num) {
    const str = num.toString();
    return str + '!';
}
warmupAndOptimize(processNumber, 42);
const result1 = processNumber(123);
assertEquals('123!', result1);
assertOptimized(processNumber);
// ===================================
// 测试 2: Object.prototype.toString()
// ===================================
// 预期优化：
// - 验证通用性（所有对象的 toString 都返回 string）
function processAny(obj) {
    const str = obj.toString();
    return str.length;
}
warmupAndOptimize(processAny, {});
const result2 = processAny({});
assertTrue(result2 > 0);
assertOptimized(processAny);
// ===================================
// 测试 3: Number.prototype.toFixed()
// ===================================
// 预期优化：
// - 识别 num.toFixed(digits) 返回 string
// - 消除后续的 CheckString
function testToFixed(num, digits) {
    const result = num.toFixed(digits);
    return result + ' units';
}
warmupAndOptimize(testToFixed, 3.14159, 2);
const result3 = testToFixed(3.14159, 2);
assertEquals('3.14 units', result3);
assertOptimized(testToFixed);
// ===================================
// 测试 4: String.prototype.repeat()
// ===================================
// 预期优化：
// - 识别 str.repeat(count) 返回 string
// - 消除后续的 CheckString
function testRepeat(str, count) {
    const result = str.repeat(count);
    return result.length;
}
warmupAndOptimize(testRepeat, 'ab', 3);
const result4 = testRepeat('ab', 3);
assertEquals(6, result4);
assertOptimized(testRepeat);
