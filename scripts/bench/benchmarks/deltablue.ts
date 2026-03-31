'use strict';

type rawint32 = number;

type Variable = {
  value: rawint32;
};

type Constraint = {
  a: Variable;
  b: Variable;
  out: Variable;
  weight: rawint32;
};

let variables: Variable[] = [];
let constraints: Constraint[] = [];

function setup(): void {
  variables = [];
  constraints = [];
  for (let i: rawint32 = 0; i < 16; i = i + 1) {
    variables.push({ value: i + 1 });
  }
  for (let i: rawint32 = 0; i < 12; i = i + 1) {
    constraints.push({
      a: variables[i],
      b: variables[i + 1],
      out: variables[i + 2],
      weight: (i % 3) + 1,
    });
  }
}

function propagate(): rawint32 {
  let checksum: rawint32 = 0;
  for (let i: rawint32 = 0; i < constraints.length; i = i + 1) {
    const c = constraints[i];
    c.out.value = c.a.value + c.b.value * c.weight;
    checksum += c.out.value;
  }
  return checksum;
}

function bench(): rawint32 {
  variables[0].value = (variables[0].value + 3) % 97;
  return propagate();
}

function teardown(): void {
  variables = [];
  constraints = [];
}
