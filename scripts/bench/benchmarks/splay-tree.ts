'use strict';

type rawint32 = number;

type SplayNode = {
  key: rawint32;
  left: SplayNode | null;
  right: SplayNode | null;
};

let root: SplayNode | null = null;
let keys: rawint32[] = [];

function rotateRight(p: SplayNode): SplayNode {
  const q = p.left as SplayNode;
  p.left = q.right;
  q.right = p;
  return q;
}

function rotateLeft(p: SplayNode): SplayNode {
  const q = p.right as SplayNode;
  p.right = q.left;
  q.left = p;
  return q;
}

function splay(node: SplayNode | null, key: rawint32): SplayNode | null {
  if (node === null) return null;
  if (key < node.key) {
    if (node.left === null) return node;
    if (key < node.left.key) {
      node.left.left = splay(node.left.left, key);
      node = rotateRight(node);
    } else if (key > node.left.key) {
      node.left.right = splay(node.left.right, key);
      if (node.left.right !== null) node.left = rotateLeft(node.left);
    }
    return node.left === null ? node : rotateRight(node);
  }

  if (key > node.key) {
    if (node.right === null) return node;
    if (key > node.right.key) {
      node.right.right = splay(node.right.right, key);
      node = rotateLeft(node);
    } else if (key < node.right.key) {
      node.right.left = splay(node.right.left, key);
      if (node.right.left !== null) node.right = rotateRight(node.right);
    }
    return node.right === null ? node : rotateLeft(node);
  }

  return node;
}

function insert(key: rawint32): void {
  if (root === null) {
    root = { key, left: null, right: null };
    return;
  }
  root = splay(root, key);
  if (root && root.key === key) return;
  const newNode: SplayNode = { key, left: null, right: null };
  if (root && key < root.key) {
    newNode.left = root.left;
    newNode.right = root;
    root.left = null;
  } else if (root) {
    newNode.right = root.right;
    newNode.left = root;
    root.right = null;
  }
  root = newNode;
}

function find(key: rawint32): SplayNode | null {
  root = splay(root, key);
  return root;
}

function setup(): void {
  root = null;
  keys = [];
  let value: rawint32 = 1;
  for (let i: rawint32 = 0; i < 64; i = i + 1) {
    value = (value * 7 + 3) % 101;
    keys.push(value);
  }
  for (let i = 0; i < keys.length; i++) {
    insert(keys[i]);
  }
}

function bench(): rawint32 {
  let sum: rawint32 = 0;
  for (let i = 0; i < keys.length; i++) {
    const node = find(keys[i]);
    if (node) sum += node.key;
  }
  return sum;
}

function teardown(): void {
  root = null;
  keys = [];
}
