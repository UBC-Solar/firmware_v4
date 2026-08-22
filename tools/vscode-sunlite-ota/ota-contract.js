'use strict';

const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const SIGNING_DOMAIN = Buffer.concat([
  Buffer.from('SUNLITE-OTA-MANIFEST-V1', 'ascii'),
  Buffer.from([0]),
]);

const APPLICATION_TARGET_IDS = Object.freeze({
  drd: 0x44524420,
  mdi: 0x4D444920,
  str: 0x53545220,
  tel: 0x54454C45,
});

function uint32(value, field) {
  const parsed = typeof value === 'string' ? Number(value) : value;
  if (!Number.isInteger(parsed) || parsed < 0 || parsed > 0xFFFFFFFF) {
    throw new Error(`${field} must be an unsigned 32-bit integer.`);
  }
  return parsed;
}

function uint16(value, field) {
  const parsed = typeof value === 'string' ? Number(value) : value;
  if (!Number.isInteger(parsed) || parsed < 0 || parsed > 0xFFFF) {
    throw new Error(`${field} must be an unsigned 16-bit integer.`);
  }
  return parsed;
}

function normalizeMetadata(raw) {
  if (!raw || typeof raw !== 'object' || Array.isArray(raw)) {
    throw new Error('OTA metadata must be a JSON object.');
  }
  const metadata = {
    schema: uint32(raw.schema, 'schema'),
    targetId: uint32(raw.targetId, 'targetId'),
    hardwareRevisionMin: uint16(raw.hardwareRevisionMin, 'hardwareRevisionMin'),
    hardwareRevisionMax: uint16(raw.hardwareRevisionMax, 'hardwareRevisionMax'),
    firmwareVersion: uint32(raw.firmwareVersion, 'firmwareVersion'),
    displayVersion: String(raw.displayVersion || ''),
    minimumBootloaderVersion: uint32(
      raw.minimumBootloaderVersion,
      'minimumBootloaderVersion',
    ),
  };
  if (metadata.schema !== 1) {
    throw new Error(`Unsupported OTA metadata schema ${metadata.schema}.`);
  }
  if (metadata.hardwareRevisionMin > metadata.hardwareRevisionMax) {
    throw new Error('OTA metadata hardware revision range is reversed.');
  }
  if (!metadata.displayVersion || metadata.displayVersion.length > 64) {
    throw new Error('displayVersion must contain 1 to 64 characters.');
  }
  return metadata;
}

function validateMetadataForTarget(metadata, targetName) {
  const expectedTargetId = APPLICATION_TARGET_IDS[targetName];
  if (expectedTargetId === undefined) {
    throw new Error(`CMake target ${targetName} is not an OTA-capable application target.`);
  }
  if (metadata.targetId !== expectedTargetId) {
    throw new Error(
      `OTA metadata target 0x${metadata.targetId.toString(16).padStart(8, '0')} does not match CMake target ${targetName} (0x${expectedTargetId.toString(16).padStart(8, '0')}).`,
    );
  }
  return expectedTargetId;
}

function nextFirmwareVersion(board, expectedTargetId) {
  if (!board || typeof board !== 'object' || Array.isArray(board) || board.schema !== 1) {
    throw new Error('Pi board query did not return a schema-1 JSON object.');
  }
  const targetId = uint32(board.target_id, 'queried target ID');
  const expected = uint32(expectedTargetId, 'expected target ID');
  if (targetId !== expected) {
    throw new Error(
      `Pi queried target 0x${targetId.toString(16).padStart(8, '0')}, expected 0x${expected.toString(16).padStart(8, '0')}.`,
    );
  }
  if (board.state !== 'application') {
    throw new Error(`Target is in ${String(board.state)} state; automatic versioning requires its application.`);
  }
  const installed = uint32(board.firmware_version, 'installed firmware version');
  if (installed === 0xFFFFFFFF) {
    throw new Error('Installed firmware version has exhausted the unsigned 32-bit counter.');
  }
  return installed + 1;
}

