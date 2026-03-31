'use strict';

const COUNT = 32;
const STEPS = 3;
const DT = 0.01;
const G = 0.01;

let x: number[] = [];
let y: number[] = [];
let z: number[] = [];
let vx: number[] = [];
let vy: number[] = [];
let vz: number[] = [];
let mass: number[] = [];

function setup(): void {
  x = new Array(COUNT);
  y = new Array(COUNT);
  z = new Array(COUNT);
  vx = new Array(COUNT);
  vy = new Array(COUNT);
  vz = new Array(COUNT);
  mass = new Array(COUNT);

  let seed = 1.0;
  for (let i = 0; i < COUNT; i++) {
    seed = (seed * 13.0 + 7.0) % 17.0;
    x[i] = seed * 0.1;
    seed = (seed * 13.0 + 7.0) % 17.0;
    y[i] = seed * 0.1;
    seed = (seed * 13.0 + 7.0) % 17.0;
    z[i] = seed * 0.1;
    vx[i] = 0.0;
    vy[i] = 0.0;
    vz[i] = 0.0;
    mass[i] = 0.5 + (seed * 0.01);
  }
}

function step(): void {
  for (let i = 0; i < COUNT; i++) {
    for (let j = i + 1; j < COUNT; j++) {
      const dx = x[j] - x[i];
      const dy = y[j] - y[i];
      const dz = z[j] - z[i];
      const distSq = dx * dx + dy * dy + dz * dz + 1e-6;
      const invDist = 1.0 / Math.sqrt(distSq);
      const force = G * invDist * invDist;
      const fx = force * dx;
      const fy = force * dy;
      const fz = force * dz;

      vx[i] += fx * mass[j] * DT;
      vy[i] += fy * mass[j] * DT;
      vz[i] += fz * mass[j] * DT;

      vx[j] -= fx * mass[i] * DT;
      vy[j] -= fy * mass[i] * DT;
      vz[j] -= fz * mass[i] * DT;
    }
  }

  for (let i = 0; i < COUNT; i++) {
    x[i] += vx[i] * DT;
    y[i] += vy[i] * DT;
    z[i] += vz[i] * DT;
  }
}

function energy(): number {
  let e = 0.0;
  for (let i = 0; i < COUNT; i++) {
    e += 0.5 * mass[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i]);
  }
  return e;
}

function bench(): number {
  for (let i = 0; i < STEPS; i++) {
    step();
  }
  return energy();
}

function teardown(): void {
  x = [];
  y = [];
  z = [];
  vx = [];
  vy = [];
  vz = [];
  mass = [];
}
