// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
// 基本类型（String/Number/Boolean/Symbol/BigInt）类型注入优化 - TS 版本
// 通过 eval 包装 V8 intrinsics，以保证 TS 可编译再生成 JS。
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
// 测试 1: String 类型
// ===================================
function twice_s(arg) {
    return arg + arg;
}
warmupAndOptimize(twice_s, 'warm');
const result1 = twice_s('test');
assertEquals('testtest', result1);
assertOptimized(twice_s);
// ===================================
// 测试 2: Number 类型 - 浮点数热身
// ===================================
function twice_f(arg) {
    return arg + arg;
}
warmupAndOptimize(twice_f, 1.5);
const result2 = twice_f(3.5);
assertEquals(7.0, result2);
assertOptimized(twice_f);
// ===================================
// 测试 2.1: Number 类型 - Smi 热身
// ===================================
function twice_smi(arg) {
    return arg + arg;
}
// 关键：用 Smi（小整数）热身
warmupAndOptimize(twice_smi, 10);
const resultSmi1 = twice_smi(5);
assertEquals(10, resultSmi1);
const resultSmi2 = twice_smi(100);
assertEquals(200, resultSmi2);
assertOptimized(twice_smi);
// ===================================
// 测试 2.2: Number 类型 - 混合热身
// ===================================
function twice_mixed(arg) {
    return arg + arg;
}
// 关键：交替使用 Smi 和 HeapNumber 热身
prepare(twice_mixed);
twice_mixed(10); // Smi
twice_mixed(1.5); // HeapNumber
twice_mixed(20); // Smi
optimize(twice_mixed);
const resultMixed1 = twice_mixed(5); // Smi
assertEquals(10, resultMixed1);
const resultMixed2 = twice_mixed(3.5); // HeapNumber
assertEquals(7.0, resultMixed2);
const resultMixed3 = twice_mixed(100); // Smi
assertEquals(200, resultMixed3);
assertOptimized(twice_mixed);
// ===================================
// 测试 3: Boolean 类型
// ===================================
function cal(f1, f2) {
    return f1 || f2;
}
warmupAndOptimize(cal, true, false);
assertEquals(true, cal(true, false));
assertEquals(true, cal(false, true));
assertEquals(false, cal(false, false));
assertOptimized(cal);
// ===================================
// 测试 4: Symbol 类型
// ===================================
function toStr(arg) {
    return arg.toString();
}
const sym1 = Symbol('test');
const sym2 = Symbol('hello');
warmupAndOptimize(toStr, sym1);
const result4 = toStr(Symbol('world'));
assertEquals('Symbol(world)', result4);
assertOptimized(toStr);
// ===================================
// 测试 5: BigInt 类型（小数值）
// ===================================
function addBigInt(a, b) {
    return a + b;
}
warmupAndOptimize(addBigInt, 1n, 2n);
const result5 = addBigInt(1000n, 2000n);
assertEquals(3000n, result5);
assertOptimized(addBigInt);
// ===================================
// 测试 6: BigInt 类型（大数值）
// ===================================
function addLargeBigInt(a, b) {
    return a + b;
}
const largeBigInt1 = 2n ** 65n; // 36893488147419103232n
warmupAndOptimize(addLargeBigInt, largeBigInt1, 1n);
const result6 = addLargeBigInt(largeBigInt1, 1n);
assertEquals(largeBigInt1 + 1n, result6);
assertOptimized(addLargeBigInt);
