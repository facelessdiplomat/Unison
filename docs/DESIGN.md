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

The simulation is fully isolated from any host engine. The same gameplay code runs, on Windows x64 and on
macOS arm64 alike:

- **headless in a terminal** (development runner, tests, CI, replay tools, console client), and
- **inside Unreal Engine 5** as a plugin (first host engine; others later).

Every client runs an identical copy of the simulation, whichever of the two platforms it runs on, so a client
on Windows and a client on macOS play one match together. Only player inputs travel over
the network. A lightweight relay server orders inputs and never simulates anything.

### 1.1 Goals

1. **Bit-exact determinism** across all clients in a session, on Windows and macOS alike, verified continuously
   by checksums.
2. **Responsive play** via input prediction and rollback (no input delay by default).
3. **Isolation**: the simulation has no rendering, no I/O, no wall clock, no host-engine types.
4. **One gameplay codebase, many hosts**: terminal runner and UE plugin consume the same static libraries.
5. **Prefer proven open source** over bespoke code wherever a suitable library exists.

### 1.2 Non-goals for v1

- Authoritative-server or peer-to-peer topologies (relay only).
- Platforms other than Windows x64 with MSVC and macOS arm64 with Apple clang (D35): Linux, x86-64 macOS and
  ARM64 Windows are backlog, while the relay, which never simulates, stays portable.
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
7. On Windows and on macOS, the Unreal sample project plays the Arena with two or more clients through the
   relay: entity actors spawn/despawn from simulation events, rendering interpolates smoothly at
   60 / 120 / 144 FPS while the simulation ticks at 60 Hz.
8. Debug and Release builds on both platforms, MSVC on Windows x64 and Apple clang on macOS arm64, produce
   identical checksums over the committed golden replay: four builds, one set of checksums. This is the
   cheapest cross-optimization and cross-compiler determinism check.
9. Documentation exists for: this charter, the session/view-bridge API, and a
   "how to add a component and a system" guide.
10. Clients on Windows and on macOS play the Arena together at 60 Hz for 10 minutes with rollback and zero
    desyncs: a `unison_console` on each platform through a `unison_relay` on Windows and again through one on
    macOS; then the Unreal sample on the two platforms together, and the Unreal sample on macOS with a
    console on Windows.

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
| D12 | Determinism platforms (v1) | ~~**Windows x64, MSVC only**~~ → **Windows x64 with MSVC and macOS arm64 with Apple clang** (superseded by D35) | Windows + Linux; + macOS/ARM64 | Smallest scope at the start; the relay does not simulate, so it could still be hosted on Linux. See D35. |
| D13 | Host API | **C++ API, core as static libraries** | C++ + C ABI; C++ now, C ABI later | UE links ThirdParty static libs natively. The public API avoids templates and inline logic at the boundary so a C wrapper can be added later. |
| D14 | View layer | **Events + direct state reads with interpolation** | Polling only | Quantum model. Events split into verified-only and predicted (cancellable on rollback). |
| D15 | Tick rate | **60 Hz default**, configurable | 30 Hz; undecided | Action-game standard; tests must also pass at 30 Hz. |
| D16 | C++ standard | **C++20** | C++17; C++23 | Matches UE 5.3+; fully supported by MSVC. |
| D17 | Build | **CMake + CPM.cmake** | vcpkg manifest; git submodules | Single build system, pinned dependency versions, Jolt built from source with our determinism flags. |
| D18 | Tests | **Catch2 v3** | GoogleTest; doctest | Sections, generators, built-in benchmarks, CTest integration. |
| D19 | UE version | **UE 5.8**, on Windows and on macOS (installed on the Windows machine; re-check at the start of Phase 5) | 5.6; 5.7; latest at Phase 5 | Plugin phase comes after the terminal phases; the installed version is the natural target. On macOS Epic lists Xcode 26.0 to 26.1.1 for UE 5.8, while the Mac has Xcode 27.0 on macOS 27.0, so task 5.1.6 checks that pair first (Q10). |
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
| D33 | Work rhythm | ~~**Stop after every micro-task**: self-review, report, wait for the owner's "commit" or "continue"~~ → **commit and go on** (superseded by D36) | Stop at task boundaries; standing commit authorisation | Maximum control for the owner; one commit per micro-task falls out naturally. Single branch `main`. See D36. |
| D34 | Units and axes | **Jolt-native: metres, seconds, kilograms, radians, Y-up, right-handed**; conversion only in host adapters | Z-up right-handed; Unreal-native | Zero conversions in the heaviest consumer (physics, character controller); one tested conversion at the Unreal boundary. |
| D35 | Platforms | **Windows x64 with MSVC and macOS arm64 with Apple clang; clients on the two play one match; the Unreal plugin runs on both** | Windows only (D12); macOS through Rosetta 2, running the SSE2 code of Windows; Linux as well | The owner's word of 2026-09-25. Jolt verifies its `CROSS_PLATFORM_DETERMINISTIC` build across MSVC on x64 and clang on ARM64 with NEON, among others, so what is left to prove is our own: the compiler contract on clang (§7.1), the floating-point environment on arm64 (§7.2), the rules that keep two compilers and two standard libraries alike (§7.3), and goldens and a mixed LAN run across the two (§7.4). Planned and recorded in `docs/CROSS_PLATFORM.md`; Rosetta 2 stays a fallback. |
| D36 | Work rhythm, from 2026-09-25 | **Commit every micro-task once its self-review is green and take the next without waiting**; stop only for what the owner alone can do or decide | Stop after every micro-task (D33) | The owner's word of 2026-09-25. The board and the history record every step, one commit per micro-task, and the owner ends the loop when needed; pushing stays the owner's. |

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
| `unison_session` | static lib | `Session` (rollback state machine), `InputBuffer`, `SnapshotRing`, `Checksum`, `ReplayWriter/Reader`, `serializeSnapshot`/`deserializeSnapshot` (late-join, desync dumps), `TimeSync` | sim, net |
| `unison_net` | static lib | `ITransport`, `LoopbackHub` + `NetworkSimulator`, `EnetTransport`, `SessionConfig`, relay protocol messages, `RelayCore` (reusable by in-process and standalone relay) | ENet, core |
| `unison_view` | static lib | Event dispatch with raise/cancel semantics, `EntityViewMap`, `TransformInterpolator`, read-only frame accessors | session |
| `unison_relay` | executable | Standalone relay server over ENet, portable (Windows and macOS; Linux untried) | net |
| `unison_runner` | executable | N clients + in-process relay + network simulator; checksum comparison; exit code for CI | session, view, game sim |
| `unison_replay` | executable | record / play / verify / diff | session, game sim |
| `unison_console` | executable | Console client with text visualisation and keyboard input | view, net, game sim, its console view |
| `arena_sim` | static lib | The sample game's deterministic code | sim |
| `arena_view_console` | static lib | Text renderer for Arena: the top-down map the console draws | arena_sim |
| `Unison` (UE plugin) | UE plugin | `UnisonRuntime` module linking the libs above on Win64 and Mac; subsystem, input, entity views, events, debug HUD | UE, all libs |

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
- **Encoding**: little-endian byte order, which both platforms share and `BinaryWriter` and `BinaryReader`
  assert, `uint32_t` frame numbers, `uint8_t` slot ids, `uint32_t` entity ids at the host boundary.
- **Logging**: `LogSink` is a process-wide callback and is one of the three pieces of global mutable state the
  engine allows. It exists because `UNISON_VERIFY` is a macro and cannot take an injected dependency, and
  because a host installs one sink for the whole process. The exception is bounded: the sink is write-only
  from the simulation's side, nothing in a deterministic library reads it back, and no simulation result
  depends on whether a sink is installed.
- **Jolt's process-wide registration**: Jolt keeps its allocator, its factory and its type list in globals of
  its own and offers nothing to inject, so `JoltRuntime` counts the scopes that need them, registers for the
  first and unregisters after the last. It is the second piece of global mutable state the engine allows.
  The exception is bounded: the registration is installed before any body exists, no deterministic library
  reads it back, the counter changes only on the simulation thread, and no simulation result depends on it.
- **A terminal host's stop request**: a signal handler can reach the program only through a flag of static
  storage, so the `main.cpp` of `unison_relay` and that of `unison_console` each keep one
  `volatile std::sig_atomic_t` that `SIGINT` and `SIGTERM` set and the main loop reads. It is the third piece
  of global mutable state, and neither flag leaves the one source file of its executable. Everything else
  keeps the constructor injection of `CLAUDE.md` 4.
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
  Release without a word. `ComponentRegistry` rejects a registration arriving from a second file, so one
  process runs one game. The engine checks in Debug that a game registered the components it puts on
  entities itself, because one the game forgot would quietly stay out of every snapshot. A game is
  built as a CMake OBJECT library, because from a static library the
  linker drops the translation unit that holds the registrations: nothing references a static initialiser.
