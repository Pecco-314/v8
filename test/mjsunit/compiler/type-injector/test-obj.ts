// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// 测试：Obj 类型注入优化（interface + class 场景）

interface Pair {
  x: string;
  y: string;
}

class Wrapper {
  first: Pair;
  second: string;
  constructor(first: Pair, second: string) {
    this.first = first;
    this.second = second;
  }
}

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
// 测试 1: interface 形状
// ===================================
function concat(data: Pair): string {
  return data.x + data.y;
}

warmupAndOptimize(concat, { x: 'warm', y: 'up' });
const result1 = concat({ x: 'hello', y: 'world' });
assertEquals('helloworld', result1);
assertOptimized(concat);

// ===================================
// 测试 2: class 形状
// ===================================
function concat_nested(wrapper: Wrapper): string {
  return wrapper.first.x + wrapper.first.y;
}

warmupAndOptimize(concat_nested, new Wrapper({ x: 'foo', y: 'foo' }, 'extra'));
const result2 = concat_nested(new Wrapper({ x: 'foo', y: 'bar' }, 'baz'));
assertEquals('foobar', result2);
assertOptimized(concat_nested);
