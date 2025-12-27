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

// ===================================
// 测试 1: interface 形状
// ===================================
function concat(data: Pair): string {
  return data.x + data.y;
}

prepare(concat);
for (let i = 0; i < 100; i++) {
  const str = i.toString();
  concat({ x: str, y: str });
}
optimize(concat);
const result1 = concat({ x: 'hello', y: 'world' });
assertEquals('helloworld', result1);
assertOptimized(concat);

// ===================================
// 测试 2: class 形状
// ===================================
function concat_nested(wrapper: Wrapper): string {
  return wrapper.first.x + wrapper.first.y;
}

prepare(concat_nested);
for (let i = 0; i < 100; i++) {
  const str = i.toString();
  const inner: Pair = { x: str, y: str };
  concat_nested(new Wrapper(inner, 'extra'));
}
optimize(concat_nested);
const result2 = concat_nested(new Wrapper({ x: 'foo', y: 'bar' }, 'baz'));
assertEquals('foobar', result2);
assertOptimized(concat_nested);
