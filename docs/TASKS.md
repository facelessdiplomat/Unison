# Unison — Task Board

Progress tracker for everything in `docs/DESIGN.md`: phases → tasks → micro-tasks.
A micro-task is one TDD cycle: one failing test, the code that makes it pass, a refactor, the self-review
from `CLAUDE.md`, and one commit. Work goes strictly in order inside a task.
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

- Next up: **X.5.7**, `ctest -L fast` green in `clang-debug`. Last finished: X.5.6. On the owner's word of
  2026-09-25 Phase X, the port to macOS and play between Windows and macOS, runs before Phase 4, and 3.4.5, the
  LAN run, stays open until X.8.2 plays it between Windows and macOS. 3.2.3 is deferred until WSL is installed.
- Phase 2 finished on 2026-09-23 with 2.8.6: every micro-task and exit criterion ticked.
- 2.7.7 ran between 2.8.5 and 2.8.6 on the owner's request of 2026-09-23, once 2.8.5 showed the clients
  running faster than the host's clock under jitter.
- 2.8.7 to 2.8.10 run before 2.8.5: measuring its profiles showed a 240 ms round trip outrunning a window of
  10 frames at 60 Hz and every lost confirmation stalling a client until the next reliable batch; the owner
  chose on 2026-09-23 to widen the window and to repeat confirmations. The arena also has to tick at the
  profiles' 30 Hz and to hold their eight players.
- Taken ahead of 2.7.1 on the owner's request of 2026-09-23: the splits 2.6.8, 2.3.9 and 1.5.13 of the
  classes nearing or past the 300 lines of `CLAUDE.md` 3, then 6.1.4, which also retired what 2.2.2 left
  behind in restores.
- 1.3.3 runs before 1.2.6: the lifecycle helpers raise events, and `raise` belongs to the event buffer, so the
  board order contradicts the dependency order it asks for. Ids stay as they are.

## Progress

| Phase | Tasks | Micro-tasks | Done |
|-------|-------|-------------|------|
| 0 Bootstrap | 2 | 15 | 15 |
| 1 Deterministic simulation core | 7 | 57 | 57 |
| 2 Rollback session (local) | 8 | 46 | 46 |
| 3 Real networking | 4 | 19 | 17 |
| X Cross-platform: macOS | 10 | 54 | 27 |
| 4 Session features | 5 | 20 | 0 |
| 5 Unreal Engine plugin | 2 | 23 | 0 |
| 6 Hardening | 3 | 12 | 1 |
| **Total** | **41** | **246** | **163** |

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
- There is no free `makeArenaPipeline()`: the systems of a game have to outlive the pipeline that points at
  them, so `ArenaSimulation` owns the systems, the assets and the frame together and puts the pipeline in
  order itself. A free function would have to be handed the systems anyway. Found in 1.6.11.
- `InputFlags::Predicted` is gone: whether an input was guessed is the session's knowledge, kept in
  `InputBuffer`, and systems never see it. A frame played on a guess that proves right is never played again,
  so a system that could tell a guess from the real input would part the clients for good without a rollback
  to notice; without the flag a right guess is byte for byte the confirmed input, and a misprediction is any
  difference at all. The owner's decision. `DESIGN.md` §6.4 updated. Found in 2.1.2.
- `SessionConfig` and its hash live in `unison_net`, not `unison_session`: the relay has to read the config
  (slot count, input size) and `Welcome` carries it, while the network layer sits below the session and cannot
  see its types. The session takes the config from there. `DESIGN.md` §5.2 and §9.2 updated. Found in 2.5.3.
- UE 5.8 confirmed as the plugin target (installed on the development machine); Q5 answered. Development
  toolchain is Visual Studio 18 with VS-bundled CMake/Ninja/clang-format, hence micro-task 0.1.8.
- Time sync keeps clients in pace with the relay's input frontier, which every pong carries, not with its
  confirmed frame: the relay has no clock, so its confirmed frame follows the slowest client and, while a player
  is out, trails everyone else by the relay's deadline, and pacing on it would slow every client down for one
  player's outage. The target of §8.3 is a band around the fastest client. The pong gained a field and the
  protocol moved to version 2. `DESIGN.md` §8.3 and §9.2 updated. Found in 2.7.3.
- Superseded on 2026-09-23 by the owner's choice in 2.7.7: the frontier is the largest of several noisy
  positions, so under jitter it stood ahead of everyone and the clients ran faster than the host's clock. The
  relay now keeps a `MatchClock` started by the first input to arrive, the pong carries the frame it has due
  instead of the frontier, and the protocol moved to version 4. `DESIGN.md` §8.3 and §9.2 updated.
- Two platforms, D35 superseding D12, on the owner's word of 2026-09-25: Windows x64 with MSVC and macOS arm64
  with Apple clang, clients on the two playing one match, the Unreal plugin on both. Definition of Done items 7
  and 8 cover both platforms and item 10 is the mixed run; §7 gained the compiler contract on clang, FPCR beside
  MXCSR, and rules on sort order, type names, scalar widths, NaN and float-to-integer conversion; §15 gained
  Phase X. Measured with Apple clang 21 on 2026-09-25: clang turns `a * b + c` into one `fmadd` unless told
  `-ffp-contract=off`, and `-ffp-model=precise` before that flag fails the build under `-Werror`, so the clang
  contract passes `-fno-fast-math -ffp-contract=off`. Found in X.0.1; plan and record in
  `docs/CROSS_PLATFORM.md`.
- Work rhythm, D36 superseding D33, on the owner's word of 2026-09-25: a micro-task is committed as soon as its
  self-review is green and the next one is taken without waiting, until something only the owner can do or
  decide stands in the way. The two-machine rule of `CLAUDE.md` follows: a commit that Windows builds is
  checked there once the owner has pulled it, and a failure becomes the next micro-task.

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
- [x] Determinism checks 1–3 of `DESIGN.md` §7.4 pass on the Arena sample.
- [x] The golden checksum file for the Arena scripted run is committed and matches in Debug and Release.
- [x] Benchmarks for tick, snapshot, restore and checksum have baselines in `tests/benchmarks/baseline.md`.
- [x] Open question Q2 (SSE2 vs AVX2) is answered in `DESIGN.md` from measured numbers.

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
- [x] 1.4.1 `InputTraits<Input>` (trivially copyable, free of padding, `sizeof <= 64`) and `FrameInputs`, a plain class holding up to 8 slots erased to bytes with per-slot flags `Present / Predicted / Dropped` and a typed accessor. Test: flags and payload round trip; an oversized input fails the trait; reading a slot as the wrong type breaks a contract. `Predicted` was removed in 2.1.2; see the charter amendments.
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
- [x] 1.5.13 (+) `PhysicsWorld` split: the bodies move to `BodyTable`, the sorted queries to `PhysicsQueries`, the shapes both of them build to `jolt_shapes`, and the world keeps the step, the contacts and the state buffer, reaching its parts through `bodies()`, `characters()` and `queries()` as Jolt's `PhysicsSystem` does. Reason: the world stood at 487 lines against the 300 of `CLAUDE.md` 3, and 6.1.4 is about to change how it restores. Test: every physics test passes, reaching bodies and queries through the new parts.

### 1.6 Arena sample simulation (`arena_sim`)
- [x] 1.6.1 Components (`Transform`, `PlayerSlot`, `CharacterState`, `Health`, `Weapon`, `Projectile`, `Lifetime`, `PhysicsBody`, `RespawnTimer`) registered; `ArenaInput{moveX, moveY, yaw, buttons}`. Test: all trivially copyable; `sizeof(ArenaInput) <= 8`.
- [x] 1.6.2 Arena assets in code: floor, walls, ramps, crates, spawn points, player stats, projectile stats. Test: the registry freezes and hashes stably.
- [x] 1.6.3 `ApplyInput` system: input to desired velocity and yaw on `CharacterState`; quantisation contract stated in the header. Test: full-forward input gives max speed along yaw.
- [x] 1.6.4 `CharacterMove` system: drives `CharacterController`, jump with ground check, gravity. Test: a jump leaves the ground and lands within the expected frames.
- [x] 1.6.5 `Weapons` system: fire button spawns a projectile with cooldown and raises `Fired`. Test: holding fire respects the cooldown; one event ordinal per shot.
- [x] 1.6.6 `Hits` system: projectile sweep, `Health` damage, `Hit` event, projectile destroyed on impact. Test: a projectile hitting a player reduces health exactly once.
- [x] 1.6.7 `Lifetime` system: despawn after `ttl` frames. Test: a projectile despawns at frame `spawn + ttl`.
- [x] 1.6.8 `Died` (verified-only) and `Respawn` system with timer and `Rng` spawn point. Test: death at 0 health, respawn after the timer at a spawn point.
- [x] 1.6.9 `MatchRules` system: warmup, playing, ended phases; score per slot. Test: a kill increments the score; the match ends at the limit.
- [x] 1.6.10 Crates pushed by characters (`CharacterVirtual` push settings). Test: walking into a crate moves it.
- [x] 1.6.11 `makeArenaPipeline()` and `ArenaSimulation` factory (frame + assets + pipeline). Test: 600 frames of scripted inputs run headless with a stable golden checksum.
- [x] 1.6.12 (+) The engine checks that a game registered the components it puts on entities itself: `addBody` needs `Transform`, `PhysicsBody` and `BodyDefinition` registered, `addCharacter` needs `CharacterController`, and a game that forgets one loses it from every snapshot without a word. Reason: found in 1.6.1, where the arena registered nothing at all and only a test noticed. Test: the registry answers which names a game registered, and the helpers check the ones they emplace.

