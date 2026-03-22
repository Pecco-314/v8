'use strict';

let targetFn;
const METADATA_HASH = '4d1f93ba5435846d2c7a641a672137f93314e02cf47b65c39e5de01187f89462';
const METADATA_DIR = 'test/mjsunit/compiler/type-injector/metadata';
const METADATA_FILE = `${METADATA_DIR}/${METADATA_HASH}.metadata`;

function setup() {
  load('test/mjsunit/mjsunit.js');
  load('test/mjsunit/compiler/type-injector/test-primitive.js');

  let metadataText = '';
  try {
    metadataText = read(METADATA_FILE);
  } catch (error) {
    throw new Error(`metadata file not found: ${METADATA_FILE}`);
  }
  if (!metadataText.includes('twice_smi')) {
    throw new Error(`metadata content missing twice_smi: ${METADATA_FILE}`);
  }
  print(`METADATA_FILE_OK:${METADATA_FILE}`);

  if (typeof twice_smi !== 'function') {
    throw new Error('twice_smi is not defined');
  }

  targetFn = twice_smi;
}

function bench() {
  return targetFn(123);
}

function teardown() {
  targetFn = null;
}
