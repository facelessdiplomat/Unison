# Unison — Task Board

Progress tracker for everything in `docs/DESIGN.md`: phases → tasks → micro-tasks.
A micro-task is one TDD cycle: one failing test, the code that makes it pass, a refactor, the self-review
from `CLAUDE.md`, and one commit on the owner's command. Work goes strictly in order inside a task.
Tasks inside a phase are listed in dependency order; a later task may start only when everything it
needs from earlier tasks is ticked.

## How to use this board

- `[ ]` not started · `[x]` done · `~~struck through~~` dropped or deferred, with the reason appended.
- The single micro-task in progress is named under **Now**. Never two at once.
- A micro-task is ticked in the same change that completes it, so the board never disagrees with the code.
- A phase is finished when every micro-task and every exit criterion of the phase is ticked.
- Work discovered mid-phase is appended as a new micro-task marked `(+)` at the right place, never done silently.
- Ids are `phase.task.micro` and stable: never renumber, only append.
- Each micro-task ends with the test or the observable check that defines "done".

## Now

- Next up: **1.6.7**, the `Lifetime` system. Last finished: 1.6.6.
- 1.3.3 runs before 1.2.6: the lifecycle helpers raise events, and `raise` belongs to the event buffer, so the
  board order contradicts the dependency order it asks for. Ids stay as they are.

## Progress

| Phase | Tasks | Micro-tasks | Done |
|-------|-------|-------------|------|
| 0 Bootstrap | 2 | 15 | 15 |
| 1 Deterministic simulation core | 7 | 56 | 46 |
| 2 Rollback session (local) | 8 | 36 | 0 |
| 3 Real networking | 4 | 15 | 0 |
| 4 Session features | 5 | 20 | 0 |
| 5 Unreal Engine plugin | 2 | 15 | 0 |
| 6 Hardening | 3 | 11 | 0 |
| **Total** | **31** | **168** | **61** |

## Charter amendments made while planning

- Components store plain POD math types (`Float3`, `Quaternion`) from `unison_core`; Jolt vector types are used
  for computation only. Reason: SIMD alignment padding would enter checksums, and Jolt headers would leak into
  host boundary headers. `DESIGN.md` §6.3 updated.
- Arena assets are defined in code in v1; loading assets from files is backlog. `DESIGN.md` §14 and §15 updated.
- `FrameInputs` erases the input to bytes instead of being a template over `Input`. A template would reach
  `ISystem`, `SystemPipeline` and `Session`, and `DESIGN.md` §7.3 forbids a host module from instantiating a
  simulation template, which the Unreal plugin would have to do to drive a session. `DESIGN.md` §6.4 updated.
- EnTT compiles differently with and without exceptions and guards it with `detect_mismatch`, so
  `ENTT_NOEXCEPTION` is defined on its interface target and reaches every consumer, the test executable
  included. Found in 1.2.3, the first time a deterministic library linked EnTT code against the tests.
- `std::has_unique_object_representations_v` cannot express "no padding": it also rejects every type holding a
  `float`, so it rejects every component this engine has. Boost.PFR is pinned instead and the `PaddingFree`
  concept compares member sizes against `sizeof`, recursing through aggregates and arrays. `DESIGN.md` §6.3.
- Registered components must be free of padding, enforced by `UNISON_COMPONENT` rather than by the `= {}` habit
  alone: a forgotten value-initialisation would put indeterminate bytes into a checksum, and that desync leaves no
  trace. `RawValue`, `Hasher` and `BinaryWriter` stay permissive, since they also carry engine-internal types.
  `DESIGN.md` §6.3 updated.
- `LogSink` is a process-wide callback, the single exception to `CLAUDE.md` 4 (no global mutable state), because
  `UNISON_VERIFY` is a macro and cannot take an injected dependency. Bounded and justified in `DESIGN.md` §5.3.
- Engine-wide conventions added as `DESIGN.md` §5.3: Jolt-native units and axes, three-tier error policy
  (`UNISON_ASSERT` / `UNISON_VERIFY` / `tl::expected`), little-endian encoding, single simulation thread (D32, D34).
- Work rhythm: stop after every micro-task, report, wait for the owner (D33). Reflected in `CLAUDE.md`.
- Jolt keeps its allocator, its factory and its type list in globals and offers nothing to inject, so
  `JoltRuntime` counts the scopes that need them and registers once for the first. It is the second bounded
  exception to `CLAUDE.md` 4 after `LogSink`. `DESIGN.md` §5.3 and §6.7 updated. Found in 1.5.1.
- `BodyId` is the engine's own handle, not `JPH::BodyID`: it lives in `unison_core` without Jolt headers, so
  `Globals` and the components that name a body stay free of them, and its bits are packed exactly as Jolt
  packs them so the physics boundary only copies. `DESIGN.md` §6.7 updated. Found in 1.5.2.
- What a body is made of travels on the entity as a `BodyDefinition` component, a copy of the asset it was
  spawned from, not as a lookup through the asset registry. Reason: a rollback has to rebuild a body and put
  back the properties Jolt does not record, and a system may have changed them since the spawn, so the live
  values must be frame state. Restoring a snapshot needs no assets at all now. `DESIGN.md` §6.7 updated.
  Found in 1.5.7.
- A character's position lives in its `Transform`, not in a second field of `CharacterController`; the component
  holds the capsule, the velocity and the ground state. Reason: two places holding one position is a desync
  waiting to happen, and `Transform` is already the component every view and system reads. `DESIGN.md` §6.7
  updated. Found in 1.5.10.
