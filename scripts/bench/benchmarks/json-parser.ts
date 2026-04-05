'use strict';

type JsonValue = string | number | boolean | null | JsonValue[] | { [key: string]: JsonValue };
type ParseState = { text: string; index: number };

let jsonText: string = '';
let parseToggle: number = 0;

const CODE_SPACE: number = 32;
const CODE_TAB: number = 9;
const CODE_NEWLINE: number = 10;
const CODE_RETURN: number = 13;
const CODE_QUOTE: number = 34;
const CODE_MINUS: number = 45;
const CODE_PLUS: number = 43;
const CODE_DOT: number = 46;
const CODE_ZERO: number = 48;
const CODE_NINE: number = 57;
const CODE_E_UPPER: number = 69;
const CODE_E_LOWER: number = 101;

function setup(): void {
  jsonText = '{"name":"v8","values":[1,2,3,4,5],"meta":{"x":7,"y":9}}';
  parseToggle = 0;
}

function isDigitCode(code: number): boolean {
  return code >= CODE_ZERO && code <= CODE_NINE;
}

function skipWhitespace(state: ParseState): void {
  while (state.index < state.text.length) {
    const code: number = state.text.charCodeAt(state.index);
    if (code !== CODE_SPACE && code !== CODE_TAB && code !== CODE_NEWLINE && code !== CODE_RETURN) {
      break;
    }
    state.index++;
  }
}

function parseHexDigitCode(state: ParseState): number {
  const code: number = state.text.charCodeAt(state.index);
  state.index++;
  if (code >= 48 && code <= 57) return code - 48;
  if (code >= 65 && code <= 70) return 10 + (code - 65);
  if (code >= 97 && code <= 102) return 10 + (code - 97);
  return 0;
}

function parseString(state: ParseState): string {
  state.index++;
  let result: string = '';
  while (state.index < state.text.length) {
    const code: number = state.text.charCodeAt(state.index);
    if (code === CODE_QUOTE) {
      state.index++;
      break;
    }

    if (code === 92) {
      state.index++;
      const esc: number = state.text.charCodeAt(state.index);
      state.index++;
      if (esc === CODE_QUOTE) {
        result += '"';
      } else if (esc === 92) {
        result += '\\';
      } else if (esc === 47) {
        result += '/';
      } else if (esc === 98) {
        result += '\b';
      } else if (esc === 102) {
        result += '\f';
      } else if (esc === 110) {
        result += '\n';
      } else if (esc === 114) {
        result += '\r';
      } else if (esc === 116) {
        result += '\t';
      } else if (esc === 117) {
        let cp: number = 0;
        cp = cp * 16 + parseHexDigitCode(state);
        cp = cp * 16 + parseHexDigitCode(state);
        cp = cp * 16 + parseHexDigitCode(state);
        cp = cp * 16 + parseHexDigitCode(state);
        result += String.fromCharCode(cp);
      } else {
        result += String.fromCharCode(esc);
      }
      continue;
    }

    result += String.fromCharCode(code);
    state.index++;
  }
  return result;
}

function parseNumber(state: ParseState): number {
  let sign: number = 1;
  if (state.text.charCodeAt(state.index) === CODE_MINUS) {
    sign = -1;
    state.index++;
  }

  let integerPart: number = 0;
  while (state.index < state.text.length) {
    const code: number = state.text.charCodeAt(state.index);
    if (!isDigitCode(code)) break;
    integerPart = integerPart * 10 + (code - CODE_ZERO);
    state.index++;
  }

  let value: number = integerPart;
  if (state.index < state.text.length && state.text.charCodeAt(state.index) === CODE_DOT) {
    state.index++;
    let fraction: number = 0;
    let scale: number = 1;
    while (state.index < state.text.length) {
      const code: number = state.text.charCodeAt(state.index);
      if (!isDigitCode(code)) break;
      fraction = fraction * 10 + (code - CODE_ZERO);
      scale = scale * 10;
      state.index++;
    }
    value += fraction / scale;
  }

  if (state.index < state.text.length) {
    const expCode: number = state.text.charCodeAt(state.index);
    if (expCode === CODE_E_UPPER || expCode === CODE_E_LOWER) {
      state.index++;
      let expSign: number = 1;
      const nextCode: number = state.text.charCodeAt(state.index);
      if (nextCode === CODE_MINUS) {
        expSign = -1;
        state.index++;
      } else if (nextCode === CODE_PLUS) {
        state.index++;
      }

      let exponent: number = 0;
      while (state.index < state.text.length) {
        const code: number = state.text.charCodeAt(state.index);
        if (!isDigitCode(code)) break;
        exponent = exponent * 10 + (code - CODE_ZERO);
        state.index++;
      }

      value = value * (10 ** (expSign * exponent));
    }
  }

  return sign * value;
}

function matchWord(state: ParseState, a: number, b: number, c: number, d?: number): boolean {
  if (state.index >= state.text.length) return false;
  if (state.text.charCodeAt(state.index) !== a) return false;
  if (state.text.charCodeAt(state.index + 1) !== b) return false;
  if (state.text.charCodeAt(state.index + 2) !== c) return false;
  if (d !== undefined && state.text.charCodeAt(state.index + 3) !== d) return false;
  return true;
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
    const key: string = parseString(state);
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
  const code: number = state.text.charCodeAt(state.index);
  if (code === CODE_QUOTE) return parseString(state);
  if (code === 123) return parseObject(state);
  if (code === 91) return parseArray(state);
  if (code === 116 && matchWord(state, 116, 114, 117, 101)) {
    state.index += 4;
    return true;
  }
  if (code === 102 && matchWord(state, 102, 97, 108, 115) && state.text.charCodeAt(state.index + 4) === 101) {
    state.index += 5;
    return false;
  }
  if (code === 110 && matchWord(state, 110, 117, 108, 108)) {
    state.index += 4;
    return null;
  }
  return parseNumber(state);
}

function parseJson(text: string): JsonValue {
  const state: ParseState = { text, index: 0 };
  return parseValue(state);
}

function isJsonObject(value: JsonValue): value is { [key: string]: JsonValue } {
  if (value === null || typeof value !== 'object') return false;
  if ('length' in value) return false;
  return true;
}

function sumNumbers(value: JsonValue): number {
  if (typeof value === 'number') return value;
  if (Array.isArray(value)) {
    const arr: JsonValue[] = value;
    let sum: number = 0;
    for (let i: number = 0; i < arr.length; i++) {
      sum += sumNumbers(arr[i]);
    }
    return sum;
  }
  if (isJsonObject(value)) {
    let sum: number = 0;
    let key: string;
    for (key in value) {
      sum += sumNumbers(value[key]);
    }
    return sum;
  }
  return 0;
}

function bench(): number {
  parseToggle = (parseToggle + 1) & 1;
  const runtimeText: string = parseToggle === 0 ? jsonText : ' ' + jsonText;
  const obj: JsonValue = parseJson(runtimeText);
  return sumNumbers(obj);
}

function teardown(): void {
  jsonText = '';
  parseToggle = 0;
}
