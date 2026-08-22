'use strict';

const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const test = require('node:test');
const {
  APPLICATION_TARGET_IDS,
  createSignedManifest,
  createSigningBytes,
  makeReleaseId,
  nextFirmwareVersion,
  normalizeMetadata,
  replaceCmakeDefinitions,
  selectExecutableTarget,
  validateConfirmationStatus,
  validateMetadataForTarget,
  validateStatusEvent,
  validateStatusTarget,
} = require('../ota-contract');

const metadata = normalizeMetadata({
  schema: 1,
  targetId: '0x54454c45',
  hardwareRevisionMin: 1,
  hardwareRevisionMax: 2,
  firmwareVersion: 17,
  displayVersion: '1.7.0',
  minimumBootloaderVersion: 1,
});

test('signs the exact schema-1 Ed25519 manifest fields', () => {
  const { privateKey, publicKey } = crypto.generateKeyPairSync('ed25519');
  const digest = crypto.createHash('sha256').update('firmware').digest();
  const manifest = createSignedManifest({ metadata, size: 8, digest, privateKey });
  const signature = Buffer.from(manifest.signature, 'base64');
  const signingBytes = createSigningBytes(metadata, 8, digest);

  assert.equal(manifest.target_id, 0x54454C45);
  assert.equal(manifest.image_sha256, digest.toString('hex'));
  assert.equal(
    crypto.verify(null, signingBytes, publicKey, signature),
    true,
  );
  assert.equal(
    signingBytes.toString('hex'),
    '53554e4c4954452d4f54412d4d414e49464553542d563100' +
      '54454c4500010002000000110000000800000001' +
      'c3bf47ea1f4a4a605470313cacb3a44f4a461f68c6faeab07e737610cb5ac835',
  );
});

test('uses a bounded safe release id', () => {
  const digest = '0123456789abcdef'.repeat(4);
  assert.equal(
    makeReleaseId(0x54454C45, 17, digest),
    'vscode-54454c45-17-0123456789ab',
  );
  assert.notEqual(
    makeReleaseId(0x54454C45, 17, digest),
    makeReleaseId(0x4D444920, 17, digest),
  );
});

test('rejects reversed hardware ranges', () => {
  assert.throws(() => normalizeMetadata({
    schema: 1,
    targetId: 1,
    hardwareRevisionMin: 3,
    hardwareRevisionMax: 2,
    firmwareVersion: 1,
    displayVersion: '1',
    minimumBootloaderVersion: 1,
  }), /reversed/);
});

test('requires confirmation for the signed target', () => {
  assert.equal(
    validateStatusTarget({ state: 'confirmed', target_id: 0x4D444920 }, 0x4D444920),
    0x4D444920,
  );
  assert.throws(
    () => validateStatusTarget({ state: 'confirmed' }, 0x4D444920),
    /target_id/,
  );
  assert.throws(
    () => validateStatusTarget({ state: 'confirmed', target_id: 0x54454C45 }, 0x4D444920),
    /expected 0x4d444920/,
  );
});

test('maps the selected ELF to one exact CMake executable target', () => {
  const candidates = [
    { name: 'tel', path: '/repo/firmware/components/tel/build/tel.elf' },
    { name: 'tel_bootloader', path: '/repo/firmware/components/tel/build/tel_bootloader.elf' },
  ];
  assert.deepEqual(
    selectExecutableTarget('/repo/firmware/components/tel/build/../build/tel.elf', candidates),
    candidates[0],
  );
  assert.equal(
    selectExecutableTarget('C:\\FW\\BUILD\\MDI.ELF', [
      { name: 'mdi', path: 'c:\\fw\\build\\mdi.elf' },
    ], 'win32').name,
    'mdi',
  );
  assert.throws(
    () => selectExecutableTarget('/repo/build/unknown.elf', candidates),
    /not an executable target/,
  );
  assert.throws(
    () => selectExecutableTarget('/repo/build/tel.elf', [
      { name: 'tel', path: '/repo/build/tel.elf' },
      { name: 'alias', path: '/repo/build/tel.elf' },
    ]),
    /ambiguous/,
  );
});

