'use strict';

type JsonValue = string | number | boolean | null | JsonValue[] | { [key: string]: JsonValue };

let jsonText = '';

function setup(): void {
  jsonText = '{"name":"v8","values":[1,2,3,4,5],"meta":{"x":7,"y":9}}';
}

function parseJson(text: string): JsonValue {
  let index = 0;

  function skipWhitespace(): void {
    while (index < text.length && text.charCodeAt(index) <= 32) {
      index++;
    }
  }

  function parseString(): string {
    index++;
    let result = '';
    while (index < text.length) {
      const ch = text[index];
      if (ch === '"') {
        index++;
        break;
      }
      result += ch;
      index++;
    }
    return result;
  }

  function parseNumber(): number {
    let start = index;
    while (index < text.length) {
      const c = text.charCodeAt(index);
      if (c < 48 || c > 57) break;
      index++;
    }
    return Number(text.slice(start, index));
  }

  function parseArray(): JsonValue[] {
    index++;
    const arr: JsonValue[] = [];
    skipWhitespace();
    if (text[index] === ']') {
      index++;
      return arr;
    }
    while (index < text.length) {
      arr.push(parseValue());
      skipWhitespace();
      if (text[index] === ',') {
        index++;
        skipWhitespace();
        continue;
      }
      if (text[index] === ']') {
        index++;
        break;
      }
    }
    return arr;
  }

  function parseObject(): { [key: string]: JsonValue } {
    index++;
    const obj: { [key: string]: JsonValue } = {};
    skipWhitespace();
    if (text[index] === '}') {
      index++;
      return obj;
    }
    while (index < text.length) {
      skipWhitespace();
      const key = parseString();
      skipWhitespace();
      if (text[index] === ':') index++;
      skipWhitespace();
      obj[key] = parseValue();
      skipWhitespace();
      if (text[index] === ',') {
        index++;
        continue;
      }
      if (text[index] === '}') {
        index++;
        break;
      }
    }
    return obj;
  }

  function parseValue(): JsonValue {
    skipWhitespace();
    const ch = text[index];
    if (ch === '"') return parseString();
    if (ch === '{') return parseObject();
    if (ch === '[') return parseArray();
    return parseNumber();
  }

  return parseValue();
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
  if (value && typeof value === 'object') {
    let sum = 0;
    for (const key in value) {
      sum += sumNumbers((value as { [key: string]: JsonValue })[key]);
    }
    return sum;
  }
  return 0;
}

function bench(): number {
  const obj = parseJson(jsonText);
  return sumNumbers(obj);
}

function teardown(): void {
  jsonText = '';
}
