// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_builtin_type_table

// 测试：内建函数类型表优化
// 通过 Builtin ID 识别内建函数的参数和返回值类型

// ===================================
// 测试 1: Number.prototype.toString()
// ===================================
// 预期优化：
// - 识别 num.toString() 返回 string
// - 消除后续的 CheckString

function processNumber(num) {
    var str = num.toString();
    return str + "!";
}

%PrepareFunctionForOptimization(processNumber);

// 预热
for (var i = 0; i < 100; i++) {
    processNumber(42);
}

%OptimizeFunctionOnNextCall(processNumber);
var result1 = processNumber(123);
assertEquals("123!", result1);
assertOptimized(processNumber);


// ===================================
// 测试 2: Object.prototype.toString()
// ===================================
// 预期优化：
// - 验证通用性（所有对象的 toString 都返回 string）

function processAny(obj) {
    var str = obj.toString();
    return str.length;
}

%PrepareFunctionForOptimization(processAny);

// 预热
for (var i = 0; i < 100; i++) {
    processAny({});
}

%OptimizeFunctionOnNextCall(processAny);
var result2 = processAny({});
assertTrue(result2 > 0);
assertOptimized(processAny);


// ===================================
// 测试 3: Number.prototype.toFixed()
// ===================================
// 预期优化：
// - 识别 num.toFixed(digits) 返回 string
// - 消除后续的 CheckString

function testToFixed(num, digits) {
    var result = num.toFixed(digits);
    return result + " units";
}

%PrepareFunctionForOptimization(testToFixed);

// 预热
for (var i = 0; i < 100; i++) {
    testToFixed(3.14159, 2);
}

%OptimizeFunctionOnNextCall(testToFixed);
var result3 = testToFixed(3.14159, 2);
assertEquals("3.14 units", result3);
assertOptimized(testToFixed);


// ===================================
// 测试 4: String.prototype.repeat()
// ===================================
// 预期优化：
// - 识别 str.repeat(count) 返回 string
// - 消除后续的 CheckString

function testRepeat(str, count) {
    var result = str.repeat(count);
    return result.length;
}

%PrepareFunctionForOptimization(testRepeat);

// 预热
for (var i = 0; i < 100; i++) {
    testRepeat("ab", 3);
}

%OptimizeFunctionOnNextCall(testRepeat);
var result5 = testRepeat("ab", 3);
assertEquals(6, result5);
assertOptimized(testRepeat);

