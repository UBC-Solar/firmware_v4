# firmware_v4
This repository contains all of the firmware for UBC Solar's fourth-generation vehicle, `Cascadia`.

Each custom device on the car for which the team has written firmware has its own folder in the `firmware/components/` directory.


When adding a firmware project for another device on the car to this repository, follow the steps on this wiki page `TODO`.

In addition to the firmware for hardware on the car in `firmware/components/`, any common library code can be found in `firmware/common`, and any tools that have been developed for working with hardware/firmware can be found in the `/tools/` folder. 


## Contributing

The firmware projects in this repository are written in C and developed using our VS Code STM32-Cube-extension based development environment. But the firmware can be built from anywhere using CMake + Ninja as long as the right dependencies are installed.

For information on getting set up to work with our build system and development environment, please visit the team's [tutorial on the STM32 VS Code extension](https://wiki.ubcsolar.com/).


Team members create branches directly on this repository to facilitate work in parallel on this codebase. Branches must use one of these naming schemes:

| Format | When to use | Example |
|---|---|---|
| `user/<name>/<project>/<feature>` | Personal work on a specific board | `user/gregorybian/tel/feature_name` |
| `user/<name>/<feature>` | Personal work not tied to one board | `user/gregorybian/hexfix` |
| `EMB-<ticket#>/<board>-<feature>` | Work tracked by a ticket | `EMB-24/tel-gps-implementation` |

- `<name>` is your first name or GitHub username.
- `<project>` / `<board>` is the board/area, e.g. `mdi`, `tel`, `drd`, `hvc`, `mst`, `str`, `common`, `tools`.
- `<ticket#>` is the EMB ticket number.
- **No spaces please.**

### Git hooks

Run this once after cloning to install the repository's git hooks:

```
make hooks
```

This sets `core.hooksPath` to `.githooks/`, which blocks commits and pushes on branches that don't follow the naming scheme above (`main` is exempt). To fix a misnamed branch, rename it with `git branch -m <new-name>`. In an emergency the check can be skipped with `--no-verify`.

### Pull requests

Once your contributions are error-free and ready to add to the main branch, create a PR with the default PR template and submit it to another team member to review and approve your work, allowing you to merge it.

PR titles must use one of these formats:

| Format | When to use | Example |
|---|---|---|
| `EMB-<ticket#> <BOARD>: <description>` | Work tracked by a ticket | `EMB-24 TEL: Add GPS` |
| `<TYPE>: <description>` | Work without a ticket | `FIX: LV current sensing` |

- `<BOARD>` is the board/area in uppercase, e.g. `MDI`, `TEL`, `DRD`, `HVC`, `MST`, `STR`, `COMMON`, `TOOLS`.
- `<TYPE>` is one of:
  - `FEAT`: new functionality
  - `FIX`: bug fix
  - `REFACTOR`: code change with no behaviour change
  - `DOCS`: documentation only
  - `TEST`: adding or changing tests
  - `BUILD`: build system, CMake, CubeMX config
  - `CI`: GitHub Actions workflows
  - `CHORE`: maintenance that doesn't fit the above
- Write `<description>` as a short imperative phrase ("Add GPS", not "Added GPS").
