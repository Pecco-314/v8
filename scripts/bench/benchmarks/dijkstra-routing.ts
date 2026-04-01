'use strict';

type rawint32 = number;

const NODE_COUNT: rawint32 = 64;
const EDGE_PER_NODE: rawint32 = 6;
const INF: rawint32 = 1_000_000;

let edgeStart: rawint32[] = [];
let edgeTo: rawint32[] = [];
let edgeWeight: rawint32[] = [];
let relaxMetric: rawint32 = 0;

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
  const used: rawint32[] = new Array(NODE_COUNT);
  let localMetric: rawint32 = 0;

  for (let i: rawint32 = 0; i < NODE_COUNT; i = i + 1) {
    dist[i] = INF;
    used[i] = 0;
  }
  dist[start] = 0;

  for (let step: rawint32 = 0; step < NODE_COUNT; step = step + 1) {
    let bestNode: rawint32 = -1;
    let bestDist: rawint32 = INF;

    for (let i: rawint32 = 0; i < NODE_COUNT; i = i + 1) {
      const cand: rawint32 = dist[i];
      if (used[i] === 0 && cand < bestDist) {
        bestDist = cand;
        bestNode = i;
      }
    }

    if (bestNode < 0) {
      break;
    }

    used[bestNode] = 1;
    const begin: rawint32 = startIndex[bestNode];
    const end: rawint32 = startIndex[bestNode + 1];

    for (let p: rawint32 = begin; p < end; p = p + 1) {
      const to: rawint32 = toList[p];
      const nextDist: rawint32 = dist[bestNode] + weightList[p];
      if (nextDist < dist[to]) {
        dist[to] = nextDist;
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
