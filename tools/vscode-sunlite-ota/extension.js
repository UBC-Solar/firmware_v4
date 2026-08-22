'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawn } = require('child_process');
const vscode = require('vscode');
const {
  APPLICATION_TARGET_IDS,
  createSignedManifest,
  makeReleaseId,
  nextFirmwareVersion,
  normalizeMetadata,
  replaceCmakeDefinitions,
  selectExecutableTarget,
  sha256File,
  validateConfirmationStatus,
  validateMetadataForTarget,
  validateStatusEvent,
} = require('./ota-contract');

const DEBUG_TYPE = 'sunlite-ota';
let activeFlash;

function activate(context) {
  const output = vscode.window.createOutputChannel('Sunlite OTA');
  context.subscriptions.push(output);

  const run = (debugConfiguration = {}) => {
    if (activeFlash) {
      output.show(true);
      throw new Error('A Sunlite OTA flash is already in progress.');
    }

    activeFlash = vscode.window.withProgress(
      {
        location: vscode.ProgressLocation.Notification,
        title: 'Sunlite OTA',
        cancellable: false,
      },
      async (progress) => runOta(debugConfiguration, output, progress),
    ).finally(() => {
      activeFlash = undefined;
    });

    return activeFlash;
  };

  context.subscriptions.push(
    vscode.commands.registerCommand('sunliteOta.flash', async () => {
      try {
        await run();
      } catch (error) {
        await reportFailure(error, output);
      }
    }),
  );

  const provider = new SunliteConfigurationProvider();
  const factory = new SunliteDebugAdapterFactory(run, output);
  context.subscriptions.push(
    vscode.debug.registerDebugConfigurationProvider(DEBUG_TYPE, provider),
    vscode.debug.registerDebugAdapterDescriptorFactory(DEBUG_TYPE, factory),
    factory,
  );
}

function deactivate() {}

class SunliteConfigurationProvider {
  resolveDebugConfiguration(folder, config) {
    const resolved = {
      name: 'Sunlite OTA (active CMake target)',
      type: DEBUG_TYPE,
      request: 'launch',
      ...config,
    };
    if (folder) {
      resolved.__sunliteWorkspaceFolder = folder.uri.fsPath;
    }
    return resolved;
  }
}

class SunliteDebugAdapterFactory {
  constructor(run, output) {
    this.run = run;
    this.output = output;
  }

  createDebugAdapterDescriptor() {
    return new vscode.DebugAdapterInlineImplementation(
      new SunliteDebugAdapter(this.run, this.output),
    );
  }

  dispose() {}
}

class SunliteDebugAdapter {
  constructor(run, output) {
    this.run = run;
    this.output = output;
    this.sequence = 1;
    this.emitter = new vscode.EventEmitter();
    this.onDidSendMessage = this.emitter.event;
  }

  handleMessage(message) {
    if (message.type !== 'request') {
      return;
    }

    if (message.command === 'initialize') {
      this.respond(message, {});
      this.event('initialized');
      return;
    }

    if (message.command === 'launch') {
      // A Debug Adapter Protocol launch request must be acknowledged promptly.
      // The OTA operation continues asynchronously and ends the pseudo-debug
      // session with a terminated event when the board confirms the image.
      this.respond(message, {});
      this.run(message.arguments || {})
        .then(() => {
          this.event('terminated');
        })
        .catch(async (error) => {
          await reportFailure(error, this.output);
          this.event('terminated');
        });
      return;
    }

    this.respond(message, {});
  }

  respond(request, body, success = true, message) {
    this.emitter.fire({
      type: 'response',
      seq: this.sequence++,
      request_seq: request.seq,
      command: request.command,
      success,
      body,
      message,
    });
  }

  event(event, body) {
    this.emitter.fire({
      type: 'event',
      seq: this.sequence++,
      event,
      body,
    });
  }

  dispose() {
    this.emitter.dispose();
  }
}

