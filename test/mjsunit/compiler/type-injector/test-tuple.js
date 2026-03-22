// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
// Flags: --turbo_builtin_type_table
// Tuple 类型优化：固定长度、索引常量、混合类型
function prepare(fn) {
    eval('%PrepareFunctionForOptimization(' + fn.name + ')');
}
function optimize(fn) {
    eval('%OptimizeFunctionOnNextCall(' + fn.name + ')');
}
// 基本 tuple: [string, string]
function concat(data) {
    return data[0] + data[1];
}
prepare(concat);
for (let i = 0; i < 100; i++) {
    const s = i.toString();
    concat([s, s]);
}
optimize(concat);
const r1 = concat(['hello', 'world']);
assertEquals('helloworld', r1);
assertOptimized(concat);
// 长度常量化: data.length === 2
function getLength(data) {
    return data.length;
}
prepare(getLength);
for (let i = 0; i < 100; i++) {
    getLength(['test', 'value']);
}
optimize(getLength);
const r2 = getLength(['hello', 'world']);
assertEquals(2, r2);
assertOptimized(getLength);
// 混合类型 tuple: [string, number]
function process(data) {
    return data[0] + data[1].toString();
}
prepare(process);
for (let i = 0; i < 100; i++) {
    process(['test', i]);
}
optimize(process);
const r3 = process(['hello', 42]);
assertEquals('hello42', r3);
assertOptimized(process);
//# sourceMappingURL=test-tuple.js.map