// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo-metadata-path=test/mjsunit/compiler/type-injector/metadata

// 测试：嵌套 Interface 类型注入
// 预期优化：注入 metadata 后，嵌套字段类型已知，
// 可以移除嵌套字段访问时的 CheckString 检查
// data 类型：{first: {x: str, y: str}, second: str}

function concat_nested(data) {
    return data.first.x + data.first.y;
}

%PrepareFunctionForOptimization(concat_nested);

// 预热：传入嵌套对象
for (var i = 0; i < 100; i++) {
    var str = i.toString();
    var inner = {x: str, y: str};
    concat_nested({first: inner, second: "extra"});
}

%OptimizeFunctionOnNextCall(concat_nested);
var result = concat_nested({first: {x: "foo", y: "bar"}, second: "baz"});
assertEquals("foobar", result);

// 验证函数已被优化
assertOptimized(concat_nested);
