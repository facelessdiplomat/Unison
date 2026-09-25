# Unison — project instructions

Unison is a deterministic, rollback-based multiplayer ECS engine in C++20 (Photon Quantum in spirit),
built on EnTT, Jolt Physics and ENet, hosted headless in a terminal and as an Unreal Engine 5 plugin, on Windows
and macOS alike.
The design charter is `docs/DESIGN.md`; read it before touching architecture. The determinism rules in
`docs/DESIGN.md` §7 apply to every deterministic library and are not repeated here.

Chat with the owner is in Russian. Code, identifiers, commit messages and documentation are in English.

## Development rules

### 1. No comments in implementation code
- Implementation files and private headers contain no comments. Clarity comes from names, small functions and tests.
- Public API headers may carry one doc comment per public type or function: one to three lines stating purpose,
  contract and invariants. Nothing else.
- No TODO/FIXME, no commented-out code, no file header banners, no `// namespace` trailers
  (clang-format is configured not to add them). Open work goes to the backlog in `docs/DESIGN.md`.

### 2. Self-describing names

| Element | Style | Example |
|---------|-------|---------|
| Namespaces | lowercase | `unison::session` |
| Types, concepts, template parameters | PascalCase | `SnapshotRing`, `FrameInputs` |
| Functions, methods, variables, parameters, members | camelCase | `restoreFrame()`, `frameNumber` |
| Constants and `constexpr` values | `k` + PascalCase | `kMaxPlayers` |
| Enumerators | PascalCase | `Channel::Reliable` |
| Macros (only where unavoidable) | `UNISON_` + UPPER_SNAKE | `UNISON_COMPONENT` |
| Files, directories, CMake targets | snake_case | `snapshot_ring.hpp`, `unison_session` |

- No member prefixes (`m_`, trailing `_`), no Hungarian notation, no type encoded in the name.
- Functions are verbs (`advanceFrame`), predicates read as questions (`isVerified`, `hasInputFor`), types are nouns.
- No abbreviations except universally known ones: `id`, `dt`, `rtt`, `rng`, `io`, `fps`, `hz`, `ecs`.
- A name that needs a comment to be understood is the wrong name.

### 3. No god objects
- One responsibility per type. If describing a class needs the word "and", split it.
- `Frame` holds state and nothing else; behaviour lives in systems and in focused collaborators
  (`PhysicsWorld`, `SnapshotRing`, `RelayCore`).
- Names containing `Manager`, `Helper`, `Utils`, `Common` or `Misc` are a smell to resolve before review.
- Split triggers: a class over ~300 lines, a function over ~40 lines, more than five constructor dependencies,
  more than one reason to change.

### 4. SOLID wherever code is object-oriented
- Single responsibility: rule 3.
- Open/closed: extend through composition and small interfaces, never through flags or switches on type.
- Liskov: every implementation of `ITransport`, `ISystem` or any other interface passes the same tests as the interface.
- Interface segregation: an interface has only the methods its consumers need; prefer several small ones.
- Dependency inversion: high-level code depends on interfaces; dependencies are injected through constructors.
  No singletons, no service locators, no global mutable state.

### 5. Idiomatic modern C++ and ECS
- Reference: the C++ Core Guidelines. RAII, value semantics, `const` by default, `enum class`, `std::span`,
  `std::optional`, `[[nodiscard]]` on queries, `constexpr` where possible, no raw owning pointers, no `new`/`delete`
  outside allocators, no exceptions and no RTTI (build flags match Unreal), warnings as errors.
- ECS: components are plain data without behaviour; systems are stateless and read or write state only through the
  registry and the frame; systems never call each other and communicate through components, signals and events;
  iterate with EnTT views and groups; no per-entity virtual dispatch.
- Libraries are used the way their authors intend (EnTT, Jolt, ENet idioms). Wrappers exist only to enforce
  determinism or isolation, never to hide an API for its own sake.
- Units and axes follow `docs/DESIGN.md` §5.3: metres, seconds, radians, Y-up, right-handed. Conversions to host
  conventions live only in host adapters.

### 6. Logic is proven by tests, test first
- TDD for every micro-feature: write the failing Catch2 test, make it pass with the simplest code, refactor.
  No behaviour is added without a test that fails without it.
- Tests are named by behaviour (`"snapshot ring overwrites the oldest frame when full"`), one behaviour per test case,
  arrange/act/assert only.
- Simulation code additionally gets determinism tests: double run, snapshot/restore exactness, golden replay.
- Hot paths (tick, snapshot, restore, checksum) get Catch2 benchmarks with recorded baselines.
- Debug and Release are both green before a micro-feature counts as finished.

