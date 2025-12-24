// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo-metadata-path=test/mjsunit/compiler/type-injector/metadata

// 测试：String 类型注入
// 预期优化：注入 metadata 后，Parameter 类型变为 String，
// 编译器可以移除 CheckedTaggedToTaggedPointer 和 CheckString 检查

function twice_s(arg) {
    return arg + arg;
}

%PrepareFunctionForOptimization(twice_s);

// 预热：传入字符串类型
for (var i = 0; i < 100; i++) {
    twice_s(i.toString());
}

%OptimizeFunctionOnNextCall(twice_s);
var result = twice_s("test");
assertEquals("testtest", result);

// 验证函数已被优化
assertOptimized(twice_s);