### 1.7 Determinism suite and benchmarks
- [x] 1.7.1 Double-run test over the Arena scripted inputs: per-frame checksums equal.
- [x] 1.7.2 Golden checksum file `tests/golden/arena_scripted.checksums`; the test compares every 10th frame; the same file is used by Debug and Release.
- [x] 1.7.3 Snapshot/restore exactness over the Arena at 20 random frames.
- [x] 1.7.4 Catch2 benchmarks: tick, `takeSnapshot`, `restoreSnapshot`, checksum, 10-frame resimulation; baselines recorded in `tests/benchmarks/baseline.md`.
- [x] 1.7.5 SSE2 vs AVX2 comparison run; Q2 answered in `DESIGN.md`.

---

## Phase 2 — Rollback session (local)

**Exit criteria**
- [x] Definition of Done item 1 passes: `unison_runner --players 4 --frames 36000 --latency 120 --jitter 30 --loss 5` exits 0. Passed 2026-09-23 in Release: 36000 frames in 35188 host frames, 17 s, every checksum alike, deepest rollback 20 of 20.
- [x] Q1 (snapshot ring vs two-frame layout) answered in `DESIGN.md` with benchmark numbers.
- [x] CTest runs runner profiles for 2/4/8 players at 30 and 60 Hz with 0/5/20 % loss.

### 2.1 Inputs and prediction (`unison_session`)
- [x] 2.1.1 `InputBuffer`: per-frame per-slot inputs with `Confirmed` / `Predicted` state, window bounded below by the verified frame. Test: set/get, eviction below verified, out-of-window rejected.
- [x] 2.1.2 `RepeatLastInputPredictor`. Test: a missing slot gets its last confirmed input, bytes and flags alike, marked `Predicted` in the buffer and nowhere else; a slot with no history gets the neutral input.
- [x] 2.1.3 Local input sampling: `setLocalInput` stored per host frame, sampled once per tick for the local slot. Test: the same input is reused when the host does not update it.

### 2.2 Snapshot ring
- [x] 2.2.1 `SnapshotRing(capacity)`: `store(frame)`, `holds(frame)`, `snapshotAt(frame)`, overwrite oldest, `evictBelow(frame)`. The ring takes the snapshot into a place it owns rather than being handed one: a snapshot holds a registry, which only moves, and 2.2.2 reuses those places. Test: capacity wraparound and lookups.
- [x] 2.2.2 Buffer reuse after warm-up, including `restoreSnapshot`, which replaces the registry wholesale today and so throws away the capacity of every pool on each rollback. Test: a match rolled back through the ring never gives room back, and a second pass over the same 600 frames needs no more room than the first; a fixed frame count for the warm-up would have measured when the script first kills a player (frame 576) rather than reuse.

### 2.3 Session state machine
- [x] 2.3.1 `SessionConfig` (tick rate, slots, seed, asset hash, pipeline hash, input size, max prediction, build id) and its hash. Test: any field change changes the hash.
- [x] 2.3.2 `Session::tick()`: simulate `P + 1` with predicted inputs, store the snapshot. Test: `P` advances; the snapshot for `P` exists.
- [x] 2.3.3 `Session::confirm(frame, inputs)` with a matching prediction advances `V` without rollback. Test: `V` follows; no frame is played twice (the rollback count arrives with `RollbackStats` in 2.3.7); a frame confirmed before it is played is played on the confirmed inputs; the window follows `V`.
- [x] 2.3.4 Rollback on misprediction: restore `F − 1`, resimulate to `P`, overwrite snapshots. Test: the final checksum equals a straight-line simulation with the true inputs, for a test pipeline and for the scripted arena match with its physics.
- [x] 2.3.5 Prediction window: `tick()` stalls when `P − V >= maxPrediction`, unless the next frame is settled and would be verified at once, which keeps `maxPrediction = 0` usable as lockstep. Test: stall flag set, `P` unchanged, resumes after confirmation; a stalled tick still rolls back; lockstep plays only confirmed frames.
- [x] 2.3.6 Verified checksums taken from the snapshot at confirmation on `checksumInterval`, which joins `SessionConfig` so every client checksums the same frames. Test: the checksum of frame `F` equals a fresh simulation to `F`; only frames on the interval are checksummed.
- [x] 2.3.7 `RollbackStats` (count, max depth, resimulated frames, stalled ticks, frames played, per-second rates over the time played). Test: values after a scripted misprediction.
- [x] 2.3.8 Optional `inputDelayFrames`, a choice of each client rather than part of `SessionConfig`. Test: delay 2 applies the input two frames later and, against a relay that answers in two ticks, removes every rollback in a scripted scenario.
- [x] 2.3.9 (+) `Session` split before the networked session, spectators and late-join grow it: which input every slot of every frame in the window plays, the local player's own, the relay's or a guess, and whether a confirmation matches what a frame holds, move to `InputTimeline`; the session keeps the frame numbers, the ring and the events. Reason: the session stood at 265 lines against the 300 of `CLAUDE.md` 3. Test: the timeline answers on its own, its contracts included, and every session test passes unchanged.

### 2.4 Event raise/cancel diffing (`unison_session`, `unison_view`)
- [x] 2.4.1 The session records event keys per frame above `V`; after resimulation it computes `cancelled` and `raised` sets. Test: an event predicted at `F` and absent after resimulation is cancelled; a new one is raised.
- [x] 2.4.2 Verified-only events released when `V` passes their frame. Test: not visible before, visible exactly once after.
- [x] 2.4.3 `EventDispatcher` with typed `on<T>` / `onCancelled<T>` handlers and per-key deduplication. Test: a handler runs once per key even if drained twice.
- [x] 2.4.4 (+) Pending event changes net out: two rollbacks between two drains can raise a key and then cancel it, or cancel it and then raise it again, and the two lists alone cannot tell the view which came first. A key raised and then cancelled before the view takes the changes is never shown; one cancelled and then raised again stays shown. Found in 2.4.3. Test: the raised and cancelled keys of a batch never overlap, and each order ends as it should.

### 2.5 Transport, loopback, network simulator (`unison_net`)
- [x] 2.5.1 `ITransport`, `PeerId`, `Channel`, `LoopbackHub` with endpoints. Test: messages delivered between two endpoints in order.
- [x] 2.5.2 `NetworkSimulator` (seeded): latency, jitter, loss, reordering; the reliable channel never loses or reorders. Test: loss rate within tolerance over 10 000 packets; the reliable channel intact.
- [x] 2.5.3 Protocol messages (`Hello`, `Welcome`, `Input`, `Confirmed`, `Checksum`, `Desync`, `SnapshotRequest`, `SnapshotChunk`, `Ping`, `Pong`, `Leave`, `Kick`) with writer and reader. Test: round trip of every message; malformed bytes are rejected.

### 2.6 Relay core
- [x] 2.6.1 `RelayCore`: one room per relay with the config it is created with (rooms that come and go are 3.2.2), slots, `Hello` → `Welcome` with slot and config, spectator role, config hash mismatch → `Kick`. Test: two players get slots 0 and 1; a third with a different config hash is kicked.
- [x] 2.6.2 Input collection and confirmation when all slots are present; `Confirmed` broadcast. Test: a confirmed frame contains every slot's input.
- [x] 2.6.3 Input deadline with a simulated clock: a missing slot input after `inputDeadline` is replaced by repeat-last and flagged `Dropped`; late input is ignored. Test: a stalled slot does not block the others.
- [x] 2.6.4 Redundant inputs (`K = 4`) deduplicated at the relay. Test: dropping one `Input` packet loses nothing. The rule that the first input a slot sends for a frame stands (2.6.2) already does the deduplication; the test pins it.
- [x] 2.6.5 Checksum comparison per frame across slots and `Desync` with the minority. Test: one deviating client is reported.
- [x] 2.6.6 Periodic reliable resend of confirmed batches for recovery. Test: a client that lost unreliable packets still receives every confirmed frame.
- [x] 2.6.7 `Ping` / `Pong` with the relay's current confirmed frame. Test: RTT and relay frame reported.
- [x] 2.6.8 (+) `RelayCore` split before late-join, reconnect and spectators grow it: who is in the match and which slot each plays moves to `Roster`, writing a message for the wire and handing it to the transport to `Outbox`, which the client side can send through as well, and the eight slots every mask of slots allows become `kMaxSlots`. Reason: the relay stood at 234 lines against the 300 of `CLAUDE.md` 3. Test: the roster and the outbox answer on their own, a roster of more slots than a mask has bits breaks a contract, and every relay test passes unchanged.