- `UNISON_FIELDS(Type, field, ...)` names a registered component's fields, every one in the order they are declared,
  in the file that registered it and after its `UNISON_COMPONENT`. It static-asserts that it names as many fields as
  Boost.PFR counts, and takes each field's bytes from the member sizes, which a padding-free component lays end to
  end; the difference between two snapshots names the field its byte falls in.
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
- `FrameInputs` = one `Input` per player slot plus a per-slot `flags` byte (present / dropped), both exactly
  as the relay settles them. Whether the session only guessed an input is not among the flags: it is the
  session's knowledge, kept in its input buffer (§8.2), and systems never see it. A frame played on a guess
  that proves right is never played again, so a system able to tell a guess from the real input could part
  the clients for good, and no rollback would ever notice.
  It is a plain class, not a template over `Input`: the slots hold the input erased to bytes, and only the
  accessor is typed; code that carries inputs without knowing their type, the session and the wire, writes
  a slot from bytes. A template would spread through `ISystem`, `SystemPipeline` and `Session`, and §7.3
  forbids a host module from instantiating simulation templates, which the Unreal plugin would have to do to
  drive a session. The 64-byte cap on `Input` exists to make the erased slot possible.
- Prediction policy: repeat the last input confirmed for each remote slot before the frame, bytes and flags
  alike; a slot with no confirmed input yet is guessed neutral, zero bytes and no flags. A right guess is then
  identical to the confirmation, so a misprediction is any difference between the two.

### 6.5 Events and signals

- **Events** carry POD payloads and an `EventKey = (frame, typeId, ordinal)`.
  - *Verified-only* events are delivered to the view only once their frame is verified, exactly once, as the
    frame raised them the last time it was played; a replay neither raises nor cancels them.
  - *Predicted* events are delivered immediately; after a rollback the view receives
    `cancelled(key)` for events not re-raised and `raised` for new ones. The view layer
    deduplicates by key. A replay counts an event as the same event when its key is the same; the payload
    is not compared, so an event re-raised with a slightly different payload is neither cancelled nor
    raised again. The session keeps the events of every frame it may still replay, indexed by the frame it
    played, since the frame in a key is the one the tick started from. The changes gathered between two
    drains net out, so their raised and cancelled keys never overlap: a key raised and cancelled in between
    is never shown, and one cancelled and raised again stays shown.
- **Signals** are synchronous sim-internal callbacks (e.g. `OnHit`) and never leave the simulation.

### 6.6 Assets

Immutable design data (arena layout, character stats, projectile definitions) is loaded
before the session starts, referenced by `AssetId` (32-bit hash of the asset name), and
hashed into `SessionConfig.assetHash`. Clients with different asset hashes cannot join
the same session.

### 6.7 Physics (Jolt)

`PhysicsWorld` wraps a Jolt `PhysicsSystem` with rules that keep it rollback-safe:

- **One world, three parts.** The bodies, the characters and the queries of a world each have a class of
  their own, `BodyTable`, `CharacterTable` and `PhysicsQueries`, reached through `bodies()`,
  `characters()` and `queries()` as Jolt reaches its own through its `PhysicsSystem`; the world steps
  them and saves and restores their state.
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
  `kMaxBodies` slots, the same limit the allocator hands indices out from. `Transform`, `PhysicsBody`
  and `BodyDefinition` are defined by `unison_sim` but registered by the game, because `UNISON_COMPONENT`
  accepts registrations from one translation unit only (§6.3). `destroyEntity` takes an entity's body
  with it, so the world never holds a body the registry no longer names.
- **Restore = reconcile, then `RestoreState`.** Jolt's `RestoreState` requires the same set of
  bodies and constraints to exist as when `SaveState` ran. After restoring the registry,
  `PhysicsWorld` destroys bodies that are no longer referenced, recreates missing ones from
  their components, and only then calls `RestoreState` with `EStateRecorderState::All`.
  What a body is made of lives in a `BodyDefinition` component beside `PhysicsBody`, a copy of the
  asset it was spawned from that a system may change; the properties Jolt leaves out of its state
  buffer (friction, restitution, motion type, object layer) are put back from it during
  reconciliation. Measurements are read when the body is built, so changing a shape or a size means
  building the body again.
- **Query results are sorted** before use. Jolt returns broad/narrow-phase results and
  `GetActiveBodies` in the order its traversal happens to reach them; `raycast`, `overlapSphere` and
  `sweepCapsule` sort by `(fraction, BodyID)` before a system sees them, and `raycastNearest` keeps the
  first hit in that order, so hits a traversal could list either way are equal values. Queries in v1
  meet every layer and carry no filters, and they fill vectors their callers keep.
- **Contact callbacks are buffered**, then sorted by `(BodyID a, BodyID b, sub-shape ids)` and
  delivered to systems in that order, the lower body id first so a pair reads the same whichever way
  Jolt reported it. The list holds what the last step found, is emptied when a step begins and when a
  state is restored, and is derived rather than frame state, so it is neither snapshotted nor hashed.
  Listener callbacks may run on multiple threads inside Jolt.
- **Character movement** uses Jolt's `CharacterVirtual` (kinematic, deterministic). A character is not
  a body and its state is not in what `PhysicsSystem::SaveState` writes, so `CharacterTable` saves and
  restores each character itself, in id order, straight after the body state in the same buffer. A
  character draws its id from the same allocator bodies do and that id is given to Jolt as the
  `CharacterID`, because the one Jolt would pick comes from a process-wide counter. The gameplay side
  of a character — the capsule, the velocity and the ground under it — travels in a component, and
  `PhysicsStep` moves the characters before it steps the bodies and writes the result back.
- **Single-threaded** Jolt job system in v1 (`JobSystemSingleThreaded`). Multi-threaded stepping
  is evaluated in Phase 6 only after a determinism test proves it identical. The body mutex count is
  pinned to one rather than auto-detected, which would derive it from the core count of the machine.
- Jolt is built with `CROSS_PLATFORM_DETERMINISTIC=ON` (≈8 % slower), exceptions off, RTTI off.

---

## 7. Determinism Rules

These rules apply to every deterministic library (`unison_core`, `unison_sim`, `*_sim`).
They are enforced by compiler flags in `cmake/UnisonDeterminism.cmake`, by code review
checklists, and by the tests in Section 11.

### 7.1 Compiler flags (MSVC and clang)

Every deterministic library and Jolt compute alike on both platforms: IEEE single and double precision,
evaluated as written, rounded to nearest, never fused into a multiply-add and never reassociated. Each compiler
is held to that by flags of its own, which `unison_apply_determinism` applies:

| Contract | MSVC, Windows x64 | Apple clang, macOS arm64 |
|----------|-------------------|--------------------------|
| Value-safe floating point | `/fp:precise`, never `/fp:fast` or `/fp:strict` | `-fno-fast-math` |
| No fused multiply-add | no `/fp:contract` | `-ffp-contract=off` |
| No excess precision | none exists on SSE2 | `-fexcess-precision=standard` |
| Instruction set | one `/arch:` baseline for every deterministic library and Jolt: SSE2, or AVX2 for all of them together should a benchmark ever prove it needed (Q2) | the arm64 baseline with NEON and nothing added, `UNISON_INSTRUCTION_SET` reading `NEON`; never `-mfma` or `-march=native` |
| Determinism guard | `determinism_guard.hpp` force-included with `/FI`: rejects the build unless `_M_FP_PRECISE` is defined and `_M_FP_CONTRACT` is not | the same header force-included with `-include`: rejects `__FAST_MATH__` and `__FINITE_MATH_ONLY__` and turns contraction off with `#pragma STDC FP_CONTRACT OFF`; any other compiler is rejected |
| Exceptions and RTTI off | `/EHs-c- /GR-` and `_HAS_EXCEPTIONS=0` | `-fno-exceptions -fno-rtti` |
| Warnings as errors, for our own code | `/permissive- /W4 /WX` | `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror` |

- `/fp:precise` is always passed explicitly: MSVC defines no `_M_FP_*` macro when no `/fp:` flag is
  given, so `determinism_guard.hpp` rejects that case too and no library rests on a compiler default.
- The clang flags are always passed as well, after anything a toolchain puts before them. Clang contracts by
  default, `a * b + c` becoming one `fmadd` on arm64, and `-fno-fast-math -ffp-contract=off` placed last takes
  back `-ffast-math` and `-ffp-model=fast` alike. `-ffp-model=precise` is not passed: it is clang's default
  model, and followed by `-ffp-contract=off` it trips `-Woverriding-option`, which `-Werror` turns into a failed
  build. All three were measured with Apple clang 21 on 2026-09-25.
