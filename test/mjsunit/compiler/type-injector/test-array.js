// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
// 数组类型注入：packed、holey、远索引、循环累加
// 通过 eval 包装 V8 intrinsics，便于 TS 编译后再执行
function prepare(fn) {
    eval('%PrepareFunctionForOptimization(' + fn.name + ')');
}
function optimize(fn) {
    eval('%OptimizeFunctionOnNextCall(' + fn.name + ')');
}
// packed array: 直接读取前两项
function concat_arr(data) {
    return data[0] + data[1];
}
prepare(concat_arr);
for (let i = 0; i < 100; i++) {
    const s = i.toString();
    concat_arr([s, s, 'extra']);
}
optimize(concat_arr);
const r1 = concat_arr(['hello', 'world', '!']);
assertEquals('helloworld', r1);
assertOptimized(concat_arr);
// holey array: 稀疏数组填充后读取首尾
// 类型安全分析：为何 holey 无法消除 CheckString
// 1. 我们始终保留 CheckMaps，动态验证数组是 PACKED 还是 HOLEY
// 2. PACKED：CheckMaps 保证 [0,length) 无 Hole + Metadata(arr<str>) → 100% 是 String
//    → 可安全消除 CheckString（packed 数组的 concat_arr/concat_far/concat_loop）
// 3. HOLEY：CheckMaps 确认存在 Hole 可能 + LoadElement 返回 (string | Hole)
//    → 必须保留 CheckString 验证非 Hole（本函数 concat_holey）
function concat_holey(data) {
    return data[0] + data[2];
}
prepare(concat_holey);
for (let i = 0; i < 100; i++) {
    const a = ['a'];
    a[2] = 'c'; // 创建 holey，随后填满
    a[1] = 'b';
    concat_holey(a);
}
optimize(concat_holey);
const holey = ['foo'];
holey[2] = 'bar';
holey[1] = 'mid';
const r2 = concat_holey(holey);
assertEquals('foobar', r2);
assertOptimized(concat_holey);
// loop access: 累加所有字符串元素
function concat_loop(data) {
    let acc = '';
    for (let i = 0; i < data.length; i++) {
        acc += data[i];
    }
    return acc;
}
prepare(concat_loop);
for (let i = 0; i < 100; i++) {
    concat_loop(['x', 'y', 'z', i.toString()]);
}
optimize(concat_loop);
const r3 = concat_loop(['v8', '-', 'tf', '-ok']);
assertEquals('v8-tf-ok', r3);
assertOptimized(concat_loop);