- A game library is a CMake OBJECT library, not a static one: the translation unit that registers the
  components is pure static initialisation, and a linker drops it out of a static library because nothing
  refers to it. `DESIGN.md` §6.3 updated. Found in 1.6.1, where the arena registered nothing at all.
- Projectiles are swept entities, not Jolt bodies: at 30 m/s a sphere moves half a metre between ticks and
  would pass through a wall unless Jolt ran continuous collision for it, while a sweep from where it was to
  where it is going is exact and needs no body at all. `DESIGN.md` §14 updated. Found in 1.6.5.
- UE 5.8 confirmed as the plugin target (installed on the development machine); Q5 answered. Development
  toolchain is Visual Studio 18 with VS-bundled CMake/Ninja/clang-format, hence micro-task 0.1.8.

---

## Phase 0 — Bootstrap

**Exit criteria**
- [x] `cmake --preset msvc-debug` and `cmake --preset msvc-release` configure and build with zero warnings.
- [x] `ctest` is green in both configurations.
- [x] `tools/ci.ps1` exits 0.

### 0.1 Repository skeleton
- [x] 0.1.1 Root `CMakeLists.txt` (project `unison`, C++20, no compiler extensions, one folder per module) and `CMakePresets.json` with `msvc-debug` / `msvc-release` (Ninja, x64). Done when: both presets configure.
- [x] 0.1.2 `cmake/CPM.cmake` pinned, `CPM_SOURCE_CACHE` honoured, `cmake/Dependencies.cmake` as the single place for versions. Done when: configure succeeds with an empty dependency list.
- [x] 0.1.3 `cmake/UnisonDeterminism.cmake` with `unison_apply_determinism(target)`: `/fp:precise`, no `/fp:contract`, SSE2 baseline, `/EHs-c-`, `/GR-`, `/permissive-`, `/W4 /WX`. Done when: the flags appear in `compile_commands.json` for a probe target.
- [x] 0.1.4 `determinism_guard.hpp`: compile error unless `_M_FP_PRECISE` is defined (so `/fp:fast`, `/fp:strict` and the compiler default are all rejected) or when `_M_FP_CONTRACT` is defined; force-included into every deterministic library by `unison_apply_determinism`. Done when: negative `try_compile` runs with `/fp:fast`, `/fp:strict`, `/fp:precise /fp:contract` and no flag all fail as expected.
- [x] 0.1.5 Static library skeletons `unison_core`, `unison_sim`, `unison_session`, `unison_net`, `unison_view` with the `include/unison/<module>/` layout; determinism applied to `core` and `sim`. Done when: all build empty.
- [x] 0.1.6 `.gitignore` (build dirs, CPM cache, IDE files, UE artefacts), `LICENSE` (MIT, copyright holder `facelessdiplomat`), `README.md` pointing to the docs.
- [x] 0.1.7 `tools/ci.ps1`: configure and build both presets, `ctest --output-on-failure`, `clang-format --dry-run --Werror` over tracked sources. Done when: exits 0 on the skeleton from a plain PowerShell through `tools/env.ps1`.
- [x] 0.1.8 (+) `tools/env.ps1`: locates the VS-bundled CMake, Ninja and clang-format through `vswhere`, enters the x64 developer environment and exports the tools for `ci.ps1` and the presets. Done when: `tools/env.ps1` followed by `cmake --preset msvc-debug` configures from a plain PowerShell without a developer prompt; the `ci.ps1` half of the original check moved to 0.1.7.
- [x] 0.1.9 (+) `unison_apply_warnings(target)` applied to `unison_net`, `unison_session` and `unison_view` as well, so `/permissive- /W4 /WX` stops being a side effect of the determinism contract and every Unison library is held to the same warning discipline. Done when: a deliberate warning in `net` fails the build.

### 0.2 Dependencies (each: pinned version in `Dependencies.cmake` plus a smoke test)
- [x] 0.2.1 Catch2 v3 and the `unison_tests` target with `catch_discover_tests`; test tree `tests/<module>/`. Test: `"test framework runs"`.
- [x] 0.2.2 EnTT. Test: a registry emplaces a component and a view finds it.
- [x] 0.2.3 Jolt with `CROSS_PLATFORM_DETERMINISTIC=ON`, exceptions and RTTI off, profiler and debug renderer off, our determinism flags applied. Test: `JPH_CROSS_PLATFORM_DETERMINISTIC` is defined and a `PhysicsSystem` steps once.
- [x] 0.2.4 ENet. Test: `enet_initialize()` returns 0.
- [x] 0.2.5 xxHash in inline mode. Test: known-answer vectors from xxHash's own test suite.
- [x] 0.2.6 Test targets split by CTest labels: `unison_tests_fast` (unit and smoke) carries `fast`, the cmake infrastructure checks carry `slow`; a separate `unison_tests_slow` executable arrives with the first determinism suite in 1.7, rather than as an empty target now. Done when: `ctest -L fast` runs only unit tests.

---

## Phase 1 — Deterministic simulation core

**Exit criteria**
- [ ] Determinism checks 1–3 of `DESIGN.md` §7.4 pass on the Arena sample.
- [ ] The golden checksum file for the Arena scripted run is committed and matches in Debug and Release.
- [ ] Benchmarks for tick, snapshot, restore and checksum have baselines in `tests/benchmarks/baseline.md`.
- [ ] Open question Q2 (SSE2 vs AVX2) is answered in `DESIGN.md` from measured numbers.