- The guard sees less on clang. Clang defines no macro for its default model, so there is nothing to require,
  and neither `-ffp-model=fast` nor `-ffp-contract=fast` defines a macro or yields to the pragma. The command
  lines are read instead: `tests/cmake/determinism_flags` and `tests/cmake/module_determinism` forbid
  `-ffast-math`, `-Ofast`, `-ffp-model=fast`, `-ffp-model=aggressive`, `-funsafe-math-optimizations`,
  `-fassociative-math`, `-freciprocal-math`, `-ffinite-math-only`, `-fno-signed-zeros`, `-fno-honor-nans`,
  `-fno-honor-infinities`, `-fapprox-func`, `-ffp-contract=on`, `-ffp-contract=fast`, `-mfma` and
  `-march=native` anywhere on the command line, even where our own flags come later and take them back, and a
  canary built under `unison_apply_determinism` shows in every build that a multiply followed by an add rounds
  twice.
- Exceptions off, RTTI off (matches Unreal's defaults; EnTT and Jolt support both), applied to every Unison
  target by `unison_apply_language_subset`, not only to the deterministic ones. `/EHs-c-` alone is not
  enough on MSVC: the STL keeps emitting `try`/`catch` that cannot unwind unless `_HAS_EXCEPTIONS=0` is
  defined as well, and Jolt defines it for itself, so a target without it also disagrees with Jolt. libc++
  needs no such macro: it follows `-fno-exceptions` by itself.
  `unison_apply_determinism` adds the floating-point half on top. EnTT needs `ENTT_NOEXCEPTION` for the same
  reason and polices it itself with `detect_mismatch`, so it is defined on its interface target and reaches
  every consumer, tests included.
- The test executables are the targets that keep exceptions, because Catch2 needs them: MSVC's declare
  `/EHsc`, clang's keep its default. They set up the scenes the goldens are recorded from in their own code,
  so on clang they take `-ffp-contract=off` too, which MSVC needs no flag for: its SSE2 baseline has no fused
  instruction to contract into. `tests/cmake/module_determinism` asserts each choice for each compiler.
- Linux and GCC are backlog; a port there adds a column.

### 7.2 Runtime environment

- `FpEnvGuard` saves the thread's floating-point control register at the start of every tick, MXCSR on x64
  and FPCR on arm64, sets the architecture's default, and restores the host's state afterwards. The default
  rounds to nearest, keeps denormals rather than flushing them and masks every exception: `0x1F80` in MXCSR,
  `0` in FPCR, which is also what a macOS process starts with. Host engines and audio libraries
  are free to change flush-to-zero on their threads; the simulation is not affected.
- Clang assumes the default floating-point environment and may move arithmetic it can see across a write of
  the control register, as the guard's own test showed at `-O2` on 2026-09-25. The guard therefore stands in
  `advanceFrame` around calls the compiler cannot see through, the systems' and the physics', and no
  deterministic library is built with link-time optimisation, which would let it see through them.
- The simulation never reads the wall clock, thread ids, addresses, environment variables, or files.

### 7.3 Code rules

| Rule | Reason |
|------|--------|
| No `std::sin/cos/tan/atan2/exp/log/pow` — use `JPH::Sin`, `JPH::Cos`, `JPH::ATan2`, etc. | libm results differ between platforms and CRT versions. `std::sqrt`, `floor`, `ceil`, `fmod`, `abs` are IEEE-exact and allowed. |
| No `std::lerp`, `std::hypot`, `std::fma` | May use FMA or extended precision internally. |
| No iteration over `std::unordered_*` | Bucket order depends on hash and allocation history. |
| `std::sort`, `std::partial_sort`, `std::nth_element`, `std::partition` and heaps only with a total order (no ties); otherwise `std::stable_sort` or `std::stable_partition` | Where equal elements end up is left to the standard library, and the MSVC STL and libc++ leave them in different places. |
| No pointer values in logic, hashes, or comparisons | Addresses differ between processes. |
| No uninitialised memory; components use `= {}` | Padding and garbage would poison checksums. |
| Simulation code lives only in deterministic libraries; host modules never instantiate simulation templates or call the simulation's inline functions | Host compilers (e.g. UE's toolchain flags) would compile the same code with different floating-point semantics, and the linker keeps one copy of an inline function for every caller. |
| Public boundary headers expose POD data and non-inline functions | Same reason; also keeps a future C ABI feasible. |
| Randomness only from `Frame::rng` (xoshiro256** seeded from the session config) | `std::rand`, `std::random_device` are not part of state. |
| Jolt query results, active-body lists and contact callbacks are sorted before use | Jolt documents these as non-deterministic in order. |
| No value derived from a type's name or from the compiler in state, hashes or on the wire: `entt::type_hash`, `typeid`, `std::hash`, `__FUNCSIG__`, `__PRETTY_FUNCTION__` | Each compiler and standard library spells and hashes them its own way; inside one process they may still find a table or a listener. |
| Components, inputs, assets and messages hold fixed-width scalars only: no `long`, `wchar_t`, `long double` or bit-fields, and every enumeration has a fixed underlying type | `long` is 32 bits on Windows and 64 on macOS and `wchar_t` 16 and 32, `long double` differs on platforms beyond these two, and MSVC and clang may lay bit-fields out differently. |
| No NaN in frame state | An invalid operation yields a NaN whose bits differ, `0.0F / 0.0F` being `0xFFC00000` on x64 and `0x7FC00000` on arm64, so a NaN reads as a desync even where both machines computed alike, besides being a bug. |
| A float converted to an integer lies in the integer's range | Outside it the conversion is undefined, and x64 yields the lowest integer where arm64 saturates; a NaN becomes the lowest integer on x64 and 0 on arm64. |

### 7.4 Determinism verification

1. Same-process double run → identical per-frame checksums.
2. Debug vs Release golden replay → identical checksums (catches contraction and optimisation differences).
3. Snapshot/restore exactness: run A→B, hash; restore A, run to B, hash; equal.
4. Multi-client runner under simulated latency, jitter, loss and reordering → identical verified checksums on every frame.
5. Replay round-trip: record over the network, verify offline.
6. Windows against macOS: every golden verifies in Debug and Release on the platform that did not record it: the
   physics pile, the scripted arena, the session config's hashes and the protocol's bytes.
7. A Windows client and a macOS client play through one relay without a desync (§2 item 10).

A golden checksum is re-recorded only deliberately: when the physics build, the scene it covers or the content
of the state buffer changes. A golden that changes for any other reason is a determinism bug, not a stale
number. Goldens are recorded on Windows and verified on macOS. A new golden may be recorded on macOS while the
Windows machine is out of reach, and the next run on Windows is its check; none is ever re-recorded on one
platform alone, and a golden the two platforms disagree on is a difference to locate, never one to record away
(Q15).

---

## 8. Rollback Session

### 8.1 Frames of reference

- **Verified frame** `V`: the newest frame for which the relay has confirmed inputs from all slots.
- **Predicted frame** `P ≥ V`: the frame the local client has simulated to, using predicted inputs for missing remote players.
- **Prediction window** `P − V` is bounded by `maxPrediction` (default 20 frames, 333 ms at 60 Hz, enough for a
  round trip of 240 ms and its jitter). The window is a ceiling, not a target: a rollback goes back as far as
  the round trip, whatever the window, and a wider window costs only the room of the snapshot ring. At 10
  frames, 167 ms at 60 Hz, a 240 ms round trip kept clients stalled for nearly half of every run.
  When the bound is reached the client stalls instead of predicting further. A frame the relay has already
  settled may still be played while every frame before it is verified, since playing it verifies it at once;
  that is what lets `maxPrediction = 0` run in lockstep. A stalled tick still rolls back.

### 8.2 State management: single live frame + snapshot ring

Unison keeps **one live `Frame`** and a **ring of snapshots**, one per simulated frame, sized
`maxPrediction + 2`. A snapshot is a bulk copy of every component pool (in registration order)
plus Jolt `SaveState` output plus globals. The ring owns its snapshots and takes each one into the place of
the frame `capacity` ticks older; copying into a registry, whether a snapshot's or the live frame's on a
restore, empties its pools without giving their room back, so once a match has been through its largest
state no buffer grows again.

Nor does a tick take anything from the C++ heap from then on: a restore reads Jolt's state straight from
the snapshot and reconciles bodies and characters without lists of its own, a query fills a vector its
caller keeps, the session's bookkeeping reuses its room, and the sample's systems collect into fixed room
or into nothing. Every snapshot of the ring and every frame of the event history grows to the largest state
on its own, so a session is warmed up only once each of them has met it. Jolt's own heap is Jolt's:
`SaveState` and `RestoreState` sort the contact cache into temporary arrays on every call, a step rebuilds
the broad phase into a fresh array while bodies move, and building a body, a character or the shape of a
sphere or capsule query allocates. Those allocations are measured and left to Jolt; serving them from a
caching allocator is 6.1.5, should a profile ever ask for it.

