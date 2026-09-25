# Unison — Cross-platform plan: macOS port and Windows ↔ macOS play

A branch off the board in `docs/TASKS.md`, written on 2026-09-25 on the owner's word. It exists because
the project has to leave its single platform: the engine, the relay, the runner and the console must build
and run on macOS as they do on Windows, and, the owner's second word of the same day, a client on Windows
and a client on macOS must play one match through one relay without a desync. That second requirement is
what makes this more than a port: the simulation must be bit-exact between MSVC on x64 with SSE2 and
Apple clang on arm64 with NEON.

This file holds the analysis, the decisions the owner has to take, the work broken into micro-tasks, the
rhythm of working across two machines, and the record of the cross-platform runs. The ticks live on the
board, as `docs/TASKS.md` requires, under a phase of their own; see §6.

The rules of `CLAUDE.md` apply unchanged: one failing test first, no comments in implementation code,
self-review, the owner's command before every commit.

---

## 1. Goal and exit criteria

**Goal.** Unison builds, tests and runs on macOS arm64 next to Windows x64, and the two platforms play
together: a `unison_console` on Windows and a `unison_console` on macOS, through a `unison_relay` on either
machine, play the Arena at 60 Hz for ten minutes with rollback and zero desyncs.

**In scope.** The CMake build, the compiler contract of `docs/DESIGN.md` §7 on clang, the floating-point
environment on ARM64, every library, the three terminal hosts (`unison_relay`, `unison_runner`,
`unison_console`), the test suite, the goldens, the CI script, the LAN test and the documentation.

**In scope later, in Phase 5.** The Unreal Engine plugin on macOS, asked for by the owner on 2026-09-25: the
libraries this phase makes buildable with clang are what the plugin links on the Mac, so its macOS half is
planned as additions to Phase 5 (§8) and starts only after this phase and Phase 4.

**Out of scope.** Linux and a hosted CI matrix stay in the backlog, as do universal (arm64 + x86_64) plugin
binaries; every decision here is taken so that they need no undoing. An x86_64 build under Rosetta 2 is a
fallback, not a target (§7).

**Exit criteria of the phase.**

- [ ] `tools/ci.sh` prints `ci: ok` on the Mac and `tools\ci.ps1` prints `ci: ok` on Windows, on the same
      commit: both configurations configure, build with warnings as errors, pass `ctest` and pass the
      formatting check on both platforms.
- [ ] Every golden recorded on Windows verifies on macOS in Debug and in Release: the physics pile, the
      scripted arena, the text map, the arena's session config hashes and the protocol bytes (§5, X.6).
- [ ] A Windows client and a macOS client play ten minutes through a relay on Windows, then ten minutes
      through a relay on the Mac, both runs with zero desyncs, recorded in `docs/LAN_TEST.md` (§5, X.8).
- [ ] The charter, the board, `README.md` and `CLAUDE.md` describe both platforms and the two-platform rule.

---

## 2. Where the project stands

### 2.1 The two machines

| | Windows (development machine) | macOS (this machine) |
|---|---|---|
| Hardware | x64 | Apple M4 Pro, 14 cores, 24 GB, arm64 |
| OS | Windows 11 | macOS 27.0 (Darwin 27.0.0) |
| Compiler | MSVC 14.51, Visual Studio 18 | Apple clang 21.0.0 (clang-2100.3.34.2) from the Command Line Tools, SDK 27.0; no Xcode.app |
| CMake | VS-bundled, through `tools/env.ps1` | 4.4.3 from Homebrew |
| Ninja | VS-bundled | missing; Homebrew has 1.13.2 |
| clang-format | VS-bundled, version to be recorded (X.1.1) | missing; Homebrew has `clang-format` 23.1.2 and `llvm@14` … `llvm@23` |
| Shell | PowerShell 5.1 | zsh; PowerShell 7.6.6 is available from Homebrew but not installed |
| Rosetta 2 | — | not installed |

### 2.2 What is bound to Windows today

Everything below was found by reading the tree on 2026-09-25; nothing else in the sources names a platform.

| Where | What | Kind |
|---|---|---|
| `cmake/UnisonDeterminism.cmake`, `cmake/UnisonLanguageSubset.cmake`, `cmake/UnisonWarnings.cmake` | `FATAL_ERROR` on any compiler but MSVC; `/fp:precise /arch:SSE2 /FI`, `/EHs-c- /GR- _HAS_EXCEPTIONS=0`, `/permissive- /W4 /WX` | build |
| `cmake/Dependencies.cmake` | `/EHsc` on Catch2; `winmm ws2_32`, `/wd5287` and `_WINSOCK_DEPRECATED_NO_WARNINGS` on ENet; Jolt's `USE_SSE*`/`USE_AVX*` options, which Jolt applies on x86 only | build |
| `CMakePresets.json` | `msvc-debug` and `msvc-release` only, conditioned on Windows | build |
| `tests/CMakeLists.txt` | `/EHsc` on the three test executables | build |
| `tests/cmake/determinism_flags`, `tests/cmake/determinism_guard`, `tests/cmake/module_determinism` | assert MSVC spellings of every flag | tests |
| `core/include/unison/core/determinism_guard.hpp` | rejects a build unless `_M_FP_PRECISE` is defined and `_M_FP_CONTRACT` is not; clang defines neither | code |
| `core/include/unison/core/fp_env_guard.hpp` | `<xmmintrin.h>`, `_mm_getcsr`/`_mm_setcsr`, MXCSR word `0x1F80`; `tests/core/fp_env_guard_test.cpp` and `tests/sim/advance_frame_test.cpp` poke MXCSR bits directly | code |
| `net/src/millisecond_timer.cpp`, `net/CMakeLists.txt` | already guarded by `_WIN32` and `if(WIN32)`; a no-op elsewhere | done |
| `tools/console/src/console_keyboard.cpp` | `ReadConsoleInputW`, Windows virtual-key codes, focus events | code |
| `tools/console/src/console_screen.cpp` | `SetConsoleMode` for VT processing | code |
| `tools/console/include/unison/console/arena_controls.hpp` | `noteKey` takes a Windows virtual-key code | code |
| `tools/console/CMakeLists.txt` | `WIN32_LEAN_AND_MEAN NOMINMAX` | build |
| `tools/env.ps1`, `tools/ci.ps1` | PowerShell 5.1, `vswhere`, VS paths | tooling |
| `docs/LAN_TEST.md` | two Windows machines, the VC++ redistributable | docs |
| `README.md`, `CLAUDE.md` | Windows only | docs |

