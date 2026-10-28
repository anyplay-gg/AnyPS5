# AnyPS5, AnyPlay fork

This is [AnyPlay](https://anyplaynet.com)'s fork of [boykopovar/AnyPS5](https://github.com/boykopovar/AnyPS5), a compatibility layer that relinks PS5 executables into native Windows and Linux programs. It is not an emulator. The engine is the work of upstream and its authors. Get AnyPS5, report engine bugs and follow its development there.

The fork adds the **network bridge** and the nightly toolkit builds that the [AnyPlay Launcher](https://github.com/anyplaynet/launcher) uses.

## What the fork adds

- **Network bridge.** A small layer inside the system libraries that hands a game's sign-in, session and web API calls to the [AnyPlay net service](https://github.com/anyplaynet/network) on the same PC, so a game you own can play online with AnyPlay accounts. It is merged in the fork for:
  - sign-in state and identity;
  - NAT information;
  - web API requests and push events;
  - the online-permission check, answered for the AnyPlay network only;
  - peer-to-peer datagram and stream sockets through the net service.

  The bridge is off unless the launcher sets `ANYPS5_NET_ENDPOINT` and `ANYPS5_NET_TOKEN` for one game start. Without them every library behaves as in upstream. Session joins are not wired yet ([debt](docs/dev/debt/libSceNetCtl.md)). The wire format is the [bridge protocol](https://github.com/anyplaynet/network/blob/main/spec/bridge-protocol.md).
- **Observation mode.** `ANYPS5_NET_OBSERVE` writes the network calls a game makes to a file, in the [observation log format](https://github.com/anyplaynet/network/blob/main/spec/observation-log.md).
- **Nightly toolkit.** Every push to `main` publishes the [nightly release](https://github.com/anyplaynet/AnyPS5/releases/tag/nightly): relinker and system libraries for Windows and Linux, in the layout the launcher downloads or accepts as a toolkit folder. It is a pre-release, and each build replaces the last.

The bridge follows [the line](https://github.com/anyplaynet/network/blob/main/docs/adr/0021-the-line-is-drawn-at-the-law.md): no Sony servers, code or keys, and only games the player owns. The variables are listed in [ENVIRONMENT](docs/dev/ENVIRONMENT.md#libscenetctl); known gaps are in [docs/dev/debt](docs/dev/debt).

## How the fork tracks upstream

A workflow keeps one open pull request, `sync/upstream-<date>`, that merges upstream `main` into the fork every day (and on demand). It lists the upstream changes and the files both sides changed, starts the Linux and Windows builds, and opens the pull request as a draft with the conflicting files when upstream does not merge cleanly. A person reads it, and then it is merged. Everything outside the bridge stays as upstream has it, which keeps those merges small. [FORK](docs/dev/FORK.md) lists every place the fork differs and why; a script checks it against upstream.

## Contributing

- **Generic engine work** (a library, a fix, a faster path) goes upstream as a pull request to [boykopovar/AnyPS5](https://github.com/boykopovar/AnyPS5). It reaches this fork with the next sync.
- **Bridge work** goes here as a pull request. Follow [AGENTS.md](AGENTS.md), [CONTRIBUTING.md](CONTRIBUTING.md) and the [conventions](docs/dev/CONVENTIONS.md). Commit subjects and PR titles are Conventional Commits.
- **Build:** the fork's CI builds the C++ tree. The full Windows job has a 120-minute limit here.
- **Questions and ideas:** the [AnyPlay Discord](https://discord.gg/9zYqf8fEsh).

---

**English** | [Português (Brasil)](docs/user/README.pt-BR.md) | [Español](docs/user/README.es.md) | [简体中文](docs/user/README.zh-CN.md) | [Русский](docs/user/README.ru.md)

# About

Tool for automatic executables porting to Linux and Windows.

The relinker-only build runs on macOS, including Apple silicon.

[**Web converter**](https://boykopovar.github.io/AnyPS5/) - files stay on your device.

![anyps5_collage1](https://github.com/user-attachments/assets/ed2f0b2d-33d3-46d7-ab88-767ca872fc99)

Includes a [relinker](core/relinker) that converts executable to the target system's native format and implementations of [system prx libraries](core/libs/prx) suitable for dynamic linking. No emulation or separate runtime process.

[**Discord**](https://discord.gg/BHFztBPUe), [Usage](docs/user/USAGE.md), [Build instructions](docs/dev/BUILD.md), [Architecture](docs/dev/ARCHITECTURE.md), [Technical debt of the project](docs/dev/TechnicalDebt.md), [code style conventions](docs/dev/CONVENTIONS.md), [contributing](CONTRIBUTING.md)

## Status

[![libraries](https://boykopovar.github.io/AnyPS5/badge-libraries.svg)](https://boykopovar.github.io/AnyPS5/progress.html) [![shaders](https://boykopovar.github.io/AnyPS5/badge-shaders.svg)](https://boykopovar.github.io/AnyPS5/progress.html)

[![progress map](https://boykopovar.github.io/AnyPS5/progress.svg)](https://boykopovar.github.io/AnyPS5/progress.html)

[Progress history](https://boykopovar.github.io/AnyPS5/history.html)

<sub>* System libraries: percentage of the functions known to the project so far (declared in [core/libs/prx](core/libs/prx)), not of every PS5 system function. The total grows as more functions are declared.</sub>

Unsupported or unexpected states strictly throw `std::runtime_error`. `what()` is printed to stderr and the process terminates.

The [shader recompiler](core/shader/recompiler/Recompiler.cpp) successfully produces SPIR-V (validated via [Spirv-Tools](3rdparty/SPIRV-Tools) when built with `ANYPS5_ENABLE_SPIRV_TOOLS`).

## Compatibility

Per-game status and test reports live in the [Game bring-up](https://github.com/boykopovar/AnyPS5/discussions/categories/game-bring-up) discussions, one thread per title.

## Input mapping

SDL-mapped game controllers are supported, including analog sticks and triggers. Keyboard and mouse controls can be configured with an `anyps5-input.ini` file. See [input mapping](docs/user/INPUT_MAPPING.md) for the supported devices and configuration format.

## Disclaimer

This project is intended for interoperability, research, preservation, and compatibility purposes. It does not include, distribute, or require copyrighted software, firmware, cryptographic keys, or proprietary libraries. Users are responsible for ensuring that any binaries used with this project are obtained and used in accordance with applicable laws and their respective license terms.

AnyPS5 is an independent project and is not affiliated with, authorized or endorsed by Sony Interactive Entertainment. "PlayStation" and "PS5" are trademarks of Sony Interactive Entertainment Inc.

## License

This project is licensed under the GNU General Public License version 2 only.
