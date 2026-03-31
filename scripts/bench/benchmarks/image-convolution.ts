'use strict';

type rawint32 = number;

const WIDTH: rawint32 = 32;
const HEIGHT: rawint32 = 32;

let input: rawint32[][] = [];
let output: rawint32[][] = [];

const KERNEL: rawint32[][] = [
  [1, 2, 1],
  [2, 4, 2],
  [1, 2, 1],
];
const KERNEL_SUM: rawint32 = 16;

function allocMatrix(w: rawint32, h: rawint32): rawint32[][] {
  const matrix: rawint32[][] = new Array(h);
  for (let y: rawint32 = 0; y < h; y = y + 1) {
    const row: rawint32[] = new Array(w);
    for (let x: rawint32 = 0; x < w; x = x + 1) {
      row[x] = 0;
    }
    matrix[y] = row;
  }
  return matrix;
}

function fillMatrix(matrix: rawint32[][]): void {
  let value: rawint32 = 7;
  for (let y: rawint32 = 0; y < matrix.length; y = y + 1) {
    const row = matrix[y];
    for (let x: rawint32 = 0; x < row.length; x = x + 1) {
      value = (value * 5 + 1) % 101;
      row[x] = value;
    }
  }
}

function convolve(src: rawint32[][], dst: rawint32[][]): rawint32 {
  let checksum: rawint32 = 0;
  for (let y: rawint32 = 1; y < HEIGHT - 1; y = y + 1) {
    const outRow = dst[y];
    for (let x: rawint32 = 1; x < WIDTH - 1; x = x + 1) {
      let sum: rawint32 = 0;
      for (let ky: rawint32 = 0; ky < 3; ky = ky + 1) {
        const row = src[y + ky - 1];
        const krow = KERNEL[ky];
        for (let kx: rawint32 = 0; kx < 3; kx = kx + 1) {
          sum += row[x + kx - 1] * krow[kx];
        }
      }
      const value = Math.floor(sum / KERNEL_SUM);
      outRow[x] = value;
      checksum += value;
    }
  }
  return checksum;
}

function setup(): void {
  input = allocMatrix(WIDTH, HEIGHT);
  output = allocMatrix(WIDTH, HEIGHT);
  fillMatrix(input);
}

function bench(): rawint32 {
  return convolve(input, output);
}

function teardown(): void {
  input = [];
  output = [];
}
