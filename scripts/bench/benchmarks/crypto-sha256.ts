'use strict';

type rawuint32 = number;

const K: rawuint32[] = [
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
  0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
  0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
  0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
];

let inputBytes: rawuint32[] = [];

function rotr(x: rawuint32, n: rawuint32): rawuint32 {
  return ((x >>> n) | (x << (32 - n))) >>> 0;
}

function prepareBlock(bytes: rawuint32[]): rawuint32[] {
  const block: rawuint32[] = new Array(64).fill(0);
  for (let i = 0; i < bytes.length; i++) {
    block[i] = bytes[i] & 255;
  }
  block[bytes.length] = 0x80;
  const bitLen = bytes.length * 8;
  block[63] = bitLen & 255;
  block[62] = (bitLen >>> 8) & 255;
  block[61] = (bitLen >>> 16) & 255;
  block[60] = (bitLen >>> 24) & 255;
  return block;
}

function sha256(bytes: rawuint32[]): rawuint32[] {
  const block = prepareBlock(bytes);
  const w: rawuint32[] = new Array(64).fill(0);
  for (let i = 0; i < 16; i++) {
    const j = i * 4;
    w[i] = (
      (block[j] << 24) |
      (block[j + 1] << 16) |
      (block[j + 2] << 8) |
      block[j + 3]
    ) >>> 0;
  }
  for (let i = 16; i < 64; i++) {
    const s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >>> 3);
    const s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >>> 10);
    w[i] = (w[i - 16] + s0 + w[i - 7] + s1) >>> 0;
  }

  let a: rawuint32 = 0x6a09e667;
  let b: rawuint32 = 0xbb67ae85;
  let c: rawuint32 = 0x3c6ef372;
  let d: rawuint32 = 0xa54ff53a;
  let e: rawuint32 = 0x510e527f;
  let f: rawuint32 = 0x9b05688c;
  let g: rawuint32 = 0x1f83d9ab;
  let h: rawuint32 = 0x5be0cd19;

  for (let i = 0; i < 64; i++) {
    const s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
    const ch = (e & f) ^ (~e & g);
    const temp1 = (h + s1 + ch + K[i] + w[i]) >>> 0;
    const s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
    const maj = (a & b) ^ (a & c) ^ (b & c);
    const temp2 = (s0 + maj) >>> 0;

    h = g;
    g = f;
    f = e;
    e = (d + temp1) >>> 0;
    d = c;
    c = b;
    b = a;
    a = (temp1 + temp2) >>> 0;
  }

  return [
    (a + 0x6a09e667) >>> 0,
    (b + 0xbb67ae85) >>> 0,
    (c + 0x3c6ef372) >>> 0,
    (d + 0xa54ff53a) >>> 0,
    (e + 0x510e527f) >>> 0,
    (f + 0x9b05688c) >>> 0,
    (g + 0x1f83d9ab) >>> 0,
    (h + 0x5be0cd19) >>> 0,
  ];
}

function setup(): void {
  const text = 'sha256-bench';
  inputBytes = [];
  for (let i = 0; i < text.length; i++) {
    inputBytes.push(text.charCodeAt(i) & 255);
  }
}

function bench(): rawuint32 {
  const digest = sha256(inputBytes);
  return (digest[0] ^ digest[1] ^ digest[2] ^ digest[3]) >>> 0;
}

function teardown(): void {
  inputBytes = [];
}
