// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// 测试：Obj 类型注入优化

// ===================================
// 测试 1: 简单 Obj
// ===================================
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
var result1 = concat({x: "hello", y: "world"});
assertEquals("helloworld", result1);

// 验证函数已被优化
assertOptimized(concat);


// ===================================
// 测试 2: 嵌套 Obj
// ===================================
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
var result2 = concat_nested({first: {x: "foo", y: "bar"}, second: "baz"});
assertEquals("foobar", result2);

// 验证函数已被优化
assertOptimized(concat_nested);

