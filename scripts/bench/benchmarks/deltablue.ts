'use strict';

type rawint32 = number;

type Variable = {
  value: rawint32;
};

type ChainConstraint = {
  src: Variable;
  dst: Variable;
  offset: rawint32;
  priority: rawint32;
  active: rawint32;
};

type ProjectionConstraint = {
  a: Variable;
  b: Variable;
  out: Variable;
  mult: rawint32;
  bias: rawint32;
  priority: rawint32;
};

const CHAIN_VAR_COUNT: rawint32 = 96;
const PROJECTION_COUNT: rawint32 = 72;
const QUERY_SEED_COUNT: rawint32 = 24;

let variables: Variable[] = [];
let chainConstraints: ChainConstraint[] = [];
let projectionConstraints: ProjectionConstraint[] = [];
let querySeeds: rawint32[] = [];
let tick: rawint32 = 0;

function setup(): void {
  variables = [];
  chainConstraints = [];
  projectionConstraints = [];
  querySeeds = [];
  tick = 0;

  for (let i: rawint32 = 0; i < CHAIN_VAR_COUNT; i = i + 1) {
    variables.push({ value: (i * 17 + 11) % 1009 });
  }

  for (let i: rawint32 = 0; i < CHAIN_VAR_COUNT - 1; i = i + 1) {
    chainConstraints.push({
      src: variables[i],
      dst: variables[i + 1],
      offset: (i % 7) + 1,
      priority: (i % 5) + 1,
      active: 1,
    });
  }

  for (let i: rawint32 = 0; i < PROJECTION_COUNT; i = i + 1) {
    projectionConstraints.push({
      a: variables[(i * 3 + 1) % CHAIN_VAR_COUNT],
      b: variables[(i * 5 + 7) % CHAIN_VAR_COUNT],
      out: variables[(i * 11 + 13) % CHAIN_VAR_COUNT],
      mult: (i % 3) + 1,
      bias: (i % 17) - 8,
      priority: (i % 4) + 1,
    });
  }

  for (let i: rawint32 = 0; i < QUERY_SEED_COUNT; i = i + 1) {
    querySeeds.push((i * 7 + 3) % CHAIN_VAR_COUNT);
  }
}

function propagateForward(): rawint32 {
  let checksum: rawint32 = 0;
  for (let i: rawint32 = 0; i < chainConstraints.length; i = i + 1) {
    const c: ChainConstraint = chainConstraints[i];
    if (c.active === 0) {
      continue;
    }
    c.dst.value = c.src.value + c.offset * c.priority;
    checksum += c.dst.value;
  }
  return checksum;
}

function propagateBackward(): rawint32 {
  let checksum: rawint32 = 0;
  for (let i: rawint32 = chainConstraints.length - 1; i >= 0; i = i - 1) {
    const c: ChainConstraint = chainConstraints[i];
    if (c.active === 0) {
      continue;
    }
    c.src.value = c.dst.value - c.offset * c.priority;
    checksum += c.src.value;
  }
  return checksum;
}

function applyProjection(roundWeight: rawint32): rawint32 {
  let checksum: rawint32 = 0;
  for (let i: rawint32 = 0; i < projectionConstraints.length; i = i + 1) {
    const c: ProjectionConstraint = projectionConstraints[i];
    c.out.value = c.a.value + c.b.value * c.mult + c.bias * (roundWeight + c.priority);
    checksum += c.out.value;
  }
  return checksum;
}

function queryNetwork(seedShift: rawint32): rawint32 {
  let checksum: rawint32 = 0;
  for (let i: rawint32 = 0; i < querySeeds.length; i = i + 1) {
    const base: rawint32 = (querySeeds[i] + seedShift) % CHAIN_VAR_COUNT;
    const left: rawint32 = variables[(base + CHAIN_VAR_COUNT - 1) % CHAIN_VAR_COUNT].value;
    const mid: rawint32 = variables[base].value;
    const right: rawint32 = variables[(base + 1) % CHAIN_VAR_COUNT].value;
    checksum += left + mid * 2 + right;
  }
  return checksum;
}

function retuneConstraints(phase: rawint32): void {
  const cIndex: rawint32 = (tick + phase * 3) % chainConstraints.length;
  const pIndex: rawint32 = (tick + phase * 5) % projectionConstraints.length;

  chainConstraints[cIndex].offset = (chainConstraints[cIndex].offset % 9) + 1;
  chainConstraints[cIndex].active = (chainConstraints[cIndex].active + 1) % 2;

  projectionConstraints[pIndex].bias = ((projectionConstraints[pIndex].bias + phase + 9) % 19) - 9;
  projectionConstraints[pIndex].mult = (projectionConstraints[pIndex].mult % 3) + 1;
}

function bench(): rawint32 {
  tick = (tick + 1) % 10007;

  variables[0].value = (variables[0].value + 3 + tick) % 65521;
  variables[CHAIN_VAR_COUNT - 1].value =
      (variables[CHAIN_VAR_COUNT - 1].value + (tick % 11) + 1) % 65521;

  let checksum: rawint32 = 0;
  for (let phase: rawint32 = 0; phase < 4; phase = phase + 1) {
    if (((tick + phase) % 2) === 0) {
      checksum += propagateForward();
    } else {
      checksum += propagateBackward();
    }

    checksum += applyProjection((tick + phase) % 3 + 1);
    checksum += queryNetwork((tick + phase) % QUERY_SEED_COUNT);

    if (phase === 1 || phase === 3) {
      retuneConstraints(phase);
    }
  }

  checksum += variables[tick % CHAIN_VAR_COUNT].value;
  return checksum;
}

function teardown(): void {
  variables = [];
  chainConstraints = [];
  projectionConstraints = [];
  querySeeds = [];
  tick = 0;
}
