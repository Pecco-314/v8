// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// 测试：混合类型 Tuple 类型注入
// data: tuple[str, num] - 第一个元素是字符串，第二个是数字
// 预期优化：
// - data[0] 的 CheckString 被移除
// - data[1] 的 CheckedTaggedToFloat64 变为 ChangeTaggedToFloat64

function process(data) { // data: tuple[str, num]
    return data[0] + data[1].toString();
}

%PrepareFunctionForOptimization(process);

// 预热：传入包含字符串和数字的数组
for (var i = 0; i < 100; i++) {
    process(["test", i]);
}

%OptimizeFunctionOnNextCall(process);
var result = process(["hello", 42]);
assertEquals("hello42", result);

// 验证函数已被优化
assertOptimized(process);

