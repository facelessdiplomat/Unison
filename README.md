# Unison

A deterministic, rollback-based multiplayer ECS engine in C++20, built on EnTT, Jolt Physics and ENet.
Every client runs an identical simulation; only player inputs travel over the network. The same
simulation runs headless in a terminal and inside Unreal Engine 5 as a plugin.

## Documentation

- [Design charter](docs/DESIGN.md) — architecture, determinism rules, decision log, roadmap.
- [Task board](docs/TASKS.md) — phases, tasks and micro-tasks with their progress.
- [Development rules](CLAUDE.md) — naming, testing discipline, self-review and commit policy.

## Build

Windows x64 with Visual Studio 18 (MSVC 14.51), from a plain PowerShell in the repository root:

```powershell
.\tools\env.ps1
cmake --workflow --preset msvc-debug
```

macOS arm64 with Apple clang from Xcode or its Command Line Tools, from Terminal in the repository root:

```sh
brew install cmake ninja llvm@20
cmake --workflow --preset clang-debug
```

A workflow configures, builds with warnings as errors and runs every test; the presets are `msvc-debug` and
`msvc-release` on Windows, `clang-debug` and `clang-release` on macOS, each building into `build/<preset>`.
`tools/env.ps1` locates Visual Studio through `vswhere`, enters the x64 developer environment for the
current session, and exports the bundled CMake, Ninja and clang-format as `UNISON_CMAKE`, `UNISON_NINJA`
and `UNISON_CLANG_FORMAT`; on macOS, `source tools/env.sh` exports Homebrew's clang-format 20, the version
Visual Studio bundles. `.\tools\ci.ps1` and `tools/ci.sh` run both configurations and the formatting check and
end with `ci: ok`.

## License

MIT, see [LICENSE](LICENSE).