### 2.7 Networked session (client side)
- [x] 2.7.1 `NetworkedSession`: sends local inputs, applies `Confirmed`, sends checksums, exposes connection state. Test: two clients over zero-latency loopback stay in sync for 1000 frames.
- [x] 2.7.2 `TimeSync` controller: target lead `RTT/2 + jitterMargin`, correction by an extra or skipped tick. Test: the lead converges under simulated 100 ms latency and stays within tolerance. The relay follows the slowest client, so the target is a band of `jitterMargin` around `RTT/2` rather than a lead beyond it (`DESIGN.md` §8.3); the networked session pings every 100 ms, feeds the pongs to its `TimeSync` and hands the host the correction, and its `tick()` no longer polls, `update(now)` does. 2.7.5 moved the band from the confirmed frame to the relay's input frontier.
- [x] 2.7.3 Stall and resume under a 500 ms outage. Test: the session stalls, then catches up without desync. The other client plays on at full pace throughout; it needed 2.7.5 and 2.7.6 first.
- [x] 2.7.4 `SessionRunner::update(hostDelta)` with an accumulator and an injectable clock, returning ticks run and the interpolation alpha. It also lets the event dispatcher forget the keys of frames below `V`, which can no longer be cancelled, so the set of shown keys stops growing. Test: 16.7 ms steps produce one tick each; 50 ms produces three.
- [x] 2.7.5 (+) The relay tells in every pong the newest frame any player has sent an input for, and `TimeSync` keeps pace with that frontier, where the fastest client stands, instead of with the confirmed frame, which follows the slowest one and, while a player is out, trails everyone else by the relay's deadline: pacing on it would have slowed every other client to a third of its speed through an outage, against the promise of 2.6.3 that a stalled slot blocks nobody. The pong gains a field, so the protocol moves to version 2. Found in 2.7.3. Test: a pong carries the input frontier, and frames the relay confirms late for a player who is out slow nobody down.
- [x] 2.7.6 (+) The client's input window reaches past its prediction window, so a client that stalled through an outage keeps every confirmation arriving meanwhile and plays through them to catch up, rather than turning away frames beyond its window that the relay resends only once. Found in 2.7.3. Test: a session keeps a confirmation far beyond the frames it has played and verifies the frame once it has played up to it.
- [x] 2.7.7 (+) The relay keeps a `MatchClock`, started by the first input to arrive, and every pong carries the frame it has due instead of the input frontier; `TimeSync` keeps half a round trip ahead of it, correcting both ways, and leaves out pongs whose pings went out before a correction had run its course. The frontier is the largest of several noisy positions, so under jitter it stood ahead of everyone and the clients ran faster than the host's clock: 2.7 % at 30 ms of jitter for four players, 5.9 % at 60 ms. The pong changes, so the protocol moves to version 4. Found in 2.8.5; the owner chose the relay's clock on 2026-09-23 over pacing on each client's own inputs, which the relay stops receiving while a client plays settled frames. Test: the clock falls due from the first input at exactly the tick rate, a pong carries its due frame, a client ahead or behind it corrects by the frames it is off, and four players at 60 ms of jitter verify 1200 frames in no fewer host frames and less than a second more.

### 2.8 Runner tool (`unison_runner`)
- [x] 2.8.1 cxxopts dependency and CLI: `--players --frames --seed --latency --jitter --loss --tick-rate --checksum-interval --record`. Test: argument parsing.
- [x] 2.8.2 In-process topology: N `NetworkedSession` + `RelayCore` over `LoopbackHub` + `NetworkSimulator`, seeded scripted inputs. Done when: 2 players, 600 frames, no faults, exit 0.
- [x] 2.8.3 Per-frame checksum comparison across clients with a failure report (frame, slots, hashes); exit code 2 on desync, 3 on window overflow. A wiretap in front of the relay writes the clients' checksums into a ledger, and the run lasts until every client has reported the last frame checked. Test: the ledger finds the first frame the clients part ways at, the wiretap files each checksum under its sender and hands every message on, the exit code ranks a desync over an overflow over missed frames, and a clean run compares every frame it verified and finds them alike.
- [x] 2.8.4 Rollback statistics summary (mean and max depth, rollbacks per second, stalls). Every report closes with a table by slot and a row for all clients together. Test: the mean depth is the frames played again per rollback, the rows give every slot's figures in slot order, the last row adds the counts up and keeps the deepest, and every client of a run reports the frames it played and the rollbacks it took.
- [x] 2.8.7 (+) A prediction window of 20 frames by default, 333 ms at 60 Hz, instead of 10: a one-way latency of 120 ms makes a round trip of 240 ms, and with the narrower window four clients stalled for nearly half of a run. Found in 2.8.5. Test: the default config allows 20 frames, and four players over a 240 ms round trip at 60 Hz verify 300 frames in fewer than 360 host frames.
- [x] 2.8.8 (+) Every confirmation also carries the frames just before it, as every client input already does, so a lost confirmation costs a client one frame rather than the wait for the next reliable batch; the protocol moves to version 3. Found in 2.8.5. Test: the codec round-trips a batch, a client settles a frame whose own confirmation was lost from the next one, and four players at 20 % loss keep pace with the host.
- [x] 2.8.9 (+) The arena ticks at the rate its session plays instead of always at 60 Hz, which the 30 Hz profiles need. Found in 2.8.5. Test: an arena made for 30 Hz steps its frame by a thirtieth of a second.
- [x] 2.8.10 (+) The arena holds eight players, with four more spawn points halfway along its walls, which the eight-player profiles need. Found in 2.8.5. Test: eight players start at eight different spawn points.
- [x] 2.8.5 CTest profiles: 2/4/8 players × 30/60 Hz × loss 0/5/20 % with latency 120 ms and jitter 30 ms. Five seconds of play each, labelled `profile`; `tools/ci.ps1` runs CTest on half the logical processors so the eight-player profiles cost Debug about fifteen seconds rather than two minutes. Test: all eighteen exit 0 in Debug and Release.
- [x] 2.8.6 Benchmark snapshot ring vs two-frame layout using Phase 1 numbers; Q1 answered in `DESIGN.md`. The cost of a second of play for one client, from the baseline after 6.1.4 and the rollbacks of the Definition of Done's run: the ring is 13 % cheaper while every guess is right and 32 % dearer under 26 rollbacks a second 16 frames deep, at most 1.4 ms a second either way, so the ring stays. Check: `DESIGN.md` §8.2 and Q1.

---

## Phase 3 — Real networking

**Exit criteria**
- [ ] Definition of Done item 2 (LAN session, 10 minutes, zero desyncs) executed and recorded in `docs/LAN_TEST.md`.
- [x] Relay plus two clients over localhost UDP pass in CTest.

### 3.1 ENet transport
- [x] 3.1.1 `EnetTransport` server: listen, accept, `PeerId` mapping, reliable and unreliable-sequenced channels. Test: the server receives a client's message on localhost. Also: an address taken or no address at all fails with `NetworkUnavailable`, the server answers a client by the peer id it gave it, and two clients go by different ids.
- [x] 3.1.2 `EnetTransport` client: connect, send, receive, disconnect. Test: echo round trip on localhost. Also: what is sent before the connection is up goes once it is, in order; an address that is none fails; a transport that goes away says goodbye, so the other side hears of it at once.
- [x] 3.1.3 Timeouts and disconnect events surfaced through `ITransport`. Test: dropping one side raises a disconnect on the other within the timeout. A receiver hears `peerLeft(peer)` on the poll after a peer said goodbye or stayed silent past the peer timeout, five seconds by default; the runner's wiretap hands it on to the relay. Also: a goodbye is reported at once, and a client hears of its server going away.
- [x] 3.1.4 Message size policy: unreliable messages capped at an MTU-safe size, reliable ones fragmented by ENet. Test: an oversized unreliable send is rejected; a large reliable message arrives whole. The cap, `kMaxUnreliableMessageSize` (1200 bytes), is part of the `ITransport` contract, so the loopback endpoint enforces it as ENet's transport does.

