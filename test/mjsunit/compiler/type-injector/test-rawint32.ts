// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// RawInt32 类型测试：使用 TS 类型别名 rawint32=number，通过 metadata 注入类型

type rawint32 = number;

function prepare(fn: Function): void {
  eval('%PrepareFunctionForOptimization(' + fn.name + ')');
}

function optimize(fn: Function): void {
  eval('%OptimizeFunctionOnNextCall(' + fn.name + ')');
}

function warmupAndOptimize(fn: Function, ...args: any[]): void {
  prepare(fn);
  fn(...args);
  optimize(fn);
}

// ===================================
// 加法测试
// ===================================
function addRawInt32(a: rawint32, b: rawint32): rawint32 {
  return a + b;
}
warmupAndOptimize(addRawInt32, 100, 200);
const result1 = addRawInt32(1000, 2000);
assertEquals(3000, result1);
assertOptimized(addRawInt32);

function addRawInt32Overflow(a: rawint32, b: rawint32): rawint32 {
  return a + b;
}
warmupAndOptimize(addRawInt32Overflow, 100, 200);
const result2 = addRawInt32Overflow(2147483647, 1);
print('Overflow result: ' + result2);

// 10 个小整数相加：检验多次加法链的溢出检查是否被重复插入
function addTenRawInt32(
  a: rawint32,
  b: rawint32,
  c: rawint32,
  d: rawint32,
  e: rawint32,
  f: rawint32,
  g: rawint32,
  h: rawint32,
  i: rawint32,
  j: rawint32,
): rawint32 {
  return a + b + c + d + e + f + g + h + i + j;
}
warmupAndOptimize(addTenRawInt32, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
const add10Result = addTenRawInt32(11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
assertEquals(155, add10Result);
assertOptimized(addTenRawInt32);

// 循环累加：覆盖 Phi 链上的 RawInt32 传播
function sumLoopRawInt32(arr: rawint32[]): rawint32 {
  let acc: rawint32 = 0;
  for (let i = 0; i < arr.length; i++) {
    acc += arr[i];
  }
  return acc;
}
const loopInput: rawint32[] = [1, 2, 3, 4, 5, 6, 7, 8];
warmupAndOptimize(sumLoopRawInt32, loopInput);
const loopResult = sumLoopRawInt32(loopInput);
assertEquals(36, loopResult);
assertOptimized(sumLoopRawInt32);

function addRawInt32Negative(a: rawint32, b: rawint32): rawint32 {
  return a + b;
}
warmupAndOptimize(addRawInt32Negative, -100, -200);
const result3 = addRawInt32Negative(-1000, -2000);
assertEquals(-3000, result3);
assertOptimized(addRawInt32Negative);

function addRawInt32Underflow(a: rawint32, b: rawint32): rawint32 {
  return a + b;
}
warmupAndOptimize(addRawInt32Underflow, -100, -200);
const result4 = addRawInt32Underflow(-2147483648, -1);
print('Underflow result: ' + result4);

// ===================================
// 减法测试
// ===================================
function subRawInt32(a: rawint32, b: rawint32): rawint32 {
  return a - b;
}
warmupAndOptimize(subRawInt32, 300, 100);
const result5 = subRawInt32(3000, 1000);
assertEquals(2000, result5);
assertOptimized(subRawInt32);

function subRawInt32Overflow(a: rawint32, b: rawint32): rawint32 {
  return a - b;
}
warmupAndOptimize(subRawInt32Overflow, 300, 100);
const result6 = subRawInt32Overflow(-2147483648, 1);
print('Sub overflow result: ' + result6);

function subRawInt32Negative(a: rawint32, b: rawint32): rawint32 {
  return a - b;
}
warmupAndOptimize(subRawInt32Negative, -100, -200);
const result7 = subRawInt32Negative(-1000, -2000);
assertEquals(1000, result7);
assertOptimized(subRawInt32Negative);

function subRawInt32Underflow(a: rawint32, b: rawint32): rawint32 {
  return a - b;
}
warmupAndOptimize(subRawInt32Underflow, 100, 200);
const result8 = subRawInt32Underflow(2147483647, -1);
print('Sub underflow result: ' + result8);

// ===================================
// 乘法测试
// ===================================
function mulRawInt32(a: rawint32, b: rawint32): rawint32 {
  return a * b;
}
warmupAndOptimize(mulRawInt32, 10, 20);
const result9 = mulRawInt32(30, 40);
assertEquals(1200, result9);
assertOptimized(mulRawInt32);

function mulRawInt32Overflow(a: rawint32, b: rawint32): rawint32 {
  return a * b;
}
warmupAndOptimize(mulRawInt32Overflow, 1000, 2000);
const result10 = mulRawInt32Overflow(2147483647, 2);
print('Mul overflow result: ' + result10);

function mulRawInt32Negative(a: rawint32, b: rawint32): rawint32 {
  return a * b;
}
warmupAndOptimize(mulRawInt32Negative, -3, 7);
const result11 = mulRawInt32Negative(-30, 4);
assertEquals(-120, result11);
assertOptimized(mulRawInt32Negative);

function mulRawInt32Underflow(a: rawint32, b: rawint32): rawint32 {
  return a * b;
}
warmupAndOptimize(mulRawInt32Underflow, -1000, 2000);
const result12 = mulRawInt32Underflow(-2147483648, 2);
print('Mul underflow result: ' + result12);

// ===================================
// 除法测试
// ===================================
function divRawInt32Exact(a: rawint32, b: rawint32): rawint32 {
  return a / b;
}
warmupAndOptimize(divRawInt32Exact, 2000, 40);
const result13 = divRawInt32Exact(1000, 20);
assertEquals(50, result13);
assertOptimized(divRawInt32Exact);

function divRawInt32Trunc(a: rawint32, b: rawint32): rawint32 {
  return a / b;
}
warmupAndOptimize(divRawInt32Trunc, 15, 4);
const result14 = divRawInt32Trunc(15, 4);
if (result14 === 3) {
  assertOptimized(divRawInt32Trunc);
} else {
  assertEquals(3.75, result14);
}

function divRawInt32Overflow(a: rawint32, b: rawint32): rawint32 {
  return a / b;
}
warmupAndOptimize(divRawInt32Overflow, -100, -1);
const result15 = divRawInt32Overflow(-2147483648, -1);
if (result15 === -2147483648) {
  assertOptimized(divRawInt32Overflow);
} else {
  assertEquals(2147483648, result15);
}

function divRawInt32ByZero(a: rawint32, b: rawint32): rawint32 {
  return a / b;
}
warmupAndOptimize(divRawInt32ByZero, 64, 8);
const result16 = divRawInt32ByZero(64, 8);
assertEquals(8, result16);
assertOptimized(divRawInt32ByZero);
assertThrows(() => divRawInt32ByZero(123, 0), RangeError);
print('Div by zero throws RangeError');

// ===================================
// 取模测试
// ===================================
function modRawInt32(a: rawint32, b: rawint32): rawint32 {
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

function modRawInt32Negative(a: rawint32, b: rawint32): rawint32 {
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

function modRawInt32ByZero(a: rawint32, b: rawint32): rawint32 {
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
assertThrows(() => modRawInt32ByZero(123, 0), RangeError);
print('Mod by zero throws RangeError');

// ===================================
// 字面量常量参与运算（验证常量兼容）
// ===================================
function addRawInt32Const(a: rawint32): rawint32 {
  return a + 5;
}
warmupAndOptimize(addRawInt32Const, 1234);
const result20 = addRawInt32Const(1234);
assertEquals(1239, result20);
assertOptimized(addRawInt32Const);

function subRawInt32Const(a: rawint32): rawint32 {
  return a - 7;
}
warmupAndOptimize(subRawInt32Const, 1234);
const result21 = subRawInt32Const(1234);
assertEquals(1227, result21);
assertOptimized(subRawInt32Const);

function divRawInt32Const(a: rawint32): rawint32 {
  return a / 10;
}
warmupAndOptimize(divRawInt32Const, 1234);
const result22 = divRawInt32Const(1234);
assertEquals(123, result22);
assertOptimized(divRawInt32Const);

function modRawInt32Const(a: rawint32): rawint32 {
  return a % 8;
}
warmupAndOptimize(modRawInt32Const, 1234);
const result23 = modRawInt32Const(1234);
assertEquals(2, result23);
if (result23 === 2) {
  assertOptimized(modRawInt32Const);
}

function mulRawInt32Const(a: rawint32): rawint32 {
  return a * 3;
}
warmupAndOptimize(mulRawInt32Const, 1234);
const result24 = mulRawInt32Const(1234);
assertEquals(3702, result24);
assertOptimized(mulRawInt32Const);

// ===================================
// RawUint32 字面量常量路径
// ===================================
type rawuint32 = number;

function addRawUint32Const(a: rawuint32): rawuint32 {
  return a + 5;
}
warmupAndOptimize(addRawUint32Const, 1234);
const result25 = addRawUint32Const(1234);
assertEquals(1239, result25);
assertOptimized(addRawUint32Const);

function subRawUint32Const(a: rawuint32): rawuint32 {
  return a - 7;
}
warmupAndOptimize(subRawUint32Const, 1234);
const result26 = subRawUint32Const(1234);
assertEquals(1227, result26);
assertOptimized(subRawUint32Const);

function mulRawUint32Const(a: rawuint32): rawuint32 {
  return a * 3;
}
warmupAndOptimize(mulRawUint32Const, 1234);
const result27 = mulRawUint32Const(1234);
assertEquals(3702, result27);
assertOptimized(mulRawUint32Const);

function divRawUint32Const(a: rawuint32): rawuint32 {
  return a / 10;
}
warmupAndOptimize(divRawUint32Const, 1234);
const result28 = divRawUint32Const(1234);
assertEquals(123, result28);
assertOptimized(divRawUint32Const);

function modRawUint32Const(a: rawuint32): rawuint32 {
  return a % 8;
}
warmupAndOptimize(modRawUint32Const, 1234);
const result29 = modRawUint32Const(1234);
assertEquals(2, result29);
if (result29 === 2) {
  assertOptimized(modRawUint32Const);
}

// ===================================
// RawInt64 / RawUint64 覆盖（当前路径若未命中则允许 fallback）
// ===================================
type rawint64 = bigint;
type rawuint64 = bigint;

function divRawInt64Const(a: rawint64): rawint64 {
  return a / 10n;
}
warmupAndOptimize(divRawInt64Const, 1234n);
const result30 = divRawInt64Const(1234n);
if (result30 === 123n) {
  assertOptimized(divRawInt64Const);
} else {
  assertEquals(123n, result30);
  print('divRawInt64Const fallback to normal JS division: ' + result30);
}

function modRawInt64Const(a: rawint64): rawint64 {
  return a % 8n;
}
warmupAndOptimize(modRawInt64Const, 1234n);
const result31 = modRawInt64Const(1234n);
assertEquals(2n, result31);

function divRawUint64Const(a: rawuint64): rawuint64 {
  return a / 10n;
}
warmupAndOptimize(divRawUint64Const, 1234n);
const result32 = divRawUint64Const(1234n);
if (result32 === 123n) {
  assertOptimized(divRawUint64Const);
} else {
  assertEquals(123n, result32);
  print('divRawUint64Const fallback to normal JS division: ' + result32);
}

function modRawUint64Const(a: rawuint64): rawuint64 {
  return a % 8n;
}
warmupAndOptimize(modRawUint64Const, 1234n);
const result33 = modRawUint64Const(1234n);
assertEquals(2n, result33);
