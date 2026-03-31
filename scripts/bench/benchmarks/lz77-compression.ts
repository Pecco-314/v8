'use strict';

type rawint32 = number;

const WINDOW_SIZE: rawint32 = 64;
const LOOKAHEAD: rawint32 = 16;

let inputData: rawint32[] = [];

function setup(): void {
  const text = 'lz77-compression-benchmark-data-';
  inputData = [];
  for (let i = 0; i < 8; i++) {
    for (let j = 0; j < text.length; j++) {
      inputData.push(text.charCodeAt(j) & 255);
    }
  }
}

function compress(data: rawint32[]): rawint32 {
  let pos: rawint32 = 0;
  let checksum: rawint32 = 0;

  while (pos < data.length) {
    let bestLen: rawint32 = 0;
    let bestOffset: rawint32 = 0;

    const maxOffset = Math.min(WINDOW_SIZE, pos);
    for (let offset: rawint32 = 1; offset <= maxOffset; offset = offset + 1) {
      let len: rawint32 = 0;
      while (
        len < LOOKAHEAD &&
        pos + len < data.length &&
        data[pos + len] === data[pos - offset + len]
      ) {
        len = len + 1;
      }
      if (len > bestLen) {
        bestLen = len;
        bestOffset = offset;
      }
    }

    const next = pos + bestLen < data.length ? data[pos + bestLen] : 0;
    checksum += bestOffset + bestLen + next;
    pos = pos + bestLen + 1;
  }

  return checksum;
}

function bench(): rawint32 {
  return compress(inputData);
}

function teardown(): void {
  inputData = [];
}