Portable already: the stop flag through `std::signal(SIGINT/SIGTERM)`, `SteadyClock` over
`std::chrono::steady_clock`, `std::format` and `std::to_chars` (libc++ on SDK 27 has both), ENet's own
CMake (it detects `poll`, `fcntl`, `getaddrinfo`, `inet_pton`, `socklen_t` and `msghdr.msg_flags` itself and
needs the `CMAKE_POLICY_VERSION_MINIMUM 3.5` we already set for CMake 4), xxHash, EnTT, Boost.PFR,
tl::expected, cxxopts and Catch2.

---

## 3. What bit-exact play between the two platforms rests on

Jolt's documentation lists macOS clang ARM64 with NEON among the platforms verified to produce the same
results as Windows MSVC x64, provided the library is built with `CROSS_PLATFORM_DETERMINISTIC` (as ours is)
and the application is compiled in precise mode with floating-point contraction off. Jolt checks this in its
own CI by hashing every body's position and rotation after fixed scenes on MSVC, clang and gcc across x64,
ARM64 and other architectures. So the physics library itself is not the risk. The risks are our own code,
the compilers, the standard libraries, and what our checksum covers beyond what Jolt guarantees. Each is
listed with what was found on 2026-09-25 and what the plan does about it.

| # | Risk | What the tree shows | What the plan does |
|---|---|---|---|
| R1 | **Fused multiply-add.** AArch64 has fused multiply-add instructions (`fmadd`, `fmla`), and clang's default `-ffp-contract=on` fuses `a * b + c` inside one expression; MSVC on its SSE2 baseline has nothing to fuse into. One fused operation anywhere in a deterministic library or in Jolt parts the platforms for good. | Measured with Apple clang 21 on 2026-09-25: `a * b + c` compiles to `fmadd` by default and under `-ffp-model=precise`, and to `fmul` and `fadd` under `-ffp-contract=off`. `-ffp-model=precise -ffp-contract=off` works but trips `-Woverriding-option`, a failed build under `-Werror`, while `-fno-fast-math -ffp-contract=off` is silent and, placed last, takes back `-ffast-math` and `-ffp-model=fast`. `#pragma STDC FP_CONTRACT OFF` from a force-included header stops fusion under the default but not under `-ffp-contract=fast` or `-ffp-model=fast`, and `-ffp-model=fast` defines no macro a header could test. Jolt keeps `JPH_USE_FMADD` off under `CROSS_PLATFORM_DETERMINISTIC`. | `unison_apply_determinism` passes `-fno-fast-math -ffp-contract=off` to every deterministic library and to Jolt (X.2.3); the guard rejects fast-math and turns contraction off by pragma (X.2.4); the flags tests read every command line (X.2.6, X.2.7); a canary built under the determinism flags proves a multiply followed by an add rounds twice (X.2.8); the test executables take `-ffp-contract=off` as well, since they set up the goldens' scenes (X.3.1). |
| R2 | **libm.** `sin`, `cos`, `atan2`, `exp`, `pow` differ between CRTs. | Banned by §7.3; `unison::math` wraps Jolt's polynomials; `sqrt`, `floor`, `ceil`, `fmod`, `abs` are exact everywhere. | Nothing new; the goldens prove it (X.6). |
| R3 | **Floating-point environment.** Denormals and rounding are per-thread state: MXCSR on x64, FPCR on AArch64. Jolt's docs ask for FTZ/DAZ set alike on every platform. | `FpEnvGuard` sets MXCSR to `0x1F80` (round to nearest, denormals kept, exceptions masked) around every tick. AArch64 has no MXCSR; a macOS process starts with FPCR at `0`, read on 2026-09-25. | A `FpControlWord` abstraction reads and writes MXCSR or FPCR and names the bits per architecture; the deterministic word on FPCR is `0` (FZ off, RMode nearest, FZ16 off, AH/FIZ/NEP off); the guard and its tests move onto it (X.4). |
| R4 | **Standard-library order.** `std::sort` orders equal elements differently in the MSVC STL and libc++; `std::unordered_*` iterates differently; heaps order ties differently. | Every `std::sort` in the deterministic libraries has a total order: contacts by body pair then sub-shapes, hits by fraction then body, bodies by id, assets by id. No `unordered_*` inside them. The runner's `NetworkSimulator` keeps a heap with ties, which only affects the order two simulated messages due at the same instant are delivered in, a host-side matter. | The rule enters §7.3 in words that cover heaps and partitions too (X.0.1). The runner's heap gets a sequence number so a runner run reads the same on both machines (X.9.5, optional). |
| R5 | **Compiler-derived values in state, hashes or on the wire.** `entt::type_hash<T>` is a hash of the compiler's pretty-function string and differs between MSVC and clang; `typeid`, `__FUNCSIG__`, `std::hash` likewise. A value like that in the session config would put the two clients in different rooms. | `entt::type_hash` is used only to find an asset table, a signal listener or the input type inside one process. Assets are keyed by `AssetId`, an xxHash of a name, and `hashOf(AssetRegistry)` sorts by that id; the pipeline hash folds the systems' literal names; components are registered from one translation unit in source order. `SessionConfig` therefore carries the same `assetHash` and `pipelineHash` on both platforms. | The rule enters §7.3 (X.0.1); a golden of the arena's `assetHash`, `pipelineHash` and config hash, recorded on Windows, is checked on the Mac (X.6.5). |
| R6 | **Layout.** `long` is 32 bits on Windows and 64 on macOS; bit-fields are laid out differently by MSVC and the Itanium ABI; an `enum` without a fixed type may differ; `std::optional` and other library types are laid out differently. A component or a message with any of these hashes or encodes differently. | No `long`, `wchar_t`, `long double` or `int_fast*` in libraries or samples; every `enum class` has a fixed underlying type; components are aggregates of fixed-width scalars checked padding-free by `UNISON_COMPONENT`; the protocol writes fields one by one, little-endian, through `BinaryWriter`. | The rule enters §7.3 (X.0.1); a golden of every message's bytes is checked on the Mac (X.6.6). |
| R7 | **Our checksum covers more than Jolt guarantees.** `FrameChecksum` hashes the raw bytes of `PhysicsSystem::SaveState(All)` plus the characters' state: positions, rotations, velocities, sleep-test spheres, the contact cache with its warm-start impulses. Jolt verifies positions and rotations across platforms, not this buffer. | Jolt's `StreamOut::Write(const Vec3&)` writes three floats and skips the SIMD lane that SSE and NEON could fill differently, and the cached impulses feed the next step, so most differences in the buffer would soon show in positions too. The exception is the sign of a zero: SSE's and NEON's minimum and maximum instructions return different zeros for +0 and −0, and a velocity of −0 moves a body no differently from +0. The buffer is expected to match, but that is an inference, not a test. | The physics golden (X.6.2) is the test. If positions match and the buffer does not, the checksum moves to a canonical hash of the physics state (X.6.8, a charter change to §8.5) rather than to a per-platform golden. |
| R8 | **Hashing itself.** XXH3 has SSE2, AVX2 and NEON code paths. | XXH3 is specified to give the same digest on every path; `tests/core/xxhash_vectors_test.cpp` checks published vectors. | Runs on the Mac as part of X.5.7; named in X.6.1. |
| R9 | **The compilers' own arithmetic.** With contraction off and no fast-math, clang and MSVC both evaluate `float` in single precision on SSE2 and NEON, both round to nearest, and neither reassociates. Auto-vectorisation keeps IEEE semantics without fast-math. | — | The goldens and the LAN run are the proof (X.6, X.8). |
| R10 | **NaN bits.** An invalid operation yields a different NaN on each platform: `0.0F / 0.0F` is `0xFFC00000` on x64 and `0x7FC00000` on arm64, the latter read on 2026-09-25. | Nothing in the tree is known to produce a NaN, and one in state would be a bug on its own. | The rule enters §7.3 of the charter (X.0.1); a golden that parts ways is diagnosed by X.6.8, which names the part. |
| R11 | **Float-to-integer conversion out of range.** It is undefined, and the hardware answers differently: x64 gives the lowest integer where arm64 saturates, and a NaN becomes the lowest integer on x64 and 0 on arm64. | No float is converted to an integer in `unison_core`, `unison_sim` or `arena_sim` on 2026-09-25; inputs reach the simulation as integers already (§6.4 of the charter). | The rule enters §7.3 of the charter (X.0.1). |

