// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
// 基本类型（String/Number/Boolean/Symbol/BigInt）类型注入优化 - TS 版本
// 通过 eval 包装 V8 intrinsics，以保证 TS 可编译再生成 JS。

function prepare(fn: Function): void {
  eval('%PrepareFunctionForOptimization(' + fn.name + ')');
}

function optimize(fn: Function): void {
  eval('%OptimizeFunctionOnNextCall(' + fn.name + ')');
}

// ===================================
// 测试 1: String 类型
// ===================================
function twice_s(arg: string): string {
  return arg + arg;
}

prepare(twice_s);
for (let i = 0; i < 100; i++) {
  twice_s(i.toString());
}
optimize(twice_s);
const result1 = twice_s('test');
assertEquals('testtest', result1);
assertOptimized(twice_s);

// ===================================
// 测试 2: Number 类型
// ===================================
function twice_f(arg: number): number {
  return arg + arg;
}

prepare(twice_f);
for (let i = 0; i < 100; i++) {
  twice_f(i + 0.5);
}
optimize(twice_f);
const result2 = twice_f(3.5);
assertEquals(7.0, result2);
assertOptimized(twice_f);

// ===================================
// 测试 3: Boolean 类型
// ===================================
function cal(f1: boolean, f2: boolean): boolean {
  return f1 || f2;
}

prepare(cal);
for (let i = 0; i < 100; i++) {
  cal(i % 2 === 0, i % 3 === 0);
}
optimize(cal);
assertEquals(true, cal(true, false));
assertEquals(true, cal(false, true));
assertEquals(false, cal(false, false));
assertOptimized(cal);

// ===================================
// 测试 4: Symbol 类型
// ===================================
function toStr(arg: symbol): string {
  return arg.toString();
}

prepare(toStr);
const sym1: symbol = Symbol('test');
const sym2: symbol = Symbol('hello');
for (let i = 0; i < 100; i++) {
  toStr(i % 2 === 0 ? sym1 : sym2);
}
optimize(toStr);
const result4 = toStr(Symbol('world'));
assertEquals('Symbol(world)', result4);
assertOptimized(toStr);

// ===================================
// 测试 5: BigInt 类型（小数值）
// ===================================
function addBigInt(a: bigint, b: bigint): bigint {
  return a + b;
}

prepare(addBigInt);
for (let i = 0n; i < 100n; i++) {
  addBigInt(i, i + 1n);
}
optimize(addBigInt);
const result5 = addBigInt(1000n, 2000n);
assertEquals(3000n, result5);
assertOptimized(addBigInt);

// ===================================
// 测试 6: BigInt 类型（大数值）
// ===================================
function addLargeBigInt(a: bigint, b: bigint): bigint {
  return a + b;
}

prepare(addLargeBigInt);
const largeBigInt1 = 2n ** 65n; // 36893488147419103232n
for (let i = 0n; i < 100n; i++) {
  addLargeBigInt(largeBigInt1, i);
}
optimize(addLargeBigInt);
const result6 = addLargeBigInt(largeBigInt1, 1n);
assertEquals(largeBigInt1 + 1n, result6);
assertOptimized(addLargeBigInt);
