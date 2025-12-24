// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo-metadata-path=test/mjsunit/compiler/type-injector/metadata

// 测试：数组元素类型注入
// 预期优化：注入 metadata 后，数组元素类型已知为 String (arr<str>)，
// 可以移除数组元素访问时的 CheckString 检查

function concat_arr(data) {
    return data[0] + data[1];
}

%PrepareFunctionForOptimization(concat_arr);

// 预热：传入字符串数组
for (var i = 0; i < 100; i++) {
    var str = i.toString();
    concat_arr([str, str, "extra"]);
}

%OptimizeFunctionOnNextCall(concat_arr);
var result = concat_arr(["hello", "world", "!"]);
assertEquals("helloworld", result);

// 验证函数已被优化
assertOptimized(concat_arr);