### 3.2 Relay server executable (`unison_relay`)
- [x] 3.2.4 (+) A `Hello` carries the whole config the client would play instead of its hash, and the relay hashes it itself, so a standalone relay, which never simulates and cannot know a game's asset and pipeline hashes, can open a room from it. The protocol moves to version 5. Found in 3.2.1; the owner chose it on 2026-09-23 over giving the relay the config on its command line, and one room per config over named rooms. Test: a hello survives the wire with the whole config, and every relay and session test joins with it.
- [x] 3.2.1 Main loop: config (port, room limits, deadlines), `RelayCore` over `EnetTransport`, stdout logging via `LogSink`. Done when: a client connects and gets `Welcome`. `RelayRooms` opens a room from the first hello of every config and passes the room its peers' messages; a `SteadyClock` gives the relay its time. Test: rooms open, join and answer as they should over loopback, a client connecting over ENet is welcomed, and `unison_relay --bind 127.0.0.1 --port 0 --run-for 1` prints the address it listens on and exits 0.
- [x] 3.2.2 Room lifecycle: created on first `Hello`, destroyed when empty; graceful shutdown on Ctrl+C. Test: room count follows joins and leaves. A peer that has gone is taken out of its room's roster, which frees its slot and stops the relay waiting for its inputs; Ctrl+C and `SIGTERM` set the stop flag of `DESIGN.md` §5, the relay's one piece of global state. Also: a player who left no longer holds up the others' frames, and a peer that leaves without a hello changes nothing. Ctrl+C itself is checked by hand.
- [ ] ~~3.2.3 Portability check: the relay builds and runs under Linux (WSL, gcc or clang). Optional in v1; record the result here.~~ Deferred on 2026-09-23: WSL is not installed on the development machine, and installing it is the owner's decision. The relay depends only on the standard library, ENet and cxxopts, all of which build on Linux; what is left untried is our own code under gcc or clang.

### 3.3 Networked session over UDP
- [x] 3.3.1 Integration test: relay core plus two `NetworkedSession`s over `EnetTransport` on localhost, 1000 frames, equal checksums. The relay side is `RelayRooms`, as in `unison_relay`, and the clients play the arena on its scripted inputs; the clock is a manual one, so the thousand frames take seconds in Debug rather than seventeen.
- [x] 3.3.2 Connection state machine (`Connecting`, `Joining`, `Playing`, `Stalled`, `Disconnected`) with events for the host. Test: transitions on connect, stall, disconnect. A transport now reports a peer whose connection comes up as well as one that has gone, the runner's wiretap and the tests' tap hand both on, and the session keeps every state it moves into until the host clears them.
- [x] 3.3.3 Real-clock pacing in `SessionRunner` with `steady_clock` behind the injectable clock interface. Test: the fake clock drives ticks exactly; the real clock is the default. `update()` lets pass what the runner's clock has counted since the last update, and a runner made without a clock owns a `SteadyClock` (3.2.1).

### 3.4 Console client (`unison_console`, `arena_view_console`)
- [x] 3.4.1 Skeleton: `--host --port --name --spectate`, connects, runs the session, prints one status line per second. Done when: two consoles on one machine play through a local relay. `--spectate` waits for spectators in 4.5.3; `--players`, `--from` and `--run-for` joined the options, and a player stands still until 3.4.2 brings the keyboard. Done on 2026-09-23: a relay on 127.0.0.1:7777 and two consoles with `--from 127.0.0.1` played slots 0 and 1, sixty verified frames a second, round trips of 1 to 4 ms.
- [x] 3.4.2 Q3 decided (FTXUI vs plain console); non-blocking keyboard input mapped to `ArenaInput` (WASD, space, Q/E yaw, F fire). Test: key state to input quantisation. Plain console, the owner's choice of 2026-09-23: Windows' console input reports keys going up as well as down, which no terminal does. `ArenaControls` turns the aim by the time a key was held, and a lost focus lets every key go; reading the keyboard itself is checked by hand.
- [x] 3.4.3 Text top-down renderer: grid, players with facing, projectiles, crates, health. Test: golden string for a known frame. `arena_view_console` draws the arena from above, half a metre a column and a metre a row, with a line for every player below it; the console shows it from 3.4.4 on. It tells which way a player looks from the yaw by quarter turns itself rather than calling the arena's inline `facingOf`, so no simulation code is compiled under a host's flags (`DESIGN.md` §7.3). Also: walls fence the map, a crate and a shot show where they are, every quarter turn has its arrow, and a player waiting to come back is off the map.
- [x] 3.4.6 (+) Every spawn point faces the middle of the arena, as `DESIGN.md` §14 says. The yaws were written as if a yaw turned from +X towards +Z, while `facingOf` reads one from +Z towards +X, so six of the eight spawns look past the middle or away from it in the frame a player appears; 3.4.3's map showed it. Test: `facingOf` of every spawn's yaw points from the spawn at the middle. The scripted match's golden checksums stay as they were, since the next input's yaw takes over from a spawn's; the map's golden shows player 1 looking at the middle. Starting a host's aim where the spawn faces went to the backlog of `DESIGN.md` §15.
- [x] 3.4.4 Stats overlay: `V`, `P`, rollbacks per second, RTT, lead, stall indicator. Done when: visible in the console. The console draws its screen over the last one ten times a second: the status line, now with the lead of `TimeSync`, above the map of 3.4.3, "stalled" standing for the stall indicator; where its output is no console it prints the status line once a second as before. The program's name moved from the status line to the log line, so the line fits a window 120 columns wide. Test: the lead, the status line, the screen's layout and its escape sequences. Done on 2026-09-23: a console in a hidden window of its own, read back through its console buffer, showed the status line, the map and the players' lines, the cursor at the end of the last line.
- [x] 3.4.7 (+) The relay and the console ask Windows for a timer of a millisecond. A thread that sleeps waits at least one tick of Windows' timer, 15.6 ms unless its process asks for less, so both loops, which sleep a millisecond a round, ran at 64 Hz: a message waited up to 16 ms at either end, and 3.4.4's screen showed round trips of 16 to 32 ms on localhost and leads of −8 to −16 ms. Done when: a relay and two consoles on localhost show round trips of a few milliseconds. `MillisecondTimer` in `unison_net` holds the timer for as long as it lives. Test: twenty sleeps of a millisecond take less than 100 ms while one lives; they took 306 ms without it and 31 ms with it. Done on 2026-09-23: a relay and two consoles on localhost showed round trips of 1 to 4 ms for six seconds, the verified frame one behind the predicted one.
- [x] 3.4.8 (+) The console tells of a desync. The relay broadcasts one when the clients' checksums part ways, but the console showed nothing of it and played on, so the zero desyncs of Definition of Done item 2 could not be read off it. A reported desync now ends the console with exit code 2, outranking a disconnect's 1, and its status line ends with the frame and the slots out of step. Test: the status line of a desync of one slot and of several, also after a disconnect, and the exit code of every ending.
- [ ] 3.4.5 `docs/LAN_TEST.md`: procedure for two machines; Definition of Done item 2 executed and the result recorded. The procedure was written on 2026-09-23; the run on two machines is the owner's, and its result goes into the record of `docs/LAN_TEST.md`. On the owner's word of 2026-09-25 the run is X.8.2's, between Windows and macOS, and 3.4.5 is ticked from it.

---

## Phase X — Cross-platform: macOS

Plan, risks R1 to R11, decisions Q-A to Q-I and the record of the runs: `docs/CROSS_PLATFORM.md`.

**Exit criteria**
- [ ] `tools/ci.sh` prints `ci: ok` on the Mac and `tools\ci.ps1` prints `ci: ok` on Windows, on the same commit: both configurations build with warnings as errors, pass `ctest` and pass the formatting check on both platforms.
- [ ] Every golden recorded on Windows verifies on macOS in Debug and in Release (X.6).
- [ ] The consoles' half of Definition of Done item 10: a Windows client and a macOS client play ten minutes through a relay on Windows, then through one on the Mac, with zero desyncs, recorded in `docs/LAN_TEST.md` (X.8).
- [ ] The charter, the board, `README.md` and `CLAUDE.md` describe both platforms and the two-platform rule.

### X.0 Charter and board
- [x] X.0.1 Amend `docs/DESIGN.md` for two platforms: D35 in place of D12; Definition of Done items 7 and 8 on both platforms and a new item 10 for the mixed run; the compiler contract on clang (§7.1), FPCR beside MXCSR (§7.2), rules on order, type names, scalar widths, NaN and float-to-integer conversion (§7.3), goldens across the platforms and the mixed run (§7.4); the Mac in §1, §5.2, §10.2, §10.3, §11, §12, §15, §16 and §17 (Q8 to Q10). Done when: the charter reads consistently and the board's amendments list records the change. Measured on the way with Apple clang 21: `-ffp-model=precise` followed by `-ffp-contract=off` fails under `-Werror`, so the clang contract is `-fno-fast-math -ffp-contract=off`.
- [x] X.0.2 Register Phase X on this board: the micro-tasks planned in `docs/CROSS_PLATFORM.md` after Phase 3, its Phase 5 additions under their own ids (5.1.6 to 5.1.10, 5.2.11 to 5.2.13), a progress row, and **Now** pointing at X.1.1 with the owner's word of 2026-09-25: Phase X runs before Phase 4, and 3.4.5 stays open until X.8.2 closes it (Q-I). `CLAUDE.md` gains the two-platform rule of Q-E in its workflow section. Done when: the board, `CLAUDE.md` and the plan agree, the plan keeping its analysis, decisions and record and pointing at this board for the micro-tasks.

