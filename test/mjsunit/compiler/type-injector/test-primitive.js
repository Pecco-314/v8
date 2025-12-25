// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata

// 测试：基本类型（String、Number、Boolean）类型注入优化

// ===================================
// 测试 1: String 类型
// ===================================
// 预期优化：注入 metadata 后，Parameter 类型变为 String，
// 编译器可以移除 CheckString 检查

function twice_s(arg) {
    return arg + arg;
}

%PrepareFunctionForOptimization(twice_s);

// 预热：传入字符串类型
for (var i = 0; i < 100; i++) {
    twice_s(i.toString());
}

%OptimizeFunctionOnNextCall(twice_s);
var result1 = twice_s("test");
assertEquals("testtest", result1);

// 验证函数已被优化
assertOptimized(twice_s);


// ===================================
// 测试 2: Number 类型
// ===================================
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
var result2 = twice_f(3.5);
assertEquals(7.0, result2);

// 验证函数已被优化
assertOptimized(twice_f);


// ===================================
// 测试 3: Boolean 类型
// ===================================
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


// ===================================
// 测试 4: Symbol 类型
// ===================================
// 预期优化：注入 metadata 后，Parameter 类型变为 Symbol，
// 编译器可以移除 CheckMaps[SYMBOL_TYPE] 检查

function toStr(arg) {
    return arg.toString();
}

%PrepareFunctionForOptimization(toStr);

// 预热：传入 Symbol
var sym1 = Symbol("test");
var sym2 = Symbol("hello");
for (var i = 0; i < 100; i++) {
    toStr(i % 2 == 0 ? sym1 : sym2);
}

%OptimizeFunctionOnNextCall(toStr);
var result4 = toStr(Symbol("world"));
assertEquals("Symbol(world)", result4);

// 验证函数已被优化
assertOptimized(toStr);
