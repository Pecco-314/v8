// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo-metadata-path=test/mjsunit/compiler/type-injector/metadata

// 测试：Boolean 类型注入
// 预期优化：注入 metadata 后，Parameter 类型变为 Boolean，
// TruncateTaggedToBit 可以变成 ChangeTaggedToBit

function cal(f1, f2) {
    return f1 || f2;
}

%PrepareFunctionForOptimization(cal);

// 预热：传入布尔值
for (var i = 0; i < 100; i++) {
    cal(i % 2 == 0, i % 3 == 0);
}

%OptimizeFunctionOnNextCall(cal);
assertEquals(true, cal(true, false));
assertEquals(true, cal(false, true));
assertEquals(false, cal(false, false));

// 验证函数已被优化
assertOptimized(cal);
