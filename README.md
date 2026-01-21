# firmware_v4

This repository contains all of the firmware for UBC Solar's fourth-generation vehicle, `Cascadia`

Each custom device on the car for which the team has written firmware has its own folder in the `firmware/components/` directory.

When adding a firmware project for another device on the car to this repository, follow the steps on this wiki page `TODO`.

In addition to the firmware for hardware on the car in `firmware/components/`, any common library code can be found in `firmware/common`, and any tools that have been developed for working with hardware/firmware can be found in the `/tools/` folder. 

## Contributing

The firmware projects in this repository are written in C and developed using our VS Code STM32-Cube-extension based development environment. But the firmware can be built from anywhere using CMake + Ninja as long as the right dependencies are installed.
For information on getting set up to work with our build system and development environment, please visit the team's [tutorial on the STM32 VS Code extension](https://wiki.ubcsolar.com/).


Team members create branches directly on this repository to facilitate work in parallel on this codebase. Branches on the repository should follow the naming scheme

`<name>-<project>-<feature>`

where the values delimited by `<>` should be replaced by your information. **No spaces please.** You may use your first name or your GitHub username for `<name>`. For example, `a2k-hanlon/bms/fsm`.

Once your contributions are error-free and ready to add to the main branch, create a PR with the default PR template and submit it to another team member to review and approve your work, allowing you you merge it.