What need not match: how long a sleep of a millisecond takes, when the relay's clock ticks, how the console
reads keys or draws its screen. Those are host matters and are allowed to differ.

---

## 4. Decisions for the owner

Each question has a recommendation. Where the owner chooses otherwise, the micro-tasks of §5 change
accordingly and X.0.1 records the choice in the charter.

The owner accepted the plan with these recommendations on 2026-09-25. Q-A still waits on its spike (X.7.2),
Q-C on the version X.1.1 records, and Q-H is taken up only if X.6 locates a difference.

- **Q-A. The keyboard on macOS.** No terminal reports a key going up; Windows' console input does, which is
  why Q3 chose plain console input. On macOS the choices are (1) the terminal in raw mode plus
  `CGEventSourceKeyState`, which answers whether a key is down right now, for all eight game keys on every
  read, so two keys held at once work as on Windows; or (2) the terminal in raw mode alone, taking a key
  for held while its auto-repeat keeps arriving, which cannot see two keys held together (macOS repeats
  only the last one) and lets a key go a few hundred milliseconds late. **Recommended: (1)**, with a spike
  first (X.7.2) to record whether macOS 27 asks for the Terminal's Input Monitoring permission for it, and
  that permission documented in `docs/LAN_TEST.md` if it does. One difference stays: the key state is
  global, so a Mac console reads the keys whichever window is in front; Windows' console reads them only
  with focus.
- **Q-B. The CI procedure on two platforms.** The procedure is: configure, build and test both
  configurations, then `clang-format --dry-run --Werror` over the tracked sources. **Recommended:** move it
  into `CMakePresets.json` as workflow presets (version 6 already supports them), one per configuration,
  so that `cmake --workflow --preset clang-debug` and `--preset msvc-debug` are the same definition; then
  `tools/ci.ps1` keeps only `env.ps1` and the two workflow calls and `tools/ci.sh` mirrors it in a dozen
  lines. The alternative, PowerShell 7 on the Mac so that one script serves both, adds a runtime the Mac
  does not otherwise need.
- **Q-C. The formatting authority.** Different majors of clang-format format the same tree differently,
  and a check that passes on one machine and fails on the other would stall every micro-task. **Recommended:**
  the VS-bundled clang-format stays the authority; the owner records its version (X.1.1) and the Mac
  installs the same major from Homebrew (`llvm@<major>`); `tools/env.sh` exports it as `UNISON_CLANG_FORMAT`
  as `env.ps1` does on Windows.
- **Q-D. The warning set on clang.** `/permissive- /W4 /WX` has no exact twin. **Recommended:**
  `-Wall -Wextra -Wpedantic -Wshadow -Werror` as the contract, then `-Wconversion -Wsign-conversion` tried in
  X.5 and kept if the tree is clean or the fixes are few, since MSVC's level 4 already made most narrowing
  explicit.
- **Q-E. When Windows is checked.** Claude works on the Mac; the owner's Windows check is a build and a test
  run the Mac cannot do. **Recommended rule:** a micro-task that only adds macOS-side code (a new `#else`
  branch, a source listed under `if(APPLE)`, a macOS document) counts as finished when the Mac is green; a
  micro-task that touches a shared CMake module, a shared header, a test or a golden waits for the owner's
  `tools\ci.ps1` on Windows before its "commit". The report of every micro-task says which of the two it is.
