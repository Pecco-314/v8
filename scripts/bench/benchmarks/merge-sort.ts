'use strict';

type rawint32 = number;
type Pair = [rawint32, rawint32];

const SIZE = 256;

let data: Pair[] = [];

function setup(): void {
  data = new Array(SIZE);
  let value: rawint32 = 1;
  for (let i = 0; i < SIZE; i++) {
    value = (value * 1103515245 + 12345) | 0;
    const first: rawint32 = value;
    value = (value * 1103515245 + 12345) | 0;
    const second: rawint32 = value;
    data[i] = [first, second];
  }
}

function pairLessOrEqual(left: Pair, right: Pair): boolean {
  if (left[0] < right[0]) return true;
  if (left[0] > right[0]) return false;
  return left[1] <= right[1];
}

function merge(arr: Pair[], left: rawint32, mid: rawint32, right: rawint32): void {
  const merged: Pair[] = new Array(right - left);
  let i: rawint32 = left;
  let j: rawint32 = mid;
  let k: rawint32 = 0;

  while (i < mid && j < right) {
    if (pairLessOrEqual(arr[i], arr[j])) {
      merged[k] = arr[i];
      i = i + 1;
    } else {
      merged[k] = arr[j];
      j = j + 1;
    }
    k = k + 1;
  }

  while (i < mid) {
    merged[k] = arr[i];
    i = i + 1;
    k = k + 1;
  }

  while (j < right) {
    merged[k] = arr[j];
    j = j + 1;
    k = k + 1;
  }

  for (let offset: rawint32 = 0; offset < merged.length; offset = offset + 1) {
    arr[left + offset] = merged[offset];
  }
}

function mergeSortRange(arr: Pair[], left: rawint32, right: rawint32): void {
  if (right - left <= 1) return;
  const mid: rawint32 = left + ((right - left) >> 1);
  mergeSortRange(arr, left, mid);
  mergeSortRange(arr, mid, right);
  merge(arr, left, mid, right);
}

function mergeSort(arr: Pair[]): Pair[] {
  mergeSortRange(arr, 0, arr.length);
  return arr;
}

function bench(): rawint32 {
  const copy = data.slice();
  const sorted = mergeSort(copy);
  const first = sorted[0];
  const last = sorted[sorted.length - 1];
  return (first[0] ^ first[1] ^ last[0] ^ last[1]) | 0;
}

function teardown(): void {
  data = [];
}
