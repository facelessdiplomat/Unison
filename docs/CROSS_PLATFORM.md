# Unison — Cross-platform plan: macOS port and Windows ↔ macOS play

A branch off the board in `docs/TASKS.md`, written on 2026-09-25 on the owner's word. It exists because
the project has to leave its single platform: the engine, the relay, the runner and the console must build
and run on macOS as they do on Windows, and, the owner's second word of the same day, a client on Windows
and a client on macOS must play one match through one relay without a desync. That second requirement is
what makes this more than a port: the simulation must be bit-exact between MSVC on x64 with SSE2 and
Apple clang on arm64 with NEON.

This file holds the analysis, the decisions the owner took, the rhythm of working across two machines and
the record of the cross-platform runs. The micro-tasks were planned here and live on the board since X.0.2,
under Phase X of `docs/TASKS.md`, as the board requires; see §6.

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

**Exit criteria of the phase**, ticked on the board under Phase X:

- `tools/ci.sh` prints `ci: ok` on the Mac and `tools\ci.ps1` prints `ci: ok` on Windows, on the same
  commit: both configurations configure, build with warnings as errors, pass `ctest` and pass the
  formatting check on both platforms.
- Every golden recorded on Windows verifies on macOS in Debug and in Release: the physics pile, the
  scripted arena, the text map, the arena's session config hashes and the protocol bytes (X.6).
- A Windows client and a macOS client play ten minutes through a relay on Windows, then ten minutes
  through a relay on the Mac, both runs with zero desyncs, recorded in `docs/LAN_TEST.md` (X.8).
- The charter, the board, `README.md` and `CLAUDE.md` describe both platforms and the two-platform rule.

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

Each question has a recommendation. Where the owner chooses otherwise, the micro-tasks of Phase X change
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
  micro-task. Amended on 2026-09-25 under the loop of D36: a golden that is new rather than re-recorded may be
  recorded on the Mac while the Windows machine is out of reach; the owner's run on Windows is then its check,
  and a disagreement there is located by X.6.8, never settled by recording again.
- **Q-H. The fallback.** If the arm64 build cannot be made to match within a bounded effort (§7), an x86_64
  build under Rosetta 2 runs the same SSE2 instructions as Windows and would still give a Mac that plays,
  though not natively. **Recommended:** not before X.6 has shown a real, located difference; the owner
  decides when.
- **Q-I. The Windows LAN test, 3.4.5.** Definition of Done item 2 asks for two machines, not two Windows
  machines. **Recommended:** X.8.2, the run with a relay on Windows and consoles on both platforms,
  satisfies item 2 as well, and 3.4.5 is ticked from it; a second Windows machine is then not needed.

---

## 5. The work: Phase X — Cross-platform: macOS

The micro-tasks planned here live on the board since X.0.2, under Phase X of `docs/TASKS.md`: ten tasks, from
X.0, the charter and the board, through the toolchain, the compiler contract on clang, the dependencies, the
floating-point environment on arm64, the port proper, the goldens across the platforms, the console on macOS
and the LAN run, to X.9, closing. They are ticked, amended and appended there like those of every other
phase; their order and the rules of the work are in §6.

---

## 6. Order, rhythm and where progress is recorded

- **Order.** X.0 → X.1 → X.2 → X.3 → X.4 → X.5 → X.6 → X.7 → X.8 → X.9. X.4 may run before X.3 when
  `unison_core` is the first thing that fails to build; X.7.1 may run inside X.5.6 if the stub is awkward
  without it. Nothing in X.6 starts before X.5.9.
- **Two machines.** Claude works on the Mac. A micro-task is reported with the Mac's own check and, per
  Q-E, either "macOS-only change" or "needs the owner's Windows check: `tools\ci.ps1` prints `ci: ok`".
  The owner runs that check before saying "commit" for the second kind. A Windows failure is fixed in the
  same micro-task, never in a later one.
- **Configuring by hand.** Until X.2.9 brings the presets, the Mac is configured with
  `cmake -S . -B build/clang-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.
- **The port proper, X.5.** One micro-task per group of targets; each fixes what Apple clang rejects or warns
  about in that group, with `-Werror`, and nothing else. A change that alters behaviour on Windows is a `(+)`
  micro-task of its own, never a side effect.
- **The goldens, X.6.** The goldens were recorded on Windows and X.6 does not touch them (Q-G). Each of its
  micro-tasks either passes on the Mac in Debug and Release, or locates the first difference and stops for
  the owner.
- **Progress.** The micro-tasks of §5 and §8.3, their ticks and the **Now** pointer live on `docs/TASKS.md`
  since X.0.2. This file keeps the analysis of §3, the decisions of §4 and the record of §9.
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

Appended ids, as the board asks of work discovered later. The existing micro-tasks of Phase 5 stay as they are and are
done on both platforms under the rule of Q-E, the Mac half following the Windows half inside the same task, since the
platform layer is written once and the second platform mostly verifies.

They are on the board under Phase 5 since X.0.2: 5.1.6 checks UE 5.8 against the Mac's Xcode first, 5.1.7 to
5.1.10 build the ThirdParty archives for arm64, the plugin, its boundary probe and the sample project on the
Mac, and 5.2.11 to 5.2.13 run the automation tests, two PIE instances through a local relay and the Unreal run
across the two platforms.

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
| Physics pile `0x214BC6AEDC1EFBB3` | recorded on Windows; Mac at `103e873` | waits for the owner's run | waits for the owner's run | passes | passes |
| `tests/golden/arena_scripted.checksums` | recorded on Windows; Mac at `103e873` | waits for the owner's run | waits for the owner's run | passes | passes |
| Text map of a new match of two | recorded on Windows; Mac at `103e873` | waits for the owner's run | waits for the owner's run | passes | passes |
| `tests/golden/arena_config.hashes` | recorded on the Mac at `bed30dd` | waits for the owner's run | waits for the owner's run | passes | passes |
| `tests/golden/protocol.bytes` | recorded on the Mac at `103e873` | waits for the owner's run | waits for the owner's run | passes | passes |

### 9.2 Console runs on the Mac (X.7.6)

| Date | Commit | Round trip | Verified vs predicted | Keys held together | Result |
|---|---|---|---|---|---|

### 9.3 The keyboard on the Mac (X.7.2)

Read on 2026-09-25. `CGPreflightListenEventAccess()`, which asks without prompting, answers that the shell Claude
works in holds no Input Monitoring. The game keys are `kVK_ANSI_W` `0x0D`, `kVK_ANSI_S` `0x01`, `kVK_ANSI_A`
`0x00`, `kVK_ANSI_D` `0x02`, `kVK_Space` `0x31`, `kVK_ANSI_F` `0x03`, `kVK_ANSI_Q` `0x0C` and `kVK_ANSI_E` `0x0E`
in HIToolbox's `Events.h`. Claude cannot hold a key down, and posting one would type into the owner's session,
so the Terminal.app half, whether a key held there reads as down with and without the permission, is read off the
console of X.7.3 by hand: it says at start whether its terminal holds Input Monitoring.

### 9.4 Cross-platform LAN runs

Recorded in `docs/LAN_TEST.md`; this table points at them.

| Date | Relay on | Console A | Console B | Round trip | Verified at the end | Exit codes | Result |
|---|---|---|---|---|---|---|---|
