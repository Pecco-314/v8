// @ts-nocheck
// Flags: --allow-natives-syntax --turbofan --no-always-turbofan
// Flags: --turbo_metadata_path=test/mjsunit/compiler/type-injector/metadata
function prepare(fn) {
    eval('%PrepareFunctionForOptimization(' + fn.name + ')');
}
function optimize(fn) {
    eval('%OptimizeFunctionOnNextCall(' + fn.name + ')');
}
function warmupAndOptimize(fn, ...args) {
    prepare(fn);
    fn(...args);
    optimize(fn);
}
function makeFilledArray(size) {
    const arr = new Array(size);
    let seed = 1;
    for (let i = 0; i < size; i++) {
        seed = (seed * 1664525 + 1013904223) >>> 0;
        arr[i] = seed;
    }
    return arr;
}
function sumRawArray(arr) {
    let acc = 0;
    for (let i = 0; i < arr.length; i++) {
        acc = (acc + arr[i]) >>> 0;
    }
    return acc;
}
function bucketRawArray(arr, shift) {
    let acc = 0;
    for (let i = 0; i < arr.length; i++) {
        const bucket = (arr[i] >>> shift) & 255;
        acc = (acc + bucket) >>> 0;
    }
    return acc;
}
const warm = makeFilledArray(256);
warmupAndOptimize(sumRawArray, warm);
const s = sumRawArray(warm);
assertTrue(s >= 0);
assertOptimized(sumRawArray);
warmupAndOptimize(bucketRawArray, warm, 8);
const b = bucketRawArray(warm, 8);
assertTrue(b >= 0);
assertOptimized(bucketRawArray);
//# sourceMappingURL=test-rawuint32-array.js.map