### X.1 Toolchain and repository hygiene
- [x] X.1.1 Tools on the Mac: `brew install ninja llvm@<major>`, the major being the VS-bundled clang-format's, which the owner reads on Windows with `& $env:UNISON_CLANG_FORMAT --version` and records in `CLAUDE.md`'s environment section together with the Mac's toolchain of `docs/CROSS_PLATFORM.md` §2.1. `tools/env.sh` exports `UNISON_CLANG_FORMAT` (and nothing else: CMake and Ninja are on `PATH` from Homebrew). Done when: `source tools/env.sh` then `"$UNISON_CLANG_FORMAT" --version` prints the recorded major. Done on 2026-09-25: the major is 20, since Visual Studio 2026 bundles LLVM 20.1.8; Ninja 1.13.2 and `llvm@20` come from Homebrew, and `tools/env.sh` also refuses to go on without `c++`, `cmake`, `ninja` and `brew` on `PATH`. clang-format 20 and 23 both accept all 329 tracked sources as they stand.
- [x] X.1.2 `.gitattributes` with `* text=auto eol=lf`, `.gitignore` with `.DS_Store`; the renormalising commit is the owner's call (Q-F). Done when: `git ls-files --eol` shows `i/lf` for every tracked text file on both machines. Done on 2026-09-25: the index already held every text file with LF, so `git add --renormalize .` changed nothing and no renormalising commit is needed. The one other entry in the listing is the empty `sim/include/unison/sim/.gitkeep`, which has no line to end and reads `i/none`.
- [x] X.1.3 Formatting parity: the Mac's clang-format passes `--dry-run --Werror` over the tracked sources exactly as Windows' does. Done when: the check passes on the tree at HEAD without a single change; any file the two versions disagree on is reported to the owner before anything is touched. Done on 2026-09-25: clang-format 20.1.8 from `tools/env.sh` passes all 329 tracked sources at HEAD with the flags of `tools/ci.ps1`, and fails with exit code 1 on a copy of `core/src/contract.cpp` spaced wrongly on purpose; no file needed a change.

