'use strict';

type rawint32 = number;

type Edge = { to: rawint32; weight: rawint32 };

const NODE_COUNT: rawint32 = 64;
const EDGE_PER_NODE: rawint32 = 4;

let graph: Edge[][] = [];

function setup(): void {
  graph = new Array(NODE_COUNT);
  let seed: rawint32 = 7;
  for (let i: rawint32 = 0; i < NODE_COUNT; i = i + 1) {
    const edges: Edge[] = [];
    for (let j: rawint32 = 0; j < EDGE_PER_NODE; j = j + 1) {
      seed = (seed * 5 + 1) % 97;
      const to = seed % NODE_COUNT;
      seed = (seed * 7 + 3) % 97;
      const weight = (seed % 9) + 1;
      edges.push({ to, weight });
    }
    graph[i] = edges;
  }
}

class MinHeap {
  private heap: rawint32[] = [];
  private dist: rawint32[];

  constructor(dist: rawint32[]) {
    this.dist = dist;
  }

  push(node: rawint32): void {
    this.heap.push(node);
    this.bubbleUp(this.heap.length - 1);
  }

  pop(): rawint32 | null {
    if (this.heap.length === 0) return null;
    const top = this.heap[0];
    const last = this.heap.pop() as rawint32;
    if (this.heap.length > 0) {
      this.heap[0] = last;
      this.bubbleDown(0);
    }
    return top;
  }

  private bubbleUp(index: rawint32): void {
    while (index > 0) {
      const parent = ((index - 1) / 2) | 0;
      if (this.dist[this.heap[parent]] <= this.dist[this.heap[index]]) break;
      const tmp = this.heap[parent];
      this.heap[parent] = this.heap[index];
      this.heap[index] = tmp;
      index = parent;
    }
  }

  private bubbleDown(index: rawint32): void {
    const length = this.heap.length;
    while (true) {
      const left = index * 2 + 1;
      const right = left + 1;
      let smallest = index;
      if (left < length && this.dist[this.heap[left]] < this.dist[this.heap[smallest]]) {
        smallest = left;
      }
      if (right < length && this.dist[this.heap[right]] < this.dist[this.heap[smallest]]) {
        smallest = right;
      }
      if (smallest === index) break;
      const tmp = this.heap[smallest];
      this.heap[smallest] = this.heap[index];
      this.heap[index] = tmp;
      index = smallest;
    }
  }
}

function dijkstra(start: rawint32): rawint32[] {
  const dist: rawint32[] = new Array(NODE_COUNT).fill(1_000_000);
  dist[start] = 0;
  const heap = new MinHeap(dist);
  heap.push(start);

  while (true) {
    const node = heap.pop();
    if (node === null) break;
    const base = dist[node];
    const edges = graph[node];
    for (let i = 0; i < edges.length; i++) {
      const next = edges[i].to;
      const cand = base + edges[i].weight;
      if (cand < dist[next]) {
        dist[next] = cand;
        heap.push(next);
      }
    }
  }

  return dist;
}

function bench(): rawint32 {
  const dist = dijkstra(0);
  return dist[NODE_COUNT - 1];
}

function teardown(): void {
  graph = [];
}