Inputs sit beside the ring in an `InputBuffer`: a window of frames that starts at `V` and only moves forward,
each frame holding the `FrameInputs` its tick reads and, per slot, whether that input is still missing, was
predicted, or is confirmed by the relay. The window keeps `V` itself, because a prediction repeats a slot's
last confirmed input and at `V` every slot has one. A frame outside the window is refused, not stored. The
window reaches `kConfirmationsAhead` (128) frames past the prediction window, about two seconds at 60 Hz:
the relay sends each frame reliably only once, so a client that stalled through an outage has to keep every
confirmation arriving meanwhile, and then plays through them one settled frame after another to catch up.
The local slot is not guessed: each tick writes the input the host gave last, as present and unconfirmed,
which is exactly how the relay will settle it unless it gives the input up. An `InputTimeline` holds the
buffer and applies these rules, so the session itself only tracks which frames are played and verified.

Each tick:

1. Collect the local input for frame `P + 1`, send it to the relay (with the last `K = 4` inputs for redundancy, unreliable channel).
2. Drain the transport: confirmed inputs for frames `≤ P` are compared with the inputs used when those frames were simulated.
3. If any confirmed input differs from what was predicted for frame `F`: **rollback** — restore snapshot `F − 1`
   (registry pools, physics reconcile + `RestoreState`, globals), then re-simulate `F … P` with corrected inputs,
   overwriting snapshots on the way. Events raised in re-simulated frames are diffed against the previous
   emissions to produce `cancelled` / `raised` notifications for the view. Confirmations are only noted as
   they arrive; the rollback happens once, at the start of the next tick, from the earliest frame guessed
   wrong, so a burst of late confirmations costs one replay rather than one each. A replayed frame guesses
   its still unconfirmed slots afresh from what has been confirmed since, and plays the local player's
   inputs exactly as they were first played.
4. Advance `V` to the newest fully confirmed frame; compute its checksum from the snapshot; send checksums on the
   configured interval (reliable channel).
5. Simulate frame `P + 1`, on the confirmed inputs of every slot the relay has already settled for it and a
   guess for the rest; store its snapshot. `V` never passes `P`: a frame is verified once it has been played
   on confirmed inputs, whether it was confirmed before it was played or matched its guess afterwards.

This is the GGPO-style layout: each frame is simulated once when prediction is right, and
snapshot cost (bulk copies + `SaveState`) is paid once per frame played, replayed frames included. The
Quantum-style two-frame layout (separate verified and predicted frames) simulates every frame twice, once
predicted and once verified, takes no snapshots, and on a rollback copies the verified frame over the
predicted one and replays everything from `V`. Q1 weighed the two on the Phase 1 numbers (a tick 4.99 µs,
a snapshot 3.71 µs, a restore 4.10 µs, a copy of a frame, which here is a snapshot and a restore since EnTT
and Jolt have no flat memory to copy, 7.81 µs) for one client at 60 Hz:

| Per second of play | Snapshot ring | Two frames |
|--------------------|---------------|------------|
| Every guess right | 0.52 ms | 0.60 ms |
| The Definition of Done's network: 25.9 rollbacks a second, 16.2 frames deep | 4.28 ms | 2.90 ms |

The two frames win once rollbacks come more often than about 1.4 a second at that depth, since the ring
pays a snapshot for every replayed frame; on the Definition of Done's network they save 1.4 ms a second,
0.14 % of one core, against a frame budget of 16.7 ms. The ring stays: that saving is not worth a second simulation path
for verified frames, a different event model and the rewrite of a session proven through Phase 2, and the
ring rolls back only from the first frame guessed wrong. Should a game's state make a snapshot cost far more
than a tick, the table is worth redoing with its numbers.

### 8.3 Time synchronisation

- A playing client pings the relay every 100 ms; the pong carries the frame the relay had confirmed and the
  frame its clock had due when it answered, and the client times the round trip on its own clock.
- The relay keeps a `MatchClock`: the newest frame of the first input to arrive falls due as it arrives,
  and one frame more every tick after it, at exactly the tick rate. It never ticks the match itself; it is
  only the one pace every client keeps to, and it runs whatever the players do, so a player who is out slows
  nobody down, as following the confirmed frame would, the relay confirming each frame only at its deadline
  then.
- The client targets `P = due + RTT`, give or take `jitterMargin` (two frames by default), where `due` is
  the frame the pong carried: half a round trip ahead of the relay's clock, so that its inputs reach the
  relay as their frames fall due. The jitter a round trip carries is as likely to lengthen it as to shorten
  it, so the corrections it causes cancel out and the clients keep to the host's clock however many there
  are. Pacing on the newest input any player had sent did not: that frontier is the largest of several
  noisy positions, so it always stood a little ahead of everyone, and at 60 ms of jitter four clients ran
  2 % faster than the host's clock and eight 2.7 %; a client measured by how far its own inputs had reached
  the relay was worse, since a client catching up on settled frames sends no inputs at all.
- A `TimeSync` judges on four pongs at a time and corrects by running one extra tick per host frame until the
  client has caught up, or one fewer while it is ahead, never by changing `dt`. Pongs from before the relay's
  clock started are left out, and so are those that come back while a correction runs or whose pings went
  out before it had run its course, however late they come back. Every pong from a started clock, those left
  out included, updates the lead a host shows: the predicted frame against the one the pong had due, less
  the half round trip since, which is how far ahead of the relay's clock the client plays.
- A client cut off from the relay stalls once its prediction window is full, while the relay confirms its
  frames at the deadline without it and everyone else plays on at full pace. Back on the network, it takes
  in every confirmation it missed, the reliable resends among them, plays through them as settled frames
  and catches up by extra ticks; its checksums agree with everyone else's all along.
- Optional local input delay (default 0) trades responsiveness for fewer rollbacks; `maxPrediction` and
  input delay together allow lockstep-like tuning without new code paths. With a delay of `d` a tick samples
  the local input for frame `P + 1 + d`, and the first `d` frames of a session play the neutral input for the
  local player. A delay as long as the relay takes to answer means every frame is settled before it is
  played. Each client chooses its own delay, so it is not part of `SessionConfig`.

### 8.4 Input confirmation at the relay

- The relay confirms frame `N` when it has inputs from every active slot for `N`, or when the deadline
  (`inputDeadline`, default 100 ms after the first input for `N` arrives) expires; missing inputs are
  replaced by "repeat last input" and flagged `dropped`. Late inputs for confirmed frames are ignored.
- Confirmed input sets are broadcast to all clients and spectators and appended to the relay's input log
  (kept in memory for late-join / reconnect and optionally written as a replay).
- The relay is given the time through an `IClock` (a `ManualClock` in tests and the runner, a real clock in
  `unison_relay`) and checks deadlines in `update()` as well as when inputs arrive. A frame nobody has sent
  an input for has no deadline: the relay waits for the first one. A dropped slot repeats the last input
  confirmed for it, neutral before the first.
- A confirmation goes out on the unreliable channel at once, carrying the three frames confirmed before it
  as well (`kRedundantConfirmations = 4`, fewer when a datagram cannot hold them), so a lost confirmation
  costs a client one frame rather than the wait for the next reliable batch: at a fifth of the messages lost
  and a 240 ms round trip, four clients kept pace with the host only once confirmations were repeated. Every
  `reliableResendInterval` frames (10 by default, about 167 ms at 60 Hz) the last batch goes out again on the
  reliable channel from the relay's log of confirmed frames, in as few messages as it fits, so a client that
  lost confirmations recovers them without asking; it simply ignores the ones it already has.
- Frames are confirmed in order and each once. A slot nobody plays is confirmed absent: no flags and the
  neutral input. The first input a slot sends for a frame is the one that stands; inputs from a client
  without a slot, of another size than the config's, for a frame already confirmed or for one more than
  128 frames ahead are dropped, which the redundant `K` inputs of every `Input` message make harmless.

### 8.5 Checksums and desync handling

- Checksum = XXH3-64 over globals, every component pool (registration order, packed arrays), and the Jolt state buffer.
- Interval: every verified frame in tools and tests; every 20 verified frames in network sessions by default.
  The interval is part of `SessionConfig`, so every client checksums the same frames and the relay always has
  something to compare. A checksum is taken from the verified frame's snapshot, never from the live frame,
  which by then has moved on.
- The relay compares checksums per frame across clients. On mismatch it broadcasts `DesyncDetected(frame, minority slots)`.
  A frame is judged once every player has reported it; the majority is a checksum more than half of them
  report, and without one every player is in the minority, since nobody can tell who is right. Only players
  report, and at most 64 frames wait for their reports, the oldest giving way.
  A client keeps the serialised snapshots of the frames it checksummed in the last 128 frames, longer than the relay
  keeps a frame waiting, and, told of a desync, writes the one of that frame into `desync_<frame>_<slot>.snapshot`
  in its dump folder: `--dump-dir`, the working folder unless told otherwise, none when empty. `unison_replay diff a
  b` names the part, the entity, the byte and the field two dumps first differ in.

### 8.6 Late-join, reconnect, spectators