### 1.1 Core primitives (`unison_core`)
- [x] 1.1.1 `FixedVector<T, N>`: push, pop, size, index, iteration, `clear`, full-capacity assert; trivially copyable when `T` is. Test: behaviour cases plus `std::is_trivially_copyable_v`.
- [x] 1.1.2 `FixedString<N>`: from `string_view`, comparison, `view()`, zeroed tail. Test: equal strings built differently are byte-identical.
- [x] 1.1.3 `Hasher` over XXH3-64: `add(span<const byte>)`, `add(const T&)` for trivially copyable `T`, `finish()`. Test: same bytes same hash, order matters, matches the one-shot XXH3 result.
- [x] 1.1.4 `BinaryWriter` / `BinaryReader`: POD values, spans, strings, bounds-checked reads that report failure instead of undefined behaviour. Test: round trip of every supported type; a truncated buffer fails cleanly.
- [x] 1.1.5 `FpEnvGuard`: sets MXCSR to round-to-nearest with denormals enabled, restores on scope exit. Test: with FTZ set outside, a denormal survives inside the guard and FTZ is back afterwards.
- [x] 1.1.6 `LogSink`: process-wide callback with levels; silent when unset. Test: the sink receives level and message.
- [x] 1.1.7 `AssetId`: `constexpr` 32-bit hash of a name, usable as a non-type template argument and in `switch`. Test: a fixed name gives a fixed id; distinct names differ.
- [x] 1.1.8 POD math storage types `Float3`, `Quaternion` (plain floats, natural alignment) with conversions to and from `JPH::Vec3` / `JPH::Quat`. Test: round trip is bit-exact; `sizeof(Float3) == 12`.
- [x] 1.1.9 Deterministic scalar facade `unison::math` (`sin`, `cos`, `atan2`, `sqrt`, `clamp`, `lerp` written without FMA) backed by Jolt's implementations. Test: golden bit patterns for a fixed input table.
- [x] 1.1.10 `Rng` (xoshiro256**): seed, `nextUint32`, `nextFloat01`, `nextInRange`. Test: golden sequence for seed 42; state is trivially copyable.
- [x] 1.1.11 (+) Error primitives: `UNISON_ASSERT` (Debug only), `UNISON_VERIFY` with an installable fatal handler reporting through `LogSink`, an `Error` type, and `tl::expected` pinned in `Dependencies.cmake`. Test: a failing verify invokes the installed handler; an assert has no effect in a Release probe; `expected` round trips value and error.
- [x] 1.1.12 (+) Compile-check target for `unison_core`: a translation unit built with `unison_apply_determinism` and `unison_apply_warnings` that instantiates the inline bodies of every public header, so headers are held to `/W4 /WX /EHs-c- /fp:precise` instead of only a syntax check. Reason: `core` is header-only, so `core.cpp` compiles no inline body, and the strict flags had to be verified by hand during 1.1.3 and 1.1.4. Done when: a deliberate warning inside a header body fails the build.
- [x] 1.1.13 (+) One declared exception setting per Unison target: `net`, `session` and `view` carry no `/EH` flag at all and no `_HAS_EXCEPTIONS=0`, so the MSVC STL still emits `try`/`catch` there that cannot unwind, and `unison_tests_fast` links libraries built with a different `_HAS_EXCEPTIONS` than its own translation units. Decide the setting for each target kind and assert it in `tests/cmake/module_determinism`. Reason: found in 1.1.8, the first time Jolt's headers reached a deterministic library. Done when: the module check fails if a Unison target disagrees with its declared exception setting.

### 1.2 Frame, registration, snapshots (`unison_sim`)
- [x] 1.2.1 `ComponentRegistry` and `UNISON_COMPONENT(Type)`: static list with name, size and alignment; `static_assert` plain data aggregate, trivially copyable and `PaddingFree` so a component carries no padding at all; registrations must all come from one translation unit, since static initialisation order across translation units is unspecified. Test: list order equals registration order; a duplicate registration is rejected; a registration from a second file is rejected; the `PaddingFree` concept accepts and rejects the right types.
- [x] 1.2.2 `Frame`: `frameNumber`, `dt`, `entt::registry`, `Globals` (rng, match phase placeholder), `EventBuffer` slot. Test: a default frame is at 0 and empty.
- [x] 1.2.3 Registry clone preserving entity ids, versions and free-list order. Test: after cloning, source and clone produce identical ids for 100 mixed creates and destroys.
- [x] 1.2.4 Frame checksum over globals and all pools in registration order. Globals are hashed member by member, not as raw bytes: `Globals` holds an `Rng` and a one-byte enum, so the compiler pads between them and `PaddingFree` does not hold for it. Test: equal frames hash equal; a one-byte change changes the hash; the hash does not depend on the order in which EnTT storages were first touched; changing a padding byte of `Globals` does not change the hash.
- [x] 1.2.5 `FrameSnapshot`, `takeSnapshot`, `restoreSnapshot` (registry, globals, physics bytes slot). Test: take, mutate, restore gives the original checksum.
- [x] 1.2.6 Entity lifecycle helpers `createEntity(frame)` / `destroyEntity(frame, entity)` raising `EntityCreated` / `EntityDestroyed`. Test: the events carry the entity and appear in the buffer.

