// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo-metadata-path=test/mjsunit/compiler/type-injector/metadata

// 测试：Interface 类型注入（对象字段为字符串）
// 预期优化：注入 metadata 后，字段类型已知为 String，
// 可以移除字段访问时的 CheckString 检查

function concat(data) {
    return data.x + data.y;
}

%PrepareFunctionForOptimization(concat);

// 预热：传入包含字符串字段的对象
for (var i = 0; i < 100; i++) {
    var str = i.toString();
    concat({x: str, y: str});
}

%OptimizeFunctionOnNextCall(concat);
var result = concat({x: "hello", y: "world"});
assertEquals("helloworld", result);

// 验证函数已被优化
assertOptimized(concat);
