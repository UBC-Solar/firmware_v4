# Sunlite OTA for VS Code

This extension adds **Sunlite OTA (active CMake target)** to VS Code's Run and
Debug selector. It builds the selected application, signs the exact schema-1
manifest expected by the Raspberry Pi and board bootloaders, uploads the bundle
over SSH, and follows Pi status until the selected application confirms its
firmware version. TEL is updated directly over UART; other targets are routed
by the running TEL application over CAN.

It is a one-shot flash operation. Breakpoints, stepping, and SWO still require
J-Link or ST-Link.

## One-time signing key

Create a development key on the laptop. Never copy the private key to the Pi:

```sh
mkdir -p ~/.config/sunlite
openssl genpkey -algorithm ED25519 \
  -out ~/.config/sunlite/firmware-ed25519-private.pem
chmod 600 ~/.config/sunlite/firmware-ed25519-private.pem
openssl pkey \
  -in ~/.config/sunlite/firmware-ed25519-private.pem \
  -pubout \
  -out ~/.config/sunlite/firmware-ed25519.pub
```

The same public key must be:

1. compiled into every OTA-capable board bootloader, and
2. installed on the Pi as `/etc/sunlite/firmware-ed25519.pub`.

Configure the selected board build with the public key. By default, the
extension queries the selected board through the Pi and temporarily configures
the build as `installed firmware version + 1`, so normal development flashes
need no manual version edits. A manual configure can still provide a version
when building outside the extension. For example, for TEL:

```sh
cmake --preset Debug \
  -S firmware/components/tel \
  -B firmware/components/tel/build \
  -DSUNLITE_OTA_PUBLIC_KEY_FILE="$HOME/.config/sunlite/firmware-ed25519.pub" \
  -DSUNLITE_OTA_FIRMWARE_VERSION=2 \
  -DSUNLITE_OTA_DISPLAY_VERSION=2.0.0
cmake --build firmware/components/tel/build
```

CMake generates `<board>.elf.ota.json` beside each OTA-capable application ELF.
The extension reads that file so the target ID, application-reported firmware
version, and signed manifest cannot silently diverge.

TEL's unsafe bench bypass defaults to disabled. For a controlled bench, add
`-DTEL_OTA_ENABLE_TEST_BYPASS=ON` to a Debug TEL configure; Release
builds force it off even if it remains enabled in a CMake cache. Without the
bypass, TEL requires fresh DBC-backed PARK/stopped/zero-command CAN state before
it permits its own update or forwards a remote board into its bootloader. The
extension allocates a new monotonically increasing internal version for every
flash and displays development builds as `dev-<version>`.

Install the bootloader initially with a wired probe. OTA accepts application
images only; the application is linked at `0x08008000`.

## Raspberry Pi setup

Provision the `sunlite` gateway package and services, install the public key,
and enable local staging:

```sh
sudo install -m 0644 firmware-ed25519.pub \
  /etc/sunlite/firmware-ed25519.pub
sudo systemctl enable --now sunlite-ota.path
```

The laptop must already be able to run `ssh sunlite@<pi-host>` without an
interactive password prompt. Normal SSH host-key verification remains enabled.

## VS Code settings

Add the following to the firmware workspace's `.vscode/settings.json`:

```json
{
  "sunliteOta.host": "sunlite-pi",
  "sunliteOta.user": "sunlite",
  "sunliteOta.sshPort": 22,
  "sunliteOta.incomingDirectory": "/var/lib/sunlite-ota/incoming",
  "sunliteOta.gatewayExecutable": "/home/sunlite/sunlite/.venv/bin/sunlite-ota",
  "sunliteOta.privateKeyPath": "~/.config/sunlite/firmware-ed25519-private.pem",
  "sunliteOta.objcopyPath": "arm-none-eabi-objcopy",
  "sunliteOta.buildBeforeFlash": true,
  "sunliteOta.automaticVersioning": true,
  "sunliteOta.statusTimeout": 900
}
```

Select an application launch target in CMake Tools, select **Sunlite OTA
(active CMake target)**, and press F5. The extension performs:

```text
query selected board version -> temporary CMake configure with current + 1
  -> CMake build -> ELF to BIN -> Ed25519 manifest
  -> SCP firmware.bin + manifest.json
  -> sunlite-ota stage-local --cleanup-sources
  -> systemd opens the dedicated TEL UART5 OTA gateway while the logger stays up
  -> TEL target: local UART bootloader; other target: TEL routes over CAN
  -> sunlite-ota status --watch
  -> application version confirmed
```

In a multi-root or multi-project workspace, the extension snapshots CMake
Tools' active project, launch executable, and exact executable target before
building. It then builds that target in its owning source directory instead of
building a workspace default. Only the generated `<selected ELF>.ota.json`
sidecar is accepted, and its target ID must match the selected TEL, MDI, DRD,
or STR application target. Bootloader, MST, and HVC targets are rejected. MST
and HVC remain outside this integration and acceptance pass.

Success is reported only when the Pi's status stream has increasing generation
numbers and the final `confirmed` record matches this release ID, target ID,
and monotonic firmware version. A stale or mismatched status terminates the
operation as a failure. The Pi retains terminal status by release ID, so a
later developer's release cannot hide this flash result; authenticated incoming
files are removed after the Pi durably publishes its systemd apply trigger. A
cleanup warning does not invalidate an already-committed stage.

## Develop and package the extension

Select **Develop Sunlite OTA extension** and press F5 to open an Extension
Development Host. Run its contract tests or build a VSIX with:

```sh
cd tools/vscode-sunlite-ota
npm run check
npm run package
code --install-extension sunlite-ota-0.3.0.vsix --force
```

Reload the normal firmware VS Code window after installation. The packaged
VSIX is ignored by Git; rebuild it after extension source changes.
