// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
// Flags: --max_inlined_bytecode_size=0
// 测试：函数返回值类型注入
// getStr: () -> str
// process: (str) -> str
// 预期优化：
// - getStr() 的返回值被标注为 string
// - process 中接收到 getStr() 的结果，可以移除 CheckString
function prepare(fn) {
    eval('%PrepareFunctionForOptimization(' + fn.name + ')');
}
function optimize(fn) {
    eval('%OptimizeFunctionOnNextCall(' + fn.name + ')');
}
function getStr() {
    return 'hello';
}
function process(data) {
    const result = getStr();
    return data + result;
}
prepare(getStr);
prepare(process);
for (let i = 0; i < 100; i++) {
    getStr();
    process('test');
}
optimize(getStr);
optimize(process);
const result1 = getStr();
const result2 = process('world');
assertEquals('hello', result1);
assertEquals('worldhello', result2);
assertOptimized(getStr);
assertOptimized(process);
//# sourceMappingURL=test-function-call.js.map