- **Q-F. Line endings.** The tree is edited on both platforms now; a `.gitattributes` with `* text=auto
  eol=lf` keeps clang-format and the golden readers seeing the same bytes. Adding it may renormalise files
  in one commit of its own. **Recommended:** add it (X.1.2); the owner decides on the renormalising commit.
- **Q-G. Golden authority.** **Recommended:** goldens stay recorded on Windows, as they are; macOS only
  verifies. A golden is never re-recorded on the Mac alone. When a golden has to change for a legitimate
  reason (§7.4 of the charter), it is re-recorded on Windows and verified on the Mac inside the same
  micro-task.
- **Q-H. The fallback.** If the arm64 build cannot be made to match within a bounded effort (§7), an x86_64
  build under Rosetta 2 runs the same SSE2 instructions as Windows and would still give a Mac that plays,
  though not natively. **Recommended:** not before X.6 has shown a real, located difference; the owner
  decides when.
- **Q-I. The Windows LAN test, 3.4.5.** Definition of Done item 2 asks for two machines, not two Windows
  machines. **Recommended:** X.8.2, the run with a relay on Windows and consoles on both platforms,
  satisfies item 2 as well, and 3.4.5 is ticked from it; a second Windows machine is then not needed.

---

## 5. The work: Phase X — Cross-platform: macOS

Micro-tasks in board order; a task may start when everything it depends on is ticked. Every micro-task
ends with the observable check that defines "done"; a micro-task that has a test names it. Ids are stable.

### X.0 Charter and board

- [x] X.0.1 Amend `docs/DESIGN.md`: D12 superseded by a new decision (v1 platforms are Windows x64 MSVC and macOS arm64
      Apple clang, and they play together); §1.2 loses the single-platform non-goal; §2 items 7 and 8 cover both
      platforms and a new item 10 states the cross-platform run, terminal and Unreal hosts alike; §7.1 becomes "Compiler
      flags (MSVC and clang)" with a column per compiler; §7.2 names FPCR next to MXCSR; §7.3 gains the rules of R4, R5
      and R6 in words (total orders for sorts, heaps and partitions; no value derived from a type's name or a compiler
      string in state, hashes or on the wire; fixed-width scalars only, no bit-fields, fixed enum types); §7.4 gains
      items 6 (Windows and macOS builds agree on every golden) and 7 (a Windows client and a macOS client play through
      one relay); §10.2 describes the Mac's keyboard and screen; §10.3 the plugin's Mac build and D19 the Mac's Xcode
      (§8); §11 and §12 name `tools/ci.sh`, the workflow presets and the per-platform ThirdParty folders; §15 names
      Phase X and the Mac half of Phase 5; §16 gains the cross-compiler risk; §17 gains the questions of §4 that stay
      open. Done when: the charter reads consistently and the board's amendments list records the change.
- [ ] X.0.2 Register Phase X on `docs/TASKS.md`: this section's micro-tasks after Phase 3, the Phase 5 additions of §8
      under their own ids, a progress row,
      and **Now** pointing at X.1.1 with the owner's word of 2026-09-25 (Phase X runs before Phase 4; 3.4.5
      stays open and closes from X.8.2 if Q-I stands). `CLAUDE.md` gains the two-platform rule of Q-E in
      its workflow section. Done when: the board, `CLAUDE.md` and this file agree.

### X.1 Toolchain and repository hygiene

- [ ] X.1.1 Tools on the Mac: `brew install ninja llvm@<major>`, the major being the VS-bundled
      clang-format's, which the owner reads on Windows with `& $env:UNISON_CLANG_FORMAT --version` and
      records in `CLAUDE.md`'s environment section together with the Mac's toolchain of §2.1. `tools/env.sh`
      exports `UNISON_CLANG_FORMAT` (and nothing else: CMake and Ninja are on `PATH` from Homebrew). Done
      when: `source tools/env.sh` then `"$UNISON_CLANG_FORMAT" --version` prints the recorded major.
- [ ] X.1.2 `.gitattributes` with `* text=auto eol=lf`, `.gitignore` with `.DS_Store`; the renormalising
      commit is the owner's call (Q-F). Done when: `git ls-files --eol` shows `i/lf` for every tracked text
      file on both machines.
- [ ] X.1.3 Formatting parity: the Mac's clang-format passes `--dry-run --Werror` over the tracked sources
      exactly as Windows' does. Done when: the check passes on the tree at HEAD without a single change;
      any file the two versions disagree on is reported here before anything is touched.

### X.2 The compiler contract on clang

Until X.2.9 brings the presets, the Mac is configured by hand:
`cmake -S . -B build/clang-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.

- [ ] X.2.1 `unison_apply_language_subset` on clang: `-fno-exceptions -fno-rtti` (no `_HAS_EXCEPTIONS`,
      which is the MSVC STL's switch). Test: a probe target built with the function carries both flags in
      `compile_commands.json`; `throw` in a probe source fails to compile.
- [ ] X.2.2 `unison_apply_warnings` on clang: the set of Q-D. Test: a deliberate shadowed variable in a
      probe fails the build.
- [ ] X.2.3 `unison_apply_determinism` on clang: `-fno-fast-math -ffp-contract=off -fexcess-precision=standard` and
      `-include` of `determinism_guard.hpp`, after every flag a toolchain puts before them; not `-ffp-model=precise`,
      which followed by `-ffp-contract=off` trips `-Woverriding-option` under `-Werror` (R1). `UNISON_INSTRUCTION_SET`
      gains `NEON`, the only value on arm64 and its default there, while `SSE2` and `AVX2` stay x86-64 values (SSE2 is
      clang's x86-64 baseline and adds no flag; `AVX2` adds `-mavx2` and never `-mfma`); Jolt's `USE_SSE*`/`USE_AVX*`
      options are passed on x86-64 only. Test: the flags appear on the probe's command line;
      `-DUNISON_INSTRUCTION_SET=SSE2` on arm64 fails at configure with a message naming the architecture.
- [ ] X.2.4 `determinism_guard.hpp` on clang: rejects `__FAST_MATH__` and `__FINITE_MATH_ONLY__`, turns
      contraction off with `#pragma STDC FP_CONTRACT OFF`, and rejects any other compiler with a message.
      The MSVC branch stays as it is. Done when: `guard_probe.cpp` compiles under the flags of X.2.3 and
      fails under `-ffast-math` with the guard's own message.
