// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
class Wrapper {
    constructor(first, second) {
        this.first = first;
        this.second = second;
    }
}
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
// 测试 1: interface 形状
// ===================================
function concat(data) {
    return data.x + data.y;
}
warmupAndOptimize(concat, { x: 'warm', y: 'up' });
const result1 = concat({ x: 'hello', y: 'world' });
assertEquals('helloworld', result1);
assertOptimized(concat);
// ===================================
// 测试 2: class 形状
// ===================================
function concat_nested(wrapper) {
    return wrapper.first.x + wrapper.first.y;
}
warmupAndOptimize(concat_nested, new Wrapper({ x: 'foo', y: 'foo' }, 'extra'));
const result2 = concat_nested(new Wrapper({ x: 'foo', y: 'bar' }, 'baz'));
assertEquals('foobar', result2);
assertOptimized(concat_nested);
//# sourceMappingURL=test-obj.js.map