# Benchmark baselines

What the hot paths of a match cost, so that a change which makes one of them several times slower is
noticed rather than discovered in a session. The benchmarks are not run by `ctest`: they are hidden
Catch2 cases in their own executable, built by every build and run on purpose.

```powershell
.\build\msvc-release\tests\unison_benchmarks.exe "[.benchmark]"
```

## Recorded 2026-09-22

- Machine: Windows 11, MSVC 14.51 x64, Release (`msvc-release`), Jolt with `CROSS_PLATFORM_DETERMINISTIC=ON`.
- Scene: `ArenaSimulation` with 4 players, played 200 frames of the scripted inputs before measuring.
  At that point the match holds the arena geometry, three crates, four characters and the shots in flight.

| What | Mean | Low | High |
|------|------|-----|------|
| One tick | 5.15 µs | 4.94 µs | 5.47 µs |
| Taking a snapshot | 20.9 µs | 20.6 µs | 22.5 µs |
| Restoring a snapshot | 22.0 µs | 21.6 µs | 23.9 µs |
| Checksumming a frame | 3.92 µs | 3.84 µs | 4.08 µs |
| Resimulating ten frames after a rollback | 87.3 µs | 86.3 µs | 89.8 µs |

## What the numbers say

- A tick costs 5 µs against the 16.7 ms a frame at 60 Hz has, so the simulation is not what a host
  will run out of time on.
- A rollback of ten frames costs 87 µs, which is the number that matters for netcode: a client can
  afford one on every frame it receives late input on.
- Taking or restoring a snapshot is four times a tick and the most expensive single operation. It is
  the registry clone and Jolt's state buffer together; if the snapshot ring becomes a cost, that is
  where to look first.
- Checksumming is 3.9 µs because it saves the physics state to hash it. Section 8.5 of the charter
  checksums every verified frame in tools and tests and every twentieth in a session, so this is
  affordable as it stands.
