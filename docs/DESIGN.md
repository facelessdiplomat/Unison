# Unison — Deterministic Multiplayer ECS Engine

**Design Charter & Expectations** · Draft v0.1 · 2026-09-21

This document records what we agreed to build, why, and how we will know it is done.
It is the reference for every later design decision: if a change contradicts this
document, either the change is wrong or this document must be updated first.

Development rules (comments, naming, SOLID, testing discipline, self-review, commit policy)
live in `CLAUDE.md` at the repository root; formatting is defined by `.clang-format`.
Progress is tracked on the task board in `docs/TASKS.md`.

---

## 1. Vision

Unison is a deterministic, rollback-based multiplayer simulation engine written in
C++20, in the spirit of **Photon Quantum**, but engine-agnostic and built from proven
open-source components.

The simulation is fully isolated from any host engine. The same gameplay code runs:

- **headless in a terminal** (development runner, tests, CI, replay tools, console client), and
- **inside Unreal Engine 5** as a plugin (first host engine; others later).

Every client runs an identical copy of the simulation. Only player inputs travel over
the network. A lightweight relay server orders inputs and never simulates anything.

### 1.1 Goals

1. **Bit-exact determinism** across all clients in a session, verified continuously by checksums.
2. **Responsive play** via input prediction and rollback (no input delay by default).
3. **Isolation**: the simulation has no rendering, no I/O, no wall clock, no host-engine types.
4. **One gameplay codebase, many hosts**: terminal runner and UE plugin consume the same static libraries.
5. **Prefer proven open source** over bespoke code wherever a suitable library exists.

### 1.2 Non-goals for v1

