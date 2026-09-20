# firmware_v4
This repository contains all of the firmware for UBC Solar's fourth-generation vehicle, `Cascadia`.

Each custom device on the car for which the team has written firmware has its own folder in the `firmware/components/` directory.


When adding a firmware project for another device on the car to this repository, follow the steps on this wiki page `TODO`.

In addition to the firmware for hardware on the car in `firmware/components/`, any common library code can be found in `firmware/common`, and any tools that have been developed for working with hardware/firmware can be found in the `/tools/` folder. 


## Flashing paths

There are two host workflows. Direct CAN runs locally through PCAN; the OTA
framework runs through the Raspberry Pi and TEL. Both use the same signed
application format and board update engine.

| | Direct CAN | OTA framework |
| --- | --- | --- |
| Entry point | `tools/bootloader_send_can.py` | Sunlite OTA extension in VS Code |
| Connection | Laptop → PCAN → selected board over CAN | Laptop → SSH → Pi → UART5 → TEL → selected board over CAN |
| Host dependencies | Python, CAN adapter driver, objcopy, local signing key | VS Code extension, SSH, Pi gateway/services, local signing key |
| Build/version | Build first with a newer numeric version | Extension can query, increment the version, and build |
| Current targets | MDI, DRD, STR | MDI, DRD, STR, plus TEL itself over UART |
| Instructions | [Direct CAN guide](tools/README-can-flash.md) | [OTA framework guide](tools/vscode-sunlite-ota/README.md) |

The direct path does not require the Pi, network access, the VS Code OTA
extension, or a working TEL gateway. The shared protocol's `sunlite_ota_*` names
do not imply a runtime dependency on the OTA framework.

Provision each board's bootloader once through SWD. Both workflows must sign
with a key matching that bootloader's public key, and both advance the same
installed firmware-version counter. Switching workflows does not reset that
counter or require a different bootloader on MDI, DRD, or STR. Run one flashing
workflow at a time because PCAN and the TEL gateway use the same CAN tester
address. HVC and MST are outside the current supported target set.

Software tests cover the shared protocol and the direct sender. Each complete
hardware path still needs its own powered flash and recovery validation.

## Contributing

The firmware projects in this repository are written in C and developed using our VS Code STM32-Cube-extension based development environment. But the firmware can be built from anywhere using CMake + Ninja as long as the right dependencies are installed.

For information on getting set up to work with our build system and development environment, please visit the team's [tutorial on the STM32 VS Code extension](https://wiki.ubcsolar.com/).


Team members create branches directly on this repository to facilitate work in parallel on this codebase. Branches on the repository should follow the naming scheme


`<name>-<project>-<feature>`

where the values delimited by `<>` should be replaced by your information. **No spaces please.** You may use your first name or your GitHub username for `<name>`. For example, `EvanO12-drd-drive-state-logic`.

Once your contributions are error-free and ready to add to the main branch, create a PR with the default PR template and submit it to another team member to review and approve your work, allowing you you merge it.
