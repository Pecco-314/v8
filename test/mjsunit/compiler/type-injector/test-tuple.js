// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
// Flags: --turbo_builtin_type_table

// 测试：Tuple 类型优化
// Tuple 是固定长度的数组，每个位置类型不同

// ===================================
// 测试 1: 基本 Tuple（同类型元素）
// ===================================
// data: tuple[str, str]
// 预期优化：
// - CheckString 被移除（因为我们知道每个位置的类型）
// - CheckBounds 被移除（因为索引是常量且在范围内）

function concat(data) {
    return data[0] + data[1];
}

%PrepareFunctionForOptimization(concat);

// 预热：传入包含两个字符串的数组
for (var i = 0; i < 100; i++) {
    var str = i.toString();
    concat([str, str]);
}

%OptimizeFunctionOnNextCall(concat);
var result1 = concat(["hello", "world"]);
assertEquals("helloworld", result1);

// 验证函数已被优化
assertOptimized(concat);


// ===================================
// 测试 2: Tuple Length 优化
// ===================================
// data: tuple[str, str] - 长度固定为 2
// 预期优化：
// - data.length 被替换为常量 2
// - LoadField[JSArrayLength] 被移除

function getLength(data) {
    return data.length;
}

%PrepareFunctionForOptimization(getLength);

// 预热：传入长度为 2 的数组
for (var i = 0; i < 100; i++) {
    getLength(["test", "value"]);
}

%OptimizeFunctionOnNextCall(getLength);
var result2 = getLength(["hello", "world"]);
assertEquals(2, result2);

// 验证函数已被优化
assertOptimized(getLength);


// ===================================
// 测试 3: 混合类型 Tuple
// ===================================
// data: tuple[str, num] - 第一个元素是字符串，第二个是数字
// 预期优化：
// - data[0] 的 CheckString 被移除
// - data[1].toString() 的返回值 CheckString 被移除（通过内建函数类型表）
// - CheckBounds 被移除

function process(data) {
    return data[0] + data[1].toString();
}

%PrepareFunctionForOptimization(process);

// 预热：传入包含字符串和数字的数组
for (var i = 0; i < 100; i++) {
    process(["test", i]);
}

%OptimizeFunctionOnNextCall(process);
var result3 = process(["hello", 42]);
assertEquals("hello42", result3);

// 验证函数已被优化
assertOptimized(process);