- **Late-join**: a player's `Hello` into a running room, one the relay has confirmed a frame of, takes the lowest
  free slot as a member still catching up, whose slot the relay confirms frames without, so the match plays on
  unslowed. The relay picks a donor among the players in play, never one still joining: the one with the lowest
  round trip as its transport measures it (`IRoundTripMeter`, ENet's peer round trip), the lowest slot among equals
  or where nothing is measured, as in-process, and sends it `SnapshotRequest` for the frame after the newest it has
  confirmed (a room with no player left to ask welcomes the joiner from frame 0, as a room that has not started
  does); the donor answers with the first frame it verifies at or after that one. The donor serialises that verified frame `F` (`serializeSnapshot`:
  registry + physics + globals) and sends it to the relay over the reliable channel in numbered `SnapshotChunk`s of
  as many bytes as a datagram carries (`snapshotBytesPerChunk`); `SnapshotAssembler` gathers them in any order,
  refusing a chunk of another frame or count, a repeat, and a snapshot of more than 4096 chunks. The relay
  (`LateJoins`) hands the chunks on to every player waiting for that donor, each taking a snapshot from its first
  chunk on, so players who join together share one: a `Welcome` whose start frame is `F`, the chunks, then every
  frame confirmed since `F`, all over the reliable channel; a chunk of a frame the relay has not confirmed is
  dropped. The joiner hears the live confirmations all along. It restores `F` and plays the confirmed frames at up
  to eight a host frame (`kCatchUpExtraTicks`), sending no input until it has reached the newest frame it has heard
  confirmed; the relay puts its slot in play from the first frame of its first input, or from the next frame to
  confirm when that one is later. A joiner whose donor leaves before the last chunk has its snapshot asked of the
  next player in play, the nearest by round trip, and is welcomed again at that snapshot's frame, the client taking
  a newer welcome in place of one whose snapshot is not whole yet. A running room with no player in play left keeps
  its joiners waiting and would welcome a new player from frame 0, behind the frames it has confirmed: Q18.
- **Reconnect**: same mechanism; the relay holds the slot for `reconnectGrace` (default 30 s) and applies the drop
  policy to the absent player's inputs meanwhile. Every player is welcomed with a reconnect token of its own, a
  joiner at the snapshot's frame included, drawn by `ReconnectTokens` from `RelaySettings::reconnectTokenSeed` and
  never nought, which stands for none; a spectator gets none. The standalone relay takes the seed from
  `std::random_device`, so nobody can guess another player's token. When a player's peer goes, the relay holds its
  slot until `RelaySettings::reconnectGraceMicroseconds` has passed: the slot stays in play but is no longer
  awaited, so the frames after it are confirmed as soon as the others' inputs are in, its last input repeated and
  flagged dropped (§8.4); nobody waits for its checksums, nothing is sent to its peer and it is never a donor. Once
  the grace has passed, `update()` releases the slot: it is confirmed absent from then on and free for a newcomer,
  and a room left with no member closes. A player still joining or a spectator that goes is let go at once. A client
  that lost the relay joins again from a new session over a new connection with the token of its last welcome
  (`NetworkedSession::join(token)`, `reconnectToken()`). The relay hands the slot of the member holding the token to
  the new peer, whether its old peer has gone or the relay has not seen it go yet, and the returning player catches
  up as a late joiner does: a donor's snapshot, a welcome at its frame with the same token, the frames confirmed
  since, and no input until it has caught up; a player catching up is never a donor. Its slot stays in play all
  along, its last input repeated as dropped, and the relay waits for its inputs again from the first frame it sends
  one for. In a match that has not started it is welcomed at frame 0 at once, and a token nobody holds seats the
  client as any other player.
- **Spectators**: receive confirmed inputs only, run without prediction (`P = V`), optionally with an added delay.

A serialised snapshot is little-endian throughout. It opens with the magic `UNSS`, the format's version and a
hash of the component layout, every registered component's name and size in registration order, so a build
with other pools refuses it; then the frame's number and step, the globals (the generator's state, the match
phase, the free body ids and the slots taken), the registry (the entity storage's identifiers in packed order
and how many are alive, then every registered pool in registration order, its count and each identifier with
its value) and the physics state with its length. Walking the pools in registration order, never in the order
EnTT's storages were first touched, makes the bytes the same on every client that holds the same state.
`deserializeSnapshot` refuses bytes without the magic, of another version or layout, with an impossible match
phase or body id allocator, an identifier named twice or no storage could hold, more entities alive than held,
a component on an entity that is not alive or two on one, and bytes that end early or run on. The physics
state is carried as bytes and checked by the physics world when the snapshot is restored, under its contract.

`firstDifferenceOf` compares two serialised snapshots of one layout. Everything before their first differing
byte is alike, so the layout of the first up to that byte is the layout of both, and the byte is placed in the
part it falls in: the frame number, the step, the globals, the entity storage, a component's pool or the
physics state. In a pool it names the entity, the offset of the byte within that entity's component and the
field it falls in when the component names its fields, or no offset when the two pools hold another entity
there. Two snapshots alike byte for byte do not differ, and bytes either reader refuses are refused.

### 8.7 Replays

A replay file contains the session config, the asset hash, all confirmed inputs per frame, and the periodic
checksums. Replays are deterministic by construction; `unison_replay play` re-simulates, `verify` compares
checksums, `diff` restores two snapshots and prints the first differing component.

The file is little-endian throughout, as the wire is. It opens with the magic `UNRP` and the format's version,
then the session config written as the protocol writes it, the asset hash among its fields, and then records
in the order they were written, each a kind byte and its fields: a frame, its number and every slot's flags
byte and input of the config's size, or a checksum, the number of a verified frame and its XXH3.
`ReplayWriter` writes one and `ReplayReader` reads it back, refusing bytes without the magic, another version
of the format, a config no session can play, ticking no times a second, seating no player or more than a frame
holds, with larger inputs than a slot holds or checksums on no interval, a record of no known kind and a
replay that ends inside its header or a record.

`ReplayPlayer` plays the frames back through a `Session` handed each frame's inputs before it plays the frame,
so the session never predicts and verifies every frame at once, taking checksums on the config's interval
exactly as the recording session did. It hands each checksum back as the session takes it and keeps neither
checksums nor events, so once warmed up it takes nothing from the heap and a replay of any length plays in
constant memory. A frame that does not follow the last one played is refused: the frames' order is the
player's to check, not the reader's.

A client records a replay as its session verifies frames. The session tells the `IVerifiedFrameReceiver` it
was given of every frame it verifies, in order and before the frame leaves its window: the frame's number, the
inputs the relay settled for it, its checksum when the config's interval falls on it, and its snapshot.
`VerifiedFrameFanOut` hands one frame to several receivers. `ReplayWriter` is such a receiver, and so is
`DesyncDumper`. `--record <file>` records the runner's first client or the console's own session and writes
the file once the match ends; `writeFileBytes` and `readFileBytes` move a replay or a dump between memory and
a file whole.

A checksum follows the frame it was taken of. `verifyReplay` plays every frame through a `ReplayPlayer` and
compares every checksum the replay recorded with the one the player took of the same frame, counting how many
it compared and how many matched and naming the first frame whose checksum differs, the first checksum taken
at or after an input that was tampered with; a checksum of a frame the replay did not just play is a malformed
replay, and a record the reader refuses stops it with the reader's error.

---

## 9. Networking

### 9.1 Transport abstraction

```cpp
class IMessageReceiver {
    virtual void receive(PeerId from, Channel channel, std::span<const std::byte> message) = 0;
    virtual void peerLeft(PeerId peer) {}
};

class ITransport {
    virtual void send(PeerId to, Channel channel /*Reliable|Unreliable*/, std::span<const std::byte>) = 0;
    virtual void poll(IMessageReceiver& receiver) = 0;
};
```

A poll hands messages to a receiver interface rather than to a `std::function`, so polling every tick
allocates nothing and the relay and the session each receive as one small interface. A reliable message may
be of any length, the transport splitting it as it must; an unreliable one carries at most
`kMaxUnreliableMessageSize` (1200) bytes, so it crosses the internet in one datagram and is lost or not as a
whole, and a longer one breaks a contract in every transport alike. ENet would otherwise split it and send
the pieces reliably. The protocol's `kMaxDatagramSize` is that same limit. A poll also reports a
peer that has gone, whether it said goodbye or stopped answering; a receiver that does not follow peers
ignores it by default, and one that hands messages on, such as the runner's checksum wiretap, hands the
departure on too. A peer whose connection comes up is reported as well, which is how a client tells a
transport still reaching its relay from a relay that has not let it in yet; a transport without
connections, as the loopback hub, never reports one.

- `LoopbackHub`: in-process endpoints that deliver to one another at once. A `SimulatedLink` wraps any
  transport so that what it sends first crosses a seeded `NetworkSimulator` with latency, jitter (the delay
  strays up to that far either way), loss and the reordering jitter brings, so that runner results are
  themselves reproducible. The reliable channel is never lost and never overtaken between the same two ends,
  and simulated time only moves when the runner advances it.