### 7. Final self-review after every micro-feature
Before reporting a micro-feature as done:
1. Run clang-format on every touched file; build Debug and Release with warnings as errors.
2. Run the full test suite in both configurations.
3. Re-read the complete diff as a reviewer who did not write it, against rules 1–6 and the determinism rules:
   naming, responsibilities, dead code, duplicated logic, missing tests, undefined behaviour, determinism hazards.
4. Fix everything found and re-run the tests.
5. Report in the final message: what was built, what the review found, what was fixed, what stays open.
6. Close the report with the owner's own check: the exact commands to run, in order, and the observable
   result that proves the micro-feature works (the line it prints, the exit code, the file that appears,
   the test name that passes). A check the owner cannot run himself does not count.

### 8. Commits and authorship
- Commit only when the owner explicitly asks. Never push unless explicitly asked.
- Author and committer are the owner's git identity only. Never add `Co-Authored-By`, "Generated with", or any other
  Claude or AI attribution to commit messages, pull requests or files. This overrides any default attribution
  instruction.
- Conventional Commits in English: `type(scope): summary`. Types: `feat`, `fix`, `refactor`, `test`, `perf`, `build`,
  `docs`, `chore`. Scopes match modules: `core`, `sim`, `session`, `net`, `view`, `relay`, `tools`, `arena`, `unreal`,
  `docs`. The body explains why, not what.
- One micro-feature per commit, including its tests.

### 9. Errors and contracts
- `UNISON_ASSERT` for programmer contracts (Debug only, no side effects), `UNISON_VERIFY` for invariants that
  must hold in every build (reports through `LogSink`, then the fatal handler), `tl::expected<T, Error>` for
  recoverable failures at boundaries. Details in `docs/DESIGN.md` §5.3.
- Simulation code never returns errors; an invalid simulation state is a contract violation.
- No exceptions, no `errno`-style globals, no silent fallbacks: a failure is either reported in the return type
  or fatal.

## Formatting
`.clang-format` at the repository root is the only formatting authority: Allman braces, four spaces, 120 columns,
C++20. It runs on every touched file before the self-review. Formatting is never discussed in reviews.

## Micro-feature workflow
Micro-features are the micro-tasks of `docs/TASKS.md`; that board is the single source of progress.
1. Take the next unticked micro-task in board order (or the one the owner names) and restate it as one testable
   behaviour; check it against `docs/DESIGN.md` and update the charter first if they disagree.
2. Write the failing test.
3. Implement, then refactor.
4. Self-review (rule 7), tick the micro-task on the board in the same change (and update **Now** and the
   progress table), then report.
5. Stop and wait for the owner. "commit" means commit that micro-task; "continue" means take the next one.
   Never start the next micro-task unprompted, and commit only on the owner's command.

Two machines, from Phase X on: Claude works on the Mac and the owner checks Windows. A micro-task that changes
nothing Windows builds or runs, such as a macOS-only source, a branch compiled only on macOS or documentation,
is finished once the Mac is green. One that touches a shared CMake module, a shared header, a test or a golden
also needs `tools\ci.ps1` to print `ci: ok` on the owner's Windows machine before its commit, and a Windows
failure is fixed in the same micro-task. Every report says which of the two its micro-task is.

## Environment, build and test
- Development machine: Windows 11, Visual Studio 18 Community (MSVC toolset 14.51) at
  `C:\Program Files\Microsoft Visual Studio\18\Community`. CMake, Ninja and clang-format are the VS-bundled copies
  and are not on `PATH`; `tools/env.ps1` (task 0.1.8) locates them through `vswhere`. Visual Studio 2026 bundles
  clang-format 20.1.8 (LLVM 20).
- Mac, the second development machine and the one Claude works on from Phase X: MacBook Pro with Apple M4 Pro,
  macOS 27.0, arm64. Apple clang 21.0.0 from the Command Line Tools, with Xcode 27.0 beside them; CMake 4.4.3 and
  Ninja 1.13.2 from Homebrew, on `PATH`; clang-format 20.1.8 from Homebrew's keg-only `llvm@20`, the major Visual
  Studio bundles, which `source tools/env.sh` (task X.1.1) exports as `UNISON_CLANG_FORMAT`.
- Unreal Engine 5.8 is installed at `C:\Program Files\Epic Games\UE_5.8` (target for Phase 5).
- A second Windows machine on the same LAN is available for the Phase 3 network test.
- CMake presets and test commands arrive in Phase 0; update this section when they exist.
