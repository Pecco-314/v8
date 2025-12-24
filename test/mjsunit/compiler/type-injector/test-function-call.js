// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
// Flags: --max_inlined_bytecode_size=0

// 测试：函数返回值类型注入
// getStr: () -> str
// process: (str) -> str
// 预期优化：
// - getStr() 的返回值被标注为 string
// - process 中接收到 getStr() 的结果，可以移除 CheckString

function getStr() { // returns: str
    return "hello";
}

function process(data) { // data: str, returns: str
    var result = getStr();
    return data + result;
}

%PrepareFunctionForOptimization(getStr);
%PrepareFunctionForOptimization(process);

// 预热
for (var i = 0; i < 100; i++) {
    getStr();
    process("test");
}

%OptimizeFunctionOnNextCall(getStr);
%OptimizeFunctionOnNextCall(process);

var result1 = getStr();
var result2 = process("world");

assertEquals("hello", result1);
assertEquals("worldhello", result2);

// 验证函数已被优化
assertOptimized(getStr);
assertOptimized(process);

