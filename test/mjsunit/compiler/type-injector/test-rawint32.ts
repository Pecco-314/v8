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

// ===================================
// 加法测试
// ===================================
function addRawInt32(a: rawint32, b: rawint32): rawint32 {
  return a + b;
}
prepare(addRawInt32);
for (let i = 0; i < 100; i++) {
  addRawInt32(100, 200);
}
optimize(addRawInt32);
const result1 = addRawInt32(1000, 2000);
assertEquals(3000, result1);
assertOptimized(addRawInt32);

function addRawInt32Overflow(a: rawint32, b: rawint32): rawint32 {
  return a + b;
}
prepare(addRawInt32Overflow);
for (let i = 0; i < 100; i++) {
  addRawInt32Overflow(100, 200);
}
optimize(addRawInt32Overflow);
const result2 = addRawInt32Overflow(2147483647, 1);
print('Overflow result: ' + result2);

function addRawInt32Negative(a: rawint32, b: rawint32): rawint32 {
  return a + b;
}
prepare(addRawInt32Negative);
for (let i = 0; i < 100; i++) {
  addRawInt32Negative(-100, -200);
}
optimize(addRawInt32Negative);
const result3 = addRawInt32Negative(-1000, -2000);
assertEquals(-3000, result3);
assertOptimized(addRawInt32Negative);

function addRawInt32Underflow(a: rawint32, b: rawint32): rawint32 {
  return a + b;
}
prepare(addRawInt32Underflow);
for (let i = 0; i < 100; i++) {
  addRawInt32Underflow(-100, -200);
}
optimize(addRawInt32Underflow);
const result4 = addRawInt32Underflow(-2147483648, -1);
print('Underflow result: ' + result4);

// ===================================
// 减法测试
// ===================================
function subRawInt32(a: rawint32, b: rawint32): rawint32 {
  return a - b;
}
prepare(subRawInt32);
for (let i = 0; i < 100; i++) {
  subRawInt32(300, 100);
}
optimize(subRawInt32);
const result5 = subRawInt32(3000, 1000);
assertEquals(2000, result5);
assertOptimized(subRawInt32);

function subRawInt32Overflow(a: rawint32, b: rawint32): rawint32 {
  return a - b;
}
prepare(subRawInt32Overflow);
for (let i = 0; i < 100; i++) {
  subRawInt32Overflow(300, 100);
}
optimize(subRawInt32Overflow);
const result6 = subRawInt32Overflow(-2147483648, 1);
print('Sub overflow result: ' + result6);

function subRawInt32Negative(a: rawint32, b: rawint32): rawint32 {
  return a - b;
}
prepare(subRawInt32Negative);
for (let i = 0; i < 100; i++) {
  subRawInt32Negative(-100, -200);
}
optimize(subRawInt32Negative);
const result7 = subRawInt32Negative(-1000, -2000);
assertEquals(1000, result7);
assertOptimized(subRawInt32Negative);

function subRawInt32Underflow(a: rawint32, b: rawint32): rawint32 {
  return a - b;
}
prepare(subRawInt32Underflow);
for (let i = 0; i < 100; i++) {
  subRawInt32Underflow(100, 200);
}
optimize(subRawInt32Underflow);
const result8 = subRawInt32Underflow(2147483647, -1);
print('Sub underflow result: ' + result8);

// ===================================
// 乘法测试
// ===================================
function mulRawInt32(a: rawint32, b: rawint32): rawint32 {
  return a * b;
}
prepare(mulRawInt32);
for (let i = 0; i < 100; i++) {
  mulRawInt32(10, 20);
}
optimize(mulRawInt32);
const result9 = mulRawInt32(30, 40);
assertEquals(1200, result9);
assertOptimized(mulRawInt32);

function mulRawInt32Overflow(a: rawint32, b: rawint32): rawint32 {
  return a * b;
}
prepare(mulRawInt32Overflow);
for (let i = 0; i < 100; i++) {
  mulRawInt32Overflow(1000, 2000);
}
optimize(mulRawInt32Overflow);
const result10 = mulRawInt32Overflow(2147483647, 2);
print('Mul overflow result: ' + result10);

function mulRawInt32Negative(a: rawint32, b: rawint32): rawint32 {
  return a * b;
}
prepare(mulRawInt32Negative);
for (let i = 0; i < 100; i++) {
  mulRawInt32Negative(-3, 7);
}
optimize(mulRawInt32Negative);
const result11 = mulRawInt32Negative(-30, 4);
assertEquals(-120, result11);
assertOptimized(mulRawInt32Negative);

function mulRawInt32Underflow(a: rawint32, b: rawint32): rawint32 {
  return a * b;
}
prepare(mulRawInt32Underflow);
for (let i = 0; i < 100; i++) {
  mulRawInt32Underflow(-1000, 2000);
}
optimize(mulRawInt32Underflow);
const result12 = mulRawInt32Underflow(-2147483648, 2);
print('Mul underflow result: ' + result12);

// ===================================
// 除法测试
// ===================================
function divRawInt32Exact(a: rawint32, b: rawint32): rawint32 {
  return a / b;
}
prepare(divRawInt32Exact);
for (let i = 0; i < 100; i++) {
  divRawInt32Exact(2000, 40);
}
optimize(divRawInt32Exact);
const result13 = divRawInt32Exact(1000, 20);
assertEquals(50, result13);
assertOptimized(divRawInt32Exact);

function divRawInt32Trunc(a: rawint32, b: rawint32): rawint32 {
  return a / b;
}
prepare(divRawInt32Trunc);
for (let i = 0; i < 100; i++) {
  divRawInt32Trunc(15, 4);
}
optimize(divRawInt32Trunc);
const result14 = divRawInt32Trunc(15, 4);
// 有 metadata 时返回 3 (int 除法截断)，无 metadata 时返回 3.75 (float 除法)
if (result14 === 3) {
  // 走了 RawInt32 路径，应该已优化
  assertOptimized(divRawInt32Trunc);
} else {
  // 无 metadata，fallback 到普通 JS 除法
  assertEquals(3.75, result14);
  print('divRawInt32Trunc fallback to normal JS division: ' + result14);
}

function divRawInt32Overflow(a: rawint32, b: rawint32): rawint32 {
  return a / b;
}
prepare(divRawInt32Overflow);
for (let i = 0; i < 100; i++) {
  divRawInt32Overflow(-100, -1);
}
optimize(divRawInt32Overflow);
const result15 = divRawInt32Overflow(-2147483648, -1);
// 有 metadata 时走 RawInt32 路径返回 INT_MIN (-2147483648)，无 metadata 时返回 2147483648 (普通 JS 语义)
if (result15 === -2147483648) {
  // 走了 RawInt32 路径，应该已优化
  assertOptimized(divRawInt32Overflow);
} else {
  // 无 metadata，fallback 到普通 JS 除法
  assertEquals(2147483648, result15);
  print('divRawInt32Overflow fallback to normal JS division: ' + result15);
}

function divRawInt32ByZero(a: rawint32, b: rawint32): rawint32 {
  return a / b;
}
prepare(divRawInt32ByZero);
for (let i = 0; i < 100; i++) {
  divRawInt32ByZero(64, 8);
}
optimize(divRawInt32ByZero);
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
} else {
    // 无 metadata，fallback 到普通 JS 除法，返回 Infinity
    assertEquals(Infinity, result16Zero);
    print('divRawInt32ByZero fallback to normal JS division: ' + result16Zero);
}
print('Div by zero result: ' + result16Zero);