- `EnetTransport`: ENet client/server with two channels (reliable, unreliable-sequenced). `listen(address,
  maxPeers)` binds an IPv4 address and a port, nought for one the system picks, and returns the transport or
  `NetworkUnavailable` in `tl::expected`; tests listen on 127.0.0.1 only, which keeps the firewall out of
  them. `connect(to, from)` returns the transport with the peer id it knows the server by, sending from
  `from` or, when it is not given, from any interface through a port the system picks; the connection
  comes up as the transport is polled, and what is sent before then waits for it and goes in order. Every
  peer gets a peer id the transport never gives again, a send is flushed at once rather than waiting for
  the next poll, a message for a peer that has gone is dropped, and a transport that goes away says goodbye
  to every peer, so the other side hears of it at once rather than at a timeout. A peer silent for longer
  than the peer timeout (five seconds unless the transport is told otherwise) is reported gone too. ENet's
  headers, which bring in `winsock2.h` on Windows and the system's socket headers elsewhere, stay inside the
  transport's source file.

### 9.2 Relay protocol (v1)

| Message | Direction | Channel | Payload |
|---------|-----------|---------|---------|
| `Hello` | client → relay | reliable | protocol version, the whole session config (the relay hashes it itself, and a standalone relay opens a room from it), requested role (player / spectator), reconnect token (0 for none) |
| `Welcome` | relay → client | reliable | slot id, session config, start frame, frame confirmed so far, reconnect token |
| `Input` | client → relay | unreliable | first frame, input size, frame count, that many inputs (K = 4 by default) |
| `Confirmed` | relay → all | unreliable (+ periodic reliable resend) | first frame, slot count, input size, frame count, then frame after frame per slot a flags byte and the input (the newest frame and up to three before it) |
| `Checksum` | client → relay | reliable | frame, hash |
| `Desync` | relay → all | reliable | frame, one bit per slot in the minority |
| `SnapshotRequest` / `SnapshotChunk` | relay ↔ clients | reliable | frame; for a chunk also its index, the chunk count and length-prefixed bytes |
| `Ping` / `Pong` | both | unreliable | sender's timestamp in microseconds; the pong echoes it with the frame the relay confirmed so far and the frame the relay's clock has due |
| `Leave` / `Kick` | both | reliable | reason (quit, protocol mismatch, config mismatch, room full) |

On the wire a message is a one-byte type tag followed by its fields in order, little-endian, with nothing
left over. The tag is the position of the message in `net::Message`, so new messages are only appended.
Decoding untrusted bytes reports truncation and anything malformed — an unknown tag or value, sizes that
disagree with the bytes, a chunk numbered past its count, bytes left over — through `tl::expected`; the
byte spans of a decoded message view the buffer it was read from. `SessionConfig` lives in `unison_net`,
because the relay reads it and `Welcome` carries it, and the session sits above the network layer.

The protocol lives in `unison_net`; the same `RelayCore` state machine runs inside the
runner (in-process) and inside `unison_relay` (ENet), so tests exercise the real relay logic.
Inside it, a `Roster` records who is in the match and which slot each of them plays, at most `kMaxSlots`
(eight) since a mask of slots has a bit for each, and an `Outbox` writes every message for the wire and
hands it to the transport.
A `RelayCore` hosts one match with the config it was created with: a `Hello` in the wrong protocol version
or whose config hashes otherwise is answered with `Kick`, a player takes the lowest free slot or is kicked
when none is left, a spectator is welcomed without a slot (`kNoSlot`), and a member's second `Hello` changes
nothing, where it once seated the member again in another slot. A `Hello` carries the whole
config rather than its hash because the relay never simulates and cannot know a game's asset and pipeline
hashes: the standalone relay opens a room from the first `Hello` of a config, one room per config, since
lobbies are out of scope in v1, and rooms that come and go with their players are its concern (3.2.2).

`unison_relay` is that standalone relay: it listens with an `EnetTransport` on `--bind` and `--port` (0.0.0.0:7777
unless told otherwise) for `--max-peers` peers, and hands every message to `RelayRooms`, which opens a `RelayCore`
for the first `Hello` of every config and passes each seated peer's messages to its room from then on; a peer that
has said no hello is not answered. The transport is every room's round-trip meter too, so a late joiner's snapshot
comes from the player in play with ENet's lowest round trip (§8.6). A peer that has gone leaves its room: a player's
slot is held for the reconnect grace, its frames confirmed dropped rather than waited for, and then released, while
a spectator's or a joiner's place goes at once; the room closes once it has no member left, which the loop checks
every round. The reconnect tokens are seeded from `std::random_device`. Ctrl+C or `SIGTERM` stops the loop, and the
transport says goodbye to every peer as it goes. The loop polls the transport, lets every room confirm the frames
whose deadline has passed and sleeps a millisecond, on a `SteadyClock` that counts from start-up. While it runs, a
`MillisecondTimer` asks Windows for a timer of a millisecond, as the console's loop does too: a sleep otherwise
lasts a tick of Windows' default 15.6 ms timer, which held both loops to 64 Hz, kept every message waiting up to 16
ms at either end and read round trips of 16 to 32 ms on localhost. On macOS the timer asks for nothing, as a thread
there sleeps a millisecond in about 1.3: twenty such sleeps took 25 to 26 ms on the Mac on 2026-09-25.
`--input-deadline`, `--resend-interval` and `--peer-timeout` set the room's `RelaySettings` and the transport, and
`--run-for` stops it after that many seconds. It logs through `LogSink` to standard output. Bytes that decode to no
message go unanswered, and so does a `Ping` from a peer that is not in the match; a member's ping is answered at
once on the unreliable channel with its own stamp, the newest frame the relay has confirmed and the frame its
`MatchClock` has due (§8.3), and the sender works out the round trip from its own clock.

A client plays through a `NetworkedSession`. It says hello when the host asks it to join, plays a `Session`
in the slot the `Welcome` names, and on every host frame takes in what the relay sent and pings it when a
ping is due (§8.3). On every tick it ticks the session, sends the input of its newest frame with up to three
before it that the relay has not confirmed yet (`K = 4`) on the unreliable channel, and sends the checksums
of the frames it verified on the reliable one, through an `Outbox` of its own. Every frame a confirmation carries is settled in turn; one the
session no longer holds, as the relay's repeats and reliable resends often are, is dropped, and one it has
settled already changes nothing. Anything from a peer other than the relay is dropped too. A `Desync` is
kept for the host to read.

Where the client stands is a `ConnectionState`: `Idle` until the host asks it to join, `Connecting` while its
hello waits for the transport to reach the relay, `Joining` once the transport has (an in-process transport
never says so, and the welcome moves a connecting client straight on), `Playing` in the slot the welcome
names, `Stalled` while its prediction window is full, and `Disconnected` once the relay sends it away or its
transport reports the relay gone. Every state it moves into is kept in order until the host clears them, as
the session's event changes are.

---

## 10. Host Integration

### 10.1 Bridge API (host side, `unison_view`)

- `SessionRunner::update(hostDeltaMicroseconds)` — takes in what the relay sent, runs as many ticks as the host's
  time holds (one more or one fewer when the session asks to keep pace, §8.3), hands the events those ticks
  raised and took back to the dispatcher, and returns the number of ticks, the rollbacks they took and the
  interpolation alpha for rendering. Host time is counted in whole microseconds, so no rounding drifts. A host
  that measures its own frames, as Unreal does, passes the delta; one that does not calls `update()`, and the
  runner lets pass what its clock has counted since the last update. A runner made without a clock runs on a
  `SteadyClock` of its own, the real time, and tests and the runner tool hand it a `ManualClock`.
- The live frame belongs to the game, which the host holds and reads (`ArenaSimulation::frame()` for the
  sample), read-only.
- `EventDispatcher` — `on<EventType>(callback)` plus `onCancelled<EventType>`; the runner has it forget the
  frames below `V` after every host frame, since nothing can take their events back any more.
- `EntityViewMap` — maps `entt::entity` to a host handle; fed by `EntityCreated` / `EntityDestroyed` events.
- `TransformInterpolator` — keeps the previous tick's transforms (host side, not simulation state) and returns
  the interpolated pose for the current render frame.
- Local input submission: `SessionRunner::setLocalInput(bytes)` once per host frame, the game's input as its bytes
  so no simulation template reaches the host (§7.3); the runner samples it at each tick.

### 10.2 Terminal hosts