function replaceCmakeDefinitions(args, definitions) {
  if (!Array.isArray(args) || !args.every((argument) => typeof argument === 'string')) {
    throw new Error('cmake.configureArgs must be an array of strings.');
  }
  const names = new Set(Object.keys(definitions));
  const output = args.filter((argument) => {
    const match = /^-D([^:=]+)(?::[^=]+)?=/.exec(argument);
    return !match || !names.has(match[1]);
  });
  for (const [name, value] of Object.entries(definitions)) {
    if (!/^[A-Za-z_][A-Za-z0-9_]*$/.test(name)) {
      throw new Error(`Invalid CMake definition name: ${name}`);
    }
    output.push(`-D${name}=${String(value)}`);
  }
  return output;
}

function createSigningBytes(metadata, size, digest) {
  const imageSize = uint32(size, 'image size');
  if (!Buffer.isBuffer(digest) || digest.length !== 32) {
    throw new Error('image digest must be a 32-byte Buffer.');
  }
  const fields = Buffer.alloc(52);
  let offset = 0;
  fields.writeUInt32BE(metadata.targetId, offset); offset += 4;
  fields.writeUInt16BE(metadata.hardwareRevisionMin, offset); offset += 2;
  fields.writeUInt16BE(metadata.hardwareRevisionMax, offset); offset += 2;
  fields.writeUInt32BE(metadata.firmwareVersion, offset); offset += 4;
  fields.writeUInt32BE(imageSize, offset); offset += 4;
  fields.writeUInt32BE(metadata.minimumBootloaderVersion, offset); offset += 4;
  digest.copy(fields, offset);
  return Buffer.concat([SIGNING_DOMAIN, fields]);
}

function createSignedManifest({ metadata, size, digest, privateKey }) {
  const signingBytes = createSigningBytes(metadata, size, digest);
  const key = privateKey?.type === 'private'
    ? privateKey
    : crypto.createPrivateKey(privateKey);
  if (key.asymmetricKeyType !== 'ed25519') {
    throw new Error('sunliteOta.privateKeyPath must contain an Ed25519 private key.');
  }
  const signature = crypto.sign(null, signingBytes, key);
  if (signature.length !== 64) {
    throw new Error('Ed25519 produced an unexpected signature length.');
  }
  return {
    schema: 1,
    target_id: metadata.targetId,
    hardware_revision_min: metadata.hardwareRevisionMin,
    hardware_revision_max: metadata.hardwareRevisionMax,
    firmware_version: metadata.firmwareVersion,
    display_version: metadata.displayVersion,
    image_size: size,
    image_sha256: digest.toString('hex'),
    minimum_bootloader_version: metadata.minimumBootloaderVersion,
    signature_algorithm: 'ed25519',
    signature: signature.toString('base64'),
  };
}

function makeReleaseId(targetId, firmwareVersion, hexDigest) {
  const target = uint32(targetId, 'target ID');
  const version = uint32(firmwareVersion, 'firmware version');
  if (!/^[0-9a-f]{64}$/i.test(hexDigest)) {
    throw new Error('release digest must contain 64 hexadecimal characters.');
  }
  const targetHex = target.toString(16).padStart(8, '0');
  return `vscode-${targetHex}-${version}-${hexDigest.slice(0, 12).toLowerCase()}`;
}

function validateStatusTarget(status, expectedTargetId) {
  const expected = uint32(expectedTargetId, 'expected target ID');
  if (!status || !Number.isInteger(status.target_id)) {
    throw new Error('Pi OTA confirmation did not include a valid target_id.');
  }
  const reported = uint32(status.target_id, 'reported target ID');
  if (reported !== expected) {
    const reportedHex = reported.toString(16).padStart(8, '0');
    const expectedHex = expected.toString(16).padStart(8, '0');
    throw new Error(`Pi confirmed target 0x${reportedHex}, expected 0x${expectedHex}.`);
  }
  return reported;
}

