# Unison

A deterministic, rollback-based multiplayer ECS engine in C++20, built on EnTT, Jolt Physics and ENet.
Every client runs an identical simulation; only player inputs travel over the network. The same
simulation runs headless in a terminal and inside Unreal Engine 5 as a plugin.

## Documentation

- [Design charter](docs/DESIGN.md) — architecture, determinism rules, decision log, roadmap.
- [Task board](docs/TASKS.md) — phases, tasks and micro-tasks with their progress.
- [Development rules](CLAUDE.md) — naming, testing discipline, self-review and commit policy.

## Build

Windows x64 with Visual Studio 18 (MSVC 14.51). From a plain PowerShell in the repository root:

```powershell
.\tools\env.ps1
cmake --preset msvc-debug
cmake --build build/msvc-debug
```

`tools/env.ps1` locates Visual Studio through `vswhere`, enters the x64 developer environment for the
current session, and exports the bundled CMake, Ninja and clang-format as `UNISON_CMAKE`, `UNISON_NINJA`
and `UNISON_CLANG_FORMAT`. The configure presets are `msvc-debug` and `msvc-release`.

## License

MIT, see [LICENSE](LICENSE).
