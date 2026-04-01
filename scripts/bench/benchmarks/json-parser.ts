'use strict';

type JsonValue = string | number | boolean | null | JsonValue[] | { [key: string]: JsonValue };
type ParseState = { text: string; index: number };

let jsonText = '';
let parseToggle = 0;

function setup(): void {
  jsonText = '{"name":"v8","values":[1,2,3,4,5],"meta":{"x":7,"y":9}}';
  parseToggle = 0;
}

function skipWhitespace(state: ParseState): void {
  while (state.index < state.text.length && state.text.charCodeAt(state.index) <= 32) {
    state.index++;
  }
}

function parseString(state: ParseState): string {
  state.index++;
  let result = '';
  while (state.index < state.text.length) {
    const ch = state.text[state.index];
    if (ch === '"') {
      state.index++;
      break;
    }
    result += ch;
    state.index++;
  }
  return result;
}

function parseNumber(state: ParseState): number {
  let start = state.index;
  while (state.index < state.text.length) {
    const c = state.text.charCodeAt(state.index);
    if (c < 48 || c > 57) break;
    state.index++;
  }
  return Number(state.text.slice(start, state.index));
}

function parseArray(state: ParseState): JsonValue[] {
  state.index++;
  const arr: JsonValue[] = [];
  skipWhitespace(state);
  if (state.text[state.index] === ']') {
    state.index++;
    return arr;
  }
  while (state.index < state.text.length) {
    arr.push(parseValue(state));
    skipWhitespace(state);
    if (state.text[state.index] === ',') {
      state.index++;
      skipWhitespace(state);
      continue;
    }
    if (state.text[state.index] === ']') {
      state.index++;
      break;
    }
  }
  return arr;
}

function parseObject(state: ParseState): { [key: string]: JsonValue } {
  state.index++;
  const obj: { [key: string]: JsonValue } = {};
  skipWhitespace(state);
  if (state.text[state.index] === '}') {
    state.index++;
    return obj;
  }
  while (state.index < state.text.length) {
    skipWhitespace(state);
    const key = parseString(state);
    skipWhitespace(state);
    if (state.text[state.index] === ':') state.index++;
    skipWhitespace(state);
    obj[key] = parseValue(state);
    skipWhitespace(state);
    if (state.text[state.index] === ',') {
      state.index++;
      continue;
    }
    if (state.text[state.index] === '}') {
      state.index++;
      break;
    }
  }
  return obj;
}

function parseValue(state: ParseState): JsonValue {
  skipWhitespace(state);
  const ch = state.text[state.index];
  if (ch === '"') return parseString(state);
  if (ch === '{') return parseObject(state);
  if (ch === '[') return parseArray(state);
  return parseNumber(state);
}

function parseJson(text: string): JsonValue {
  const state: ParseState = { text, index: 0 };
  return parseValue(state);
}

function isJsonObject(value: JsonValue): value is { [key: string]: JsonValue } {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

function sumNumbers(value: JsonValue): number {
  if (typeof value === 'number') return value;
  if (Array.isArray(value)) {
    let sum = 0;
    for (let i = 0; i < value.length; i++) {
      sum += sumNumbers(value[i]);
    }
    return sum;
  }
  if (isJsonObject(value)) {
    let sum = 0;
    for (const key in value) {
      sum += sumNumbers(value[key]);
    }
    return sum;
  }
  return 0;
}

function bench(): number {
  parseToggle = (parseToggle + 1) & 1;
  const runtimeText = parseToggle === 0 ? jsonText : ` ${jsonText}`;
  const obj = parseJson(runtimeText);
  return sumNumbers(obj);
}

function teardown(): void {
  jsonText = '';
  parseToggle = 0;
}
