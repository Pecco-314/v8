'use strict';

type rawint32 = number;

const NODE_COUNT: rawint32 = 64;
const EDGE_PER_NODE: rawint32 = 6;
const INF: rawint32 = 1_000_000;

let edgeStart: rawint32[] = [];
let edgeTo: rawint32[] = [];
let edgeWeight: rawint32[] = [];
let relaxMetric: rawint32 = 0;

type HeapEntry = [rawint32, rawint32];

function heapPush(heap: HeapEntry[], entry: HeapEntry): void {
  heap.push(entry);
  let idx: rawint32 = heap.length - 1;
  while (idx > 0) {
    const parent: rawint32 = (idx - 1) >> 1;
    if (heap[parent][0] <= heap[idx][0]) {
      break;
    }
    const tmp: HeapEntry = heap[parent];
    heap[parent] = heap[idx];
    heap[idx] = tmp;
    idx = parent;
  }
}

function heapPopMin(heap: HeapEntry[]): HeapEntry | null {
  const n: rawint32 = heap.length;
  if (n === 0) {
    return null;
  }
  const minEntry: HeapEntry = heap[0];
  const last: HeapEntry = heap[n - 1];
  heap.pop();
  if (n > 1) {
    heap[0] = last;
    let idx: rawint32 = 0;
    const size: rawint32 = heap.length;
    while (true) {
      const left: rawint32 = (idx << 1) + 1;
      const right: rawint32 = left + 1;
      let smallest: rawint32 = idx;

      if (left < size && heap[left][0] < heap[smallest][0]) {
        smallest = left;
      }
      if (right < size && heap[right][0] < heap[smallest][0]) {
        smallest = right;
      }
      if (smallest === idx) {
        break;
      }

      const tmp: HeapEntry = heap[idx];
      heap[idx] = heap[smallest];
      heap[smallest] = tmp;
      idx = smallest;
    }
  }
  return minEntry;
}

function setup(): void {
  edgeStart = new Array(NODE_COUNT + 1);
  edgeTo = [];
  edgeWeight = [];

  let seed: rawint32 = 17;
  let cursor: rawint32 = 0;

  for (let from: rawint32 = 0; from < NODE_COUNT; from = from + 1) {
    edgeStart[from] = cursor;
    for (let k: rawint32 = 0; k < EDGE_PER_NODE; k = k + 1) {
      seed = (seed * 13 + 7) % 257;
      const to: rawint32 = seed % NODE_COUNT;
      seed = (seed * 11 + 5) % 257;
      const weight: rawint32 = (seed % 9) + 1;
      edgeTo.push(to);
      edgeWeight.push(weight);
      cursor = cursor + 1;
    }
  }

  edgeStart[NODE_COUNT] = cursor;
}

function dijkstra(start: rawint32): rawint32[] {
  const startIndex: rawint32[] = edgeStart;
  const toList: rawint32[] = edgeTo;
  const weightList: rawint32[] = edgeWeight;
  const dist: rawint32[] = new Array(NODE_COUNT);
  const heap: HeapEntry[] = [];
  let localMetric: rawint32 = 0;

  for (let i: rawint32 = 0; i < NODE_COUNT; i = i + 1) {
    dist[i] = INF;
  }
  dist[start] = 0;
  heapPush(heap, [0, start]);

  while (heap.length > 0) {
    const current = heapPopMin(heap);
    if (current === null) {
      break;
    }
    const bestDist: rawint32 = current[0];
    const bestNode: rawint32 = current[1];
    if (bestDist !== dist[bestNode]) {
      continue;
    }

    const begin: rawint32 = startIndex[bestNode];
    const end: rawint32 = startIndex[bestNode + 1];

    for (let p: rawint32 = begin; p < end; p = p + 1) {
      const to: rawint32 = toList[p];
      const nextDist: rawint32 = bestDist + weightList[p];
      if (nextDist < dist[to]) {
        dist[to] = nextDist;
        heapPush(heap, [nextDist, to]);
      }

      const mix: rawint32 = (to + p) * weightList[p];
      const denom: rawint32 = weightList[p] | 1;
      const q: rawint32 = mix / denom;
      const r: rawint32 = mix % denom;
      localMetric = localMetric + q + r;
    }
  }

  relaxMetric = localMetric;

  return dist;
}

function bench(): rawint32 {
  const dist = dijkstra(0);
  let checksum: rawint32 = 0;
  for (let i: rawint32 = 0; i < NODE_COUNT; i = i + 1) {
    checksum = checksum + dist[i];
  }
  return dist[NODE_COUNT - 1] + (checksum % 97) + (relaxMetric % 31);
}

function teardown(): void {
  edgeStart = [];
  edgeTo = [];
  edgeWeight = [];
  relaxMetric = 0;
}