### 1.3 Systems, signals, events
- [x] 1.3.1 `ISystem` and `SystemPipeline`: ordered `update(Frame&, const FrameInputs&)`. Test: systems run in registration order exactly once per `advance`.
- [x] 1.3.2 Pipeline hash from system names in order, for `SessionConfig`. Test: reordering changes the hash.
- [x] 1.3.3 `EventBuffer`: `raise<T>(payload)`, `EventKey{frame, typeId, ordinal}`, per-type kind trait (`VerifiedOnly` / `Predicted`), cleared on `advance`. Test: ordinals increase per type per frame; the buffer is empty after advance.
- [x] 1.3.4 `Signals`: typed synchronous subscriptions invoked in subscription order. Test: two subscribers receive a signal in order; an unsubscribed one does not.
- [x] 1.3.5 `Frame::advance(inputs)`: `FpEnvGuard`, pipeline, event flush, `frameNumber` increment. Test: the guard is active during systems (checked through a probe system).

### 1.4 Inputs and assets
- [x] 1.4.1 `InputTraits<Input>` (trivially copyable, free of padding, `sizeof <= 64`) and `FrameInputs`, a plain class holding up to 8 slots erased to bytes with per-slot flags `Present / Predicted / Dropped` and a typed accessor. Test: flags and payload round trip; an oversized input fails the trait; reading a slot as the wrong type breaks a contract.
- [x] 1.4.2 `AssetRegistry`: typed tables keyed by `AssetId`, `freeze()`, `get<T>(id)` returning `const T&`, missing id is a hard error, and registering an id that is already taken is a hard error, so a 32-bit name-hash collision cannot pass silently. Test: lookups; mutation after freeze is rejected; a duplicate id is rejected.
- [x] 1.4.3 Asset hash over frozen tables, independent of insertion order. Test: two registries with the same content inserted in different order hash equal.

### 1.5 Physics world (Jolt wrapper)
- [x] 1.5.1 `PhysicsWorld` construction: allocator, factory, type registration, `JobSystemSingleThreaded`, broadphase and object layers, `step(dt)`. Test: an empty world steps 100 times.
- [x] 1.5.2 `BodyIdAllocator` in `Globals`: deterministic index + sequence allocation with recycling. Test: golden id sequence; released ids are reused with an incremented sequence.
- [x] 1.5.3 `BodyDefinition` asset (shape, size, motion type, layer, friction, restitution) and `PhysicsBody` component; create via `CreateBodyWithID`, destroy on component removal. Test: the body exists with the requested id; a destroyed body is gone and its id is recyclable.
- [x] 1.5.4 `PhysicsStep` system: step, then write positions and rotations into `Transform` in ECS view order (never `GetActiveBodies`). Test: a dynamic box falls and rests on a static floor; `Transform` follows.
- [x] 1.5.5 Physics bytes in snapshots via `SaveState` / `RestoreState` with `EStateRecorderState::All`. Test: run A→B and hash; restore A, run to B, hash equal (with sleeping and active bodies).
- [x] 1.5.6 Body-set reconciliation before `RestoreState`: destroy bodies absent from the restored registry, recreate missing ones from `PhysicsBody` + `BodyDefinition`. Test: a body created after a snapshot disappears on restore; a body destroyed after a snapshot returns with identical state.
- [x] 1.5.7 Reapply properties Jolt does not record (friction, restitution, motion type) from components on restore. Test: a friction change made after the snapshot is reverted by restore.
- [x] 1.5.8 Query wrappers `raycast`, `overlapSphere`, `sweepCapsule` returning hits sorted by `(fraction, BodyID)`. Test: results are sorted regardless of body creation order.
- [x] 1.5.9 Contact listener buffering: contacts collected during `step`, sorted by `(BodyID a, BodyID b, sub-shape ids)`, exposed as `ContactEvents` on the frame. Test: two overlapping bodies yield exactly one ordered pair.
- [x] 1.5.10 `CharacterController` over `CharacterVirtual`: component holds position, velocity and ground state; explicit save/restore because it lives outside `PhysicsSystem` state. Test: walks on the floor, stops at a wall, snapshot/restore round trip is exact.
- [x] 1.5.11 Physics determinism test: 50 dynamic boxes for 600 frames, double run equal checksums, plus a golden checksum shared by Debug and Release.
- [x] 1.5.12 (+) `destroyEntity` takes the body of the entity with it: an entity destroyed with a `PhysicsBody` leaves its Jolt body and its id behind until the next reconciliation. Reason: found in 1.5.6, where reconciliation made the leak visible. Test: destroying an entity with a body leaves the world empty and hands the id back.

