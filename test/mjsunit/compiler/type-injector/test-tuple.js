// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// 测试：Tuple 类型注入
// Tuple 是固定长度的数组，每个位置类型不同
// 预期优化：CheckString 被移除（因为我们知道每个位置的类型）
// 注意：CheckMaps 保留（因为 Map 可能不同，取决于是否有 SMI）

function concat(data) { // data: tuple[str, str]
    return data[0] + data[1];
}

%PrepareFunctionForOptimization(concat);

// 预热：传入包含两个字符串的数组
for (var i = 0; i < 100; i++) {
    var str = i.toString();
    concat([str, str]);
}

%OptimizeFunctionOnNextCall(concat);
var result = concat(["hello", "world"]);
assertEquals("helloworld", result);

// 验证函数已被优化
assertOptimized(concat);