async function runOta(debugConfiguration, output, progress) {
  output.clear();
  output.show(true);
  output.appendLine('Starting Sunlite OTA flash...');

  const workspaceFolders = vscode.workspace.workspaceFolders;
  if (!workspaceFolders?.length) {
    throw new Error('Open a firmware workspace before starting Sunlite OTA.');
  }

  progress.report({ message: 'Resolving active CMake target' });
  const selection = await resolveSelectedCMakeTarget(debugConfiguration, workspaceFolders);
  const {
    sourceDirectory,
    targetName,
    workspaceFolder,
  } = selection;
  let elfPath = selection.elfPath;

  if (/bootloader/i.test(targetName) || /bootloader/i.test(path.basename(elfPath))) {
    throw new Error('Sunlite OTA only accepts application images; select an application CMake target.');
  }

  const settings = vscode.workspace.getConfiguration('sunliteOta', workspaceFolder.uri);
  const buildBeforeFlash = debugConfiguration.buildBeforeFlash
    ?? settings.get('buildBeforeFlash', true);
  const automaticVersioning = settings.get('automaticVersioning', true);

  const host = settings.get('host', '').trim();
  const user = settings.get('user', 'tonychen').trim();
  const sshPort = settings.get('sshPort', 22);
  const incomingDirectory = settings.get(
    'incomingDirectory',
    '/var/lib/sunlite-ota/incoming',
  ).trim();
  const gatewayExecutable = settings.get(
    'gatewayExecutable',
    '/home/tonychen/sunlite/.venv/bin/sunlite-ota',
  ).trim();
  const objcopyPath = settings.get('objcopyPath', 'arm-none-eabi-objcopy').trim();
  const privateKeyPath = expandHome(settings.get('privateKeyPath', '').trim());
  const statusTimeout = settings.get('statusTimeout', 900);

  validateSettings({
    host,
    user,
    sshPort,
    incomingDirectory,
    gatewayExecutable,
    objcopyPath,
    privateKeyPath,
    statusTimeout,
  });

  let allocatedVersion;
  let restoreConfigureArgs;
  if (automaticVersioning) {
    if (!buildBeforeFlash) {
      throw new ConfigurationError(
        'Automatic versioning requires sunliteOta.buildBeforeFlash to be enabled.',
      );
    }
    const targetId = APPLICATION_TARGET_IDS[targetName];
    if (targetId === undefined) {
      throw new Error(`CMake target ${targetName} is not an OTA-capable application target.`);
    }
    progress.report({ message: `Querying ${formatTargetId(targetId)} version` });
    const board = await queryBoard(
      { gatewayExecutable, host, sshPort, targetId, user },
      output,
    );
    allocatedVersion = nextFirmwareVersion(board, targetId);
    output.appendLine(
      `Automatic firmware version: ${board.firmware_version} -> ${allocatedVersion}`,
    );
    restoreConfigureArgs = await configureAutomaticVersion(
      workspaceFolder,
      sourceDirectory,
      allocatedVersion,
      output,
    );
  }

  let metadata;
  let metadataPath;
  try {
    if (buildBeforeFlash) {
      progress.report({ message: `Building ${targetName}` });
      output.appendLine(`Building selected CMake target ${targetName}...`);
      const buildResult = await vscode.commands.executeCommand(
        'cmake.build',
        workspaceFolder,
        targetName,
        sourceDirectory,
      );
      if (buildResult !== 0) {
        throw new Error(
          `CMake build failed${typeof buildResult === 'number' ? ` with exit code ${buildResult}` : ''}.`,
        );
      }

      elfPath = await resolveNamedCMakeTargetPath(
        workspaceFolder,
        sourceDirectory,
        targetName,
      );
    }

    progress.report({ message: 'Locating active application image' });
    if (!fs.existsSync(elfPath)) {
      throw new Error(
        `The selected CMake target does not exist: ${elfPath}. Build it before flashing.`,
      );
    }

    metadataPath = `${elfPath}.ota.json`;
    if (!fs.existsSync(metadataPath)) {
      throw new Error(
        `OTA metadata was not generated for this target: ${metadataPath}. Reconfigure and rebuild CMake.`,
      );
    }
    metadata = normalizeMetadata(JSON.parse(await fs.promises.readFile(metadataPath, 'utf8')));
    validateMetadataForTarget(metadata, targetName);
    if (allocatedVersion !== undefined && metadata.firmwareVersion !== allocatedVersion) {
      throw new Error(
        `Automatic CMake version ${allocatedVersion} did not reach ${targetName} metadata (reported ${metadata.firmwareVersion}).`,
      );
    }
  } finally {
    if (restoreConfigureArgs) {
      await restoreConfigureArgs();
    }
  }

  const binaryPath = path.join(path.dirname(elfPath), `${targetName}.ota.bin`);
  const manifestPath = path.join(path.dirname(elfPath), `${targetName}.ota.manifest.json`);
  progress.report({ message: 'Creating and signing OTA release' });
  output.appendLine(`ELF: ${elfPath}`);
  output.appendLine(`Metadata: ${metadataPath}`);
  await runProcess(objcopyPath, ['-O', 'binary', elfPath, binaryPath], output);

  const { size, digest, hexDigest } = await sha256File(binaryPath);
  const privateKey = await fs.promises.readFile(privateKeyPath);
  const manifest = createSignedManifest({ metadata, size, digest, privateKey });
  await fs.promises.writeFile(manifestPath, `${JSON.stringify(manifest, null, 2)}\n`, {
    encoding: 'utf8',
    mode: 0o600,
  });

  const releaseId = makeReleaseId(metadata.targetId, metadata.firmwareVersion, hexDigest);
  const remoteSessionDirectory = `${incomingDirectory}/${releaseId}`;
  const remoteImagePath = `${remoteSessionDirectory}/firmware.bin`;
  const remoteManifestPath = `${remoteSessionDirectory}/manifest.json`;
  const sshTarget = `${user}@${host}`;

  output.appendLine(`Binary: ${binaryPath} (${size} bytes)`);
  output.appendLine(`SHA-256: ${hexDigest}`);
  output.appendLine(`Target: ${formatTargetId(metadata.targetId)}`);
  output.appendLine(`Firmware version: ${metadata.firmwareVersion}`);
  output.appendLine(`Release: ${releaseId}`);

  progress.report({ message: `Preparing ${host}` });
  await runProcess(
    'ssh',
    ['-p', String(sshPort), sshTarget, remoteCommand('mkdir', ['-p', '--', remoteSessionDirectory])],
    output,
  );

  progress.report({ message: `Uploading signed release to ${host}` });
  await runProcess(
    'scp',
    ['-P', String(sshPort), binaryPath, `${sshTarget}:${remoteImagePath}`],
    output,
  );
  await runProcess(
    'scp',
    ['-P', String(sshPort), manifestPath, `${sshTarget}:${remoteManifestPath}`],
    output,
  );

  progress.report({ message: 'Authenticating and staging release on Pi' });
  await runProcess(
    'ssh',
    [
      '-p', String(sshPort), sshTarget,
      remoteCommand(gatewayExecutable, [
        'stage-local',
        '--image', remoteImagePath,
        '--manifest', remoteManifestPath,
        '--release-id', releaseId,
        '--cleanup-sources',
        '--json',
      ]),
    ],
    output,
  );

  progress.report({ message: `Waiting for target ${formatTargetId(metadata.targetId)}` });
  const expectedStatus = {
    releaseId,
    targetId: metadata.targetId,
    firmwareVersion: metadata.firmwareVersion,
  };
  let lastStatus;
  let lastGeneration = -1;
  let lastPercent = 0;
  try {
    await runProcess(
      'ssh',
      [
        '-p', String(sshPort), sshTarget,
        remoteCommand(gatewayExecutable, [
          'status',
          '--watch',
          '--json',
          '--timeout', String(statusTimeout),
          '--release-id', releaseId,
        ]),
      ],
      output,
      {
        onStdoutLine(line) {
          try {
            const status = JSON.parse(line);
            lastGeneration = validateStatusEvent(status, expectedStatus, lastGeneration);
            lastStatus = status;
            const percent = Number.isInteger(status.progress_percent)
              ? status.progress_percent
              : lastPercent;
            progress.report({
              message: formatStatus(status),
              increment: Math.max(0, percent - lastPercent),
            });
            lastPercent = Math.max(lastPercent, percent);
          } catch (error) {
            if (error instanceof SyntaxError) {
              // Forward-compatible status output may contain non-JSON diagnostics.
              return;
            }
            throw error;
          }
        },
      },
    );
  } catch (error) {
    if (lastStatus?.error) {
      throw new Error(`Pi OTA failed in ${lastStatus.state}: ${lastStatus.error}`);
    }
    throw error;
  }

  validateConfirmationStatus(lastStatus, expectedStatus);

  output.appendLine(
    `Firmware ${metadata.displayVersion} confirmed by target ${formatTargetId(metadata.targetId)}.`,
  );
  void vscode.window.showInformationMessage(
    `Sunlite OTA confirmed ${metadata.displayVersion} on ${formatTargetId(metadata.targetId)}.`,
  );
}