### 1.6 Arena sample simulation (`arena_sim`)
- [x] 1.6.1 Components (`Transform`, `PlayerSlot`, `CharacterState`, `Health`, `Weapon`, `Projectile`, `Lifetime`, `PhysicsBody`, `RespawnTimer`) registered; `ArenaInput{moveX, moveY, yaw, buttons}`. Test: all trivially copyable; `sizeof(ArenaInput) <= 8`.
- [x] 1.6.2 Arena assets in code: floor, walls, ramps, crates, spawn points, player stats, projectile stats. Test: the registry freezes and hashes stably.
- [x] 1.6.3 `ApplyInput` system: input to desired velocity and yaw on `CharacterState`; quantisation contract stated in the header. Test: full-forward input gives max speed along yaw.
- [x] 1.6.4 `CharacterMove` system: drives `CharacterController`, jump with ground check, gravity. Test: a jump leaves the ground and lands within the expected frames.
- [x] 1.6.5 `Weapons` system: fire button spawns a projectile with cooldown and raises `Fired`. Test: holding fire respects the cooldown; one event ordinal per shot.
- [x] 1.6.6 `Hits` system: projectile sweep, `Health` damage, `Hit` event, projectile destroyed on impact. Test: a projectile hitting a player reduces health exactly once.
- [ ] 1.6.7 `Lifetime` system: despawn after `ttl` frames. Test: a projectile despawns at frame `spawn + ttl`.
- [ ] 1.6.8 `Died` (verified-only) and `Respawn` system with timer and `Rng` spawn point. Test: death at 0 health, respawn after the timer at a spawn point.
- [ ] 1.6.9 `MatchRules` system: warmup, playing, ended phases; score per slot. Test: a kill increments the score; the match ends at the limit.
- [ ] 1.6.10 Crates pushed by characters (`CharacterVirtual` push settings). Test: walking into a crate moves it.
- [ ] 1.6.11 `makeArenaPipeline()` and `ArenaSimulation` factory (frame + assets + pipeline). Test: 600 frames of scripted inputs run headless with a stable golden checksum.
- [x] 1.6.12 (+) The engine checks that a game registered the components it puts on entities itself: `addBody` needs `Transform`, `PhysicsBody` and `BodyDefinition` registered, `addCharacter` needs `CharacterController`, and a game that forgets one loses it from every snapshot without a word. Reason: found in 1.6.1, where the arena registered nothing at all and only a test noticed. Test: the registry answers which names a game registered, and the helpers check the ones they emplace.

### 1.7 Determinism suite and benchmarks
- [ ] 1.7.1 Double-run test over the Arena scripted inputs: per-frame checksums equal.
- [ ] 1.7.2 Golden checksum file `tests/golden/arena_scripted.checksums`; the test compares every 10th frame; the same file is used by Debug and Release.
- [ ] 1.7.3 Snapshot/restore exactness over the Arena at 20 random frames.
- [ ] 1.7.4 Catch2 benchmarks: tick, `takeSnapshot`, `restoreSnapshot`, checksum, 10-frame resimulation; baselines recorded in `tests/benchmarks/baseline.md`.
- [ ] 1.7.5 SSE2 vs AVX2 comparison run; Q2 answered in `DESIGN.md`.

---

## Phase 2 — Rollback session (local)

**Exit criteria**
- [ ] Definition of Done item 1 passes: `unison_runner --players 4 --frames 36000 --latency 120 --jitter 30 --loss 5` exits 0.
- [ ] Q1 (snapshot ring vs two-frame layout) answered in `DESIGN.md` with benchmark numbers.
- [ ] CTest runs runner profiles for 2/4/8 players at 30 and 60 Hz with 0/5/20 % loss.

### 2.1 Inputs and prediction (`unison_session`)
- [ ] 2.1.1 `InputBuffer`: per-frame per-slot inputs with `Confirmed` / `Predicted` state, window bounded below by the verified frame. Test: set/get, eviction below verified, out-of-window rejected.
- [ ] 2.1.2 `RepeatLastInputPredictor`. Test: a missing slot gets its last confirmed input flagged `Predicted`; a slot with no history gets the neutral input.
- [ ] 2.1.3 Local input sampling: `setLocalInput` stored per host frame, sampled once per tick for the local slot. Test: the same input is reused when the host does not update it.

### 2.2 Snapshot ring
- [ ] 2.2.1 `SnapshotRing(capacity)`: `store(frame, snapshot)`, `get(frame)`, overwrite oldest, `evictBelow(frame)`. Test: capacity wraparound and lookups.
- [ ] 2.2.2 Buffer reuse after warm-up, including `restoreSnapshot`, which replaces the registry wholesale today and so throws away the capacity of every pool on each rollback. Test: internal buffer capacities stop growing over 1000 frames.

### 2.3 Session state machine
- [ ] 2.3.1 `SessionConfig` (tick rate, slots, seed, asset hash, pipeline hash, input size, max prediction, build id) and its hash. Test: any field change changes the hash.
- [ ] 2.3.2 `Session::tick()`: simulate `P + 1` with predicted inputs, store the snapshot. Test: `P` advances; the snapshot for `P` exists.
- [ ] 2.3.3 `Session::onConfirmed(frame, inputs)` with a matching prediction advances `V` without rollback. Test: `V` follows; rollback count stays 0.
- [ ] 2.3.4 Rollback on misprediction: restore `F − 1`, resimulate to `P`, overwrite snapshots. Test: the final checksum equals a straight-line simulation with the true inputs.
- [ ] 2.3.5 Prediction window: `tick()` stalls when `P − V >= maxPrediction`. Test: stall flag set, `P` unchanged, resumes after confirmation.
- [ ] 2.3.6 Verified checksums taken from the snapshot at confirmation on `checksumInterval`. Test: the checksum of frame `F` equals a fresh simulation to `F`.
- [ ] 2.3.7 `RollbackStats` (count, max depth, resimulated frames, per-second rates). Test: values after a scripted misprediction.
- [ ] 2.3.8 Optional `inputDelayFrames`. Test: delay 2 applies the input two frames later and reduces rollbacks in a scripted scenario.