function validateStatusEvent(status, expected, previousGeneration = -1) {
  if (!status || typeof status !== 'object' || Array.isArray(status)) {
    throw new Error('Pi OTA status is not a JSON object.');
  }
  if (status.schema !== 1) {
    throw new Error(`Pi OTA status has unsupported schema ${String(status.schema)}.`);
  }
  if (!Number.isSafeInteger(status.generation) || status.generation < 0) {
    throw new Error('Pi OTA status did not include a valid generation.');
  }
  if (status.generation <= previousGeneration) {
    throw new Error(
      `Pi OTA returned stale status generation ${status.generation} after ${previousGeneration}.`,
    );
  }
  if (typeof status.state !== 'string' || !status.state) {
    throw new Error('Pi OTA status did not include a valid state.');
  }
  if (typeof expected?.releaseId !== 'string' || !expected.releaseId) {
    throw new Error('expected release ID must be a non-empty string.');
  }
  if (status.release_id !== expected.releaseId) {
    throw new Error(
      `Pi OTA status belongs to release ${String(status.release_id)}, expected ${expected.releaseId}.`,
    );
  }
  validateStatusTarget(status, expected.targetId);
  const expectedVersion = uint32(expected.firmwareVersion, 'expected firmware version');
  if (!Number.isInteger(status.firmware_version)) {
    throw new Error('Pi OTA status did not include a valid firmware_version.');
  }
  const reportedVersion = uint32(status.firmware_version, 'reported firmware version');
  if (reportedVersion !== expectedVersion) {
    throw new Error(
      `Pi reported firmware version ${reportedVersion}, expected ${expectedVersion}.`,
    );
  }
  return status.generation;
}

function validateConfirmationStatus(status, expected, previousGeneration = -1) {
  validateStatusEvent(status, expected, previousGeneration);
  if (status.state !== 'confirmed') {
    throw new Error(
      `Pi OTA status ended without confirmation (last state: ${status.state}).`,
    );
  }
  return status;
}

function normalizedFilePath(value, platform = process.platform) {
  if (typeof value !== 'string' || !value) {
    throw new Error('CMake target path must be a non-empty string.');
  }
  const resolved = path.resolve(value);
  return platform === 'win32' ? resolved.toLowerCase() : resolved;
}

function selectExecutableTarget(selectedPath, candidates, platform = process.platform) {
  if (!Array.isArray(candidates)) {
    throw new Error('CMake executable targets must be an array.');
  }
  const selected = normalizedFilePath(selectedPath, platform);
  const matches = candidates.filter((candidate) => (
    candidate
    && typeof candidate.name === 'string'
    && candidate.name
    && typeof candidate.path === 'string'
    && normalizedFilePath(candidate.path, platform) === selected
  ));
  if (matches.length === 0) {
    throw new Error(
      `The selected CMake launch executable is not an executable target in its active project: ${selectedPath}`,
    );
  }
  if (matches.length !== 1) {
    throw new Error(`The selected CMake launch executable is ambiguous: ${selectedPath}`);
  }
  return matches[0];
}

function sha256File(filename) {
  return new Promise((resolve, reject) => {
    const hash = crypto.createHash('sha256');
    let size = 0;
    const stream = fs.createReadStream(filename);
    stream.on('error', reject);
    stream.on('data', (chunk) => {
      size += chunk.length;
      hash.update(chunk);
    });
    stream.on('end', () => {
      const digest = hash.digest();
      resolve({ size, digest, hexDigest: digest.toString('hex') });
    });
  });
}

module.exports = {
  APPLICATION_TARGET_IDS,
  SIGNING_DOMAIN,
  createSignedManifest,
  createSigningBytes,
  makeReleaseId,
  nextFirmwareVersion,
  normalizeMetadata,
  replaceCmakeDefinitions,
  selectExecutableTarget,
  sha256File,
  validateConfirmationStatus,
  validateMetadataForTarget,
  validateStatusEvent,
  validateStatusTarget,
};