function validateSettings(settings) {
  const {
    host,
    user,
    sshPort,
    incomingDirectory,
    gatewayExecutable,
    objcopyPath,
    privateKeyPath,
    statusTimeout,
  } = settings;
  if (!host) {
    throw new ConfigurationError('Set sunliteOta.host to the Raspberry Pi hostname or IP address.');
  }
  if (!/^[A-Za-z0-9._:-]+$/.test(host)) {
    throw new ConfigurationError(`Invalid Raspberry Pi host: ${host}`);
  }
  if (!user || !/^[A-Za-z0-9._-]+$/.test(user)) {
    throw new ConfigurationError(`Invalid Raspberry Pi SSH user: ${user}`);
  }
  if (!Number.isInteger(sshPort) || sshPort < 1 || sshPort > 65535) {
    throw new ConfigurationError(`Invalid SSH port: ${sshPort}`);
  }
  validateRemotePath(incomingDirectory, 'incoming directory');
  if (!/^\/?[A-Za-z0-9._/-]+$/.test(gatewayExecutable)) {
    throw new ConfigurationError(`Invalid gateway executable: ${gatewayExecutable}`);
  }
  if (!objcopyPath) {
    throw new ConfigurationError('Set sunliteOta.objcopyPath to an objcopy executable.');
  }
  if (!privateKeyPath) {
    throw new ConfigurationError('Set sunliteOta.privateKeyPath to the local Ed25519 private key.');
  }
  if (!fs.existsSync(privateKeyPath)) {
    throw new ConfigurationError(`Ed25519 private key does not exist: ${privateKeyPath}`);
  }
  if (!Number.isInteger(statusTimeout) || statusTimeout < 1 || statusTimeout > 3600) {
    throw new ConfigurationError('sunliteOta.statusTimeout must be an integer from 1 to 3600 seconds.');
  }
}