### 2.4 Event raise/cancel diffing (`unison_session`, `unison_view`)
- [ ] 2.4.1 The session records event keys per frame above `V`; after resimulation it computes `cancelled` and `raised` sets. Test: an event predicted at `F` and absent after resimulation is cancelled; a new one is raised.
- [ ] 2.4.2 Verified-only events released when `V` passes their frame. Test: not visible before, visible exactly once after.
- [ ] 2.4.3 `EventDispatcher` with typed `on<T>` / `onCancelled<T>` handlers and per-key deduplication. Test: a handler runs once per key even if drained twice.

### 2.5 Transport, loopback, network simulator (`unison_net`)
- [ ] 2.5.1 `ITransport`, `PeerId`, `Channel`, `LoopbackHub` with endpoints. Test: messages delivered between two endpoints in order.
- [ ] 2.5.2 `NetworkSimulator` (seeded): latency, jitter, loss, reordering; the reliable channel never loses or reorders. Test: loss rate within tolerance over 10 000 packets; the reliable channel intact.
- [ ] 2.5.3 Protocol messages (`Hello`, `Welcome`, `Input`, `Confirmed`, `Checksum`, `Desync`, `SnapshotRequest`, `SnapshotChunk`, `Ping`, `Pong`, `Leave`, `Kick`) with writer and reader. Test: round trip of every message; malformed bytes are rejected.

### 2.6 Relay core
- [ ] 2.6.1 `RelayCore`: rooms, slots, `Hello` → `Welcome` with slot and config, spectator role, config hash mismatch → `Kick`. Test: two players get slots 0 and 1; a third with a different config hash is kicked.
- [ ] 2.6.2 Input collection and confirmation when all slots are present; `Confirmed` broadcast. Test: a confirmed frame contains every slot's input.
- [ ] 2.6.3 Input deadline with a simulated clock: a missing slot input after `inputDeadline` is replaced by repeat-last and flagged `Dropped`; late input is ignored. Test: a stalled slot does not block the others.
- [ ] 2.6.4 Redundant inputs (`K = 4`) deduplicated at the relay. Test: dropping one `Input` packet loses nothing.
- [ ] 2.6.5 Checksum comparison per frame across slots and `Desync` with the minority. Test: one deviating client is reported.
- [ ] 2.6.6 Periodic reliable resend of confirmed batches for recovery. Test: a client that lost unreliable packets still receives every confirmed frame.
- [ ] 2.6.7 `Ping` / `Pong` with the relay's current confirmed frame. Test: RTT and relay frame reported.

### 2.7 Networked session (client side)
- [ ] 2.7.1 `NetworkedSession`: sends local inputs, applies `Confirmed`, sends checksums, exposes connection state. Test: two clients over zero-latency loopback stay in sync for 1000 frames.
- [ ] 2.7.2 `TimeSync` controller: target lead `RTT/2 + jitterMargin`, correction by an extra or skipped tick. Test: the lead converges under simulated 100 ms latency and stays within tolerance.
- [ ] 2.7.3 Stall and resume under a 500 ms outage. Test: the session stalls, then catches up without desync.
- [ ] 2.7.4 `SessionRunner::update(hostDelta)` with an accumulator and an injectable clock, returning ticks run and the interpolation alpha. Test: 16.7 ms steps produce one tick each; 50 ms produces three.

### 2.8 Runner tool (`unison_runner`)
- [ ] 2.8.1 cxxopts dependency and CLI: `--players --frames --seed --latency --jitter --loss --tick-rate --checksum-interval --record`. Test: argument parsing.
- [ ] 2.8.2 In-process topology: N `NetworkedSession` + `RelayCore` over `LoopbackHub` + `NetworkSimulator`, seeded scripted inputs. Done when: 2 players, 600 frames, no faults, exit 0.
- [ ] 2.8.3 Per-frame checksum comparison across clients with a failure report (frame, slots, hashes); exit code 2 on desync, 3 on window overflow.
- [ ] 2.8.4 Rollback statistics summary (mean and max depth, rollbacks per second, stalls).
- [ ] 2.8.5 CTest profiles: 2/4/8 players × 30/60 Hz × loss 0/5/20 % with latency 120 ms and jitter 30 ms.
- [ ] 2.8.6 Benchmark snapshot ring vs two-frame layout using Phase 1 numbers; Q1 answered in `DESIGN.md`.

---

## Phase 3 — Real networking

**Exit criteria**
- [ ] Definition of Done item 2 (LAN session, 10 minutes, zero desyncs) executed and recorded in `docs/LAN_TEST.md`.
- [ ] Relay plus two clients over localhost UDP pass in CTest.

### 3.1 ENet transport
- [ ] 3.1.1 `EnetTransport` server: listen, accept, `PeerId` mapping, reliable and unreliable-sequenced channels. Test: the server receives a client's message on localhost.
- [ ] 3.1.2 `EnetTransport` client: connect, send, receive, disconnect. Test: echo round trip on localhost.
- [ ] 3.1.3 Timeouts and disconnect events surfaced through `ITransport`. Test: dropping one side raises a disconnect on the other within the timeout.
- [ ] 3.1.4 Message size policy: unreliable messages capped at an MTU-safe size, reliable ones fragmented by ENet. Test: an oversized unreliable send is rejected; a large reliable message arrives whole.

