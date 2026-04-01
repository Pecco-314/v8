'use strict';

type rawint32 = number;

const BASE64_CHARS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
const BASE64_INV: rawint32[] = (() => {
  const table: rawint32[] = new Array(128).fill(-1);
  for (let i = 0; i < BASE64_CHARS.length; i++) {
    table[BASE64_CHARS.charCodeAt(i)] = i;
  }
  return table;
})();

let inputBytes: rawint32[] = [];
let encoded: string = '';

function encodeBase64(bytes: rawint32[]): string {
  let out = '';
  for (let i = 0; i < bytes.length; i += 3) {
    const b0: rawint32 = bytes[i];
    const b1: rawint32 = i + 1 < bytes.length ? bytes[i + 1] : 0;
    const b2: rawint32 = i + 2 < bytes.length ? bytes[i + 2] : 0;
    const triple: rawint32 = (b0 << 16) | (b1 << 8) | b2;

    out += BASE64_CHARS[(triple >> 18) & 63];
    out += BASE64_CHARS[(triple >> 12) & 63];
    out += i + 1 < bytes.length ? BASE64_CHARS[(triple >> 6) & 63] : '=';
    out += i + 2 < bytes.length ? BASE64_CHARS[triple & 63] : '=';
  }
  return out;
}

function decodeBase64(text: string): rawint32[] {
  const out: rawint32[] = [];
  for (let i = 0; i < text.length; i += 4) {
    const c0: rawint32 = BASE64_INV[text.charCodeAt(i)];
    const c1: rawint32 = BASE64_INV[text.charCodeAt(i + 1)];
    const c2: rawint32 = text[i + 2] === '=' ? -1 : BASE64_INV[text.charCodeAt(i + 2)];
    const c3: rawint32 = text[i + 3] === '=' ? -1 : BASE64_INV[text.charCodeAt(i + 3)];

    const triple: rawint32 = (c0 << 18) | (c1 << 12) | ((c2 & 63) << 6) | (c3 & 63);
    out.push((triple >> 16) & 255);
    if (c2 >= 0) out.push((triple >> 8) & 255);
    if (c3 >= 0) out.push(triple & 255);
  }
  return out;
}

function setup(): void {
  const text = 'v8-base64-benchmark';
  inputBytes = [];
  for (let i = 0; i < text.length; i++) {
    inputBytes.push(text.charCodeAt(i) & 255);
  }
  encoded = encodeBase64(inputBytes);
}

function bench(): rawint32 {
  const bytes = decodeBase64(encoded);
  let sum: rawint32 = 0;
  for (let i = 0; i < bytes.length; i++) {
    sum += bytes[i];
  }
  return sum;
}

function teardown(): void {
  inputBytes = [];
  encoded = '';
}
