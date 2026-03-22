'use strict';

let obj;

function setup() {
  obj = { x: 1, y: 2, z: 3 };
}

function bench() {
  const v = obj.x + obj.y + obj.z;
  obj.x = v;
  return v;
}

function teardown() {
  obj = null;
}