### 3.2 Relay server executable (`unison_relay`)
- [ ] 3.2.1 Main loop: config (port, room limits, deadlines), `RelayCore` over `EnetTransport`, stdout logging via `LogSink`. Done when: a client connects and gets `Welcome`.
- [ ] 3.2.2 Room lifecycle: created on first `Hello`, destroyed when empty; graceful shutdown on Ctrl+C. Test: room count follows joins and leaves.
- [ ] 3.2.3 Portability check: the relay builds and runs under Linux (WSL, gcc or clang). Optional in v1; record the result here.

### 3.3 Networked session over UDP
- [ ] 3.3.1 Integration test: relay core plus two `NetworkedSession`s over `EnetTransport` on localhost, 1000 frames, equal checksums.
- [ ] 3.3.2 Connection state machine (`Connecting`, `Joining`, `Playing`, `Stalled`, `Disconnected`) with events for the host. Test: transitions on connect, stall, disconnect.
- [ ] 3.3.3 Real-clock pacing in `SessionRunner` with `steady_clock` behind the injectable clock interface. Test: the fake clock drives ticks exactly; the real clock is the default.

### 3.4 Console client (`unison_console`, `arena_view_console`)
- [ ] 3.4.1 Skeleton: `--host --port --name --spectate`, connects, runs the session, prints one status line per second. Done when: two consoles on one machine play through a local relay.
- [ ] 3.4.2 Q3 decided (FTXUI vs plain console); non-blocking keyboard input mapped to `ArenaInput` (WASD, space, Q/E yaw, F fire). Test: key state to input quantisation.
- [ ] 3.4.3 Text top-down renderer: grid, players with facing, projectiles, crates, health. Test: golden string for a known frame.
- [ ] 3.4.4 Stats overlay: `V`, `P`, rollbacks per second, RTT, lead, stall indicator. Done when: visible in the console.
- [ ] 3.4.5 `docs/LAN_TEST.md`: procedure for two machines; Definition of Done item 2 executed and the result recorded.

---

## Phase 4 — Session features

**Exit criteria**
- [ ] Definition of Done items 3–6 pass in the runner and over UDP.

### 4.1 Replays
- [ ] 4.1.1 Replay format (magic, version, `SessionConfig`, asset hash, per-frame confirmed inputs, periodic checksums) and `ReplayWriter`. Test: write then read yields identical records.
- [ ] 4.1.2 `ReplayReader` and `ReplayPlayer` driving a verified-only `Session`. Test: checksums match the recorded ones.
- [ ] 4.1.3 `--record` in runner and console. Test: a runner-recorded replay verifies.
- [ ] 4.1.4 `unison_replay play | verify` CLI with exit codes. Test: `verify` exits 0 on a good file.
- [ ] 4.1.5 Divergence report: first divergent frame on a tampered replay. Test: a tampered input at frame 300 is reported at the first checksum after it.

### 4.2 Snapshot serialisation and state diff
- [ ] 4.2.1 `SnapshotSerializer`: full frame to bytes and back (entity storage, pools in registration order, physics bytes, character states, globals). Test: round trip is checksum-exact and independent of EnTT storage touch order.
- [ ] 4.2.2 `StateDiff`: first differing entity, component and byte offset between two serialised snapshots. Test: a single field change is located.
- [ ] 4.2.3 Field-name reflection `UNISON_FIELDS(...)` so diffs print field names. Test: the diff names the field.
- [ ] 4.2.4 Desync dumps: on `Desync`, clients write `desync_<frame>_<slot>.snapshot`; `unison_replay diff a b` prints the report. Test: a runner with an injected fault produces dumps that diff to the injected field.

### 4.3 Late-join
- [ ] 4.3.1 Relay: `Hello` into a running room selects a donor (lowest RTT) and sends `SnapshotRequest(frame)`. Test: donor chosen, request issued.
- [ ] 4.3.2 Donor client: serialises verified frame `F` and streams `SnapshotChunk`s. Test: chunks reassemble to the serialised snapshot.
- [ ] 4.3.3 Relay input log retention: confirmed inputs since `F` forwarded to the joiner. Test: no gap between the snapshot frame and live frames.
- [ ] 4.3.4 Joiner: restore, fast-forward at up to 8× until inside the prediction window, then play. Test: joiner checksums equal the others from `F` onward.
- [ ] 4.3.5 Runner scenario `--late-join-at <frame>` in CTest.

### 4.4 Reconnect
- [ ] 4.4.1 Reconnect token in `Welcome`; the relay holds the slot for `reconnectGrace` with the drop policy applied. Test: the slot is preserved within grace and released after.
- [ ] 4.4.2 Client reconnect flow reusing the late-join path into the same slot. Test: the reconnecting client resumes with equal checksums.
- [ ] 4.4.3 Runner scenario `--disconnect <slot> <atFrame> <seconds>` in CTest.

### 4.5 Spectators
- [ ] 4.5.1 Spectator role at the relay: no slot, receives `Confirmed`, may request late-join snapshots. Test: a spectator join does not change the slot count.
- [ ] 4.5.2 Spectator session mode: verified-only with optional delay. Test: `P == V` always; checksums equal the players'.
- [ ] 4.5.3 `--spectate` in the console and a runner scenario in CTest.

---

## Phase 5 — Unreal Engine plugin

**Exit criteria**
- [ ] Definition of Done item 7 executed and recorded in `docs/UNREAL.md`.

