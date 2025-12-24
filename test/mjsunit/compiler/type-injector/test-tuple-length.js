// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// 测试：Tuple Length 优化
// data: tuple[str, str] - 长度固定为 2
// 预期优化：
// - data.length 被替换为常量 2
// - LoadField[JSArrayLength] 被移除

function getLength(data) { // data: tuple[str, str]
    return data.length;
}

%PrepareFunctionForOptimization(getLength);

// 预热：传入长度为 2 的数组
for (var i = 0; i < 100; i++) {
    getLength(["test", "value"]);
}

%OptimizeFunctionOnNextCall(getLength);
var result = getLength(["hello", "world"]);
assertEquals(2, result);

// 验证函数已被优化
assertOptimized(getLength);