function validateRemotePath(value, label) {
  if (!value.startsWith('/') || !/^\/[A-Za-z0-9._/-]+$/.test(value)) {
    throw new ConfigurationError(`Invalid ${label}: ${value}`);
  }
}

function expandHome(value) {
  return value.startsWith('~/') ? path.join(os.homedir(), value.slice(2)) : value;
}

async function resolveSelectedCMakeTarget(debugConfiguration, workspaceFolders) {
  await Promise.all([
    ensureCommand('cmake.activeFolderPath'),
    ensureCommand('cmake.getLaunchTargetPath'),
    ensureCommand('cmake.executableTargets'),
    ensureCommand('cmake.build'),
    ensureCommand('cmake.configure'),
  ]);

  const sourceDirectory = await vscode.commands.executeCommand('cmake.activeFolderPath');
  if (typeof sourceDirectory !== 'string' || !sourceDirectory) {
    throw new Error('CMake Tools has no active project. Select a CMake project folder first.');
  }

  const workspaceFolder = vscode.workspace.getWorkspaceFolder(vscode.Uri.file(sourceDirectory));
  if (!workspaceFolder) {
    throw new Error(`The active CMake project is outside the open workspace: ${sourceDirectory}`);
  }

  const requestedWorkspace = debugConfiguration.__sunliteWorkspaceFolder;
  if (requestedWorkspace) {
    const requestedFolder = workspaceFolders.find(
      (folder) => sameFilePath(folder.uri.fsPath, requestedWorkspace),
    );
    if (!requestedFolder) {
      throw new Error(`The Sunlite OTA launch workspace is no longer open: ${requestedWorkspace}`);
    }
    if (requestedFolder.index !== workspaceFolder.index) {
      throw new Error(
        `CMake Tools is active in ${workspaceFolder.name}, but Sunlite OTA was launched from ${requestedFolder.name}. Select the intended CMake folder and retry.`,
      );
    }
  }

  const elfPath = await vscode.commands.executeCommand(
    'cmake.getLaunchTargetPath',
    workspaceFolder,
  );
  if (typeof elfPath !== 'string' || !elfPath) {
    throw new Error('CMake Tools did not return an active ELF path. Select a CMake launch target first.');
  }

  const targetNames = await vscode.commands.executeCommand(
    'cmake.executableTargets',
    workspaceFolder,
  );
  if (!Array.isArray(targetNames) || !targetNames.every((name) => typeof name === 'string')) {
    throw new Error('CMake Tools did not return the executable targets for its active project.');
  }

  const candidates = [];
  for (const name of [...new Set(targetNames)]) {
    const candidatePath = await vscode.commands.executeCommand(
      'cmake.getLaunchTargetPath',
      { folder: workspaceFolder, targetName: name },
    );
    if (typeof candidatePath === 'string' && candidatePath) {
      candidates.push({ name, path: candidatePath });
    }
  }
  const target = selectExecutableTarget(elfPath, candidates);
  return {
    elfPath,
    sourceDirectory,
    targetName: target.name,
    workspaceFolder,
  };
}