- [ ] X.2.5 `tests/cmake/determinism_guard` on clang: `-ffast-math`, `-ffinite-math-only` and `-ffp-model=aggressive`
      are rejected; the contract of X.2.3 is accepted. Recorded difference from MSVC: no flag at all is accepted on
      clang, because clang defines no macro for its default model; `-ffp-contract=fast` and `-ffp-model=fast` cannot be
      caught by a header either, since they define no macro and ignore the pragma (R1). Those are caught by X.2.6 and
      X.2.8 instead. Test: the CTest case `determinism_guard` passes on the Mac.
- [ ] X.2.6 `tests/cmake/determinism_flags` per compiler: required on clang `-fno-fast-math`, `-ffp-contract=off`,
      `-fno-exceptions`, `-fno-rtti`, the `-include` of the guard; forbidden `-ffast-math`, `-Ofast`, `-ffp-model=fast`,
      `-ffp-model=aggressive`, `-funsafe-math-optimizations`, `-fassociative-math`, `-freciprocal-math`,
      `-ffp-contract=on`, `-ffp-contract=fast`, `-fexceptions`, `-frtti`, `-mfma`, `-march=native`. Test: the CTest case
      passes on the Mac and still on Windows.
- [ ] X.2.7 `tests/cmake/module_determinism` per compiler: the same walk over `compile_commands.json` with
      clang spellings, Jolt included (`-ffp-contract=off` present, `-mfma` and `-ffast-math` absent, no
      `-fexceptions`); the test executables carry no `-fno-exceptions`. Test: the CTest case passes on the
      Mac and still on Windows.
- [ ] X.2.8 A behavioural canary: a non-inline `multiplyThenAdd(float, float, float)` and its `double` twin in a
      test-side library built under `unison_apply_determinism`, as `unison_core_header_check` is, so no test-only code
      enters the engine. Test: `"a multiply followed by an add rounds twice"`: with `a = b = 1 + 2^-23` and
      `c = -(1 + 2^-22)` the result is `0`, where a fused evaluation gives `2^-46`, and likewise in `double` with
      `2^-52` and `2^-51`, where fusing gives `2^-104`. It runs on both platforms and guards against a future compiler
      default as much as against a wrong flag.
- [ ] X.2.9 `CMakePresets.json`: `clang-base` (hidden, host `Darwin`, Ninja, `cc`/`c++`, compile
      commands), `clang-debug`, `clang-release`; build, test and workflow presets for all four
      configurations (Q-B), tests with output on failure and parallelism from `CTEST_PARALLEL_LEVEL`. Done
      when: `cmake --preset clang-debug` configures on the Mac and fetches every dependency; `cmake
      --list-presets` on Windows still shows only the `msvc-*` configure presets.

### X.3 Dependencies and targets on macOS

- [ ] X.3.1 Catch2 and the test executables declare exceptions per compiler: `/EHsc` on MSVC, nothing on clang, where
      exceptions are the default. On clang the test executables also take `-ffp-contract=off`: the scenes the goldens
      are recorded from are set up in test code, which clang would otherwise contract and MSVC, on its SSE2 baseline,
      cannot. The `module_determinism` walk of X.2.7 asserts both. Test: the CTest case passes; `unison_tests_fast`
      links on the Mac once X.5 is through.
- [ ] X.3.2 ENet's `winmm ws2_32`, `/wd5287` and `_WINSOCK_DEPRECATED_NO_WARNINGS` under `if(WIN32)` and
      `if(MSVC)`. Test: `enet` compiles on the Mac; `tests/net/enet_smoke_test.cpp` passes there.
- [ ] X.3.3 Jolt on arm64 configured and verified: `CROSS_PLATFORM_DETERMINISTIC ON`, exceptions and RTTI
      off, `-ffp-contract=off`, no `-mfma`; the `-faligned-allocation` Jolt adds for Apple clang noted.
      Test: X.2.7's case; `tests/sim/jolt_smoke_test.cpp` passes on the Mac.
- [ ] X.3.4 The console's `WIN32_LEAN_AND_MEAN NOMINMAX` under `if(WIN32)`; its keyboard and screen sources
      chosen per platform (`console_keyboard_windows.cpp` / `console_keyboard_macos.cpp`,
      `console_screen_windows.cpp` / `console_screen_posix.cpp`) behind the same headers. Done when: the
      console target configures on both platforms with the right sources listed.

### X.4 The floating-point environment on ARM64

- [ ] X.4.1 `core/include/unison/core/fp_control_word.hpp`: `readFpControlWord()`,
      `writeFpControlWord()`, and the named bits per architecture: the deterministic word (`0x1F80` on
      x64, `0` on AArch64), flush-to-zero (`FTZ | DAZ` on x64, `FZ`, bit 24, on AArch64), the rounding
      field and its round-toward-zero value (bits 13–14 on x64, bits 22–23 on AArch64). AArch64 reads and
      writes `fpcr` the way Jolt's `FPControlWord` does. Test: `"the control word reads back what was
      written"` for the rounding field and the flush bits, host state restored.
- [ ] X.4.2 `FpEnvGuard` over `fp_control_word.hpp`, behaviour unchanged. Test: the existing
      `fp_env_guard_test.cpp` cases rewritten onto the named bits pass on both platforms.
- [ ] X.4.3 `tests/sim/advance_frame_test.cpp` onto the named bits: a tick under a host that set
      flush-to-zero runs with denormals kept and gives the host its word back. Test: the existing cases
      pass on both platforms.

### X.5 The port proper: every target builds and every test passes on the Mac

One micro-task per group of targets; each fixes what Apple clang rejects or warns about in that group, with
`-Werror`, and nothing else. A change that alters behaviour on Windows is a `(+)` micro-task of its own,
never a side effect.

- [ ] X.5.1 `unison_core` and `unison_core_header_check` build with `-Werror`; the byte-order assertions of
      `BinaryWriter` and `BinaryReader` stop saying that Unison targets x64. Done when:
      `cmake --build build/clang-debug --target unison_core unison_core_header_check` exits 0.