test('binds generated metadata to the selected application target', () => {
  for (const [targetName, targetId] of Object.entries(APPLICATION_TARGET_IDS)) {
    assert.equal(validateMetadataForTarget({ targetId }, targetName), targetId);
  }
  assert.throws(
    () => validateMetadataForTarget({ targetId: 0x48564320 }, 'hvc'),
    /not an OTA-capable application target/,
  );
  assert.throws(
    () => validateMetadataForTarget({ targetId: 0x4D535420 }, 'mst'),
    /not an OTA-capable application target/,
  );
  assert.throws(
    () => validateMetadataForTarget({ targetId: APPLICATION_TARGET_IDS.tel }, 'mdi'),
    /does not match CMake target mdi/,
  );
  assert.throws(
    () => validateMetadataForTarget({ targetId: 1 }, 'mdi_bootloader'),
    /not an OTA-capable application target/,
  );
});

test('allocates the next firmware version from the exact running target', () => {
  assert.equal(nextFirmwareVersion({
    schema: 1,
    target_id: APPLICATION_TARGET_IDS.tel,
    firmware_version: 37,
    state: 'application',
  }, APPLICATION_TARGET_IDS.tel), 38);
  assert.throws(() => nextFirmwareVersion({
    schema: 1,
    target_id: APPLICATION_TARGET_IDS.mdi,
    firmware_version: 37,
    state: 'application',
  }, APPLICATION_TARGET_IDS.tel), /expected 0x54454c45/);
  assert.throws(() => nextFirmwareVersion({
    schema: 1,
    target_id: APPLICATION_TARGET_IDS.tel,
    firmware_version: 37,
    state: 'bootloader',
  }, APPLICATION_TARGET_IDS.tel), /requires its application/);
});

test('replaces only OTA CMake definitions without changing other arguments', () => {
  assert.deepEqual(replaceCmakeDefinitions([
    '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
    '-DSUNLITE_OTA_FIRMWARE_VERSION:STRING=3',
    '-DSUNLITE_OTA_DISPLAY_VERSION=3.0.0',
    '-DTEL_OTA_ENABLE_TEST_BYPASS=ON',
  ], {
    SUNLITE_OTA_FIRMWARE_VERSION: 4,
    SUNLITE_OTA_DISPLAY_VERSION: 'dev-4',
  }), [
    '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
    '-DTEL_OTA_ENABLE_TEST_BYPASS=ON',
    '-DSUNLITE_OTA_FIRMWARE_VERSION=4',
    '-DSUNLITE_OTA_DISPLAY_VERSION=dev-4',
  ]);
});

test('accepts only an increasing status stream for the exact release and firmware', () => {
  const expected = {
    releaseId: 'vscode-4d444920-17-0123456789ab',
    targetId: APPLICATION_TARGET_IDS.mdi,
    firmwareVersion: 17,
  };
  const staged = {
    schema: 1,
    generation: 40,
    state: 'staged',
    release_id: expected.releaseId,
    target_id: expected.targetId,
    firmware_version: expected.firmwareVersion,
  };
  const confirmed = { ...staged, generation: 41, state: 'confirmed' };

  assert.equal(validateStatusEvent(staged, expected), 40);
  assert.equal(validateStatusEvent(confirmed, expected, 40), 41);
  assert.equal(validateConfirmationStatus(confirmed, expected), confirmed);
  assert.throws(() => validateStatusEvent(staged, expected, 40), /stale status generation/);
  assert.throws(
    () => validateStatusEvent({ ...staged, release_id: 'old-release' }, expected),
    /belongs to release old-release/,
  );
  assert.throws(
    () => validateStatusEvent({ ...staged, target_id: APPLICATION_TARGET_IDS.tel }, expected),
    /expected 0x4d444920/,
  );
  assert.throws(
    () => validateStatusEvent({ ...staged, firmware_version: 16 }, expected),
    /firmware version 16, expected 17/,
  );
  assert.throws(
    () => validateConfirmationStatus(staged, expected),
    /without confirmation/,
  );
});