### 5.1 ThirdParty build and module
- [ ] 5.1.1 `tools/build_unreal_thirdparty.ps1`: builds the deterministic libraries, Jolt and ENet in Release with UE-compatible settings (`/MD`, exceptions and RTTI off, matching toolset) and copies libs and headers into the plugin's `ThirdParty/`. Done when: the script produces the libs from a clean tree.
- [ ] 5.1.2 Jolt configuration macros exported from CMake into a generated `unison_jolt_config.hpp` so every translation unit including Jolt headers sees identical settings. Test: a probe compares the generated values with the built library's.
- [ ] 5.1.3 Host boundary headers `unison/view/*.hpp` include neither Jolt nor EnTT; the view API exposes POD types and non-inline functions only. Test: a probe translation unit compiled with only the boundary headers links.
- [ ] 5.1.4 `Unison.uplugin`, `UnisonRuntime` module, `Unison.Build.cs` linking the ThirdParty libs; the plugin compiles in UE 5.8 (installed; compatibility of Visual Studio 18 / MSVC 14.51 with UE's toolchain confirmed here and recorded in `DESIGN.md` Q5).
- [ ] 5.1.5 Sample project `integrations/unreal/UnisonArena` (C++ project) referencing the plugin. Done when: it builds and opens in the editor.

### 5.2 Runtime integration
- [ ] 5.2.1 `UUnisonSessionSubsystem`: creates the `SessionRunner`, connects to a relay, ticks before physics on the game thread, exposes state and stats to Blueprints. Done when: the subsystem joins a local relay from PIE.
- [ ] 5.2.2 `FpEnvGuard` around every tick on the game thread; a warning is logged if the host changed FTZ/DAZ. Test: UE automation test toggles FTZ and observes the warning.
- [ ] 5.2.3 Coordinate conversion (Unison metres, Y-up, right-handed ↔ UE centimetres, Z-up, left-handed). Test: UE automation round trips for positions and rotations.
- [ ] 5.2.4 `UUnisonInputComponent`: Enhanced Input actions to `ArenaInput` quantisation. Test: UE automation test for quantisation edges.
- [ ] 5.2.5 `UUnisonEventBus`: dynamic multicast delegates for every Arena event and its cancellation. Done when: Blueprint receives `Fired` and its cancellation in a rollback.
- [ ] 5.2.6 Entity views: `UUnisonEntityViewComponent` and a `UDataAsset` map from prefab id to actor class; spawn on `EntityCreated`, despawn on `EntityDestroyed` and on cancellation. Done when: projectiles appear and vanish correctly during rollbacks.
- [ ] 5.2.7 Interpolation: `TransformInterpolator` fed per tick; actors positioned each render frame with the runner's alpha. Done when: motion is smooth at 60/120/144 FPS with 60 Hz simulation.
- [ ] 5.2.8 Debug HUD: `V`, `P`, rollbacks per second, RTT, lead, snapshot cost. Done when: visible in PIE.
- [ ] 5.2.9 Arena content: capsule and box meshes, map, spawn visuals; PIE multi-instance session through a local `unison_relay`. Done when: two PIE instances play together.
- [ ] 5.2.10 `docs/UNREAL.md`: setup, build, run; Definition of Done item 7 executed and recorded.

---

## Phase 6 — Hardening

**Exit criteria**
- [ ] Definition of Done items 8 and 9 pass.
- [ ] The build-and-test section of `CLAUDE.md` holds the real commands.

### 6.1 Performance
- [ ] 6.1.1 Profile tick, snapshot, restore and 10-frame resimulation at 8 players and 200 bodies; recorded in `tests/benchmarks/baseline.md`.
- [ ] 6.1.2 Snapshot cost reduction (dirty-pool skipping, `EStateRecorderState` subsets) if the budget requires it; Q1 closed for good.
- [ ] 6.1.3 Jolt multithreaded stepping determinism test (Q4); adopted only if bit-identical over the golden replay.
- [ ] 6.1.4 Steady-state allocation test: zero heap allocations per tick after warm-up (probe allocator).

### 6.2 Robustness
- [ ] 6.2.1 Fuzz `BinaryReader` and protocol parsing with random and truncated bytes. Test: no crashes, clean rejections.
- [ ] 6.2.2 Relay hardening: inputs for far-future frames, oversized packets, spoofed slots and duplicate `Hello` rejected. Test: each case.
- [ ] 6.2.3 Long-session test: one simulated hour at accelerated speed. Test: ring wraparound and counters behave.

### 6.3 API polish and documentation
- [ ] 6.3.1 Public API review against the boundary rules; anything leaking implementation types removed. Done when: the 5.1.3 probe still links after the review.
- [ ] 6.3.2 `docs/API.md` for the session and view bridge.
- [ ] 6.3.3 `docs/ADDING_SYSTEMS.md`: component + system + test walkthrough.
- [ ] 6.3.4 Definition of Done items 8 and 9 verified; `CLAUDE.md` build section updated; v1 ready for the owner to tag.

---

## Backlog (unscheduled)

From `DESIGN.md` §15: C ABI and Unity/Godot bindings; Linux/macOS/ARM64 determinism matrix and hosted CI;
navigation (Recast for baking, Detour at runtime with deterministic math shims); authoritative-server mode;
DSL/codegen; 2D physics module (Box2D v3); encryption / Steam relay (GameNetworkingSockets); lobbies and
matchmaking; multiple local players per client; delta-compressed inputs for 16+ players; frame-local heap
allocator; asset loading from files.