### X.2 The compiler contract on clang
- [x] X.2.1 `unison_apply_language_subset` on clang: `-fno-exceptions -fno-rtti` (no `_HAS_EXCEPTIONS`, which is the MSVC STL's switch). Test: a probe target built with the function carries both flags in `compile_commands.json`; `throw` in a probe source fails to compile. Done on 2026-09-25: the CTest case `language_subset` configures a probe project of its own, as `determinism_guard` does, and reads the flags off the probe target rather than `compile_commands.json`, which X.2.7 walks for every module; on clang it also finds `throw` and `typeid` compiling without the flags and rejected under them for the reason clang gives. It fails on a module that forgets `-fno-rtti` and on one that turns both back on after them. Until the main project configures on the Mac in X.3, the probe runs there on its own: `cmake -S tests/cmake/language_subset -B build/language_subset -G Ninja`.
- [x] X.2.2 `unison_apply_warnings` on clang: the set of Q-D. Test: a deliberate shadowed variable in a probe fails the build. Done on 2026-09-25: clang gets `-Wall -Wextra -Wpedantic -Wshadow -Werror`. The CTest case `warnings` checks the flags each compiler gets and that a shadowed local compiles without them and fails under them, through `-Wshadow` on clang and C4456 on MSVC; the helpers it shares with `language_subset` live in `tests/cmake/probe_expectations.cmake`. It fails on a module without `-Wshadow` and on one that turns it off again. `-Wconversion` and `-Wsign-conversion` are tried in X.5 (Q-D).
- [x] X.2.3 `unison_apply_determinism` on clang: `-fno-fast-math -ffp-contract=off -fexcess-precision=standard` and `-include` of `determinism_guard.hpp`, after every flag a toolchain puts before them; not `-ffp-model=precise`, which followed by `-ffp-contract=off` trips `-Woverriding-option` under `-Werror` (R1). `UNISON_INSTRUCTION_SET` gains `NEON`, the only value on arm64 and its default there, while `SSE2` and `AVX2` stay x86-64 values (SSE2 is clang's x86-64 baseline and adds no flag; `AVX2` adds `-mavx2` and never `-mfma`); Jolt's `USE_SSE*`/`USE_AVX*` options are passed on x86-64 only. Test: the flags appear on the probe's command line; `-DUNISON_INSTRUCTION_SET=SSE2` on arm64 fails at configure with a message naming the architecture. Done on 2026-09-25: the module reads the architecture off `CMAKE_SYSTEM_PROCESSOR` and allows NEON on arm64 and SSE2 or AVX2 on x86-64, the first of them by default. `determinism_flags` reads the probe's command line by the compiler the probe names in `compiler_id.txt`, and the CTest case `determinism_refuses_a_foreign_instruction_set` configures the probe with the other architecture's instruction set and expects the refusal that names this one. They fail on a module without `-ffp-contract=off`, on one without the guard and on one that lets SSE2 onto arm64. Passing Jolt's `USE_SSE*` and `USE_AVX*` on x86-64 only moved to X.3.3, since Jolt ignores them on arm64. The guard itself refuses clang until X.2.4.
- [x] X.2.4 `determinism_guard.hpp` on clang: rejects `__FAST_MATH__` and `__FINITE_MATH_ONLY__`, turns contraction off with `#pragma STDC FP_CONTRACT OFF`, and rejects any other compiler with a message. The MSVC branch stays as it is. Done when: `guard_probe.cpp` compiles under the flags of X.2.3 and fails under `-ffast-math` with the guard's own message. Done on 2026-09-25: the MSVC branch reads as before; the clang branch refuses `__FAST_MATH__` and a `__FINITE_MATH_ONLY__` of 1, which clang always defines, 0 or 1, and turns contraction off by pragma; any other compiler is refused. `determinism_guard` expects per compiler now: on clang it accepts the contract of X.2.3, rejects `-ffast-math` with the guard's own message, and runs a probe that fuses a multiply and an add under `-ffp-contract=on` without the guard and rounds twice with it. It fails on a guard without the pragma and on one without the fast-math check.
- [x] X.2.5 `tests/cmake/determinism_guard` on clang: `-ffast-math`, `-ffinite-math-only` and `-ffp-model=aggressive` are rejected; the contract of X.2.3 is accepted. Recorded difference from MSVC: no flag at all is accepted on clang, because clang defines no macro for its default model; `-ffp-contract=fast` and `-ffp-model=fast` cannot be caught by a header either, since they define no macro and ignore the pragma (R1). Those are caught by X.2.6 and X.2.8 instead. Test: the CTest case `determinism_guard` passes on the Mac. Done on 2026-09-25: the test also sees the guard refuse `-ffinite-math-only`, `-ffp-model=aggressive` and `-Ofast`, which clang deprecates but still takes, and accept no flag at all, since clang's default model is value-safe but for contraction, which the pragma turns off; its messages name an empty flag set. It fails on a guard that watches `__FAST_MATH__` alone. `-ffp-contract=fast` and `-ffp-model=fast` stay out of a header's sight, as `DESIGN.md` §7.1 records, and are left to X.2.6 and X.2.8.
- [x] X.2.6 `tests/cmake/determinism_flags` per compiler: required on clang `-fno-fast-math`, `-ffp-contract=off`, `-fno-exceptions`, `-fno-rtti`, the `-include` of the guard; forbidden `-ffast-math`, `-Ofast`, `-ffp-model=fast`, `-ffp-model=aggressive`, `-funsafe-math-optimizations`, `-fassociative-math`, `-freciprocal-math`, `-ffp-contract=on`, `-ffp-contract=fast`, `-fexceptions`, `-frtti`, `-mfma`, `-march=native`. Test: the CTest case passes on the Mac and still on Windows. Done on 2026-09-25: the forbidden list also holds `-ffinite-math-only`, `-fno-signed-zeros`, `-fno-honor-nans`, `-fno-honor-infinities` and `-fapprox-func`, which change values as the others do, and `DESIGN.md` §7.1 lists the same. A flag is forbidden anywhere on the line, even where ours come later and take it back. The check fails with `-ffast-math`, `-ffp-contract=fast`, `-fno-signed-zeros`, `-march=native` or `-frtti` put into `CMAKE_CXX_FLAGS`. `-mfma` cannot reach a line on arm64, where clang refuses it, and stays forbidden for x86-64.
- [x] X.2.7 `tests/cmake/module_determinism` per compiler: the same walk over `compile_commands.json` with clang spellings, Jolt included (`-ffp-contract=off` present, `-mfma` and `-ffast-math` absent, no `-fexceptions`); the test executables carry no `-fno-exceptions`. Test: the CTest case passes on the Mac and still on Windows. Done on 2026-09-25: the walk takes the main build's compiler from `tests/CMakeLists.txt` and reads every group with that compiler's spellings; the MSVC checks are the ones it held before, and on both compilers a plain module may no longer carry the guard. The forbidden list lives in `tests/cmake/determinism_contract.cmake`, shared with `determinism_flags`. The main project has configured on the Mac by hand since X.2.3, and the walk passes over its compile commands; it fails on copies of them with `-ffp-contract=off` taken from core or Jolt, `-ffast-math` given to the arena, `-Wshadow` taken from net, `-fno-fast-math` given to view and `-fno-exceptions` given to a test.
- [x] X.2.8 A behavioural canary: a non-inline `multiplyThenAdd(float, float, float)` and its `double` twin in a test-side library built under `unison_apply_determinism`, as `unison_core_header_check` is, so no test-only code enters the engine. Test: `"a multiply followed by an add rounds twice"`: with `a = b = 1 + 2^-23` and `c = -(1 + 2^-22)` the result is `0`, where a fused evaluation gives `2^-46`, and likewise in `double` with `2^-52` and `2^-51`, where fusing gives `2^-104`. It runs on both platforms and guards against a future compiler default as much as against a wrong flag. Done on 2026-09-25: `unison_determinism_canary` is a static library of `tests/support/multiply_then_add.cpp` under `unison_apply_determinism` and the warnings, with two Catch2 cases in `unison_tests_fast`, one per precision, and the module walk holds the canary to the determinism flags as well. On the Mac the canary builds and returns 0 in both precisions, while the same function under clang's defaults returns 2^-46 and 2^-104. The Catch2 cases run on Windows now and on the Mac once X.5 builds the test executable.
- [x] X.2.9 `CMakePresets.json`: `clang-base` (hidden, host `Darwin`, Ninja, `cc`/`c++`, compile commands), `clang-debug`, `clang-release`; build, test and workflow presets for all four configurations (Q-B), tests with output on failure and parallelism from `CTEST_PARALLEL_LEVEL`. Done when: `cmake --preset clang-debug` configures on the Mac and fetches every dependency; `cmake --list-presets` on Windows still shows only the `msvc-*` configure presets. Done on 2026-09-25: `clang-base` holds the Mac's half, conditioned on a Darwin host as the MSVC half is on a Windows one, and every configuration has a build, a test and a workflow preset; the test presets print a failure's output and leave parallelism to `CTEST_PARALLEL_LEVEL`. `cmake --preset clang-debug` configures from an empty directory with all eight dependencies and NEON as the instruction set, and `clang-release` alike. `cmake --list-presets` shows the clang presets alone on the Mac, and the same condition leaves the MSVC ones alone on Windows. A clang workflow stops at the build until X.3 and X.5.

### X.3 Dependencies and targets on macOS
- [x] X.3.1 Catch2 and the test executables declare exceptions per compiler: `/EHsc` on MSVC, nothing on clang, where exceptions are the default. On clang the test executables also take `-ffp-contract=off`: the scenes the goldens are recorded from are set up in test code, which clang would otherwise contract and MSVC, on its SSE2 baseline, cannot. The `module_determinism` walk of X.2.7 asserts both. Test: the CTest case passes; `unison_tests_fast` links on the Mac once X.5 is through. Done on 2026-09-25: Catch2 takes `/EHsc` on MSVC alone, and `unison_apply_test_flags` in `tests/CMakeLists.txt` gives the three test executables `/EHsc` on MSVC and `-ffp-contract=off` on clang. The module walk requires `-ffp-contract=off` of a clang test line and forbids `/EHsc` there, and fails on the build before the change and on a copy of its commands with `/EHsc` put into a test line. No `/EHsc` is left in the Mac's compile commands, and Catch2 builds on the Mac.
- [x] X.3.2 ENet's `winmm ws2_32`, `/wd5287` and `_WINSOCK_DEPRECATED_NO_WARNINGS` under `if(WIN32)` and `if(MSVC)`. Test: `enet` compiles on the Mac; `tests/net/enet_smoke_test.cpp` passes there. Done on 2026-09-25: ENet links `winmm` and `ws2_32` and defines `_WINSOCK_DEPRECATED_NO_WARNINGS` on Windows alone, and takes `/wd5287` from MSVC alone. Before the change ENet failed to build on the Mac, clang reading `/wd5287` as a missing input file; after it ENet builds, and `tests/net/enet_smoke_test.cpp`, built on its own against Catch2 and ENet, passes there. It runs inside `unison_tests_fast` once X.5 builds that.
- [x] X.3.3 Jolt on arm64 configured and verified: `CROSS_PLATFORM_DETERMINISTIC ON`, exceptions and RTTI off, `-ffp-contract=off`, no `-mfma`; the `-faligned-allocation` Jolt adds for Apple clang noted. Test: X.2.7's case; `tests/sim/jolt_smoke_test.cpp` passes on the Mac. Done on 2026-09-25: Jolt builds on arm64 with `CROSS_PLATFORM_DETERMINISTIC`, exceptions and RTTI off and `-ffp-contract=off` from its own flags and ours, and Apple clang also gets Jolt's `-faligned-allocation`. Jolt 5.6 turns on GPU compute backends for its hair system by default: Metal on the Mac, with Objective-C++ sources and the Foundation, Metal and MetalKit frameworks linked into every executable, and DirectX 12, Vulkan and a CPU fallback on Windows. The engine steps physics on the CPU alone, so all four are off on both platforms, and the module walk requires Jolt's lines to carry `JPH_CROSS_PLATFORM_DETERMINISTIC` and none of the backends; it failed on the build before the change. Jolt's x86 options stay passed as off on arm64, where Jolt ignores them, since leaving them out would show Jolt's defaults, on, in the cache. `tests/sim/jolt_smoke_test.cpp`, built from its own compile command against Catch2 and Jolt, passes on the Mac and links no graphics framework.
- [x] X.3.4 The console's `WIN32_LEAN_AND_MEAN NOMINMAX` under `if(WIN32)`; its keyboard and screen sources chosen per platform (`console_keyboard_windows.cpp` / `console_keyboard_macos.cpp`, `console_screen_windows.cpp` / `console_screen_posix.cpp`) behind the same headers. Done when: the console target configures on both platforms with the right sources listed. Done on 2026-09-25: `WIN32_LEAN_AND_MEAN` and `NOMINMAX` are defined on Windows alone, and the console's keyboard and screen keep their platform's state in a `Terminal` of their own that each platform's source defines, so `console_keyboard.hpp` and `console_screen.hpp` serve both. Windows keeps its implementation, unchanged in behaviour, in `console_keyboard_windows.cpp` and `console_screen_windows.cpp`; the Mac gets `console_keyboard_macos.cpp` and `console_screen_posix.cpp`, which report no keyboard and no screen until X.7, so a Mac console prints its status line once a second. On the Mac the compile commands list the Mac's sources alone and both build under `-Werror`; the Windows half waits for the owner's check.

### X.4 The floating-point environment on ARM64
- [x] X.4.1 `core/include/unison/core/fp_control_word.hpp`: `readFpControlWord()`, `writeFpControlWord()`, and the named bits per architecture: the deterministic word (`0x1F80` on x64, `0` on AArch64), flush-to-zero (`FTZ | DAZ` on x64, `FZ`, bit 24, on AArch64), the rounding field and its round-toward-zero value (bits 13–14 on x64, bits 22–23 on AArch64). AArch64 reads and writes `fpcr` the way Jolt's `FPControlWord` does. Test: `"the control word reads back what was written"` for the rounding field and the flush bits, host state restored. Done on 2026-09-25: `FpControlWord`, `readFpControlWord` and `writeFpControlWord` come with `kDeterministicFpControlWord`, `kFlushToZeroBits`, `kRoundingModeBits` and `kRoundTowardZeroBits` for each architecture; FPCR goes through ACLE's `__arm_rsr64` and `__arm_wsr64`, and on x86-64 `kFlushToZeroBits` holds DAZ as well as FTZ, matching FPCR's FZ, which flushes inputs and results alike. The two tests failed to build before the header and pass on the Mac after it, built on their own against Catch2, and the header compiles alone under the determinism flags and the warnings. The core header check takes the header in as well and builds on arm64 once X.4.2 has taken `FpEnvGuard` off `<xmmintrin.h>`.
- [x] X.4.2 `FpEnvGuard` over `fp_control_word.hpp`, behaviour unchanged. Test: the existing `fp_env_guard_test.cpp` cases rewritten onto the named bits pass on both platforms. Done on 2026-09-25: `FpEnvGuard` reads and writes through `fp_control_word.hpp` and no longer includes `<xmmintrin.h>`, and its three tests set the host's state with the named bits; before the change they failed to build on the Mac, and the core header check builds on arm64 since. At `-O2` the rounding test failed on the Mac: clang, which assumes the default environment, moved a division it could see across the write of FPCR. The probes moved to `tests/support/floating_point_probes.cpp`, where no call can be seen through, and the tests pass at `-O0`, `-O2` and `-O3`; `DESIGN.md` §7.2 records why the guard stands around calls in `advanceFrame` and why no deterministic library uses link-time optimisation.
- [x] X.4.3 `tests/sim/advance_frame_test.cpp` onto the named bits: a tick under a host that set flush-to-zero runs with denormals kept and gives the host its word back. Test: the existing cases pass on both platforms. Done on 2026-09-25: the tick's tests set and read the control word through `fp_control_word.hpp`, and the host's flush bits are FTZ and DAZ on x86-64 now, as FZ on arm64 flushes inputs as well as results. `unison_core` and `unison_sim` build on the Mac without a warning, and all five cases pass there, built from their own compile command against those libraries.

### X.5 The port proper: every target builds and every test passes on the Mac
- [x] X.5.1 `unison_core` and `unison_core_header_check` build with `-Werror`; the byte-order assertions of `BinaryWriter` and `BinaryReader` stop saying that Unison targets x64. Done when: `cmake --build build/clang-debug --target unison_core unison_core_header_check` exits 0. Done on 2026-09-25: both build on the Mac without a warning, the header check since X.4.2 took `FpEnvGuard` off `<xmmintrin.h>`, and the assertions of `BinaryWriter` and `BinaryReader` say that x86-64 and arm64 are both little-endian.
- [x] X.5.2 `unison_sim` builds. Done when: `--target unison_sim` exits 0. Done on 2026-09-25: `unison_sim` builds on the Mac without a warning and without a change, EnTT, Jolt and Boost.PFR included.
- [x] X.5.3 `unison_net` builds, ENet included. Done when: `--target unison_net` exits 0. Done on 2026-09-25: `unison_net` builds on the Mac without a warning and without a change; ENet has built there since X.3.2, and `MillisecondTimer` compiles its no-op half.
- [x] X.5.4 `unison_session` and `unison_view` build. Done when: `--target unison_session unison_view` exits 0. Done on 2026-09-25: `unison_session` and `unison_view` build on the Mac without a warning and without a change.
- [x] X.5.5 `arena_sim`, `arena_view_console`, `unison_console_arena`, `unison_console_core`, `unison_relay_core`, `unison_runner_core` and `unison_runner_match` build. Done when: the build of those targets exits 0. Done on 2026-09-25: `arena_sim`, `arena_view_console`, `unison_console_arena`, `unison_console_core`, `unison_relay_core`, `unison_runner_core` and `unison_runner_match` build on the Mac without a warning and without a change.
- [x] X.5.6 The executables build and link: `unison_relay`, `unison_runner`, `unison_console` (its macOS keyboard a stub that reads nothing until X.7, so the player stands still, as the log line already says), `unison_tests_fast`, `unison_tests_arena` and `unison_benchmarks`. The test executables link only once every library builds, which is why X.5.1 to X.5.5 stop at building. Done when: `cmake --build build/clang-debug` exits 0 and `unison_benchmarks "[.benchmark]"` prints its table. Done on 2026-09-25: two tests stood in the way. `contract_test.cpp` tied active asserts to `_DEBUG`, which only MSVC's debug runtime defines, and now reads the configuration from `UNISON_DEBUG_CONFIGURATION`, which `unison_apply_test_flags` sets from `$<CONFIG:Debug>` on both compilers; the allocation probe took aligned blocks from MSVC's `_aligned_malloc` and takes them from `posix_memalign` elsewhere. `cmake --build build/clang-debug` exits 0 with every target built, and `unison_benchmarks "[.benchmark]"` prints its table on the Mac. Apple's linker warns of static libraries CMake lists twice on a link line; the warning fails nothing.
- [ ] X.5.7 `ctest -L fast` green in `clang-debug`: every Catch2 case of both test executables, the goldens included (their verdict is X.6's business; here they merely run and any failure is noted there), the relay listening then stopping, the runner playing 600 frames, the console listing its options, and `millisecond_timer_test` holding on macOS' own sleep granularity with the timer a no-op. Done when: `ctest --test-dir build/clang-debug -L fast` exits 0.
- [ ] X.5.8 `ctest` green in `clang-debug` in full: the eighteen `profile` runs of the runner and the `slow` CMake cases of X.2. Done when: `ctest --test-dir build/clang-debug` exits 0.
- [ ] X.5.9 `ctest` green in `clang-release` in full. Done when: `ctest --test-dir build/clang-release` exits 0.
- [ ] X.5.10 `tools/ci.sh` (Q-B): `source tools/env.sh`, `CTEST_PARALLEL_LEVEL` at half the cores, the two workflow presets, the formatting check, `ci: ok`; `tools/ci.ps1` reduced to the same shape over the `msvc-*` workflow presets. Done when: `tools/ci.sh` prints `ci: ok` on the Mac and `tools\ci.ps1` prints `ci: ok` on Windows.

### X.6 Cross-platform goldens
- [ ] X.6.1 XXH3 on NEON: `xxhash_vectors_test` and `hasher_test` pass on the Mac (part of X.5.7; ticked here so the record is explicit).
- [ ] X.6.2 The physics pile: `"fifty falling boxes come to rest where they always have"` reproduces `0x214BC6AEDC1EFBB3` on the Mac in Debug and Release. This is the R7 test: it hashes the raw state buffer.
- [ ] X.6.3 The scripted arena: `tests/golden/arena_scripted.checksums` verifies on the Mac in Debug and Release, all sixty frames.
- [ ] X.6.4 The text map: `"the map of a new match of two looks as it did when it was recorded"` passes on the Mac.
- [ ] X.6.5 The arena's session config: a new golden `tests/golden/arena_config.hashes` with `assetHash`, `pipelineHash` and `hashOf(SessionConfig)` for 2, 4 and 8 players, recorded on Windows by a `[.record]` case as `arena_scripted.checksums` is, verified on the Mac. This is the R5 test: two clients whose configs hash alike land in the same relay room.
- [ ] X.6.6 The protocol bytes: a new golden `tests/golden/protocol.bytes`, one hex line per message kind (`Hello`, `Welcome`, `Input`, `Confirmed`, `Checksum`, `Desync`, `Ping`, `Pong`, `Leave` and the rest of the charter's §9.2) encoded from fixed values, recorded on Windows, verified on the Mac. This is the R6 test.
- [ ] X.6.7 The matrix recorded in `docs/CROSS_PLATFORM.md` §9: Windows Debug, Windows Release, macOS Debug, macOS Release, one row per golden, with the commit it was taken at.
- [ ] X.6.8 (+, only if X.6.2 or X.6.3 fails) Locate the difference: the checksum split into its parts (globals, identifiers, each pool, physics, characters) printed per frame by a `[.diagnose]` case on both machines, the first differing part and frame recorded in `docs/CROSS_PLATFORM.md`. If positions and rotations agree while the state buffer does not (R7), the checksum moves to a canonical hash of the physics state and §8.5 of the charter changes with it; if a position differs, the cause is found in our code or reported to Jolt before anything else moves.

### X.7 The console on macOS
- [ ] X.7.1 `noteKey` takes a platform-neutral `GameKey` instead of a Windows virtual-key code; the Windows reader maps `VK_*` to it. Test: `arena_controls_test` cases rewritten onto `GameKey` pass on both platforms; the Windows console reads keys as before (checked by hand, as 3.4.2 was).
- [ ] X.7.2 Spike (Q-A): a throwaway probe under `tools/console` asks `CGEventSourceKeyState` for W from Terminal.app on macOS 27, with and without Input Monitoring granted. Recorded in `docs/CROSS_PLATFORM.md` and `docs/LAN_TEST.md`: whether the permission is asked for, and how it is granted. Nothing of the probe is committed.
- [ ] X.7.3 macOS `ConsoleKeyboard`: the terminal in raw mode (echo and canonical input off, `ISIG` kept so Ctrl+C still stops the console), stdin drained, the eight keys read from `CGEventSourceKeyState` by their `kVK_ANSI_*` codes on every `readInto`. Done when: on the Mac, two consoles and a relay on localhost, a player walks, turns and fires from the keyboard, two keys held at once included.
- [ ] X.7.4 The terminal is given back: raw mode and the cursor restored on every ending, Ctrl+C, `--run-for` and a disconnect alike. Done when: the shell prompt after each ending echoes typed text again.
- [ ] X.7.5 POSIX `ConsoleScreen`: available when standard output is a terminal, VT sequences as on Windows, the cursor hidden and shown as before. Done when: the screen of 3.4.4 redraws in place in Terminal.app and the status line prints once a second when output is redirected to a file.
- [ ] X.7.6 A relay and two consoles on localhost on the Mac for a minute, round trips of a few milliseconds, the verified frame a frame behind the predicted one, as 3.4.7 recorded on Windows. Recorded in `docs/CROSS_PLATFORM.md` §9.

### X.8 The cross-platform LAN run
- [ ] X.8.1 `docs/LAN_TEST.md` extended: either machine may be the Mac; building there (`cmake --workflow --preset clang-release`), the binaries' paths, the macOS application firewall's prompt for a relay that accepts connections, the Input Monitoring permission of X.7.2, `echo $?` for the exit code, `--from` for a console next to its relay. Done when: the owner can follow it on the Mac without asking.
- [ ] X.8.2 Run one: the relay on Windows, one console on Windows, one on the Mac, `--run-for 660`, zero desyncs, both exit codes 0, recorded in `docs/LAN_TEST.md`. 3.4.5 is ticked from this run (Q-I).
- [ ] X.8.3 Run two: the relay on the Mac, the same consoles, the same criteria, recorded.
- [ ] X.8.4 The consoles' half of Definition of Done item 10 recorded as met in the exit criteria of Phase X from the two records; the Unreal half follows in 5.2.13.

### X.9 Closing
- [ ] X.9.1 `CLAUDE.md`'s environment, build and test section holds the real commands for both platforms.
- [ ] X.9.2 `README.md` builds on both platforms in two short blocks.
- [ ] X.9.3 `tests/benchmarks/baseline.md` gains a section recorded on the Mac (Apple M4 Pro, Release), informative only; the Windows numbers stay the baseline the budget is read against.
- [ ] X.9.4 Final pass over the charter: the plan's questions answered in the charter's §17, the backlog line rewritten (Linux, ARM64 Linux, the hosted CI matrix and universal plugin binaries stay there).
- [ ] X.9.5 (optional) The runner's `NetworkSimulator` orders messages due at the same instant by a sequence number, so a runner run reads the same on both machines. Test: two messages due together are delivered in the order they were sent.

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
- [ ] Definition of Done item 7 executed on Windows and on macOS and recorded in `docs/UNREAL.md`.
- [ ] The Unreal half of Definition of Done item 10 executed and recorded in `docs/UNREAL.md` (5.2.13).

### 5.1 ThirdParty build and module
- [ ] 5.1.1 `tools/build_unreal_thirdparty.ps1`: builds the deterministic libraries, Jolt and ENet in Release with UE-compatible settings (`/MD`, exceptions and RTTI off, matching toolset) and copies libs and headers into the plugin's `ThirdParty/`. Done when: the script produces the libs from a clean tree.
- [ ] 5.1.2 Jolt configuration macros exported from CMake into a generated `unison_jolt_config.hpp` so every translation unit including Jolt headers sees identical settings. Test: a probe compares the generated values with the built library's.
- [ ] 5.1.3 Host boundary headers `unison/view/*.hpp` include neither Jolt nor EnTT; the view API exposes POD types and non-inline functions only. Test: a probe translation unit compiled with only the boundary headers links.
- [ ] 5.1.4 `Unison.uplugin`, `UnisonRuntime` module, `Unison.Build.cs` linking the ThirdParty libs; the plugin compiles in UE 5.8 (installed; compatibility of Visual Studio 18 / MSVC 14.51 with UE's toolchain confirmed here and recorded in `DESIGN.md` Q5).
- [ ] 5.1.5 Sample project `integrations/unreal/UnisonArena` (C++ project) referencing the plugin. Done when: it builds and opens in the editor.
- [ ] 5.1.6 (+) Spike: UE 5.8 on this Mac builds and opens a fresh C++ project with Xcode 27.0 on macOS 27.0; if it refuses, Xcode 26.1.1 is installed next to it and selected. Recorded in `docs/UNREAL.md` with the versions that worked. Nothing of the probe project is committed.
- [ ] 5.1.7 (+) The ThirdParty build on macOS: the Release build of the deterministic libraries, Jolt and ENet as arm64 `.a` archives with the determinism flags of X.2, headers included, copied into the plugin's `ThirdParty/lib/Mac/`. One CMake-driven procedure with a thin wrapper per platform, as Q-B shapes the CI. Done when: the script produces the archives from a clean tree on the Mac and `module_determinism` passes over that build directory.
- [ ] 5.1.8 (+) `Unison.uplugin` allows `Win64` and `Mac`; `Unison.Build.cs` links `.lib` on Win64 and `.a` on Mac; the Jolt configuration header of 5.1.2 is generated per platform, since on arm64 Jolt selects NEON and none of the SSE macros. Done when: the plugin compiles in UE 5.8 on the Mac.
- [ ] 5.1.9 (+) 5.1.3's boundary probe on the Mac, compiled on purpose with `-ffp-contract=on`, the host's default, so that any simulation code reaching a host translation unit would be fused and the probe would fail. Done when: the probe links on the Mac as it does on Windows.
- [ ] 5.1.10 (+) The sample project `integrations/unreal/UnisonArena` builds and opens in the Mac editor (5.1.5's Mac half). Done when: it opens without a warning about the plugin.

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
- [ ] 5.2.11 (+) The automation tests of 5.2.2, 5.2.3 and 5.2.4 run on the Mac; 5.2.2 toggles flush-to-zero through FPCR by way of X.4's `fp_control_word.hpp` and sees the warning. Done when: the three tests pass in the Mac editor.
- [ ] 5.2.12 (+) Two PIE instances on the Mac through a local `unison_relay` (5.2.9's Mac half). Done when: two instances play together with the debug HUD of 5.2.8 showing equal verified frames.
- [ ] 5.2.13 (+) The cross-platform Unreal run: a UE client on the Mac and a UE client on Windows through one relay, ten minutes, zero desyncs; then a UE client on the Mac with the Windows console. Recorded in `docs/UNREAL.md`; Definition of Done item 7 holds on both platforms and the Unreal half of item 10 is met.

---

## Phase 6 — Hardening

**Exit criteria**
- [ ] Definition of Done items 8 and 9 pass.
- [ ] The build-and-test section of `CLAUDE.md` holds the real commands.

### 6.1 Performance
- [ ] 6.1.1 Profile tick, snapshot, restore and 10-frame resimulation at 8 players and 200 bodies; recorded in `tests/benchmarks/baseline.md`.
- [ ] 6.1.2 Snapshot cost reduction (dirty-pool skipping, `EStateRecorderState` subsets) if the budget requires it; Q1 closed for good.
- [ ] 6.1.3 Jolt multithreaded stepping determinism test (Q4); adopted only if bit-identical over the golden replay.
- [x] 6.1.4 Steady-state allocation test: zero heap allocations per tick after warm-up (probe allocator). Known since 2.2.2: every restore still gathers bodies and characters into fresh vectors and copies the physics state buffer before Jolt reads it. Restated with the owner once the probe had measured it: the zero holds for the C++ heap, while what Jolt allocates inside itself stays Jolt's (`DESIGN.md` §8.2). Test: a match played through its rollbacks a second time, deaths and respawns included, and a session that keeps rolling back take nothing from the C++ heap once warmed up; reconciling a frame and casting rays take nothing from Jolt's heap either.
- [ ] 6.1.5 (+) Jolt's own temporaries, the arrays `SaveState` and `RestoreState` sort the contact cache into and the one a step rebuilds the broad phase into, served from a caching allocator that `JoltRuntime` installs, if the profile of 6.1.1 shows the allocator matters; otherwise dropped with the numbers. Found in 6.1.4. Test: a session that keeps rolling back takes nothing from Jolt's heap either once warmed up.

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

From `DESIGN.md` §15: C ABI and Unity/Godot bindings; Linux (x64 and ARM64) determinism and a hosted CI matrix;
universal arm64 and x86-64 macOS binaries of the plugin;
navigation (Recast for baking, Detour at runtime with deterministic math shims); authoritative-server mode;
DSL/codegen; 2D physics module (Box2D v3); encryption / Steam relay (GameNetworkingSockets); lobbies and
matchmaking; multiple local players per client; delta-compressed inputs for 16+ players; frame-local heap
allocator; asset loading from files.