- [ ] X.5.2 `unison_sim` builds. Done when: `--target unison_sim` exits 0.
- [ ] X.5.3 `unison_net` builds, ENet included. Done when: `--target unison_net` exits 0.
- [ ] X.5.4 `unison_session` and `unison_view` build. Done when: `--target unison_session unison_view`
      exits 0.
- [ ] X.5.5 `arena_sim`, `arena_view_console`, `unison_console_arena`, `unison_console_core`,
      `unison_relay_core`, `unison_runner_core` and `unison_runner_match` build. Done when: the build of
      those targets exits 0.
- [ ] X.5.6 The executables build and link: `unison_relay`, `unison_runner`, `unison_console` (its macOS
      keyboard a stub that reads nothing until X.7, so the player stands still, as the log line already
      says), `unison_tests_fast`, `unison_tests_arena` and `unison_benchmarks`. The test executables link
      only once every library builds, which is why X.5.1 to X.5.5 stop at building. Done when:
      `cmake --build build/clang-debug` exits 0 and `unison_benchmarks "[.benchmark]"` prints its table.
- [ ] X.5.7 `ctest -L fast` green in `clang-debug`: every Catch2 case of both test executables, the
      goldens included (their verdict is X.6's business; here they merely run and any failure is noted
      there), the relay listening then stopping, the runner playing 600 frames, the console listing its
      options, and `millisecond_timer_test` holding on macOS' own sleep granularity with the timer a no-op.
      Done when: `ctest --test-dir build/clang-debug -L fast` exits 0.
- [ ] X.5.8 `ctest` green in `clang-debug` in full: the eighteen `profile` runs of the runner and the
      `slow` CMake cases of X.2. Done when: `ctest --test-dir build/clang-debug` exits 0.
- [ ] X.5.9 `ctest` green in `clang-release` in full. Done when: `ctest --test-dir build/clang-release`
      exits 0.
- [ ] X.5.10 `tools/ci.sh` (Q-B): `source tools/env.sh`, `CTEST_PARALLEL_LEVEL` at half the cores, the two
      workflow presets, the formatting check, `ci: ok`; `tools/ci.ps1` reduced to the same shape over the
      `msvc-*` workflow presets. Done when: `tools/ci.sh` prints `ci: ok` on the Mac and `tools\ci.ps1`
      prints `ci: ok` on Windows.

### X.6 Cross-platform goldens

The goldens were recorded on Windows and are not touched here (Q-G). Each micro-task either passes on the
Mac in Debug and Release, or locates the first difference and stops for the owner.

- [ ] X.6.1 XXH3 on NEON: `xxhash_vectors_test` and `hasher_test` pass on the Mac (part of X.5.7; ticked
      here so the record is explicit).
- [ ] X.6.2 The physics pile: `"fifty falling boxes come to rest where they always have"` reproduces
      `0x214BC6AEDC1EFBB3` on the Mac in Debug and Release. This is the R7 test: it hashes the raw state
      buffer.
- [ ] X.6.3 The scripted arena: `tests/golden/arena_scripted.checksums` verifies on the Mac in Debug and
      Release, all sixty frames.
- [ ] X.6.4 The text map: `"the map of a new match of two looks as it did when it was recorded"` passes on
      the Mac.
- [ ] X.6.5 The arena's session config: a new golden `tests/golden/arena_config.hashes` with `assetHash`,
      `pipelineHash` and `hashOf(SessionConfig)` for 2, 4 and 8 players, recorded on Windows by a
      `[.record]` case as `arena_scripted.checksums` is, verified on the Mac. This is the R5 test: two
      clients whose configs hash alike land in the same relay room.
