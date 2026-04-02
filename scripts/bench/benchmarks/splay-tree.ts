'use strict';

type rawint32 = number;

type SplayNode = [rawint32, rawint32, rawint32];

const NIL: rawint32 = -1;
const KEY: rawint32 = 0;
const LEFT: rawint32 = 1;
const RIGHT: rawint32 = 2;

let nodes: SplayNode[] = [];
let root: rawint32 = NIL;
let keys: rawint32[] = [];

function getNode(index: rawint32): SplayNode {
  return nodes[index];
}

function makeNode(key: rawint32, left: rawint32, right: rawint32): rawint32 {
  nodes.push([key, left, right]);
  return nodes.length - 1;
}

function rotateRight(p: rawint32): rawint32 {
  const pNode: SplayNode = getNode(p);
  const q: rawint32 = pNode[LEFT];
  if (q === NIL) return p;
  pNode[LEFT] = getNode(q)[RIGHT];
  getNode(q)[RIGHT] = p;
  return q;
}

function rotateLeft(p: rawint32): rawint32 {
  const pNode: SplayNode = getNode(p);
  const q: rawint32 = pNode[RIGHT];
  if (q === NIL) return p;
  pNode[RIGHT] = getNode(q)[LEFT];
  getNode(q)[LEFT] = p;
  return q;
}

function splay(node: rawint32, key: rawint32): rawint32 {
  if (node === NIL) return NIL;

  const nodeData: SplayNode = getNode(node);

  if (key < nodeData[KEY]) {
    const left: rawint32 = nodeData[LEFT];
    if (left === NIL) return node;

    const leftNode: SplayNode = getNode(left);

    if (key < leftNode[KEY]) {
      const leftLeft: rawint32 = leftNode[LEFT];
      leftNode[LEFT] = splay(leftLeft, key);
      node = rotateRight(node);
    } else if (key > leftNode[KEY]) {
      const leftRight: rawint32 = leftNode[RIGHT];
      leftNode[RIGHT] = splay(leftRight, key);
      const currentNode: SplayNode = getNode(node);
      const curLeft: rawint32 = currentNode[LEFT];
      if (curLeft !== NIL && getNode(curLeft)[RIGHT] !== NIL) {
        currentNode[LEFT] = rotateLeft(curLeft);
      }
    }
    return getNode(node)[LEFT] === NIL ? node : rotateRight(node);
  }

  if (key > nodeData[KEY]) {
    const right: rawint32 = nodeData[RIGHT];
    if (right === NIL) return node;

    const rightNode: SplayNode = getNode(right);

    if (key > rightNode[KEY]) {
      const rightRight: rawint32 = rightNode[RIGHT];
      rightNode[RIGHT] = splay(rightRight, key);
      node = rotateLeft(node);
    } else if (key < rightNode[KEY]) {
      const rightLeft: rawint32 = rightNode[LEFT];
      rightNode[LEFT] = splay(rightLeft, key);
      const currentNode: SplayNode = getNode(node);
      const curRight: rawint32 = currentNode[RIGHT];
      if (curRight !== NIL && getNode(curRight)[LEFT] !== NIL) {
        currentNode[RIGHT] = rotateRight(curRight);
      }
    }
    return getNode(node)[RIGHT] === NIL ? node : rotateLeft(node);
  }

  return node;
}

function insert(key: rawint32): void {
  if (root === NIL) {
    root = makeNode(key, NIL, NIL);
    return;
  }

  root = splay(root, key);
  const rootNode: SplayNode = getNode(root);
  if (rootNode[KEY] === key) return;

  if (key < rootNode[KEY]) {
    const leftSubtree: rawint32 = rootNode[LEFT];
    const newRoot: rawint32 = makeNode(key, leftSubtree, root);
    rootNode[LEFT] = NIL;
    root = newRoot;
  } else {
    const rightSubtree: rawint32 = rootNode[RIGHT];
    const newRoot: rawint32 = makeNode(key, root, rightSubtree);
    rootNode[RIGHT] = NIL;
    root = newRoot;
  }
}

function find(key: rawint32): rawint32 {
  root = splay(root, key);
  return root;
}

function setup(): void {
  nodes = [];
  root = NIL;
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
    const nodeIndex: rawint32 = find(keys[i]);
    if (nodeIndex !== NIL) sum += getNode(nodeIndex)[KEY];
  }
  return sum;
}

function teardown(): void {
  nodes = [];
  root = NIL;
  keys = [];
}
