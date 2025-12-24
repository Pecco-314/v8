// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// 测试：Number 类型注入
// 预期优化：注入 metadata 后，Parameter 类型变为 Number，
// CheckedTaggedToFloat64 可以变成无检查的 ChangeTaggedToFloat64

function twice_f(arg) {
    return arg + arg;
}

%PrepareFunctionForOptimization(twice_f);

// 预热：传入浮点数
for (var i = 0; i < 100; i++) {
    twice_f(i + 0.5);
}

%OptimizeFunctionOnNextCall(twice_f);
var result = twice_f(3.5);
assertEquals(7.0, result);

// 验证函数已被优化
assertOptimized(twice_f);

