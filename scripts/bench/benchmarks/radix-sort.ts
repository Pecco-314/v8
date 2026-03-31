'use strict';

type rawuint32 = number;

const SIZE = 256;

let data: rawuint32[] = [];

function setup(): void {
  data = new Array(SIZE);
  let value: rawuint32 = 1;
  for (let i = 0; i < SIZE; i++) {
    value = (value * 1664525 + 1013904223) >>> 0;
    data[i] = value;
  }
}

function radixSort(arr: rawuint32[]): rawuint32[] {
  const output: rawuint32[] = new Array(arr.length);
  const count: rawuint32[] = new Array(256).fill(0);

  for (let shift = 0; shift < 32; shift += 8) {
    count.fill(0);
    for (let i = 0; i < arr.length; i++) {
      const bucket: rawuint32 = (arr[i] >>> shift) & 255;
      count[bucket] = (count[bucket] + 1) >>> 0;
    }
    let sum: rawuint32 = 0;
    for (let i = 0; i < 256; i++) {
      const c: rawuint32 = count[i];
      count[i] = sum;
      sum = (sum + c) >>> 0;
    }
    for (let i = 0; i < arr.length; i++) {
      const bucket: rawuint32 = (arr[i] >>> shift) & 255;
      output[count[bucket]] = arr[i];
      count[bucket] = (count[bucket] + 1) >>> 0;
    }
    for (let i = 0; i < arr.length; i++) {
      arr[i] = output[i];
    }
  }

  return arr;
}

function bench(): rawuint32 {
  const copy = data.slice();
  const sorted = radixSort(copy);
  return (sorted[0] ^ sorted[sorted.length - 1]) >>> 0;
}

function teardown(): void {
  data = [];
}
