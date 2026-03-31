'use strict';

type rawint32 = number;

const DEFAULT_SIZE: rawint32 = 16;

let size: rawint32 = 0;
let left: rawint32[][] = [];
let right: rawint32[][] = [];
let result: rawint32[][] = [];

function allocMatrix(n: rawint32): rawint32[][] {
  const matrix: rawint32[][] = new Array(n);
  for (let i: rawint32 = 0; i < n; i = i + 1) {
    const row: rawint32[] = new Array(n);
    for (let j: rawint32 = 0; j < n; j = j + 1) {
      row[j] = 0;
    }
    matrix[i] = row;
  }
  return matrix;
}

function fillMatrix(matrix: rawint32[][], seed: rawint32): void {
  const n: rawint32 = matrix.length;
  let value: rawint32 = seed;
  for (let i: rawint32 = 0; i < n; i = i + 1) {
    const row = matrix[i];
    for (let j: rawint32 = 0; j < n; j = j + 1) {
      value = (value * 7 + 3) % 101;
      row[j] = value + 1;
    }
  }
}

function matmul(
  a: rawint32[][],
  b: rawint32[][],
  out: rawint32[][]
): rawint32 {
  const n: rawint32 = a.length;
  let checksum: rawint32 = 0;

  for (let i: rawint32 = 0; i < n; i = i + 1) {
    const outRow = out[i];
    const aRow = a[i];
    for (let j: rawint32 = 0; j < n; j = j + 1) {
      let sum: rawint32 = 0;
      for (let k: rawint32 = 0; k < n; k = k + 1) {
        sum += Math.imul(aRow[k], b[k][j]);
      }
      outRow[j] = sum;
      checksum += sum;
    }
  }

  return checksum;
}

function setup(): void {
  size = DEFAULT_SIZE;
  left = allocMatrix(size);
  right = allocMatrix(size);
  result = allocMatrix(size);
  fillMatrix(left, 1);
  fillMatrix(right, 2);
}

function bench(): rawint32 {
  return matmul(left, right, result);
}

function teardown(): void {
  size = 0;
  left = [];
  right = [];
  result = [];
}