- Authoritative-server or peer-to-peer topologies (relay only).
- Platforms other than Windows x64 / MSVC for the *simulation* (the relay is portable).
- A DSL / code generator for components (Quantum's `.qtn`).
- A stable C ABI for non-C++ hosts (Unity, Godot).
- Navigation / pathfinding, 2D-specific physics, lobbies, matchmaking, authentication, encryption.
- Multiple local players per client (one client = one player slot in v1).

---

## 2. Definition of Done for v1

v1 is complete when **all** of the following hold:

1. `unison_runner --players 4 --frames 36000 --latency 120 --jitter 30 --loss 5` runs
   four clients plus an in-process relay through a simulated network and exits with code 0:
   all clients' verified-frame checksums are equal on every frame, and rollback depth
   never exceeds the configured window.
2. Two machines on a LAN run `unison_relay` and two `unison_console` clients and play the
   Arena sample at 60 Hz for 10 minutes with rollback and zero desyncs.
3. A late-joining client can join at any moment (runner and real network), receives a
   snapshot, fast-forwards, and its checksums match from the first verified frame onward.
4. A client that disconnects for 5 seconds reconnects into the same player slot.
5. A spectator client follows a match without an input slot.
6. A replay recorded from a network session verifies with `unison_replay verify`; a
   deliberately corrupted replay reports the first divergent frame and a state diff.
7. The Unreal sample project plays the Arena with two or more clients through the relay:
   entity actors spawn/despawn from simulation events, rendering interpolates smoothly at
   60 / 120 / 144 FPS while the simulation ticks at 60 Hz.
8. Debug and Release builds (MSVC, Windows x64) produce identical checksums over the
   committed golden replay. This is the cheapest cross-optimization determinism check.
9. Documentation exists for: this charter, the session/view-bridge API, and a
   "how to add a component and a system" guide.

---

## 3. Decision Log

Decisions taken in the kickoff discussion. Superseded entries are kept on purpose:
they explain why the architecture looks the way it does.

| # | Topic | Decision | Alternatives considered | Rationale |
|---|-------|----------|-------------------------|-----------|
| D1 | Sync model | **Predict + Rollback** (Quantum / GGPO style) | Lockstep with input delay; both modes | Zero input delay; one player's lag does not stall others. Input delay stays configurable (0 by default) so a lockstep-like tuning remains possible. |
| D2 | Topology | **Relay server** (orders inputs, stores nothing about game state, brokers late-join snapshots) | Authoritative server; P2P; abstract transport | Cheap servers, simulation only on clients, anti-cheat by checksum comparison. The core sees only an abstract transport anyway. |
| D3 | Math | ~~Fixed-point~~ → **Deterministic float** (superseded by D9) | Fixed-point; hybrid | See D9. |
| D4 | Docs language | **English** | Russian; mixed | Open-source readiness. Discussion may continue in Russian. |
| D5 | Component definition | **Plain C++ POD structs**, no code generation | Quantum-like DSL + codegen; C++ now, codegen later | No extra build step, trivial UE integration. Reflection needed for checksums/diffs is done with a small registration macro. Codegen remains a backlog item. |
| D6 | State storage | ~~Single linear memcpy block~~ → **EnTT registry + Jolt physics state + globals**, snapshot ring (superseded by D10) | Archetypes (flecs); sparse sets | See D10. |
| D7 | Memory policy | ~~Own heap inside Frame~~ → **Components are trivially copyable; variable-length data via fixed-capacity containers or child entities** | Fixed arrays only; STL containers | Trivially copyable components make per-pool snapshots a bulk copy and checksums a plain hash. A frame-local heap can be added later if a real need appears. |
| D8 | Physics scope | **3D from the start** | 2D first; none in v1; both later | Target games are 3D. Made affordable by D9. |
| D9 | Math + physics route | **Jolt Physics + deterministic float** | Fixed-point + own physics; hybrid fixed-point gameplay + Jolt | No mature open-source fixed-point 3D physics exists. Jolt (MIT) ships a `CROSS_PLATFORM_DETERMINISTIC` build option, `SaveState`/`RestoreState` for rollback, a character controller, and a deterministic math library. The cost is strict floating-point discipline (Section 7). |
| D10 | ECS | **EnTT** (MIT, header-only) | flecs; custom arena ECS | Most widely used C++ ECS, deterministic iteration order given deterministic operations. Snapshot is a per-pool bulk copy rather than one memcpy; acceptable and benchmarked early. flecs v4 dropped built-in snapshots. |
| D11 | Transport | **ENet** (MIT) | GameNetworkingSockets; custom UDP | Small, mature, reliable + unreliable channels over UDP, builds everywhere. Encryption / Steam relay is a backlog item. |
| D12 | Determinism platforms (v1) | **Windows x64, MSVC only** | Windows + Linux; + macOS/ARM64 | Smallest scope. The relay does not simulate, so it can still be hosted on Linux. Wider matrix is backlog. |
| D13 | Host API | **C++ API, core as static libraries** | C++ + C ABI; C++ now, C ABI later | UE links ThirdParty static libs natively. The public API avoids templates and inline logic at the boundary so a C wrapper can be added later. |
| D14 | View layer | **Events + direct state reads with interpolation** | Polling only | Quantum model. Events split into verified-only and predicted (cancellable on rollback). |
| D15 | Tick rate | **60 Hz default**, configurable | 30 Hz; undecided | Action-game standard; tests must also pass at 30 Hz. |
| D16 | C++ standard | **C++20** | C++17; C++23 | Matches UE 5.3+; fully supported by MSVC. |
| D17 | Build | **CMake + CPM.cmake** | vcpkg manifest; git submodules | Single build system, pinned dependency versions, Jolt built from source with our determinism flags. |
| D18 | Tests | **Catch2 v3** | GoogleTest; doctest | Sections, generators, built-in benchmarks, CTest integration. |
| D19 | UE version | **UE 5.8** (installed on the development machine; re-check at the start of Phase 5) | 5.6; 5.7; latest at Phase 5 | Plugin phase comes after the terminal phases; the installed version is the natural target. |
| D20 | Sample game | **3D Arena** | Physics sandbox; 1v1 fighter | Exercises input, prediction, rollback, events, character kinematics and rigid-body dynamics at once. |
| D21 | Players | **2–4 in MVP, protocol designed for 8** | Strictly 2; 16+ | Typical rollback range. |
| D22 | v1 session features | **Replay, desync detection, late-join, reconnect, spectators, network simulator** | subsets | All selected. |
| D23 | Terminal tooling | **Multi-client runner, replay CLI, real relay + console client executables, text visualization** | subsets | All selected. |
| D24 | Name | **Unison** — namespace `unison::`, CMake targets `unison_*`, UE plugin `Unison` | Metronome; Concord; Replica | All clients play the same simulation in unison. |
| D25 | License | **MIT** | Apache-2.0; private | Compatible with every dependency. |
| D26 | Repository | **Monorepo**; git initialised by the owner later | Separate repos | Lower overhead at this stage. |
| D27 | Team & pace | **Solo with Claude Code, no deadline** | — | Phase-based roadmap without dates; automated tests and a mandatory self-review replace peer review. |
| D28 | Naming | **PascalCase types, camelCase functions and variables, `kConstants`, snake_case files** | snake_case everywhere (STL/EnTT); Unreal style | Most common convention in game engines; matches this document. |
| D29 | Formatting | **clang-format: Allman braces, 4 spaces, 120 columns** | Attached braces (LLVM); Allman + tabs | Owner's preference, close to Unreal and Jolt. Never discussed in reviews. |
| D30 | Testing discipline | **TDD: a failing test precedes every micro-feature** | Tests in the same change; tests per phase | Forces API design from usage; makes rule 6 of `CLAUDE.md` mechanical. |
| D31 | Commits | **Conventional Commits in English, only on the owner's command, authored by the owner alone** | Automatic commits after self-review; free-form messages | Owner keeps control of history; no AI attribution anywhere. |
| D32 | Errors | **Three tiers: Debug-only `UNISON_ASSERT`, always-on `UNISON_VERIFY` with a fatal handler, `tl::expected` for recoverable boundary errors** | Asserts always on; error-code enums | Debug and Release stay behaviourally identical; no exceptions; recoverable errors are explicit in signatures. |
| D33 | Work rhythm | **Stop after every micro-task**: self-review, report, wait for the owner's "commit" or "continue" | Stop at task boundaries; standing commit authorisation | Maximum control for the owner; one commit per micro-task falls out naturally. Single branch `main`. |
| D34 | Units and axes | **Jolt-native: metres, seconds, kilograms, radians, Y-up, right-handed**; conversion only in host adapters | Z-up right-handed; Unreal-native | Zero conversions in the heaviest consumer (physics, character controller); one tested conversion at the Unreal boundary. |

---

## 4. Photon Quantum Concept Mapping

| Quantum | Unison | Notes |
|---------|--------|-------|
| `Frame` | `unison::Frame` | Mutable simulation state: EnTT registry + Jolt physics + globals. |
| `QuantumRunner` | `unison::SessionRunner` | Drives the session from host time; produces ticks. |
| `DeterministicSession` | `unison::Session` | Rollback orchestration, inputs, checksums. |
| Photon Server (input relay) | `unison_relay` | Orders inputs, never simulates. |
| `FP`, `FPVector3` | `float`, `JPH::Vec3` (deterministic build) | See Section 7. |
| Systems (`SystemMainThread`) | `unison::ISystem` | Ordered pipeline, single-threaded in v1. |
| Input struct (`.qtn`) | Game-defined POD `Input` | Fixed size, quantised fields. |
| Events (synced / non-synced) | Events (verified-only / predicted) | Predicted events can be cancelled on rollback. |
| Signals | Signals | Sim-internal callbacks between systems (e.g. `OnCollision`). |
| Assets (`AssetObject`) | Asset tables (`AssetId`) | Immutable data, hashed into the session config. |
| `EntityView` / `EntityViewUpdater` | `EntityView` bridge (host side) | Maps `entt::entity` to actors/objects. |
| Checksums, replays | Same | Built in from day one. |

---

## 5. Architecture

### 5.1 Layers

```
+-------------------------------------------------------------------------+
|  Hosts:  Unreal plugin  |  unison_console (TUI)  |  unison_runner (CI)  |
+-------------------------------------------------------------------------+
|  unison_view      event dispatch, entity views, interpolation helpers   |
+-------------------------------------------------------------------------+
|  unison_session   rollback, input buffers, prediction, snapshot ring,   |
|                   checksums, replays, late-join, time sync              |
+-----------------------------------+-------------------------------------+
|  unison_sim                        |  unison_net                        |
|  Frame, systems, events, signals,  |  ITransport, loopback + network    |
|  RNG, assets, physics glue (Jolt)  |  simulator, ENet, relay protocol   |
+-----------------------------------+-------------------------------------+
|  unison_core      math (Jolt), hashing (xxHash), fixed containers,      |
|                   serialization, FP-environment guard, log hooks        |
+-------------------------------------------------------------------------+
|  game_sim (e.g. arena_sim)  components, systems, input, assets — the    |
|  ONLY place gameplay logic lives; compiled with determinism flags       |
+-------------------------------------------------------------------------+
```

Dependency direction is strictly top-down. `unison_sim`, `unison_core`, and every
`*_sim` game library are **deterministic libraries**: they are compiled with the flags
of Section 7 and contain everything that mutates simulation state. Hosts may *read*
state freely; they never mutate it except through `Session` inputs.

### 5.2 Modules and responsibilities

| Module | Type | Responsibilities | Dependencies |
|--------|------|------------------|--------------|
| `unison_core` | static lib | Math re-exports (Jolt `Vec3`, `Quat`, `Mat44`, trig), `FixedVector<T,N>`, `FixedString<N>`, `Hasher` (XXH3), `BinaryWriter/Reader`, `FpEnvGuard`, `LogSink` callback, `AssetId` | Jolt (math only), xxHash |
| `unison_sim` | static lib | `Frame`, `SystemPipeline`, `ISystem`, `EventBuffer`, `Signals`, `Rng`, `AssetRegistry`, `PhysicsWorld` (Jolt wrapper with deterministic body lifecycle) | EnTT, Jolt, core |
| `unison_session` | static lib | `Session` (rollback state machine), `InputBuffer`, `SnapshotRing`, `Checksum`, `ReplayWriter/Reader`, `SnapshotSerializer` (late-join), `TimeSync` | sim, net |
| `unison_net` | static lib | `ITransport`, `LoopbackTransport` + `NetworkSimulator`, `EnetTransport`, relay protocol messages, `RelayCore` (reusable by in-process and standalone relay) | ENet, core |
| `unison_view` | static lib | Event dispatch with raise/cancel semantics, `EntityViewMap`, `TransformInterpolator`, read-only frame accessors | session |
| `unison_relay` | executable | Standalone relay server over ENet, portable (Windows/Linux) | net |
| `unison_runner` | executable | N clients + in-process relay + network simulator; checksum comparison; exit code for CI | session, view, game sim |
| `unison_replay` | executable | record / play / verify / diff | session, game sim |
| `unison_console` | executable | Console client with text visualisation and keyboard input | view, net, game sim |
| `arena_sim` | static lib | The sample game's deterministic code | sim |
| `arena_view_console` | static lib | Text renderer for Arena | view, arena_sim |
| `Unison` (UE plugin) | UE plugin | `UnisonRuntime` module linking the libs above; subsystem, input, entity views, events, debug HUD | UE, all libs |

### 5.3 Engine-wide conventions

- **Units and axes**: the simulation uses Jolt's native conventions: metres, seconds, kilograms, radians,
  Y-up, right-handed. Host adapters convert at the boundary (Unreal: centimetres, Z-up, left-handed);
  nothing else in the engine knows about host conventions.
- **Errors and contracts** (exceptions are disabled):
  - `UNISON_ASSERT(cond)` checks programmer contracts. Active in Debug only, compiled out in Release, never
    has side effects, so Debug and Release simulate identically.
  - `UNISON_VERIFY(cond)` checks invariants that must hold in every build. On failure it reports through
    `LogSink` and calls the installed fatal handler (default: abort; the Unreal adapter installs its own).
  - Recoverable failures at boundaries (network parsing, joining a session, file formats) return
    `tl::expected<T, Error>`. Simulation code never returns errors: an invalid state is a contract violation.
- **Encoding**: little-endian byte order (v1 targets x64 only), `uint32_t` frame numbers, `uint8_t` slot ids,
  `uint32_t` entity ids at the host boundary.
- **Logging**: `LogSink` is a process-wide callback and is one of the two pieces of global mutable state the
  engine allows. It exists because `UNISON_VERIFY` is a macro and cannot take an injected dependency, and
  because a host installs one sink for the whole process. The exception is bounded: the sink is write-only
  from the simulation's side, nothing in a deterministic library reads it back, and no simulation result
  depends on whether a sink is installed.
- **Jolt's process-wide registration**: Jolt keeps its allocator, its factory and its type list in globals of
  its own and offers nothing to inject, so `JoltRuntime` counts the scopes that need them, registers for the
  first and unregisters after the last. It is the second piece of global mutable state the engine allows.
  The exception is bounded: the registration is installed before any body exists, no deterministic library
  reads it back, the counter changes only on the simulation thread, and no simulation result depends on it.
  Everything else keeps the constructor injection of `CLAUDE.md` 4.
- **Threading**: the simulation runs on one thread chosen by the host (the game thread in Unreal). Unison never
  creates threads inside deterministic libraries.

---

## 6. Simulation Model

### 6.1 Frame

A `Frame` is the complete mutable state of the game at tick `N`:

- `frameNumber`, `tickRate`, `dt` (fixed, e.g. 1/60 s as `float`);
- an `entt::registry` holding all entities and components;
- a Jolt `PhysicsSystem` wrapped by `PhysicsWorld` (bodies, contacts, constraints);
- globals: `Rng` state, match state (phase, timers), player-slot table;
- the `EventBuffer` for events raised during this tick (cleared each tick).

`advanceFrame(frame, pipeline, inputs)` runs the system pipeline exactly once. It is a free function, not a
method: a frame holds state and nothing else, and the pipeline belongs to the game rather than to the frame.
It clears the events of the previous tick before the systems run, so the events a tick raises are still there
for the view when it returns and none of them outlives the tick. Given the same starting
state and the same inputs, `advance` produces a bit-identical result on every client.

### 6.2 Systems

- Systems are plain classes implementing `void update(Frame&, const FrameInputs&)`.
- The pipeline order is a static list defined by the game and hashed into the session config.
- Single-threaded in v1. Parallelism inside a tick is a Phase 6 evaluation.
- Systems communicate through components, signals (synchronous sim-internal callbacks), and events (sim → view only).

### 6.3 Components

- Trivially copyable POD structs. No pointers, no references, no STL containers, no virtuals.
- Entity references are `entt::entity`. Because the simulation is deterministic, entity ids
  are identical on all clients and can be used as network-stable identifiers.
- Variable-length data uses `FixedVector<T, N>` / `FixedString<N>` (inline storage, memcpy-safe)
  or is modelled as child entities (e.g. inventory items with an `Owner` component).
  Note: ETL's fixed containers hold internal pointers and are therefore not memcpy-safe, so we
  keep our own ~200-line implementations.
- Every component is registered with `UNISON_COMPONENT(Type)` in a static list. The registration
  order defines the order used by snapshots and checksums; EnTT's runtime `type_index` order is never used.
  All registrations live in one translation unit: static initialisation order across translation units is
  unspecified, so the list order, and with it every snapshot and checksum, would differ between Debug and
  Release without a word. `ComponentRegistry` rejects a registration arriving from a second file.
- Math fields in components are plain POD types from `unison_core` (`Float3`, `Quaternion`: three or four
  floats, natural alignment, no padding). Jolt vector types are used for computation inside systems and
  physics code only. This keeps padding bytes out of checksums and Jolt headers out of host-facing headers.
- All fields are value-initialised. Padding bytes are zeroed by construction (`= {}`) so hashes are stable.
- A component that is registered carries no padding at all: `UNISON_COMPONENT` static-asserts the `PaddingFree`
  concept, which walks the members with Boost.PFR and compares their sizes against `sizeof`, recursing through
  aggregate members and arrays. `std::has_unique_object_representations_v` cannot serve here: it rejects every
  type holding a `float`, because `-0` and NaN give one value several bit patterns, and every component holds
  floats. `RawValue`, `Hasher` and `BinaryWriter` stay permissive, because they also carry engine-internal
  types whose padding never reaches a frame.

### 6.4 Inputs

- The game defines a fixed-size POD `Input` (max 64 bytes) with quantised fields:
  movement as `int8`, look angles as `int16`, buttons as bit flags. Hosts convert analog
  values to these integers; no host float reaches the simulation.
- `FrameInputs` = one `Input` per player slot plus a per-slot `flags` byte (present / predicted / dropped).
  It is a plain class, not a template over `Input`: the slots hold the input erased to bytes, and only the
  accessor is typed. A template would spread through `ISystem`, `SystemPipeline` and `Session`, and §7.3
  forbids a host module from instantiating simulation templates, which the Unreal plugin would have to do to
  drive a session. The 64-byte cap on `Input` exists to make the erased slot possible.
- Prediction policy: repeat the last known input of each remote player.

### 6.5 Events and signals

- **Events** carry POD payloads and an `EventKey = (frame, typeId, ordinal)`.
  - *Verified-only* events are delivered to the view only once their frame is verified.
  - *Predicted* events are delivered immediately; after a rollback the view receives
    `cancelled(key)` for events not re-raised and `raised` for new ones. The view layer
    deduplicates by key.
- **Signals** are synchronous sim-internal callbacks (e.g. `OnHit`) and never leave the simulation.

### 6.6 Assets

Immutable design data (arena layout, character stats, projectile definitions) is loaded
before the session starts, referenced by `AssetId` (32-bit hash of the asset name), and
hashed into `SessionConfig.assetHash`. Clients with different asset hashes cannot join
the same session.

### 6.7 Physics (Jolt)

`PhysicsWorld` wraps a Jolt `PhysicsSystem` with rules that keep it rollback-safe:

- **Process-wide setup is scoped.** Every `PhysicsWorld` holds a `JoltRuntime`, which installs Jolt's
  allocator, factory and type list for the first world and removes them after the last one is gone,
  so several worlds may exist at once (§5.3). A world also owns its own fixed scratch block and
  single-threaded job system; the limits it is built with are `PhysicsWorldSettings`.
- **Two object layers, `Static` and `Moving`**, each with a broad phase tree of its own. Two static
  bodies never collide, which keeps the pairs that cannot move out of the broad phase.
- **Body lifecycle is owned by the ECS.** A `PhysicsBody` component holds a `BodyId`, the engine's own
  handle packed exactly as Jolt packs a `BodyID` and defined without Jolt headers, so components and
  frame globals stay free of them, and an `AssetId` for the shape and settings. Bodies are created
  with `CreateBodyWithID` from ids that `BodyIdAllocator` hands out in frame globals, so recreated
  bodies get the same ids on every client and after every restore. The body table of a world holds
  `kMaxBodies` slots, the same limit the allocator hands indices out from.
- **Restore = reconcile, then `RestoreState`.** Jolt's `RestoreState` requires the same set of
  bodies and constraints to exist as when `SaveState` ran. After restoring the registry,
  `PhysicsWorld` destroys bodies that are no longer referenced, recreates missing ones from
  their components, and only then calls `RestoreState` with `EStateRecorderState::All`.
  Properties Jolt does not record (friction, restitution, motion type, shape changes) are
  stored in components and reapplied during reconciliation.
- **Query results are sorted** before use. Jolt returns broad/narrow-phase results and
  `GetActiveBodies` in non-deterministic order; the wrapper sorts by `BodyID`.
- **Contact callbacks are buffered**, then sorted by `(BodyID a, BodyID b, sub-shape ids)` and
  delivered to systems in that order. Listener callbacks may run on multiple threads inside Jolt.
- **Character movement** uses Jolt's `CharacterVirtual` (kinematic, deterministic), whose
  state is also stored in components and restored explicitly.
- **Single-threaded** Jolt job system in v1 (`JobSystemSingleThreaded`). Multi-threaded stepping
  is evaluated in Phase 6 only after a determinism test proves it identical. The body mutex count is
  pinned to one rather than auto-detected, which would derive it from the core count of the machine.
- Jolt is built with `CROSS_PLATFORM_DETERMINISTIC=ON` (≈8 % slower), exceptions off, RTTI off.

---

## 7. Determinism Rules

These rules apply to every deterministic library (`unison_core`, `unison_sim`, `*_sim`).
They are enforced by compiler flags in `cmake/UnisonDeterminism.cmake`, by code review
checklists, and by the tests in Section 11.

### 7.1 Compiler flags (MSVC, v1)

- `/fp:precise` — never `/fp:fast`. Do not pass `/fp:contract`; fused multiply-add
  contraction must stay off.
  `/fp:precise` is always passed explicitly: MSVC defines no `_M_FP_*` macro when no `/fp:` flag is
  given, so `determinism_guard.hpp` rejects that case too and no library rests on a compiler default.
- A single, fixed `/arch:` baseline for all deterministic libraries and for Jolt (default: SSE2;
  AVX2 only if a benchmark proves it is needed, in which case all libraries move together).
- Exceptions off, RTTI off (matches Unreal's defaults; EnTT and Jolt support both), applied to every Unison
  target by `unison_apply_language_subset`, not only to the deterministic ones. `/EHs-c-` alone is not
  enough on MSVC: the STL keeps emitting `try`/`catch` that cannot unwind unless `_HAS_EXCEPTIONS=0` is
  defined as well, and Jolt defines it for itself, so a target without it also disagrees with Jolt.
  `unison_apply_determinism` adds the floating-point half on top. EnTT needs `ENTT_NOEXCEPTION` for the same
  reason and polices it itself with `detect_mismatch`, so it is defined on its interface target and reaches
  every consumer, tests included. The test executable is the one target that
  declares exceptions on, because Catch2 needs them; `tests/cmake/module_determinism` asserts each choice.
- `/W4 /WX` for our own code.
- Future Clang/GCC ports: `-ffp-model=precise -ffp-contract=off`, no `-ffast-math`,
  `-fexcess-precision=standard`.

### 7.2 Runtime environment

- `FpEnvGuard` saves the MXCSR control word at the start of every tick, sets round-to-nearest with
  denormals enabled, and restores the host's state afterwards. Host engines and audio libraries
  are free to change flush-to-zero on their threads; the simulation is not affected.
- The simulation never reads the wall clock, thread ids, addresses, environment variables, or files.

### 7.3 Code rules

| Rule | Reason |
|------|--------|
| No `std::sin/cos/tan/atan2/exp/log/pow` — use `JPH::Sin`, `JPH::Cos`, `JPH::ATan2`, etc. | libm results differ between platforms and CRT versions. `std::sqrt`, `floor`, `ceil`, `fmod`, `abs` are IEEE-exact and allowed. |
| No `std::lerp`, `std::hypot`, `std::fma` | May use FMA or extended precision internally. |
| No iteration over `std::unordered_*` | Bucket order depends on hash and allocation history. |
| `std::sort` only with a total order (no ties); otherwise `std::stable_sort` | Order of equal elements in `std::sort` is implementation-defined. |
| No pointer values in logic, hashes, or comparisons | Addresses differ between processes. |
| No uninitialised memory; components use `= {}` | Padding and garbage would poison checksums. |
| Simulation code lives only in deterministic libraries; host modules never instantiate simulation templates | Host compilers (e.g. UE's toolchain flags) would compile the same code with different floating-point semantics. |
| Public boundary headers expose POD data and non-inline functions | Same reason; also keeps a future C ABI feasible. |
| Randomness only from `Frame::rng` (xoshiro256** seeded from the session config) | `std::rand`, `std::random_device` are not part of state. |
| Jolt query results, active-body lists and contact callbacks are sorted before use | Jolt documents these as non-deterministic in order. |

### 7.4 Determinism verification

1. Same-process double run → identical per-frame checksums.
2. Debug vs Release golden replay → identical checksums (catches contraction and optimisation differences).
3. Snapshot/restore exactness: run A→B, hash; restore A, run to B, hash; equal.
4. Multi-client runner under simulated latency, jitter, loss and reordering → identical verified checksums on every frame.
5. Replay round-trip: record over the network, verify offline.

---

## 8. Rollback Session

### 8.1 Frames of reference

- **Verified frame** `V`: the newest frame for which the relay has confirmed inputs from all slots.
- **Predicted frame** `P ≥ V`: the frame the local client has simulated to, using predicted inputs for missing remote players.
- **Prediction window** `P − V` is bounded by `maxPrediction` (default 10 frames at 60 Hz ≈ 167 ms).
  When the bound is reached the client stalls instead of predicting further.

### 8.2 State management: single live frame + snapshot ring

Unison keeps **one live `Frame`** and a **ring of snapshots**, one per simulated frame, sized
`maxPrediction + 2`. A snapshot is a bulk copy of every component pool (in registration order)
plus Jolt `SaveState` output plus globals.

Each tick:

1. Collect the local input for frame `P + 1`, send it to the relay (with the last `K = 4` inputs for redundancy, unreliable channel).
2. Drain the transport: confirmed inputs for frames `≤ P` are compared with the inputs used when those frames were simulated.
3. If any confirmed input differs from what was predicted for frame `F`: **rollback** — restore snapshot `F − 1`
   (registry pools, physics reconcile + `RestoreState`, globals), then re-simulate `F … P` with corrected inputs,
   overwriting snapshots on the way. Events raised in re-simulated frames are diffed against the previous
   emissions to produce `cancelled` / `raised` notifications for the view.
4. Advance `V` to the newest fully confirmed frame; compute its checksum from the snapshot; send checksums on the
   configured interval (reliable channel).
5. Simulate frame `P + 1`; store its snapshot.

This is the GGPO-style layout: each frame is simulated once when prediction is right, and
snapshot cost (bulk copies + `SaveState`) is paid once per frame. The Quantum-style
two-frame layout (separate verified and predicted frames) would simulate every frame
twice; it remains an alternative if snapshot cost proves worse than a physics step
(Phase 2 benchmark decides).

### 8.3 Time synchronisation

- The relay stamps confirmed frames; ping messages measure RTT.
- The client targets `P = V_server + RTT/2 + jitterMargin` (in frames) and corrects drift by occasionally
  running one extra or one fewer tick per host frame, never by changing `dt`.
- Optional local input delay (default 0) trades responsiveness for fewer rollbacks; `maxPrediction` and
  input delay together allow lockstep-like tuning without new code paths.

### 8.4 Input confirmation at the relay

- The relay confirms frame `N` when it has inputs from every active slot for `N`, or when the deadline
  (`inputDeadline`, default 100 ms after the first input for `N` arrives) expires; missing inputs are
  replaced by "repeat last input" and flagged `dropped`. Late inputs for confirmed frames are ignored.
- Confirmed input sets are broadcast to all clients and spectators and appended to the relay's input log
  (kept in memory for late-join / reconnect and optionally written as a replay).

### 8.5 Checksums and desync handling

- Checksum = XXH3-64 over globals, every component pool (registration order, packed arrays), and the Jolt state buffer.
- Interval: every verified frame in tools and tests; every 20 verified frames in network sessions by default.
- The relay compares checksums per frame across clients. On mismatch it broadcasts `DesyncDetected(frame, minority slots)`.
  Clients dump the offending snapshot to disk; `unison_replay diff` shows the first differing component.

### 8.6 Late-join, reconnect, spectators

- **Late-join**: the relay picks a donor client, requests a serialised snapshot of verified frame `F`
  (`SnapshotSerializer`: registry + physics + globals), streams it in chunks over the reliable channel to the
  joiner together with confirmed inputs since `F`; the joiner restores and fast-forwards at up to `N×` speed.
- **Reconnect**: same mechanism; the relay holds the slot for `reconnectGrace` (default 30 s) and applies the
  drop policy to the absent player's inputs meanwhile.
- **Spectators**: receive confirmed inputs only, run without prediction (`P = V`), optionally with an added delay.

### 8.7 Replays

A replay file contains the session config, the asset hash, all confirmed inputs per frame, and the
periodic checksums. Replays are deterministic by construction; `unison_replay play` re-simulates,
`verify` compares checksums, `diff` restores two snapshots and prints the first differing component.

---

## 9. Networking

### 9.1 Transport abstraction

```cpp
struct ITransport {
    virtual void send(PeerId, Channel /*Reliable|Unreliable*/, std::span<const std::byte>) = 0;
    virtual void poll(std::function<void(PeerId, Channel, std::span<const std::byte>)>) = 0;
    // connect / disconnect / peers ...
};
```

- `LoopbackTransport`: in-process; wraps a `NetworkSimulator` with seeded latency, jitter, loss and reordering
  so that runner results are themselves reproducible.
- `EnetTransport`: ENet client/server with two channels (reliable, unreliable-sequenced).

### 9.2 Relay protocol (v1)

| Message | Direction | Channel | Payload |
|---------|-----------|---------|---------|
| `Hello` | client → relay | reliable | protocol version, session config hash, requested role (player / spectator), reconnect token |
| `Welcome` | relay → client | reliable | slot id, session config, start frame, current verified frame, reconnect token |
| `Input` | client → relay | unreliable | first frame, K inputs |
| `Confirmed` | relay → all | unreliable (+ periodic reliable resend) | frame, all slots' inputs and flags |
| `Checksum` | client → relay | reliable | frame, hash |
| `Desync` | relay → all | reliable | frame, slots in minority |
| `SnapshotRequest` / `SnapshotChunk` | relay ↔ clients | reliable | frame, chunk index, bytes |
| `Ping` / `Pong` | both | unreliable | timestamps, relay frame |
| `Leave` / `Kick` | both | reliable | reason |

The protocol lives in `unison_net`; the same `RelayCore` state machine runs inside the
runner (in-process) and inside `unison_relay` (ENet), so tests exercise the real relay logic.

---

## 10. Host Integration

### 10.1 Bridge API (host side, `unison_view`)

- `SessionRunner::update(hostDeltaSeconds)` — runs zero or more ticks and returns the number of ticks,
  rollbacks and the interpolation alpha for rendering.
- `SessionRunner::frame()` — read-only view of the live frame (`const Frame&`).
- `EventDispatcher` — `on<EventType>(callback)` plus `onCancelled<EventType>`.
- `EntityViewMap` — maps `entt::entity` to a host handle; fed by `EntityCreated` / `EntityDestroyed` events.
- `TransformInterpolator` — keeps the previous tick's transforms (host side, not simulation state) and returns
  the interpolated pose for the current render frame.
- Local input submission: `SessionRunner::setLocalInput(const Input&)` once per host frame; the runner samples it at each tick.

### 10.2 Terminal hosts

- `unison_runner`: CI workhorse; arguments for players, frames, seed, network simulator parameters, checksum interval,
  replay recording; non-zero exit code on desync or window overflow; prints rollback statistics.
- `unison_console`: text visualisation (top-down ASCII map of the arena, health, rollback/ping stats), keyboard input,
  connects to `unison_relay`. TUI library candidate: FTXUI (MIT); fallback is plain console output.
- `unison_replay`: record / play / verify / diff.

### 10.3 Unreal Engine plugin (Phase 5)

- Plugin `Unison` with module `UnisonRuntime` (and optional `UnisonEditor` for debug tooling).
- `tools/build_unreal_thirdparty.ps1` builds all deterministic libraries and Jolt/ENet with CMake and copies
  `.lib` + headers into `integrations/unreal/Unison/Source/ThirdParty/`. `Unison.Build.cs` links them.
  Toolchain, CRT (`/MD`), C++20, exceptions-off and RTTI-off settings match UE's build.
- `UUnisonSessionSubsystem` (`UGameInstanceSubsystem`): owns `SessionRunner`, ticks it on the game thread
  before physics, exposes connection state and statistics.
- `UUnisonInputComponent`: converts Enhanced Input actions into the game's quantised `Input`.
- `AUnisonEntityActor` / `UUnisonEntityViewComponent`: spawned from `EntityCreated` events via a
  component-type → actor-class map; positions updated from `TransformInterpolator`.
- `UUnisonEventBus`: Blueprint-assignable delegates for game events.
- Debug HUD: verified/predicted frame, rollback count per second, ping, snapshot cost.
- The simulation never touches `UObject`s; the plugin only reads the frame and feeds inputs.

---

## 11. Testing Strategy

| Level | Tool | What |
|-------|------|------|
| Unit | Catch2 | fixed containers, hashing, serialization round-trips, input buffer, snapshot ring, relay state machine |
| Determinism | Catch2 + golden files | items 1–5 of Section 7.4 |
| Integration | `unison_runner` via CTest | 2/4/8 clients under several network profiles at 30 and 60 Hz |
| Benchmarks | Catch2 `BENCHMARK` | tick, snapshot, restore, resimulate k frames, checksum |
| Manual | LAN session, UE sample | Definition of Done items 2 and 7 |

CI is a local script in v1 (`tools/ci.ps1`) that configures, builds Debug and Release,
runs all tests and the golden replay. A hosted CI matrix is a backlog item.

Development follows TDD: every micro-feature starts with a failing Catch2 test, ends with a
self-review of the full diff against the rules in `CLAUDE.md` and the determinism rules of
Section 7, and is committed only on the owner's command.

---

## 12. Repository Layout (monorepo)

```
deterministic-multiplayer-ecs-engine/
  CMakeLists.txt
  cmake/                  # CPM.cmake, UnisonDeterminism.cmake, helpers
  core/                   # unison_core
  sim/                    # unison_sim
  session/                # unison_session
  net/                    # unison_net
  view/                   # unison_view
  relay/                  # unison_relay executable
  tools/                  # unison_runner, unison_replay, unison_console, scripts (ci.ps1, build_unreal_thirdparty.ps1)
  samples/arena/          # arena_sim, arena_view_console, assets/
  integrations/unreal/    # Unison plugin + sample UE project (Phase 5)
  tests/                  # Catch2 tests, golden replays, benchmarks
  docs/                   # this charter, API docs, guides
  third_party/            # patches/overrides only; sources are fetched by CPM into the build cache
```

---

## 13. Dependencies

| Library | Use | License | Notes |
|---------|-----|---------|-------|
| EnTT | ECS | MIT | header-only, C++17+; the 3.x line, since v4.0 (July 2026) reworks the storage API the registry clone of 1.2.3 depends on |
| Jolt Physics | 3D physics, math | MIT | built from source, `CROSS_PLATFORM_DETERMINISTIC=ON`, no exceptions/RTTI |
| ENet | UDP transport | MIT | C |
| xxHash (XXH3) | checksums | BSD-2 | header mode |
| Catch2 v3 | tests, benchmarks | BSL-1.0 | |
| CPM.cmake | dependency fetching | MIT | |
| cxxopts | CLI parsing (tools only) | MIT | |
| tl::expected | recoverable errors without exceptions | CC0-1.0 | header-only |
| FTXUI | console UI (candidate, tools only) | MIT | |

Rules: no dependency enters a deterministic library without a determinism review;
tools-only dependencies never leak into libraries linked by the UE plugin;
`std::format` replaces fmt; logging in libraries goes through a callback, never to stdout.

---

## 14. Sample Game: Arena

- **World**: flat floor, static boxes and ramps (Jolt static bodies from asset tables defined in code; file loading is backlog), 4 spawn points.
- **Players**: capsule `CharacterVirtual`; move, jump, aim yaw, fire.
- **Projectiles**: kinematic spheres with lifetime; hit → damage event; kill → respawn timer.
- **Dynamic props**: a few rigid-body crates that players and projectiles can push.
- **Components**: `Transform`, `PlayerSlot`, `CharacterState`, `Health`, `Weapon`, `Projectile`, `Lifetime`, `PhysicsBody`, `RespawnTimer`.
- **Systems** (in order): `ApplyInput`, `CharacterMove`, `Weapons`, `PhysicsStep`, `Hits`, `Lifetime`, `Respawn`, `MatchRules`.
- **Input**: `moveX/moveY: int8`, `yaw: int16`, `buttons: uint8 {Jump, Fire}`.
- **Events**: `EntityCreated`, `EntityDestroyed`, `Fired` (predicted), `Hit` (predicted), `Died` (verified-only), `Respawned` (verified-only).
- **Views**: text top-down map in the console; capsule/box meshes in UE.

---

## 15. Roadmap

Phases have exit criteria, not dates.

**Phase 0 — Bootstrap.** CMake + CPM, dependencies pinned, determinism flags module, library skeletons,
first Catch2 test, `tools/ci.ps1`. *Exit*: `ctest` green in Debug and Release.

**Phase 1 — Deterministic simulation core.** `Frame`, pipeline, component registration, RNG, events/signals,
assets, `PhysicsWorld` with body-id policy and reconcile + `SaveState`/`RestoreState`, snapshot copy and checksum,
Arena sim running headless. *Exit*: determinism checks 1–3 of Section 7.4 pass; snapshot/restore/tick benchmarks recorded.

**Phase 2 — Rollback session (local).** Inputs, prediction, snapshot ring, rollback and resimulation, event
raise/cancel diffing, `RelayCore`, loopback transport + network simulator, `unison_runner`, desync detection.
*Exit*: Definition of Done item 1; decision recorded on snapshot ring vs two-frame layout.

**Phase 3 — Real networking.** ENet transport, `unison_relay`, time sync, input deadline policy,
`unison_console` with text visualisation. *Exit*: Definition of Done item 2.

**Phase 4 — Session features.** Replay files and `unison_replay`, snapshot serialisation, late-join,
reconnect, spectators, state diff tool. *Exit*: Definition of Done items 3–6.

**Phase 5 — Unreal plugin.** ThirdParty build script, `UnisonRuntime`, subsystem, input, entity views,
events, interpolation, debug HUD, Arena UE sample. *Exit*: Definition of Done item 7.

**Phase 6 — Hardening.** Profiling, Jolt multithreading evaluation, memory budgets, API polish, docs.
*Exit*: Definition of Done items 8–9.

**Backlog (post-v1).** C ABI + Unity/Godot bindings; Linux/macOS/ARM64 determinism matrix and hosted CI;
navigation (Recast for baking, Detour at runtime with deterministic math shims); authoritative-server mode;
DSL/codegen; 2D physics module (Box2D v3, cross-platform deterministic); encryption / Steam relay via
GameNetworkingSockets; lobbies and matchmaking; multiple local players per client; delta-compressed inputs
for 16+ players; frame-local heap allocator; asset loading from files.

---

## 16. Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Float determinism breaks with a future compiler or platform | Desyncs | Flags module shared by all deterministic libs, no libm trig, FP-environment guard, Debug-vs-Release and cross-compiler checks; fallback: fixed-point gameplay math behind the same API (Jolt would stay float). |
| Snapshot cost too high at 60 Hz (EnTT pools + Jolt `SaveState`) | Frame budget | Benchmark in Phase 1; `EStateRecorderState` subsets; body count budget; two-frame layout as alternative; custom arena storage as last resort. |
| Jolt `RestoreState` not bit-exact after body reconciliation | Rollback desync | Deterministic body ids via `CreateBodyWithID`, reconcile before restore, dedicated exactness test in Phase 1. |
| Non-deterministic order in Jolt queries and callbacks | Desyncs | Wrapper sorts every result set; contact events buffered and sorted. |
| Duplicate or ghost view effects after rollback | Visual glitches | Event keys with raise/cancel diffing; verified-only events for irreversible effects. |
| UE toolchain flags leaking into simulation code | Desyncs between UE and console clients | Simulation only in deterministic static libs; boundary headers without simulation logic; build script pins flags. |
| Scope creep | Never shipping v1 | Definition of Done and phase exit criteria; backlog is explicit. |

---

## 17. Open Questions

| # | Question | Decide by |
|---|----------|-----------|
| Q1 | Snapshot ring (single live frame) vs Quantum-style verified + predicted frames | Phase 2 benchmark |
| Q2 | SSE2 vs AVX2 baseline for deterministic libraries | Phase 1 benchmark |
| Q3 | FTXUI vs plain console output for `unison_console` | Phase 3 |
| Q4 | Jolt multithreaded stepping inside the simulation | Phase 6 |
| Q5 | UE version and whether to support UE's Linux server target | Version answered 2026-09-21: UE 5.8 is installed (Visual Studio 18, MSVC 14.51); toolchain compatibility is confirmed in task 5.1.4. The Linux server target stays open until Phase 5. |
| Q6 | Input deadline and reconnect grace defaults | Phase 3 LAN tests |
| Q7 | Checksum interval default for production sessions | Phase 3 |

---

## 18. Glossary

- **Tick / frame**: one fixed simulation step (1/60 s by default).
- **Verified frame**: newest frame whose inputs are confirmed by the relay for every slot.
- **Predicted frame**: newest frame the local client has simulated, possibly with predicted inputs.
- **Rollback**: restoring an earlier snapshot and re-simulating with corrected inputs.
- **Snapshot**: a copy of a frame's full state sufficient to restore it bit-exactly.
- **Checksum**: XXH3-64 hash of a verified frame used to detect desyncs.
- **Relay**: server that orders and broadcasts inputs and brokers snapshots; it never simulates.
- **Slot**: a player position in the session (input source); spectators have no slot.
- **Deterministic library**: any library compiled with the Section 7 flags that may mutate simulation state.