async function queryBoard(connection, output) {
  const {
    gatewayExecutable,
    host,
    sshPort,
    targetId,
    user,
  } = connection;
  const sshTarget = `${user}@${host}`;
  let board;
  await runProcess(
    'ssh',
    [
      '-p', String(sshPort), sshTarget,
      remoteCommand(gatewayExecutable, [
        'query-board',
        '--target-id', formatTargetId(targetId),
        '--json',
      ]),
    ],
    output,
    {
      onStdoutLine(line) {
        try {
          const candidate = JSON.parse(line);
          if (candidate && typeof candidate === 'object' && candidate.schema === 1) {
            board = candidate;
          }
        } catch (error) {
          if (!(error instanceof SyntaxError)) {
            throw error;
          }
        }
      },
    },
  );
  if (!board) {
    throw new Error('Pi did not return machine-readable board information.');
  }
  return board;
}

async function configureAutomaticVersion(workspaceFolder, sourceDirectory, version, output) {
  const cmake = vscode.workspace.getConfiguration('cmake', workspaceFolder.uri);
  const inspection = cmake.inspect('configureArgs');
  const originalFolderArgs = inspection?.workspaceFolderValue;
  const effectiveArgs = cmake.get('configureArgs', []);
  const automaticArgs = replaceCmakeDefinitions(effectiveArgs, {
    SUNLITE_OTA_FIRMWARE_VERSION: version,
    SUNLITE_OTA_DISPLAY_VERSION: `dev-${version}`,
  });
  await cmake.update(
    'configureArgs',
    automaticArgs,
    vscode.ConfigurationTarget.WorkspaceFolder,
  );
  output.appendLine(`Configuring CMake with automatic firmware version ${version}...`);
  try {
    const result = await vscode.commands.executeCommand(
      'cmake.configure',
      workspaceFolder,
      false,
      sourceDirectory,
    );
    if (result !== 0) {
      throw new Error(
        `CMake configure failed${typeof result === 'number' ? ` with exit code ${result}` : ''}.`,
      );
    }
  } catch (error) {
    await cmake.update(
      'configureArgs',
      originalFolderArgs,
      vscode.ConfigurationTarget.WorkspaceFolder,
    );
    throw error;
  }
  return () => cmake.update(
    'configureArgs',
    originalFolderArgs,
    vscode.ConfigurationTarget.WorkspaceFolder,
  );
}

async function resolveNamedCMakeTargetPath(workspaceFolder, sourceDirectory, targetName) {
  const activeSourceDirectory = await vscode.commands.executeCommand('cmake.activeFolderPath');
  if (
    typeof activeSourceDirectory !== 'string'
    || !activeSourceDirectory
    || !sameFilePath(activeSourceDirectory, sourceDirectory)
  ) {
    throw new Error(
      'The active CMake project changed during the build. Select the intended project and retry.',
    );
  }
  const elfPath = await vscode.commands.executeCommand(
    'cmake.getLaunchTargetPath',
    { folder: workspaceFolder, targetName },
  );
  if (typeof elfPath !== 'string' || !elfPath) {
    throw new Error(`CMake Tools could not resolve the built ${targetName} executable.`);
  }
  return elfPath;
}

