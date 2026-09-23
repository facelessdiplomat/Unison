# Benchmark baselines

What the hot paths of a match cost, so that a change which makes one of them several times slower is
noticed rather than discovered in a session. The benchmarks are not run by `ctest`: they are hidden
Catch2 cases in their own executable, built by every build and run on purpose.

```powershell
.\build\msvc-release\tests\unison_benchmarks.exe "[.benchmark]"
```

A baseline is only ever recorded on an idle machine: numbers taken while a game or a build shares the
processor measure the neighbour, not the engine.

## Recorded 2026-09-23, after 6.1.4

- Machine: Windows 11, MSVC 14.51 x64, Release (`msvc-release`), Jolt with `CROSS_PLATFORM_DETERMINISTIC=ON`.
  Idle: no process but the benchmark used a tenth of a core before or after the run, and three runs agreed
  within 5 %.
- Scene: `ArenaSimulation` with 4 players, played 200 frames of the scripted inputs before measuring.
  At that point the match holds the arena geometry, three crates, four characters and the shots in flight.

| What | Mean | Low | High |
|------|------|-----|------|
| One tick | 4.99 µs | 4.79 µs | 5.24 µs |
| Taking a snapshot | 3.71 µs | 3.69 µs | 3.82 µs |
| Restoring a snapshot | 4.10 µs | 4.05 µs | 4.31 µs |
| Checksumming a frame | 3.76 µs | 3.75 µs | 3.84 µs |
| Resimulating ten frames after a rollback | 66.1 µs | 65.9 µs | 67.1 µs |

## What the numbers say

- A tick costs 5 µs against the 16.7 ms a frame at 60 Hz has, so the simulation is not what a host
  will run out of time on.
- A rollback of ten frames costs 66 µs, which is the number that matters for netcode: a client can
  afford one on every frame it receives late input on.
- Taking or restoring a snapshot costs less than a tick. Both copy into registries that have held a match
  before and keep the room of every pool (2.2.2), and a restore reads Jolt's state straight from the
  snapshot (6.1.4); what is left is the copy itself and the work Jolt does on its state buffer.
- Checksumming is 3.7 µs because it saves the physics state to hash it. A session checksums the snapshot
  of a verified frame instead, which already holds that state, and section 8.5 of the charter checksums
  every twentieth verified frame in a session, so this is affordable as it stands.

## Since a tick takes nothing from the C++ heap

The baseline recorded after 2.2.2, earlier the same day, against the one above. The scene and the build are
the same.

| What | After 2.2.2 | After 6.1.4 | Difference |
|------|-------------|-------------|------------|
| One tick | 5.31 µs | 4.99 µs | −6 % |
| Taking a snapshot | 3.70 µs | 3.71 µs | none |
| Restoring a snapshot | 4.31 µs | 4.10 µs | −5 % |
| Checksumming a frame | 3.75 µs | 3.76 µs | none |
| Resimulating ten frames after a rollback | 66.8 µs | 66.1 µs | −1 % |

A restore no longer copies Jolt's state buffer or gathers bodies and characters into lists first, and a
shot asks for the nearest thing in its way instead of every hit; allocations that cost little alone, and
that 6.1.4 removed for the steadiness of a host's frame rather than for speed.

## Since the snapshot ring keeps its room

The first baseline, recorded on 2026-09-22 before 2.2.2, against the one recorded after 2.2.2. The scene and
the build are the same.

| What | 2026-09-22 | After 2.2.2 | Difference |
|------|------------|-------------|------------|
| One tick | 5.15 µs | 5.31 µs | within the noise |
| Taking a snapshot | 20.9 µs | 3.70 µs | −82 % |
| Restoring a snapshot | 22.0 µs | 4.31 µs | −80 % |
| Checksumming a frame | 3.92 µs | 3.75 µs | −4 % |
| Resimulating ten frames after a rollback | 87.3 µs | 66.8 µs | −24 % |

A snapshot used to build every pool of its registry anew, and a restore threw the live registry away; that
was four fifths of the cost of either. A tick moves within its own spread: three runs gave 5.05 to 5.33 µs.

## SSE2 against AVX2, 2026-09-22

The same scene and the same build type, configured with `-DUNISON_INSTRUCTION_SET=AVX2`, which moves our
deterministic libraries and Jolt together. Every test passes in both, and both settle on the same golden
checksums, so what follows is only about speed. It was measured before 2.2.2, so its snapshot numbers
belong to the first baseline.

| What | SSE2 | AVX2 | Difference |
|------|------|------|------------|
| One tick | 5.15 µs | 4.81 µs | −6 % |
| Taking a snapshot | 20.9 µs | 20.9 µs | none |
| Restoring a snapshot | 22.0 µs | 22.0 µs | none |
| Checksumming a frame | 3.92 µs | 3.86 µs | −2 % |
| Resimulating ten frames after a rollback | 87.3 µs | 84.1 µs | −4 % |

Six per cent of five microseconds against a frame of 16.7 ms is not worth a binary that refuses to start on
a machine without AVX2, so SSE2 stays the baseline. That the checksums agree is worth more than the speed:
it says the determinism contract is doing the work, not the instruction set.