- `unison_runner`: CI workhorse; arguments for players, frames, seed, network simulator parameters, checksum
  interval, replay recording; non-zero exit code on desync or window overflow; prints rollback statistics. Its
  clients dump the snapshot of a desync into `--dump-dir`, the working folder unless told otherwise, and `--fault
  <client>` starts that client with the first player one health point low, to show a desync and its dumps.
  `--late-join-at <frame>` lets the last client join once the first has verified that frame, through a snapshot as
  §8.6 has it; the ledger compares the frames every client reported, the late one's from its snapshot on, and the
  report names every client that joined late and the frame of its snapshot. Its config carries the arena's asset and
  pipeline hashes, as the console's does, so a replay it records names what it was played with. It plays the arena
  with every client and the relay in one process, each client's link crossing one seeded simulated network and each
  player scripted from the seed, one host frame at a time; a run that has not verified every frame after twice as
  many host frames and ten seconds more fails. A wiretap in front of the relay writes every checksum the clients
  report into a ledger, and the run lasts until every client has reported the last frame it checks; the first frame
  the clients report different checksums for is the desync, printed with every slot's checksum of it. The exit code
  is 0 for a run that verified every frame alike within its window, 2 for a desync, 3 for a rollback deeper than the
  prediction window, and 1 for a run that missed frames, a command line it could not read or a replay it could not
  write; a desync outranks an overflow, and both outrank missed frames. After the verdict every run prints a table
  of rollbacks by slot, in slot order: how many, how many per second of play, how deep on average and at most, and
  how many ticks the client stalled with its window full, closed by a row for every client together.
- `unison_console`: text visualisation (top-down ASCII map of the arena, health, rollback/ping stats), keyboard input,
  connects to `unison_relay`. TUI library candidate: FTXUI (MIT); fallback is plain console output. It joins
  the relay at `--host` and `--port` (127.0.0.1:7777 unless told otherwise), sending from `--from` or from
  whatever address the system picks, plays the arena for `--players` players, which every console of a match
  must agree on as the config's seed, asset hash and pipeline hash come from the build, and ten times a second
  draws its screen over the last one in place: a status line, where it stands, its slot, its verified and
  predicted frames, the rollbacks of the last second, the round trip and the lead, and below it the map of the
  frame it predicts. Stalled is where it stands while its prediction window is full. The lead is how far
  ahead of the relay's clock it played when the last pong came back (§8.3), about half a round trip while it
  keeps pace. The screen needs a console that understands the terminal's escape sequences, on macOS any terminal on
  standard output; with its output redirected the console prints the status line once a second instead. The screen
  is drawn in the window's own buffer, so when the console ends, its last screen and its last status line stay in
  view.
  It runs on the real clock until Ctrl+C, `--run-for` seconds, a disconnect, which ends it with exit code 1,
  or a desync the relay reports, which ends it with exit code 2 whatever else happened and puts the frame and
  the slots out of step at the end of the status line. With `--record` it writes the replay of the frames it
  verified into a file when it ends, and a replay it could not write turns an exit code of 0 into 1. Told of a desync, it
  dumps the snapshot of that frame into `--dump-dir`, the working folder unless told otherwise.
  `--spectate` comes with spectators (4.5.3). The keyboard is read without
  blocking from Windows' console input, which reports keys going down and up while the window has focus, and
  a lost focus lets every key go: W and S move forward and back, A and D to the sides, Space jumps, F fires,
  and Q and E turn the aim half a turn a second for as long as they are held, by the time held rather than by
  how often the loop asks. A console without a console window, its input redirected, stands still. On macOS
  (X.7), where no terminal reports a key going up, the console takes the terminal into raw mode, so keys neither echo
  nor wait for a line, and on every read asks the window server which game keys are down
  (`CGEventSourceKeyState`), so keys held together work as on Windows; the answer is the whole login
  session's, so a Mac console reads the keys whichever window is in front, and macOS may ask for the
  Terminal's Input Monitoring permission (Q8); a console whose terminal lacks it says so when it stops, below its
  last screen, since the first screen draws over whatever was printed before it. Its map comes from
  `arena_view_console`: the arena from above, +X to the right and +Z down, half a metre a column and a metre a row,
  `#` for walls, `=` for ramps, `C` for crates, `*` for shots and every player by the
  number of their slot with an arrow for the quarter turn they look along, and below it a line for every
  player with their health, or that they wait to come back, and their kills. The map reads the arena's
  components and runs none of its code (§7.3).
- `unison_replay`: record / play / verify / diff. It builds the arena for the players and the tick rate of a
  replay's config and refuses a replay recorded with other assets or systems than the build's, whose hashes the
  config carries. `play <file>` re-simulates the replay and prints how many frames it played and the checksum of the
  last; `verify <file>` also compares every checksum the replay recorded and prints how many of them match and the
  first frame that differs. The exit code is 0 for a replay played through with every checksum matching, 2 for a
  checksum that differs, and 1 for a command line it could not read, a file it could not read, a malformed replay or
  one of other assets or systems. `diff <a> <b>` compares two serialised snapshots, desync dumps above all, and
  prints that they are alike, exit code 0, or the part, entity, byte and field they first differ in, exit code 2.

### 10.3 Unreal Engine plugin (Phase 5)

- Plugin `Unison` with module `UnisonRuntime` (and optional `UnisonEditor` for debug tooling).
- `tools/build_unreal_thirdparty.ps1` builds all deterministic libraries and Jolt/ENet with CMake and copies
  `.lib` + headers into `integrations/unreal/Unison/Source/ThirdParty/`. `Unison.Build.cs` links them.
  Toolchain, CRT (`/MD`), C++20, exceptions-off and RTTI-off settings match UE's build.
- On macOS the same CMake build, run by a shell twin of that script, makes arm64 `.a` archives with the clang
  flags of §7.1 and copies them into `ThirdParty/lib/Mac/` beside `lib/Win64/`; `Unison.uplugin` allows `Win64`
  and `Mac`, and `Unison.Build.cs` links the archives of its platform. Universal arm64 and x86-64 binaries are
  backlog.
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
| Determinism | Catch2 + golden files | items 1–6 of Section 7.4 |
| Integration | `unison_runner` via CTest | 2/4/8 clients at 30 and 60 Hz losing 0, 5 or 20 % of the unreliable messages, over a one-way latency of 120 ms with 30 ms of jitter: eighteen profiles of five seconds of play each, labelled `profile` |
| Integration over UDP | Catch2 | the relay's rooms and two arena clients over `EnetTransport` on 127.0.0.1, a thousand frames, every checksum alike, on a manual clock so the test runs as fast as the machine does |
| Benchmarks | Catch2 `BENCHMARK` | tick, snapshot, restore, resimulate k frames, checksum |
| Manual | LAN sessions and the UE sample, on both platforms | Definition of Done items 2, 7 and 10 |

CI is a local script per platform in v1, `tools/ci.ps1` on Windows and `tools/ci.sh` on macOS, over the same
workflow presets of `CMakePresets.json`: configure, build Debug and Release, run all tests and the golden
replay, then check formatting. CTest runs on half the logical processors, since the eight-player
profiles take half a minute each in Debug. A change to code both platforms share is finished once both
machines are green. A hosted CI matrix is a backlog item.

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
  tools/                  # unison_runner, unison_replay, unison_console, and scripts: env, ci and
                          # build_unreal_thirdparty, as .ps1 on Windows and .sh on macOS
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
| cxxopts | CLI parsing (tools only) | MIT | built without exceptions or RTTI (`CXXOPTS_NO_EXCEPTIONS`, `CXXOPTS_NO_RTTI`), so a malformed command line ends the tool with its own message |
| tl::expected | recoverable errors without exceptions | CC0-1.0 | header-only |
| FTXUI | console UI (candidate, tools only) | MIT | |

Rules: no dependency enters a deterministic library without a determinism review;
tools-only dependencies never leak into libraries linked by the UE plugin;
`std::format` replaces fmt; logging in libraries goes through a callback, never to stdout.

---

## 14. Sample Game: Arena

- **World**: flat floor, static boxes and ramps (Jolt static bodies from asset tables defined in code; file loading is backlog), 8 spawn points, in the corners and halfway along the walls, each facing the middle; a match holds up to eight players, as many as a relay has slots.
- **Tick**: a match ticks at the rate its host asks for, 60 Hz unless told otherwise, and each tick steps the
  frame by that share of a second. Timers counted in frames (warm-up, respawn, cooldown, lifetime) count
  ticks, so at 30 Hz they last twice as long.
- **Players**: capsule `CharacterVirtual`; move, jump, aim yaw, fire. A yaw of nothing looks along +Z and a
  quarter turn looks along +X (`facingOf`); every yaw of the arena reads that way, a spawn point's included.
  An input's yaw is absolute, so a player faces the way their spawn point does only in the frame they appear,
  until a host starts its aim there (backlog).
- **Projectiles**: swept spheres with a lifetime, carried as entities rather than as Jolt bodies; each tick a
  shot sweeps from where it was to where it is going, so a shot at 30 m/s cannot pass through a wall between
  two ticks. Hit → damage event; kill → respawn timer.
- **Dynamic props**: a few rigid-body crates, light enough for a player to shove, that `CharacterVirtual`
  pushes as it walks into them. A shot that reaches a crate stops at it without moving it; pushing bodies
  with a shot needs an impulse the physics wrapper does not offer yet, and is backlog.