- [ ] X.6.6 The protocol bytes: a new golden `tests/golden/protocol.bytes`, one hex line per message kind
      (`Hello`, `Welcome`, `Input`, `Confirmed`, `Checksum`, `Desync`, `Ping`, `Pong`, `Leave` and the rest
      of the charter's §9.2) encoded from fixed values, recorded on Windows, verified on the Mac. This is the R6 test.
- [ ] X.6.7 The matrix recorded in §9: Windows Debug, Windows Release, macOS Debug, macOS Release, one
      row per golden, with the commit it was taken at.
- [ ] X.6.8 (+, only if X.6.2 or X.6.3 fails) Locate the difference: the checksum split into its parts
      (globals, identifiers, each pool, physics, characters) printed per frame by a `[.diagnose]` case on
      both machines, the first differing part and frame named here. If positions and rotations agree while
      the state buffer does not (R7), the checksum moves to a canonical hash of the physics state and §8.5
      of the charter changes with it; if a position differs, the cause is found in our code or reported to
      Jolt before anything else moves.

### X.7 The console on macOS

- [ ] X.7.1 `noteKey` takes a platform-neutral `GameKey` instead of a Windows virtual-key code; the Windows
      reader maps `VK_*` to it. Test: `arena_controls_test` cases rewritten onto `GameKey` pass on both
      platforms; the Windows console reads keys as before (checked by hand, as 3.4.2 was).
- [ ] X.7.2 Spike (Q-A): a throwaway probe under `tools/console` asks `CGEventSourceKeyState` for W from
      Terminal.app on macOS 27, with and without Input Monitoring granted. Recorded here and in
      `docs/LAN_TEST.md`: whether the permission is asked for, and how it is granted. Nothing of the probe
      is committed.
- [ ] X.7.3 macOS `ConsoleKeyboard`: the terminal in raw mode (echo and canonical input off, `ISIG` kept
      so Ctrl+C still stops the console), stdin drained, the eight keys read from `CGEventSourceKeyState`
      by their `kVK_ANSI_*` codes on every `readInto`. Done when: on the Mac, two consoles and a relay on
      localhost, a player walks, turns and fires from the keyboard, two keys held at once included.
- [ ] X.7.4 The terminal is given back: raw mode and the cursor restored on every ending, Ctrl+C, `--run-for`
      and a disconnect alike. Done when: the shell prompt after each ending echoes typed text again.
- [ ] X.7.5 POSIX `ConsoleScreen`: available when standard output is a terminal, VT sequences as on
      Windows, the cursor hidden and shown as before. Done when: the screen of 3.4.4 redraws in place in
      Terminal.app and the status line prints once a second when output is redirected to a file.
- [ ] X.7.6 A relay and two consoles on localhost on the Mac for a minute, round trips of a few
      milliseconds, the verified frame a frame behind the predicted one, as 3.4.7 recorded on Windows.
      Recorded in §9.

### X.8 The cross-platform LAN run

- [ ] X.8.1 `docs/LAN_TEST.md` extended: either machine may be the Mac; building there
      (`cmake --workflow --preset clang-release`), the binaries' paths, the macOS application firewall's
      prompt for a relay that accepts connections, the Input Monitoring permission of X.7.2, `echo $?` for
      the exit code, `--from` for a console next to its relay.
      Done when: the owner can follow it on the Mac without asking.
- [ ] X.8.2 Run one: the relay on Windows, one console on Windows, one on the Mac, `--run-for 660`, zero
      desyncs, both exit codes 0, recorded in `docs/LAN_TEST.md`. If Q-I stands, 3.4.5 is ticked from this
      run.
- [ ] X.8.3 Run two: the relay on the Mac, the same consoles, the same criteria, recorded.
- [ ] X.8.4 The consoles' half of Definition of Done item 10 recorded as met in the exit criteria of Phase X from the
      two records; the Unreal half follows in 5.2.13.

### X.9 Closing

- [ ] X.9.1 `CLAUDE.md`'s environment, build and test section holds the real commands for both platforms.
- [ ] X.9.2 `README.md` builds on both platforms in two short blocks.
- [ ] X.9.3 `tests/benchmarks/baseline.md` gains a section recorded on the Mac (Apple M4 Pro, Release),
      informative only; the Windows numbers stay the baseline the budget is read against.
- [ ] X.9.4 Final pass over the charter: the questions of §4 answered in §17, the backlog line rewritten
      (Linux, ARM64 Linux, the hosted CI matrix and universal plugin binaries stay there).
- [ ] X.9.5 (optional) The runner's `NetworkSimulator` orders messages due at the same instant by a sequence
      number, so a runner run reads the same on both machines. Test: two messages due together are
      delivered in the order they were sent.

---

## 6. Order, rhythm and where progress is recorded

- **Order.** X.0 → X.1 → X.2 → X.3 → X.4 → X.5 → X.6 → X.7 → X.8 → X.9. X.4 may run before X.3 when
  `unison_core` is the first thing that fails to build; X.7.1 may run inside X.5.6 if the stub is awkward
  without it. Nothing in X.6 starts before X.5.9.
- **Two machines.** Claude works on the Mac. A micro-task is reported with the Mac's own check and, per
  Q-E, either "macOS-only change" or "needs the owner's Windows check: `tools\ci.ps1` prints `ci: ok`".
  The owner runs that check before saying "commit" for the second kind. A Windows failure is fixed in the
  same micro-task, never in a later one.
- **Progress.** The ticks and the **Now** pointer live on `docs/TASKS.md` under Phase X (X.0.2 puts them
  there). This file is amended only for the analysis of §3, the decisions of §4 and the record of §9.
- **Size.** Fifty-four micro-tasks, two of them conditional or optional (X.6.8 and X.9.5). No dates (D27).

---

## 7. Risks and fallbacks

| Risk | Signal | Fallback |
|---|---|---|
| A golden differs on the Mac while Jolt's positions agree (R7) | X.6.2 fails, X.6.8's diagnosis shows the physics part alone differing | Canonical physics hashing (X.6.8); the charter's §8.5 changes; the goldens are re-recorded on Windows once, deliberately. |
| A golden differs because a position differs | X.6.8 names a pool or a body on a frame | Bisect by frame and system with the `[.diagnose]` case on both machines; the same method Q2 used for AVX2. A cause in our code is a determinism bug and is fixed; a cause in Jolt is reported upstream with a minimal scene and, meanwhile, Q-H's Rosetta build keeps the Mac playing on SSE2. |
| Apple clang rejects code MSVC accepted | X.5 build errors | Fixed in place; anything that changes behaviour on Windows becomes a `(+)` micro-task with its own test. |
| The two clang-format versions disagree | X.1.3 reports files | The owner picks the major both machines use; the tree is formatted once by that version in a commit of its own. |
| `CGEventSourceKeyState` needs Input Monitoring or reads nothing from a terminal | X.7.2 | The permission documented and granted once; if it reads nothing even so, option (2) of Q-A is built and its limits (one key at a time, late releases) are recorded in the charter's §10.2. |
| `MillisecondTimer` has no macOS half and sleeps drift | X.5.3's timer test, X.7.6's round trips | Measured on 2026-09-25: twenty sleeps of a millisecond took 25 to 26 ms on the Mac, well inside the test's 100 ms, so the no-op holds; if the round trips of X.7.6 show otherwise, a `mach_wait_until`-based sleep goes behind the same class. |
| Old third-party CMake under CMake 4 | configure errors on the Mac | `CMAKE_POLICY_VERSION_MINIMUM` already covers ENet; the same line covers another dependency if one needs it. |
| The Mac's firewall or Input Monitoring blocks the LAN run | X.8 | Documented in `docs/LAN_TEST.md`; neither is a verdict on determinism. |

---

## 8. The Unreal plugin on macOS

Asked for by the owner on 2026-09-25, after the sections above were written: the plugin of Phase 5 is to run
on macOS as well. It is realistic, and it is cheaper than the port this file plans, for the reason the
charter was built on: the simulation is a set of static libraries behind boundary headers that expose POD
data and non-inline functions only (§7.3 of the charter, micro-task 5.1.3), so the plugin on the Mac links
the very libraries Phase X makes buildable with clang and adds no simulation code of its own. What is left
is packaging, the host's platform layer, which Unreal abstracts, and verification. An Unreal client on the
Mac and a console on Windows play together for the same reason a console on each does.

### 8.1 What the Mac has and what Epic asks for

- Installed: Xcode 27.0 (build 27A266a) at `/Applications/Xcode.app`, carrying the same Apple clang 21.0.0
  (clang-2100.3.34.2) as the Command Line Tools the build of §2.1 uses; 213 GB free on the disk; 24 GB of
  memory. `xcode-select` still points at the Command Line Tools.
- Missing: the Epic Games Launcher and Unreal Engine 5.8 for macOS, tens of gigabytes through the launcher.
- Epic's requirements page for UE 5.8 on macOS, read on 2026-09-25: macOS Sonoma 14.5 at least, Xcode 26.0
  at least and 26.1.1 recommended, Xcode 26.4 named incompatible. This Mac runs macOS 27.0 with Xcode 27.0,
  which the page does not list, so whether UE 5.8 builds a C++ project here is the first thing to find out
  (5.1.6). Xcodes coexist on one Mac, so Xcode 26.1.1 next to 27.0 and `xcode-select` pointing at it is the
  way out if it does not.
- The editor ships as a universal binary and compiles a project for the host architecture, arm64, in the
  editor. A universal packaged build would need x86_64 libraries built with the same flags (SSE2, as on
  Windows); that stays in the backlog until someone needs it.

### 8.2 The owner's preparation

Both steps need the administrator's password, so they are the owner's.

1. `sudo xcode-select -s /Applications/Xcode.app` and `sudo xcodebuild -license accept`, so that CMake,
   `tools/ci.sh` and Unreal use one toolchain. Today both toolchains carry the same clang, so nothing in
   the libraries changes; the step only removes a second compiler from the picture.
2. The Epic Games Launcher for macOS and Unreal Engine 5.8 through it.

### 8.3 Additions to Phase 5

Appended ids, as the board asks of work discovered later; X.0.2 puts them on the board. The existing
micro-tasks of Phase 5 stay as they are and are done on both platforms under the rule of Q-E, the Mac half
following the Windows half inside the same task, since the platform layer is written once and the second
platform mostly verifies.

- [ ] 5.1.6 (+) Spike: UE 5.8 on this Mac builds and opens a fresh C++ project with Xcode 27.0 on macOS
      27.0; if it refuses, Xcode 26.1.1 is installed next to it and selected. Recorded in `docs/UNREAL.md`
      with the versions that worked. Nothing of the probe project is committed.
- [ ] 5.1.7 (+) The ThirdParty build on macOS: the Release build of the deterministic libraries, Jolt and
      ENet as arm64 `.a` archives with the determinism flags of X.2, headers included, copied into the
      plugin's `ThirdParty/lib/Mac/`. One CMake-driven procedure with a thin wrapper per platform, as Q-B
      shapes the CI. Done when: the script produces the archives from a clean tree on the Mac and
      `module_determinism` passes over that build directory.
- [ ] 5.1.8 (+) `Unison.uplugin` allows `Win64` and `Mac`; `Unison.Build.cs` links `.lib` on Win64 and `.a`
      on Mac; the Jolt configuration header of 5.1.2 is generated per platform, since on arm64 Jolt selects
      NEON and none of the SSE macros. Done when: the plugin compiles in UE 5.8 on the Mac.
- [ ] 5.1.9 (+) 5.1.3's boundary probe on the Mac, compiled on purpose with `-ffp-contract=on`, the host's
      default, so that any simulation code reaching a host translation unit would be fused and the probe
      would fail. Done when: the probe links on the Mac as it does on Windows.
- [ ] 5.1.10 (+) The sample project `integrations/unreal/UnisonArena` builds and opens in the Mac editor
      (5.1.5's Mac half). Done when: it opens without a warning about the plugin.
- [ ] 5.2.11 (+) The automation tests of 5.2.2, 5.2.3 and 5.2.4 run on the Mac; 5.2.2 toggles flush-to-zero
      through FPCR by way of X.4's `fp_control_word.hpp` and sees the warning. Done when: the three tests
      pass in the Mac editor.
- [ ] 5.2.12 (+) Two PIE instances on the Mac through a local `unison_relay` (5.2.9's Mac half). Done when:
      two instances play together with the debug HUD of 5.2.8 showing equal verified frames.
- [ ] 5.2.13 (+) The cross-platform Unreal run: a UE client on the Mac and a UE client on Windows through
      one relay, ten minutes, zero desyncs; then a UE client on the Mac with the Windows console.
      Recorded in `docs/UNREAL.md`; Definition of Done item 7 holds on both platforms and item 10 names
      the Unreal hosts as well.

### 8.4 Risks particular to the plugin

| Risk | Signal | Fallback |
|---|---|---|
| UE 5.8 refuses Xcode 27.0 or macOS 27.0 | 5.1.6 | Xcode 26.1.1 next to 27.0, selected with `xcode-select`; if macOS 27 itself is the obstacle, the plugin's Mac half waits for a UE release that lists it, and nothing else in the plan waits. |
| Unreal's Mac toolchain compiles host code with contraction on | by design; 5.1.9 proves nothing of the simulation is compiled that way | The boundary rule of §7.3; the same risk the charter's §16 already lists for the Windows toolchain. |
| Our libraries are built without exceptions and RTTI, Unreal's Mac build may carry either | none expected | libc++ has no switch like MSVC's `_HAS_EXCEPTIONS`, so the archives link either way, and the boundary carries POD only. |
| The launcher and the engine fill the disk | 8.2 | 213 GB free today; the engine takes tens of gigabytes. |

---

## 9. Record

### 9.1 Goldens across platforms

| Golden | Commit | Windows Debug | Windows Release | macOS Debug | macOS Release |
|---|---|---|---|---|---|
| Physics pile `0x214BC6AEDC1EFBB3` | | | | | |
| `tests/golden/arena_scripted.checksums` | | | | | |
| Text map of a new match of two | | | | | |
| `tests/golden/arena_config.hashes` | | | | | |
| `tests/golden/protocol.bytes` | | | | | |

### 9.2 Console runs on the Mac (X.7.6)

| Date | Commit | Round trip | Verified vs predicted | Keys held together | Result |
|---|---|---|---|---|---|

### 9.3 Cross-platform LAN runs

Recorded in `docs/LAN_TEST.md`; this table points at them.

| Date | Relay on | Console A | Console B | Round trip | Verified at the end | Exit codes | Result |
|---|---|---|---|---|---|---|---|
