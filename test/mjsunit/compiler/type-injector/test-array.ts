// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
// 数组类型注入：packed、holey、远索引、循环累加
// 通过 eval 包装 V8 intrinsics，便于 TS 编译后再执行

function prepare(fn: Function): void {
  eval('%PrepareFunctionForOptimization(' + fn.name + ')');
}

function optimize(fn: Function): void {
  eval('%OptimizeFunctionOnNextCall(' + fn.name + ')');
}

// packed array: 直接读取前两项
function concat_arr(data: string[]): string {
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
function concat_holey(data: string[]): string {
  return data[0] + data[2];
}

prepare(concat_holey);
for (let i = 0; i < 100; i++) {
  const a: string[] = ['a'];
  a[2] = 'c'; // 创建 holey，随后填满
  a[1] = 'b';
  concat_holey(a);
}
optimize(concat_holey);
const holey: string[] = ['foo'];
holey[2] = 'bar';
holey[1] = 'mid';
const r2 = concat_holey(holey);
assertEquals('foobar', r2);
assertOptimized(concat_holey);

// 远索引访问：确保大索引也使用元数据类型
function concat_far(data: string[]): string {
  return data[0] + data[10];
}

prepare(concat_far);
for (let i = 0; i < 100; i++) {
  concat_far(['x', 'y', 'z', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i']);
}
optimize(concat_far);
const rFar = concat_far(['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K']);
assertEquals('AK', rFar);
assertOptimized(concat_far);

// loop access: 累加所有字符串元素
function concat_loop(data: string[]): string {
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