- **Components**: `Transform`, `PlayerSlot`, `CharacterState`, `Health`, `Weapon`, `Projectile`, `Lifetime`, `PhysicsBody`, `RespawnTimer`.
- **Systems** (in order): `ApplyInput`, `CharacterMove`, `Weapons`, `PhysicsStep`, `Hits`, `Lifetimes`, `Respawn`, `MatchRules` (the system is plural, because `Lifetime` is the component it counts down).
- **Input**: `moveX/moveY: int8`, `yaw: int16`, `buttons: uint16 {Jump, Fire}`. The buttons take sixteen bits
  rather than eight so that `ArenaInput` carries no padding, which `InputTraits` requires of an input.
- **Events**: `EntityCreated`, `EntityDestroyed`, `Fired` (predicted), `Hit` (predicted), `Died` (verified-only), `Respawned` (verified-only).
- **Views**: text top-down map in the console (`arena_view_console`, §10.2); capsule/box meshes in UE.

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

**Phase X — Cross-platform: macOS**, between Phases 3 and 4 on the owner's word of 2026-09-25. The compiler
contract on clang, the floating-point environment on arm64, every target and test on macOS, goldens recorded on
Windows verified on macOS, the console on macOS, a LAN run between the two platforms; planned and recorded in
`docs/CROSS_PLATFORM.md`. *Exit*: its exit criteria there, and Definition of Done item 10 for the consoles.

**Phase 4 — Session features.** Replay files and `unison_replay`, snapshot serialisation, late-join,
reconnect, spectators, state diff tool. *Exit*: Definition of Done items 3–6.

**Phase 5 — Unreal plugin.** ThirdParty build script, `UnisonRuntime`, subsystem, input, entity views,
events, interpolation, debug HUD, Arena UE sample, on Windows and on macOS. *Exit*: Definition of Done item 7
on both platforms, and item 10 for the Unreal hosts.

**Phase 6 — Hardening.** Profiling, Jolt multithreading evaluation, memory budgets, API polish, docs.
*Exit*: Definition of Done items 8–9.

**Backlog (post-v1).** C ABI + Unity/Godot bindings; determinism on Linux, x64 and ARM64, on x86-64 macOS
and on ARM64 Windows, with a hosted CI matrix over every platform; universal arm64 and x86-64 macOS binaries
of the plugin;
navigation (Recast for baking, Detour at runtime with deterministic math shims); authoritative-server mode;
DSL/codegen; 2D physics module (Box2D v3, cross-platform deterministic); encryption / Steam relay via
GameNetworkingSockets; lobbies and matchmaking; multiple local players per client; delta-compressed inputs
for 16+ players; frame-local heap allocator; asset loading from files; a host's aim that starts where the
player's spawn point faces; a physics state from an untrusted peer checked before it is restored.

---

## 16. Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Float determinism breaks with a future compiler or platform | Desyncs | Flags module shared by all deterministic libs, no libm trig, FP-environment guard, Debug-vs-Release and cross-compiler checks; fallback: fixed-point gameplay math behind the same API (Jolt would stay float). |
| Two compilers, two standard libraries and two instruction sets disagree: MSVC with SSE2 on x64, Apple clang with NEON on arm64 | Desyncs between Windows and macOS clients | Jolt's `CROSS_PLATFORM_DETERMINISTIC` build, which Jolt verifies across both; the clang contract of §7.1 read off every command line and proven by a canary; FPCR held like MXCSR (§7.2); the rules of §7.3 on order, type names, widths, NaN and conversions; goldens recorded on Windows verified on macOS (§7.4); fallback: an x86-64 build under Rosetta 2, which runs the SSE2 code of Windows. |
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
| Q1 | Snapshot ring (single live frame) vs Quantum-style verified + predicted frames | Answered 2026-09-23 from the Phase 1 numbers and the Definition of Done's run (§8.2): the ring stays. It is 13 % cheaper while every guess is right, the two frames 32 % cheaper under 26 rollbacks a second 16 frames deep, and the difference is at most 1.4 ms a second of play, 0.14 % of a core, not worth a second simulation path and a rewrite of the session. |
| Q2 | SSE2 vs AVX2 baseline for deterministic libraries | Answered 2026-09-22 from measured numbers: SSE2 stays. Building everything for AVX2, our libraries and Jolt together, settles on the same golden checksums for the physics pile and for the scripted arena, so the determinism contract does not rest on the instruction set. It buys 6 % on a tick and nothing on snapshots or restores, while a binary that needs AVX2 cannot run on a machine without it, so shipping it would mean shipping two. Re-run the comparison with `-DUNISON_INSTRUCTION_SET=AVX2`. |
| Q3 | FTXUI vs plain console output for `unison_console` | Answered 2026-09-23 by the owner: plain console. A game needs to know which keys are held, and a terminal, FTXUI's included, reports only presses and their auto-repeat, never a release; Windows' console input reports both. Output is plain text and VT sequences, which Windows 10 and later understand, and no dependency is added. On macOS, which has no such input, see Q8. |
| Q4 | Jolt multithreaded stepping inside the simulation | Phase 6 |
| Q5 | UE version and whether to support UE's Linux server target | Version answered 2026-09-21: UE 5.8 is installed (Visual Studio 18, MSVC 14.51); toolchain compatibility is confirmed in task 5.1.4. The Linux server target stays open until Phase 5. |
| Q6 | Input deadline and reconnect grace defaults | Phase 3 LAN tests |
| Q7 | Checksum interval default for production sessions | Phase 3 |
| Q8 | How the console reads held keys on macOS | Answered 2026-09-25 with the plan of `docs/CROSS_PLATFORM.md` (Q-A): the terminal in raw mode and `CGEventSourceKeyState` for the game keys, built in X.7.3. The spike, X.7.2, found the shell it ran in without Input Monitoring; a console whose terminal lacks the permission says so when it stops, and whether a key held in Terminal.app reads as down without it is the owner's hand check. |
| Q9 | Which clang-format both machines format with | Answered 2026-09-25 in X.1.1 (Q-C): clang-format 20.1.8, the version Visual Studio 2026 bundles, and on the Mac the same from Homebrew's `llvm@20`, which `tools/env.sh` exports as `UNISON_CLANG_FORMAT`. The tree passed the check on both without a change (X.1.3). |
| Q10 | Whether UE 5.8 builds on the Mac's Xcode 27.0 and macOS 27.0, which Epic's requirements do not list | Phase 5, task 5.1.6; Xcode 26.1.1 beside 27.0 if not. |
| Q11 | How the CI procedure runs on two platforms | Answered 2026-09-25 with the plan (Q-B): the procedure is a workflow preset of `CMakePresets.json` for each configuration, which `tools/ci.ps1` and `tools/ci.sh` run in the same shape before the formatting check (X.5.10); the Mac needs no PowerShell. |
| Q12 | Which warnings clang holds our code to | Answered 2026-09-26 (Q-D): `-Wall -Wextra -Wpedantic -Wshadow -Werror` from X.2.2, and `-Wconversion -Wsign-conversion` since X.5.12, once every library and tool built under them without a change. |
| Q13 | When a change is checked on Windows | Answered 2026-09-25 (Q-E) and reshaped by D36: a change that touches nothing Windows builds or runs is finished once the Mac is green; one that touches a shared CMake module, header, test or golden also needs `tools\ci.ps1` to print `ci: ok` on Windows once the owner has pulled it, and a failure there is fixed first as a `(+)` micro-task (`CLAUDE.md`). |
| Q14 | Line endings on two platforms | Answered 2026-09-25 (Q-F): `.gitattributes` holds `* text=auto eol=lf`, so clang-format and the golden readers see the same bytes on both machines; the index already held every text file with LF, so no commit renormalised the tree (X.1.2). |
| Q15 | Which platform records a golden | Answered 2026-09-25 (Q-G) and amended the same day under D36: goldens are recorded on Windows and verified on the Mac, a new golden may be recorded on the Mac while Windows is out of reach and the next Windows run is its check, and none is re-recorded on one platform alone (§7.4). |
| Q16 | An x86-64 build under Rosetta 2 as the Mac's fallback | Answered 2026-09-25 by X.6 (Q-H): not needed. Every golden recorded on Windows reproduces on arm64 in Debug and in Release, so the Mac plays natively, and x86-64 macOS stays in the backlog. |
| Q17 | Whether Definition of Done item 2 needs two Windows machines | Answered 2026-09-25 (Q-I): no. X.8.2, the run with the relay on Windows and a console on each platform, counts for item 2 as well, and 3.4.5 is ticked from it. |
| Q18 | What a running room with no player in play left does with the players still joining it, and with a new or a returning player's `Hello` | Open, raised 2026-09-26 in 4.3.7 for the owner. Nobody left holds the match's state, so no snapshot can come: the joiners wait for good, and a new player, or one back with its token while every other player is away, is welcomed from frame 0 and plays behind every frame the relay has confirmed. (a) Turn them away with a new `LeaveReason`, the match being over, as a Quantum room ends with its last player; the protocol's version goes to 6 and its golden is recorded again on Windows. (b) Start the room over from frame 0 with whoever is still joining, which leaves the protocol alone but gives the relay core a match to reset. (c) Leave it as it is, for the host to give up. Recommended: (a). |

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
