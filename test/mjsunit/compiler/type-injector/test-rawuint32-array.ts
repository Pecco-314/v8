// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// 目标：用户只写 rawuint32 + 普通数组，编译器应避免把已确定非 undefined 的元素访问
// 走成 holey-double/hole 检查链路。

type rawuint32 = number;

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

function makeFilledArray(size: rawuint32): rawuint32[] {
  const arr: rawuint32[] = new Array(size);
  let seed: rawuint32 = 1;
  for (let i = 0; i < size; i++) {
    seed = (seed * 1664525 + 1013904223) >>> 0;
    arr[i] = seed;
  }
  return arr;
}

function sumRawArray(arr: rawuint32[]): rawuint32 {
  let acc: rawuint32 = 0;
  for (let i = 0; i < arr.length; i++) {
    acc = (acc + arr[i]) >>> 0;
  }
  return acc;
}

function bucketRawArray(arr: rawuint32[], shift: rawuint32): rawuint32 {
  let acc: rawuint32 = 0;
  for (let i = 0; i < arr.length; i++) {
    const bucket: rawuint32 = (arr[i] >>> shift) & 255;
    acc = (acc + bucket) >>> 0;
  }
  return acc;
}

const warm = makeFilledArray(256);
warmupAndOptimize(sumRawArray, warm);
const s = sumRawArray(warm);
assertTrue(s >= 0);
assertOptimized(sumRawArray);

warmupAndOptimize(bucketRawArray, warm, 8);
const b = bucketRawArray(warm, 8);
assertTrue(b >= 0);
assertOptimized(bucketRawArray);