function sameFilePath(left, right) {
  const leftPath = path.resolve(left);
  const rightPath = path.resolve(right);
  return process.platform === 'win32'
    ? leftPath.toLowerCase() === rightPath.toLowerCase()
    : leftPath === rightPath;
}

function formatStatus(status) {
  const state = String(status.state || 'working').replaceAll('_', ' ');
  const target = Number.isInteger(status.target_id)
    ? `${formatTargetId(status.target_id)}: `
    : '';
  if (Number.isInteger(status.progress_percent)) {
    return `${target}${state}: ${status.progress_percent}%`;
  }
  return `${target}${state}`;
}

function formatTargetId(targetId) {
  const value = Number(targetId);
  if (!Number.isInteger(value) || value < 0 || value > 0xFFFFFFFF) {
    return String(targetId);
  }
  return `0x${value.toString(16).padStart(8, '0')}`;
}

function shellQuote(value) {
  return `'${String(value).replace(/'/g, `'"'"'`)}'`;
}

function remoteCommand(executable, args) {
  return [executable, ...args].map(shellQuote).join(' ');
}

function runProcess(command, args, output, options = {}) {
  output.appendLine(`> ${command} ${args.map(displayArgument).join(' ')}`);

  return new Promise((resolve, reject) => {
    const child = spawn(command, args, {
      stdio: ['ignore', 'pipe', 'pipe'],
    });
    let pendingStdout = '';
    let stdoutCallbackError;

    const deliverStdoutLine = (line) => {
      if (!options.onStdoutLine || stdoutCallbackError) {
        return;
      }
      try {
        options.onStdoutLine(line);
      } catch (error) {
        stdoutCallbackError = error instanceof Error ? error : new Error(String(error));
        child.kill();
      }
    };

    child.stdout.on('data', (data) => {
      const value = data.toString();
      output.append(value);
      if (options.onStdoutLine) {
        pendingStdout += value;
        const lines = pendingStdout.split(/\r?\n/);
        pendingStdout = lines.pop();
        for (const line of lines) {
          if (line) {
            deliverStdoutLine(line);
          }
        }
      }
    });
    child.stderr.on('data', (data) => output.append(data.toString()));
    child.on('error', (error) => reject(
      new Error(`Could not run ${command}: ${error.message}`),
    ));
    child.on('close', (code, signal) => {
      if (options.onStdoutLine && pendingStdout) {
        deliverStdoutLine(pendingStdout);
      }
      if (stdoutCallbackError) {
        reject(stdoutCallbackError);
        return;
      }
      if (code === 0) {
        resolve();
        return;
      }
      const ending = signal ? `signal ${signal}` : `exit code ${code}`;
      reject(new Error(`${command} failed with ${ending}. See the Sunlite OTA output for details.`));
    });
  });
}

async function ensureCommand(command) {
  const commands = await vscode.commands.getCommands(true);
  if (!commands.includes(command)) {
    throw new Error('The CMake Tools extension is required and must have an active configure/launch target.');
  }
}

function displayArgument(argument) {
  const value = String(argument);
  return /\s/.test(value) ? JSON.stringify(value) : value;
}

async function reportFailure(error, output) {
  const message = errorMessage(error);
  output.appendLine(`ERROR: ${message}`);
  output.show(true);
  const action = error instanceof ConfigurationError ? 'Open Settings' : undefined;
  const selected = await vscode.window.showErrorMessage(`Sunlite OTA: ${message}`, action);
  if (selected === 'Open Settings') {
    await vscode.commands.executeCommand('workbench.action.openSettings', '@ext:ubc-solar.sunlite-ota');
  }
}

function errorMessage(error) {
  return error instanceof Error ? error.message : String(error);
}

class ConfigurationError extends Error {}

module.exports = {
  activate,
  deactivate,
};
