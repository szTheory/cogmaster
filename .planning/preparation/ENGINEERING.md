---
title: "Engineering: C rules, tests, CI, release, repo settings and quality loops"
summary: "Build, test, CI, release, repository, triage, performance, portability and documentation decisions for cogmaster, with mutable facts re-verified on 2026-10-18 and the sibling lessons folded in as rules with checks."
read_when: "Before creating or changing CMake files, presets, tests, workflows, repository settings, the release pipeline, quality gates or engineering docs."
updated: 2026-10-18
---

# Engineering: C rules, tests, CI, release, repo settings and quality loops

Mutable facts were re-read on 2026-10-18 [ENG.01–ENG.16]; sibling practice is cited by repo and commit [ENG.17–ENG.22]; sibling lessons by ID in `.planning/LESSONS.md` [ENG.28]; (tested) marks runs made here [ENG.23]. Choices read **Rec** (recommendation), **Alt** (strongest alternative), **Flip** (the observation that would change it). muhtari's ENGINEERING.md is the direct ancestor [ENG.17]; what differs is SMS/GG specific or a folded lesson.

## Where to look
| Question | Section |
|---|---|
| Runner labels and vCPUs, action SHAs, tool versions, release-please semantics, GitHub token rules | 1 |
| Flags, presets, integer-only / no-globals / no-UB checks, analyzers, coverage, WLA-DX fixture tool | 2 |
| Test tiers, exact inventory, no-skip, negative controls, property tests, fuzz targets | 3 |
| Workflows, jobs, aggregator, triggers, timeouts, caching, cores, minutes per PR | 4 |
| release-please config, App token, archives, attestations, rehearsal, withdrawal | 5 |
| Repo settings script, ruleset, branch policy; issue forms, labels, digest, Dependabot, YAGNI | 6, 7 |
| Ir/frame gate, wall-clock tails, frame budgets, allocations, `quality.json` ratchet | 8 |
| Big-endian, 32-bit and console lanes; style contract | 9, 10 |
| Docs budgets, evidence format; milestone tune-up; local paths and secrets; WASM and low-power | 11, 12, 13, 14 |
| Which sibling lesson became which rule and check | 15 |

## 1. Re-verified facts (2026-10-18)
**Runners** (standard, public repo, free) [ENG.01][ENG.02][ENG.03]:
| OS × arch | Label | vCPU / RAM | On the image (image version) | Notes |
|---|---|---|---|---|
| Linux x64 | `ubuntu-26.04` | 4 / 16 GB | GCC 13.4, 14.3, 15.2; Clang, clang-format, clang-tidy 20.1.8, 21.1.8, 22.1.2; CMake 4.4.4; Ninja 1.13.2; jq 1.8.1 (20261012.163.1) | GA 2026-09-17 (#14747); `ubuntu-latest` still = `ubuntu-24.04` this month, moves to 26.04 in Nov 2026 (#14748) |
| Linux arm64 | `ubuntu-26.04-arm` | 4 / 16 GB | same | |
| macOS arm64 | `macos-15` | 3 (M1) / 7 GB | Apple clang 17.0.0; Homebrew `llvm@18` 18.1.8; CMake 4.4.3 (20260907.0337.1) | `macos-latest` = `macos-26` |
| macOS x64 | `macos-15-intel` | 4 / 14 GB | Apple clang 17.0.0; CMake 4.4.2 (20260824.0482.1) | x86_64 offered until August 2027 (#13045); `macos-26-intel` listed |
| Windows x64 | `windows-2025` | 4 / 16 GB | VS 2026 18.10; LLVM 20.1.8; CMake 4.4.4 (20261007.264.1) | `windows-latest` = `windows-2025` = `windows-2025-vs2026` |
| Windows arm64 | `windows-11-arm` | 4 / 16 GB | VS 2026 18.10; LLVM 22.1.8; CMake 4.4.4 (20261013.183.4) | resolves to the `Windows11-VS2026-Arm64` image since September 2026 (#14602) |

Change since muhtari's 2026-10-15 read: Ubuntu 26.04 and Windows x64 moved to CMake 4.4.4 (was 4.4.3); every other value held (tested) [ENG.02].
- **Rec** these six, Ubuntu 26.04 for every Linux job. **Alt** Ubuntu 24.04. **Flip** a 26.04 defect costs a red day, or 26.04-arm queue p95 > 2 min.

Apt-installable on 26.04 ("resolute"), not preinstalled [ENG.13]: Valgrind 3.26.0, cppcheck 2.19.0, `qemu-user` 10.2.1, `gcc-{powerpc,arm}-linux-gnu(eabihf)` 15.2 (tested, unchanged).

**Actions**: pin the full SHA with a `# vX.Y.Z` comment. muhtari's live pins [ENG.17] were compared with `releases/latest` and `git/ref/tags` on 2026-10-18 (tested) [ENG.04]; no newer release exists for any of them:
| Action | muhtari pin | Latest release | Commit SHA |
|---|---|---|---|
| actions/checkout | v7.0.1 | v7.0.1 (2026-07-20) | `3d3c42e5aac5ba805825da76410c181273ba90b1` |
| actions/upload-artifact | v7.0.2 | v7.0.2 (2026-10-07) | `cf430e030ddbb5b0abf93d22962f4752f3646cd9` |
| actions/download-artifact | v8.0.2 | v8.0.2 (2026-10-07) | `9000827ccba6bdab643e8b6fd33ac0654aef8333` |
| actions/attest | v4.2.2 | v4.2.2 (2026-08-04) | `1e69f48acb82d1966a394da916b4c1698aa569d6` |
| actions/create-github-app-token | v3.3.0 | v3.3.0 (2026-10-12) | `64ab729a6294000f5aa4a4bc0b73d1f32bc45510` |
| googleapis/release-please-action | v5.0.0 | v5.0.0 (2026-04-22) | `45996ed1f6d02564a971a2fa1b5860e934307cf7` |
| dependabot/fetch-metadata | v3.1.0 | v3.1.0 (2026-04-20) | `25dd0e34f4fe68f24cc83900b1fe3fe149efef98` |
| actions/attest-build-provenance (do not use; a wrapper over `actions/attest`) | none | v4.2.2 (2026-08-06) | `4d101475d8b20a2381f78447822ac1eab6504dd8` |

`create-github-app-token` takes `client-id` (`app-id` deprecated) and `permission-<name>: write` inputs that narrow the token [ENG.11]; `actions/attest` with no predicate makes SLSA build provenance and accepts `subject-checksums` [ENG.12].

**release-please**: the action's `package.json` depends on `release-please` `^17.6.0` and its `package-lock.json` resolves 17.6.0 (re-read at v5.0.0 on 2026-10-18); the library is at 17.11.2 (2026-08-24). The 17.6.0 → 17.11.2 changelog touches dependencies and other strategies, not `simple`, the pre-major bump options, `initial-version`, `draft` or `force-tag-creation`, so muhtari's reading of 17.6.0 is exactly what the action runs [ENG.05][ENG.06]:
| Option | Behaviour |
|---|---|
| `release-type: simple` | edits `version.txt` (must exist) and `CHANGELOG.md`; the manifest releaser edits `.release-please-manifest.json` |
| `bump-minor-pre-major: true`; `bump-patch-for-minor-pre-major: false` | below 1.0.0 a breaking change bumps minor; `feat` bumps minor |
| `initial-version: "0.1.0"` | used only when no release exists; manifest `"0.0.0"` counts as none |
| `draft: true`, `force-tag-creation: true` | draft release whose tag exists at once |
| `Release-As: x.y.z` footer | newest such footer since the last release wins; config key `release-as` is deprecated and sticks |

**GitHub behaviour** [ENG.07][ENG.08][ENG.09][ENG.10][ENG.22]:
- An event caused by `GITHUB_TOKEN` starts no workflow run (PR `opened`/`synchronize`/`reopened` runs wait for approval); an App token avoids both.
- Dependabot-triggered runs get a read-only `GITHUB_TOKEN` and only Dependabot secrets; whether `vars` resolve there is (unverified).
- Immutable releases: tag and assets lock at publish; a deleted immutable release's tag name is never reusable, even in a re-created repository; title, notes, pre-release and latest flags stay editable; publishing also creates a release attestation; attach every asset to the draft first (docs at `dae83f7d28f3`, accessed 2026-10-18).
- `PUT /repos/{o}/{r}/actions/permissions` takes `sha_pinning_required` ("actions must be pinned to a full-length commit SHA"); `…/permissions/workflow` takes `default_workflow_permissions` (`read`|`write`) and `can_approve_pull_request_reviews` (accessed 2026-10-18) [ENG.10].
- A head commit message with `[skip ci]`, `[ci skip]`, `[no ci]`, `[skip actions]`, `[actions skip]` or `skip-checks:true` starts no `push`/`pull_request` run. GSD `/gsd-ship` titles the PR `Phase N: Name` and pushes a `[ci skip]` ship note.

**Fixture tool: WLA-DX** (`wla-z80` + `wlalink`) [ENG.14]: latest release v10.7 (2026-06-28), `master` at `7ae11df1657d` (2026-10-07), GPL-2.0-or-later. It is a build tool, never linked and never seeded: its licence does not reach the assembled fixtures or the core (AGENTS.md rule 1 covers emulator source). CI fetches the pinned source tarball, checks its SHA-256 before extraction, builds it with CMake in `build/tools/wla-dx/`, and `tests/roms/manifest.txt` records each fixture's SHA-1 as assembled; a digest mismatch fails configure through `cmake/WlaDx.cmake`. Signature or git-chain verification stays off the critical path (sibling lesson XFER-056).
| Item | Value |
|---|---|
| Tarball URL | `https://github.com/vhelin/wla-dx/archive/refs/tags/<tag>.tar.gz` |
| Tag and SHA-256 | (pin recorded in SMSPOWER-EXTRACTS 7) |
| Negative control | `fixtures.wla_pin.self_test`: a wrong digest fails with `WLA-DX digest mismatch` |

## 2. C rules, flags and presets
C17 with `CMAKE_C_EXTENSIONS OFF`; `cmake_minimum_required(VERSION 3.25)` for embedders; `CMakePresets.json` version 11 (CMake ≥ 4.3, on every chosen image) so test presets can say `"jobs": ""` [ENG.16]; one entrypoint `cmake --workflow --preset <lane>` locally and in CI [ENG.17].
| Rule | Check |
|---|---|
| Unsigned fixed-width guest arithmetic, cast to `uint32_t` before `<<` or `*`; no signed shifts, VLAs, recursion, bit-fields, packed structs, punning unions, FP | warnings; UBSan in `asan`; property tests |
| Integer-only core | `hygiene.no_float` scans `src/` and `include/`; preset `nofp` builds with `-mgeneral-regs-only` (GCC, x64 + arm64); `abi.undefined` runs `nm -u` against `tests/abi/allow-<os>.txt` (`memcpy memmove memset memcmp` plus per-OS helpers: Apple clang lowers `memset` to `_bzero`, ppc32 needs `__udivdi3`; sibling lesson XFER-060); a float fixture must fail `hygiene.no_float` and `nofp`; `abi.undefined`'s negative control is a libc fixture (a call to `abort`) |
| No mutable globals or statics | `abi.globals`: no `B b D d C` symbol in the Release static library; `llvm-nm` reads COFF. File-scope `const` tables hold integers only: under PIC a `const` pointer table lands in `.data.rel.ro` (`d`) [ENG.17], so mapper dispatch uses `switch` or pointers stored in the instance at load |
| One symbol namespace | `abi.symbols`: each defined external symbol is `cogmaster_*` declared in `include/cogmaster.h`, or internal `cogmaster__*` |
| Header is C and C++ | `abi.header.c17`, `abi.header.cxx17` (`-Wall -Wextra -Wpedantic -Werror`; MSVC `/TC`, `/TP`), `install.consumer`, `embed.subdirectory` |
| No host paths in binaries | `-ffile-prefix-map=<source dir>=.`; `release.no_build_paths` scans each archive |
| Hash and inventory files are LF | written with `file(CONFIGURE … NEWLINE_STYLE UNIX)`, never `file(WRITE)`; `hygiene.lf_only` rejects CRLF in `tests/**/*.txt` (sibling lesson XFER-061) |
| Git revision embedded at configure | `build.revision_matches_head` compares the embedded revision with `git rev-parse HEAD`; the CI preset configures after checkout; `docs/testing.md` says to reconfigure after every commit (sibling lesson GB-GSD-020) |

Warnings (GCC, Clang): `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wundef -Wcast-qual -Wimplicit-fallthrough -Wmissing-variable-declarations` [ENG.24]; GCC adds `-Wduplicated-cond -Wlogical-op`; MSVC `/W4 /utf-8`. Applied only when `PROJECT_IS_TOP_LEVEL`, fatal only through presets (`CMAKE_COMPILE_WARNING_AS_ERROR`). The set gave 0 warnings on nesturbator `src/` under Apple clang 21; `-Wswitch-default`, `-Wdeclaration-after-statement` and `-Wpadded` stay out (tested on 2026-10-15 [ENG.23]).

| Preset | Compiler and flags | Where | Tests |
|---|---|---|---|
| `dev` | Debug, host compiler | local | inventory `ci` |
| `ci`, `ci-msvc` | Release, warnings fatal; MSVC environment from vswhere + vcvarsall (no third-party action) | six legs | inventory `ci`, then package |
| `asan` | clang-21 `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`; `COGMASTER_SHORT_RUNS` shortens long runs, never omits a test; TIMEOUT × 3 | ubuntu x64 | inventory `ci` |
| `nofp` | GCC 15 + `-mgeneral-regs-only` | ubuntu x64, arm64 | label `abi` |
| `hygiene` | clang-format-21 (configure fails on another major), clang-tidy-21, jq, git, sh, dash; sets `CMAKE_OSX_SYSROOT` from `xcrun --show-sdk-path` on macOS because CMake 4 leaves it empty for Homebrew clang-tidy (sibling lesson XFER-060) | ubuntu x64 (local: Mac) | label `hygiene` |
| `be32` | `powerpc-linux-gnu-gcc` 15 + qemu-ppc as `CMAKE_CROSSCOMPILING_EMULATOR` | ubuntu x64 | label `portable` (section 9) |
| `wasm32` | wasi-sdk clang + wasmtime as `CMAKE_CROSSCOMPILING_EMULATOR` | ubuntu x64 | label `portable` (section 14) |
| `fuzz`, `coverage`, `vectors-full`, `oracle`, `bench` | clang-21 libFuzzer + ASan/UBSan; clang-21 source coverage; full SingleStepTests z80 set; reference binaries; Release | nightly | sections 3, 8 |
| `library` | Release; private paths from `.env.local` | owner's Mac | commercial dumps, real BIOS |

- **Analyzers. Rec** `clang-tidy-21 -checks='-*,clang-analyzer-*'` over `src/` in `hygiene`, zero findings (0 on nesturbator `src/`, 10 s with `bugprone-*`, 2 false positives (tested) [ENG.23]); GCC `-fanalyzer` and cppcheck at the tune-up. **Alt** no analyzer. **Flip** three `NOLINT`s needed → tune-up only; a tune-up run finds a real bug → per PR.
- **clang-format. Rec** 21 (preinstalled on 26.04, equals Homebrew `llvm@21`); LLVM base, indent 4, 100 columns, Linux braces [ENG.17]. **Alt** 18. **Flip** Linux jobs stay on 24.04.
- **Sanitizers. Rec** one ASan + UBSan lane; no MSan/TSan. **Alt** MSan nightly. **Flip** a hash mismatch traces to uninitialised memory.
- **Coverage. Rec** nightly llvm-cov: line % reported; function coverage of `src/` held at 100 %. **Alt** per-PR line ratchet (invites test-gaming [ENG.20]). **Flip** a shipped regression sat in uncovered lines twice.

## 3. Tests
| Tier | Trigger | Content |
|---|---|---|
| PR, required | every PR; heavy jobs skip on `.planning/**`-only changes | units, committed SingleStepTests `z80` sample, WLA-DX fixtures' frame/audio/state hashes, `fuzz.regress.*`, `policy.*`, `hygiene.*`, `abi.*`, `install.consumer`, `embed.subdirectory`, `libretro.host`, `rehearsal` when release inputs change (section 5) |
| Nightly | schedule, `workflow_dispatch`, PRs touching the nightly's inputs | full z80 vectors at their pin, libFuzzer, `suite-flake` (`--repeat until-fail:3 --schedule-random`), coverage, bench, `armhf`, oracles; red → one rolling `nightly` issue |
| Local | owner | `library` preset (dumps, real BIOS), one RetroArch session per milestone |

- **Exact inventory** (gabbaboy `cmake/ExpectedTests.cmake` [ENG.19]): `tests/inventory/<preset>.txt` holds sorted names; configure fails unless the directory `TESTS` property equals it. Subsystem files are pulled in with `include()`, never `add_subdirectory()`. One `ci.txt` serves all six legs.
- After the test step `tests/cmake/check_junit.cmake` requires each listed name exactly once with no `failure`, `error` or `skipped`. It never matches CTest's summary text: CTest 4 changed "100% tests passed" wording and CMake 4.4 changed ConfigVersion and `FATAL_ERROR` wrapping between minors; every acceptance check in plans and tests asserts on exit status and a tolerant regex, and `policy.no_exact_output` rejects literal tool summary strings in `tests/` (sibling lesson XFER-058).
- **No self-skip:** `policy.no_skip` rejects `SKIP_RETURN_CODE`, `SKIP_REGULAR_EXPRESSION` and `DISABLED`; a missing tool fails configure through `find_program(... REQUIRED)`.
- **Negative controls:** every gate test `x` has `x.self_test` through `tests/cmake/expect_fail.cmake -DCODE=<n> -DREGEX=<rule>: <file>`; bare `WILL_FAIL` is banned. A failing control must exit with its own code under ASan too: the fixture frees what it allocated before failing, and `expect_fail.cmake` runs it with `ASAN_OPTIONS=detect_leaks=0`, because LSan otherwise replaces the exit code with 23 (sibling lesson GB-CGB-002); `asan` runs every `*.self_test`.
- **Registration:** `cogmaster_add_test(NAME … LABELS … TIMEOUT … COST …)` in `cmake/CogmasterTests.cmake`; after all `include()`s, one pass over the directory's `TESTS` property multiplies every TIMEOUT by `COGMASTER_TIMEOUT_SCALE` (3 under sanitizers, the measured factor under qemu and wasmtime), so a test registered by a helper that forgot the scale is still scaled (sibling lessons GB-CI-004/005); `policy.timeout_scaled` checks that no test under `asan` has the unscaled value. One file per subsystem, `tests/{z80,bus,vdp,psg,fm,io,cart,mapper,bios,state,runner,libretro,policy,fuzz,perf,release}/tests.cmake`, each ≤ 400 lines. Names are `<subsystem>.<behaviour>`, never phase, plan or requirement IDs.
- **Property tests. Rec** in-repo `tests/prop.h` (~60 lines): splitmix64 seeded per test; failure prints seed, case and repro; 100 cases per PR, 10,000 nightly; size-halving shrink. Properties: `run(N)` then `run(M)` equals `run(N+M)`; save → load → continue equals uninterrupted; state round trip byte-identical; a rejected load or state leaves the instance unchanged; `probe` is pure; two instances never interact; input applies only at the next frame start; SMS and GG profiles of the same ROM differ only where the profile says. **Alt** `theft` v0.4.5 (ISC, `fork`/`waitpid`, no MSVC) [ENG.15]. **Flip** hand-minimising a failure takes > 30 min twice.
- **Fuzz corpus. Rec** seeds generated at build time by `tests/fuzz/seeds.c`; only minimised regressions committed; `tests/fuzz/**` ≤ 512 KiB. **Alt** commit evolved corpora (nesturbator's is 9.0 MB). **Flip** nightly coverage plateaus → nightly artifact, still uncommitted.

Fuzz targets, each an `LLVMFuzzerTestOneInput` plus a replay `main` registered as `fuzz.regress.<target>` on all six legs; nightly 60 s per target, clang-21 + ASan/UBSan, `-timeout=2 -max_len=524800` (512 KiB ROM + 512-byte dump header, derived):
| Target | Entry | Arrives with |
|---|---|---|
| `fuzz.loader` | `cogmaster_probe` + `cogmaster_load`: ROM image plus header probe (TMR SEGA header, product code, region and size fields, 512-byte copier header, mapper guess) | cartridge loader |
| `fuzz.state` | `cogmaster_load_state` after a fixed load (validate, then apply) | save states |
| `fuzz.save` | battery-RAM image given to `cogmaster_load_save` (size, mapper mismatch) | battery saves |
| `fuzz.run` | random bytes as a ROM, fast boot, 2 frames: no crash, UB or timeout | cartridge loader |

## 4. CI design
Workflows: `ci.yml` (required), `nightly.yml`, `release.yml`, `digest.yml` (weekly, section 7), `dependabot-automerge.yml`.
| Job (`ci.yml`) | Runner | Content | Skipped when |
|---|---|---|---|
| `changes` | ubuntu x64 | `heavy=false` only if every changed path, old and new name, is under `.planning/`; empty or unknown → heavy; `ci.classify` CTest covers mixed, renamed and empty lists; `release_inputs=true` when section 5's rehearsal inputs changed | never |
| `hygiene`, `title` | ubuntu x64 | preset `hygiene`; live PR title and body via `gh pr view` | `title`: not a PR |
| `build` × 6 | section 1 | `ci`/`ci-msvc`; archive check; `hashes.txt`; README install line on macOS arm64 | light |
| `asan`, `nofp` × 2, `be32`, `wasm32` | ubuntu | presets of the same names | light |
| `hash-equality` | ubuntu x64 | eight byte-identical `hashes.txt` (six legs + `be32` + `wasm32`) | light |
| `perf`, `quality` | ubuntu x64 | section 8 | light; `perf` also when not a PR |
| `rehearsal` | ubuntu x64 | section 5 | `release_inputs=false` |
| `CI required` | ubuntu x64 | `if: always()`; jq over `toJSON(needs)`: every result `success`, except heavy jobs `skipped` when `heavy=false`, `perf` outside PRs and `rehearsal` when its inputs did not change; a CTest runs the expression on synthetic `needs` | never |

- **Aggregator. Rec** one required check, `CI required`. **Alt** each job required (19 contexts on exifcleaner-node [ENG.17]). **Flip** the aggregator lets one real failure through.
- **Matrix. Rec** all six legs on every PR: identical output on six platforms is a product claim. **Alt** three per PR. **Flip** measured queueing dominates the critical path.
- **`.planning` skip. Rec** job-level skip via `changes`, aggregator still reporting; no trigger-level `paths` (a skipped workflow leaves the required check pending). **Alt** run everything (a `.planning`-only PR cost 15 jobs and 3,089 job-seconds on nesturbator [ENG.18]). **Flip** the classifier lets a code change through.
- **Title. Rec** a regex job inside `ci.yml`: `^(feat|fix|perf|docs|test|build|ci|chore|refactor|style|revert)(\([a-z][a-z0-9._-]*\))?!?: [a-z]`, so `feat(02-03):` and `Phase N: Name` fail (gabbaboy PR #10 reached `main` as `Phase 6: …` [ENG.19]); a second pattern rejects `feat`/`fix`/`perf` scoped `ci`, `release`, `build`, `test` or `docs` (lattice_stripe cut ten patch releases fixing its pipeline [ENG.25]); the job scans both the live title and the live body for every skip token of section 1, because the squash body becomes `main`'s commit message and a token there silences the release run (sibling lesson XFER-063); `ci.title` CTest holds title and body fixtures. **Alt** `amannn/action-semantic-pull-request` (not required on lattice_stripe, so bad titles reached `main` [ENG.21][ENG.25]). **Flip** title rules outgrow one regex.
- **Shell. Rec** every script in `scripts/` and `tests/` is POSIX sh, run by `hygiene.dash` under `dash` as well as the default shell; a script that reads a pipe under `pipefail` consumes all of stdin before any early exit (`gh api … | jq` dies with SIGPIPE otherwise, sibling lesson XFER-062), checked by `policy.sh_stdin` feeding a 1 MiB pipe to each script's early-exit path; no stateful shell or Perl harness wraps RetroArch (sibling lesson XFER-067). **Alt** bash with `set -euo pipefail`. **Flip** a POSIX-only script exceeds 150 lines (then a CMake `-P` script).
- **Triggers:** `pull_request` (opened, synchronize, reopened, edited), `push` to `main`, `workflow_dispatch`, `workflow_call`. **Concurrency** `ci-${{ github.workflow }}-${{ github.event.pull_request.number || github.sha }}`, `cancel-in-progress` only for PRs; `release.yml` has its own group with `queue: max` (untested) and no `cancel-in-progress` (a newly queued run otherwise cancels the pending one [ENG.27]; three lattice_stripe release runs ended that way [ENG.25]).
- **Permissions:** workflow `contents: read`; `title` adds `pull-requests: read`; every checkout `persist-credentials: false`. **Pins:** full SHA everywhere; `sha_pinning_required: true` in repo settings, watched by `scripts/apply-repo-settings.sh --check` (nesturbator's live value was `false` although its doc said on [ENG.18]).
- **Timeouts:** twice the slowest successful run among the last 40, rounded up to minutes, with run ID and sample count in a comment; 5-minute floor for network-bound and roll-up jobs; `provisional` until 10 samples; `hygiene.timeout_comments` enforces it [ENG.17].
- **Cores:** Ninja default parallelism; test presets `"jobs": ""` [ENG.16]; long tests carry `COST`; local `ctest` in the foreground.
- **Caching. Rec** none; fetched suites never cached. **Alt** ccache keyed by preset + label + compiler version. **Flip** the tune-up measures compile share ≥ 40 % of a critical-path leg (nesturbator: 4–22 % [ENG.17]).
- **Minutes per PR** (derived from nesturbator [ENG.18]): phase-1 scale ≈ 2 min wall, 13 jobs; v1-close ≈ 23 min wall, 93 runner-minutes per run; target critical path ≤ 12 min at v1 close; first lever: the Intel leg runs label `portable` only.
- **Post-phase-1 tail. Rec** one `ci:` PR is budgeted right after the first merge for what only the first `push` run shows (ruleset, required check name, App merge, release PR) (sibling lesson XFER-065). **Alt** fold it into phase 2. **Flip** the first merge is green end to end.

## 5. Release
```json
{"packages": {".": {"release-type": "simple", "initial-version": "0.1.0", "bump-minor-pre-major": true,
  "bump-patch-for-minor-pre-major": false, "draft": true, "force-tag-creation": true, "include-v-in-tag": true}}}
```
Manifest `{".": "0.0.0"}` and `version.txt` `0.0.0` until the first release, which is then 0.1.0 whatever the commit types [ENG.05].
1. A squash merge to `main` runs `release.yml`: App token (`client-id: ${{ vars.COGMASTER_APP_CLIENT_ID }}`, `private-key: ${{ secrets.COGMASTER_APP_PRIVATE_KEY }}`, `permission-contents`, `-pull-requests`, `-issues: write`), release-please, then `gh pr merge --auto --squash` on the release PR.
2. The App-made release PR runs CI without approval; GitHub merges it as the App when `CI required` passes; that push starts `release.yml` again (nesturbator run 38432194730 [ENG.18]).
3. On `release_created`, `uses: ./.github/workflows/ci.yml` runs CI with release-please's `sha` output as checkout ref, never `github.sha` (lattice_stripe, exifcleaner-node [ENG.25][ENG.26]); `publish` fails unless every archive records the tag's commit.
4. `publish`: exactly 18 zips `cogmaster-{library,runner,libretro}-{linux,macos,windows}-{x64,arm64}.zip` → `SHA256SUMS` → `actions/attest` with `subject-checksums` → upload to the draft → `gh attestation verify` each → a copy with its last byte flipped must fail → publish → `gh release verify vX` → re-read until 19 non-empty assets → download each from `releases/latest/download/`, check against `SHA256SUMS`, `gh attestation verify --signer-workflow <repo>/.github/workflows/release.yml`.
- **Re-runs:** `publish` is safe to re-run. The draft is addressed by release ID from `gh api --paginate repos/{o}/{r}/releases` filtered on `draft and tag_name`, because `GET releases/tags/{tag}` omits drafts and the unpaginated list stops at 30 (sibling lesson XFER-039); `release.draft_by_id` is `publish.sh`'s self-test on a fixture list of 35 releases. A version already published with 19 verified assets exits 0. Recovery is "Re-run failed jobs" [ENG.25][ENG.26].
- **Pre-1.0 policy:** `feat` → minor, breaking → minor, `fix`/`perf` → patch; `docs chore ci test build refactor style` cut no release; 1.0.0 only through a `Release-As: 1.0.0` final line in the PR body.
- **Version source. Rec** release-please edits only `version.txt`, the manifest and `CHANGELOG.md`; CMake `configure_file` generates the version header, `cogmaster_libretro.info` (`display_version`) and the runner's `--version`; no `extra-files`; `release.single_version_source` fails when the version string appears in any other tracked file; asset names carry no version. **Alt** `extra-files` markers (broke nesturbator's format check when 9 became 10 [ENG.17]). **Flip** a consumer needs the version in the file name.
- **Archives:** `LICENSE` and `THIRD-PARTY-NOTICES.md` in every zip (ISC and BSD-3 notices travel with binaries); no top-level folder; the libretro zip holds `cores/` and `info/`; `release.archive_contents` checks names, members and build paths on every leg.
- **Rehearsal** of every publish step before the first real tag, as job `rehearsal` inside `ci.yml` and therefore inside `CI required`, on each same-repository PR whose `changes` output `release_inputs` is true (`release.yml`, `scripts/publish.sh`, `release-please-config.json`, `cmake/packaging.cmake`, `.github/workflows/ci.yml` itself); it re-runs on every push to such a PR (sibling lesson XFER-064): CI on the head commit, then `publish.sh` against a prerelease `rehearsal-<run_id>-<run_attempt>` with `make_latest=false`, and a `cleanup` job that deletes the release and its tag whatever failed. Fallback if `GITHUB_TOKEN` on `pull_request` loses `contents`, `id-token` or `attestations: write`: the draft-only form. Precedent: muhtari run 38942043999 green; gabbaboy merged 18 release-fix PRs on its first tag [ENG.19].
- **Phase close:** no open `autorelease: pending` PR; `releases/latest` is the expected version with 19 assets.
- **Withdrawal** (`docs/maintaining.md`): `gh release edit vGOOD --latest`; edit the bad notes to "Withdrawn: reason; use vX"; fix forward with a `fix:` PR; delete only content that must not be distributed (its tag name is then gone for good) [ENG.09].
- **Bot credential. Rec** GitHub App token, minted only on `push` to `main` and in the Dependabot job. **Alt** a fine-grained PAT. **Flip** the owner declines to create an App.
- **Release-PR merge. Rec** native auto-merge. **Alt** a `workflow_run` merger (lattice_stripe, 239 lines [ENG.21]). **Flip** auto-merge merges a head whose `CI required` is not green.

## 6. Repository settings as code and branch policy
`scripts/apply-repo-settings.sh` (POSIX sh + `gh api`, idempotent; `--check` prints all drift before exiting 1) [ENG.10]:
| Setting | Call |
|---|---|
| squash only; `PR_TITLE` / `PR_BODY`; auto-merge; delete branch on merge; no wiki/projects | `PATCH /repos/{o}/{r}` |
| secret scanning + push protection | same `PATCH`, `security_and_analysis` |
| private vulnerability reporting; Dependabot alerts + security updates | `PUT …/private-vulnerability-reporting`, `…/vulnerability-alerts`, `…/automated-security-fixes` |
| SHA-pinned actions; read-only default token; Actions cannot approve PRs | `PUT …/actions/permissions` `sha_pinning_required=true`; `…/actions/permissions/workflow` `default_workflow_permissions=read can_approve_pull_request_reviews=false` |
| immutable releases; the eight labels; the ruleset | `PUT …/immutable-releases`; `gh label create --force`; `.github/rulesets/main.json` |

`PR_BODY` matters because, with `COMMIT_MESSAGES`, GSD's `[ci skip]` ship note would land in `main`'s squash commit. The ruleset copies nesturbator's: `~DEFAULT_BRANCH`; `deletion`, `non_fast_forward`, `required_linear_history`; `pull_request` with 0 approvals and `allowed_merge_methods: ["squash"]`; required check `CI required` with `integration_id` 15368; no bypass actors [ENG.17]. Apply it in phase 1 once a run has reported `CI required`.
- **Rec** script + committed ruleset, drift checked at the tune-up; `--check` covers every setting the script applies from phase 1 (lattice_stripe's unchecked `allow_auto_merge: false` broke every Dependabot merge [ENG.25]). **Alt** a scheduled drift workflow. **Flip** a setting drifts unnoticed between two tune-ups.

Branch policy: `.githooks/pre-commit` runs `hygiene.sh --staged` and refuses a commit on `main` once `origin` exists; `.githooks/pre-push` runs `hygiene.sh --history`; one branch and one squash-merged PR per phase; right after `/gsd-ship` the agent retitles the PR (`gh pr edit N --title "feat: …"`). Code review runs before verification and never after it, at most two fix rounds and one gap round per phase (AGENTS.md rule 7; sibling lesson XFER-064). No attempt quotas, slots, admission or publication ceremonies anywhere in plans, requirements or gates (AGENTS.md rule 8; sibling lesson XFER-045); review checks REQUIREMENTS.md and PLAN.md for them at each discuss step.

## 7. Triage automation (minimal)
- **Issue forms** (`.github/ISSUE_TEMPLATE/`, lattice_stripe's `bug_report.yml` as template [ENG.21]): `bug.yml` (cogmaster version, OS/arch, frontend + version, machine profile, steps, expected/actual); `compatibility.yml` with required ROM SHA-1 (as `cogmaster-run --probe` prints it, headered and headerless), header product code and region/size byte, title, machine (SMS NTSC/PAL, GG, Mark III, FM on/off), frontend + version, cogmaster version, symptom checkboxes (boot, graphics, sound, FM, input, save, speed, crash); `feature.yml`; `config.yml` turns off blank issues and links private vulnerability reporting. Every form says: never attach ROM or BIOS files.
- **Labels (8):** `bug`, `compatibility`, `enhancement`, `docs`, `performance`, `needs-info`, `nightly`, `digest` (plus Dependabot's `dependencies` and release-please's `autorelease: *`).
- **`digest.yml`**, Mondays 06:00 UTC, sh + gh + jq: rewrites the one open `digest` issue with unlabelled issues, PRs idle > 7 days, the open `nightly` issue and its age, Dependabot PRs > 7 days, a release PR open > 24 h or red, runner-image deprecations; first Monday of a month adds the CI-efficiency section (section 12). **Rec** this one issue. **Alt** notifications. **Flip** the agent skips the digest for two phases.
- **Rolling nightly issue:** opened or updated on failure, closed on success [ENG.17]. Every scheduled reporter decides from `needs.*.result` or a step's `outcome`, never a piped `$?`: the default shell is `bash -e` without `pipefail` [ENG.27]; lattice_stripe's drift job stayed green while listing 109 additions [ENG.25]. A `workflow_dispatch` input that forces a failure is run once in phase 1.
- **Dependabot:** `github-actions` only, weekly, one group for minor and patch (`update-types: [minor, patch]`), majors as their own PRs (all five of lattice_stripe's updates were majors [ENG.25]); `commit-message.prefix: ci`. `dependabot-automerge.yml` (~20 lines): `fetch-metadata`, App token with `permission-contents`, `-pull-requests`, `-workflows: write` (unverified need), `gh pr merge --auto --squash` for patch and minor; the private key is also a Dependabot secret [ENG.08].
- **YAGNI:** stale bot; CLA/DCO bots; auto-labellers; CodeQL per PR; Codecov; merge queue; Renovate; commitlint; release-drafter; AI triage; PR template; CODEOWNERS; Discussions; Pages; SBOM files; code signing and notarisation.

## 8. Performance engineering and the ratchet
| Measure | Where | Rule |
|---|---|---|
| Cachegrind Ir/frame = (Ir at 360 frames − Ir at 60) / 300 per ROM (`--cache-sim=no`, `env -i`, equal-length paths); head = PR merge commit, base = first parent measured twice (unequal → UNSTABLE) | `perf`, ubuntu x64, GCC 15, ≤ 4 ROMs (WLA-DX test card first, then one per subsystem: VDP, PSG, mapper) | report-only until two `workflow_dispatch` control runs (one commit measured three times) show zero spread, armed in phase 1 or 2 at the latest (sibling lesson XFER-051); then fail above +1.00 %; exceptions are dated lines in `tests/perf/allow.txt` [ENG.17] |
| Tracing costs nothing when off | `perf` | `cogmaster-run` with the dual-compiled loop and no hooks against `COGMASTER_TRACE_LOOP=OFF`: Ir/frame differs by ≤ 32 (derived); size delta goes to `quality.json` |
| Zero allocations after load | `core.no_alloc_after_load`, six legs | counting allocator through the config hook; after `cogmaster_load`, 600 frames plus a state save and load into caller buffers make 0 calls; the frame loop itself never calls an allocator |
| Nearest-rank p50, p90, p99, p99.9, p99.99, p99.999 and max µs per frame; fps; max RSS; deadline misses per budget profile | nightly `bench`, ubuntu x64 + arm64 + macos-15, ≥ 300,000 headless frames per fixture | trend data, never a gate; budgets below |

Frame budgets, `tests/perf/budgets.txt` (228 Z80 cycles per line; master clocks and line counts per SMS-SYSTEM; derived):
| Profile | Lines × cycles | Z80 clock | µs per frame | fps |
|---|---|---|---|---|
| `ntsc` | 262 × 228 = 59,736 | 3,579,545 Hz | 16,688 | 59.92 |
| `pal` | 313 × 228 = 71,364 | 3,546,895 Hz | 20,120 | 49.70 |
| `lowpower` | NTSC ÷ 10 | | 1,669 | hypothesis for a target 10× slower than the x64 runner (section 14) |

- **Rec** Ir/frame is the only per-PR speed gate; wall-clock tails are nightly trend data. **Alt** a wall-clock PR gate. **Flip** a quiet dedicated benchmark host appears. A zero-miss 95 % bound at p = 10⁻⁵ needs n ≥ 299,572 independent frames, 83 min of NTSC play (glueyneo's calculation [ENG.20]); shared VMs measure the host. Ir is deterministic: 23.3–24.7 M Ir/frame at +0.00 % on the NES sibling (job 115428512765) [ENG.18].

**Ratchet contract:** one committed `quality.json` with floors (`tests.<preset>`, `compat.public`, `fuzz.targets`, `coverage.functions_pct`), ceilings (`warnings`, `analyzer_findings`, `fuzz.open_crashes`, `ir_per_frame.<rom>`, `size.<zip>`, `size.wasm`, `ci.critical_path_s`) and `reasons[]`. Job `quality` runs `cmake -P tests/cmake/quality.cmake` with `string(JSON)`: a floor may not fall and a ceiling may not rise unless the same PR loosens the bound with a reason; tightening is free at each tune-up. `compat.local` (owner dumps) is recorded by the `library` preset and checked at the tune-up.
- **Rec** one file, one check. **Alt** per-metric scoreboards. **Flip** one metric's updates dominate PR churn.

## 9. Portability lanes
| Lane | Mechanism | Tier | Reason |
|---|---|---|---|
| `be32` | `gcc-powerpc-linux-gnu` 15.2 + `qemu-user` 10.2.1 via `CMAKE_CROSSCOMPILING_EMULATOR`; scripts that start the runner receive the same emulator | per PR from phase 1: label `portable` (core units, header parse, state round trip, hash list) and `hashes.txt` into `hash-equality`; full `ci` nightly | one lane covers big-endian, ILP32 and unsigned `char`; endian bugs live in serialisation |
| `wasm32` | wasi-sdk + wasmtime (section 14) | per PR: label `portable`; hash lane seven | 32-bit, no FP, no libc surprises, browser ports |
| `armhf` | `gcc-arm-linux-gnueabihf` + `qemu-arm` | nightly | 32-bit ARM handhelds and Raspberry Pi |
| s390x, ppc64, i686, Win32 | | not used | covered by `be32`, `wasm32` and `armhf` (derived) |
| Console compile-only | devkitPro and pspdev containers pinned by digest (versions re-read when the lane is added) | nightly, later milestone | core library with each SDK's toolchain file; no tests |

- **Rec** `be32` and `wasm32` per PR. **Alt** nightly only. **Flip** either job exceeds 5 min wall once measured in phase 1. If Intel macOS runners end in August 2027, build a universal macOS binary on arm64 and test the x86_64 slice under Rosetta (unverified on runners).

## 10. Style contract (joy-to-read C)
| Topic | Rule |
|---|---|
| Names | public `cogmaster_<noun>_<verb>()`, types `cogmaster_<noun>`, macros and enum constants `COGMASTER_`; cross-file internals `cogmaster__<module>_<verb>`, module = its file (z80, bus, vdp, psg, fm, io, cart, mapper, bios, state, synth); file-local `static` names unprefixed; units in names (`cycles`, `lines`, `_bytes`, `_num`/`_den`); hardware registers keep the names of the Sega and SMS Power! documents (`VCOUNT`, `HCOUNT`, VDP `R0`–`R10`, port `$3E` memory control, `$DC`/`$DD`) |
| Size | one module per chip; a file stays under 800 lines (generated tables exempt); a function fits one screen (60 lines) unless it is a dispatch switch of one-line cases; anything longer needs a reason in review |
| Comments | say what the hardware does and why; every hardware fact cites its primary source and the `docs/hardware/<chip>.md` section; no planning IDs in code, tests or user docs; no other emulator's identifiers |
| Ladder | ponytail.dev: remove the need → reuse → standard library → platform → minimum new code; reused permissive code keeps its licence header and gets a THIRD-PARTY-NOTICES row |
| Errors | every fallible public call returns `cogmaster_status`; an error changes no state (validate, then apply); untrusted bytes take checked paths; `assert` only states programmer invariants; the core never aborts, exits, prints, allocates after load, or reads files, clocks or the environment |
| Macros | constants and include guards only, plus the single dual-compiled traced loop (`run_loop.inc` included twice under `COGMASTER_TRACE` 0 and 1); no X-macros, no function-like macros |
| Format | clang-format 21 and the committed `.clang-format`; no hand-aligned columns |
| Tests | a test name states the behaviour (`vdp.hcount.latches_on_th_fall`); each failure prints expected and actual |

- **Rec** enforce names, macros and format by `abi.symbols`, hygiene and clang-format; size and comments by review. **Alt** clang-tidy `readability-function-size` and naming checks. **Flip** size or naming comes up in two consecutive reviews.

## 11. Docs standards
| Item | Standard | Check |
|---|---|---|
| `README.md` | for users: status → install one-liners from `releases/latest/download/` → RetroArch and runner quick starts → compatibility summary → docs ladder → licence; ≤ 12 KB | `docs.budgets`; install line run against the built zip on macOS arm64 |
| `docs/` | `embedding.md`, `runner.md`, `testing.md`, `maintaining.md` (App setup, repo settings, release, rehearsal, withdrawal, tune-up), `hardware/<chip>.md` graduated from `.planning/preparation/SMS-*.md` when the subsystem lands, `compatibility.md` generated from `tests/compat/ledger.csv`; each ≤ 40 KB | `docs.budgets`; `docs.compatibility_fresh` |
| Public header | terse, one line per function and field, no hardware prose; ≤ 24 KB | `docs.budgets` |
| Evidence | command + exit status + run URL in SUMMARY and VERIFICATION files, written by the command from the repository root (AGENTS.md rule 11), never pasted logs | review |
| `.planning` | ≤ 3 MB per milestone, archived at close | digest reports size |
| Gate code | policy and hygiene scripts ≤ 10 % of `src/` lines from phase 3; CMake `-P` or POSIX sh only; real-host e2e code (RetroArch) ≤ 300 lines in total (sibling lesson XFER-044) | `policy.gate_budget`, `policy.e2e_budget` CTests |

- **Rec** byte and line budgets as CTests. **Alt** review-only. **Flip** a budget blocks a correct change twice (raise it with a reason in `quality.json`).

## 12. Recurring loops
At each milestone close (about four phases), one tune-up phase fed by the monthly digest section:
| Loop | Data | Output |
|---|---|---|
| CI efficiency | per-job p50/p95 duration and queue time over 30 days; flake rate; runner-minutes per merged PR; critical-path job; cores used; compile share for the caching rule; lowest-value tests (never failed in 90 days and duplicated by a stronger signal) | timeouts re-derived; caching decision; tests removed only with the redundant signal named |
| Determinism | `hash-equality` and nightly results; every intentional hash change has a behaviour-revision bump and reason | issue or fix |
| Weakest dimension | score 1–5 with links for performance, CI/CD, ease of use, architecture, docs/README, upgrade path/semver | lowest becomes the next milestone's tune-up goal |
| Perf baseline | Ir/frame, sizes and the section 14 instructions-per-frame table on `main` | `quality.json` ceilings tightened to measured |
| Docs freshness, drift | docs whose covered code changed since their last edit; budgets; install line; `apply-repo-settings.sh --check`; Dependabot majors; runner-image announcements; WLA-DX and wasi-sdk pins | doc, settings or upgrade PRs |

Results go into a seed under `.planning/seeds/` plus `quality.json`; the release PR is green at every phase close.
- **Rec** per milestone. **Alt** per phase. **Flip** a phase-level regression goes unaddressed for a whole milestone.

## 13. Configuration and secrets (12-factor)
- Local-only paths live in gitignored `.env.local` (`COGMASTER_ROM_DIR`, `COGMASTER_BIOS_DIR`); committed `.env.example` lists the names with empty values. Only the local presets' CMake drivers parse it as `KEY=VALUE` lines; the core and runner read no environment. **Rec** this. **Alt** a gitignored `CMakeUserPresets.json`. **Flip** a second machine needs the same lanes.
- The secret-read guard refuses any agent command that names `.env`, `.env.<suffix>` (except `.example`, `.sample`, `.template`, `.dist`) or `.secrets` files (tested on muhtari, 2026-10-15 [ENG.23]). Agents read `.env.example` only.
- CI holds one variable and one secret: `COGMASTER_APP_CLIENT_ID` (Actions variable) and `COGMASTER_APP_PRIVATE_KEY` (Actions secret and Dependabot secret), set by the owner through GitHub's App manifest flow (GSD-HANDOFF says how, and how to rotate); agents never handle the key. App permissions: Contents, Pull requests, Issues and Workflows read/write, Metadata read. Attestations use OIDC (`id-token: write`).

## 14. WASM and low-power
- **Lane seven. Rec** `wasm32` preset: wasi-sdk clang with `--target=wasm32-wasi`, wasmtime as `CMAKE_CROSSCOMPILING_EMULATOR`, both fetched from their GitHub releases and pinned by SHA-256 exactly as WLA-DX is (section 1); label `portable` per PR, its `hashes.txt` the seventh input to `hash-equality` after the six legs and `be32`; versions are recorded when the lane is added, not here. **Alt** Emscripten first. **Flip** wasi-sdk cannot build the runner's file I/O shim (then the lane builds the library only and a 40-line WASI host runs the hash list).
- **Emscripten** comes later, for a browser frontend, as a nightly compile-only lane; it never replaces the WASI hash lane because its JavaScript glue hides core behaviour.
- **`-Os` versus `-O2`** is measured, not guessed: the nightly `bench` builds the `wasm32` library both ways and records size and Ir/frame (wasmtime's instruction count) in `quality.json` (`size.wasm`, `ir_per_frame.wasm.<rom>`); the release flag is chosen from that table at the tune-up.
- **No floating point** and **zero allocations in the frame loop** are the section 2 and section 8 rules; on wasm32 `abi.undefined` also rejects any `__wasm_*` float helper import, and `core.no_alloc_after_load` runs under wasmtime.
- **Instructions-per-frame budgets** for the handheld targets (SMS-ECOSYSTEM-AND-USERS 4 lists the devices and clocks) feed the ratchet as derived ceilings: `budget = device_mips × frame_µs` for each row, and `ir_per_frame.<rom>` must stay under the slowest listed device's budget divided by two (headroom for the host). The `lowpower` wall-clock profile (section 8) is a hypothesis until a device run calibrates it.

## 15. Sibling lessons folded in
| Lesson | Rule here | Check |
|---|---|---|
| XFER-058 | exit status and tolerant regexes, never exact tool text | `policy.no_exact_output`; `check_junit.cmake` |
| XFER-060 | `CMAKE_OSX_SYSROOT` set in `hygiene`; per-OS `nm -u` allowlist | `hygiene` configure; `abi.undefined` |
| XFER-061 | `file(CONFIGURE … NEWLINE_STYLE UNIX)` for hash and inventory files | `hygiene.lf_only` |
| XFER-062 | scripts consume stdin before early exit | `policy.sh_stdin` |
| XFER-039 | drafts by release ID, `--paginate` | `release.draft_by_id` |
| XFER-063 | title job scans title and body for skip tokens | `ci.title` |
| XFER-064 | review before verification; rehearsal re-runs on input change, inside `CI required` | `ci.classify`; aggregator CTest |
| XFER-065 | one `ci:` PR budgeted after the first merge | ROADMAP phase 1 exit |
| XFER-044 | e2e code ≤ 300 lines | `policy.e2e_budget` |
| XFER-067 | no stateful harness around RetroArch; every script also runs under dash | `hygiene.dash` |
| XFER-045 | no quotas, slots or ceremonies | discuss-step review of REQUIREMENTS and PLAN |
| XFER-051 | arm the Ir/frame gate in phase 1–2 after two zero-spread controls | `perf` job mode flag |
| XFER-056 | tarballs pinned by SHA-256; signature chain off the critical path | `fixtures.wla_pin.self_test` |
| GB-CI-004/005 | timeouts scaled in one pass after registration | `policy.timeout_scaled` |
| GB-GSD-020 | reconfigure after committing | `build.revision_matches_head` |
| GB-CGB-002 | negative controls do not leak; `detect_leaks=0` in `expect_fail.cmake` | `*.self_test` under `asan` |

## Open questions
Do `vars` resolve in Dependabot-triggered runs, and does an App merge of a workflow-file PR need `workflows: write`? · `be32` and `wasm32` per-PR cost (phase 1) · Ubuntu 26.04 arm64 queue times (first 40 runs) · whether wasmtime's instruction count is stable enough to be a ceiling (two control runs, as for Cachegrind).

## Sources
| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| ENG.01 | T1 | github/docs `data/reusables/actions/supported-github-runners.md` (https://github.com/github/docs) | dae83f7d28f3, accessed 2026-10-18 | labels, vCPU/RAM, `ubuntu-latest` = 24.04 |
| ENG.02 | T1 | actions/runner-images README and `images/{ubuntu,macos,windows}/*-Readme.md` (https://github.com/actions/runner-images) | ee1ff4569e38, accessed 2026-10-18 | preinstalled versions, image versions |
| ENG.03 | T1 | actions/runner-images announcements #13045, #14602, #14747, #14748 | accessed 2026-10-18 | Intel end, 26.04 GA, label moves |
| ENG.04 | T1 | GitHub REST `releases/latest` and `git/ref/tags` for the eight actions (https://api.github.com) | accessed 2026-10-18 (tested) | tags, SHAs, no newer releases |
| ENG.05 | T1 | googleapis/release-please `src/strategies/simple.ts`, `src/versioning-strategies/default.ts`, `src/manifest.ts`, `CHANGELOG.md` (https://github.com/googleapis/release-please) | v17.6.0 (712fcf01effd); changelog to v17.11.2, accessed 2026-10-18 | option semantics unchanged |
| ENG.06 | T1 | googleapis/release-please-action `package.json`, `package-lock.json` | v5.0.0 (45996ed1f6d0), accessed 2026-10-18 | bundles release-please 17.6.0 (`^17.6.0`) |
| ENG.07 | T1 | github/docs `data/reusables/actions/actions-do-not-trigger-workflows.md`, `content/actions/concepts/security/github_token.md` | dae83f7d28f3 | token trigger rules |
| ENG.08 | T1 | github/docs `content/code-security/reference/supply-chain-security/dependabot-on-actions.md` | dae83f7d28f3 | Dependabot restrictions |
| ENG.09 | T1 | github/docs `content/code-security/concepts/supply-chain-security/immutable-releases.md` | dae83f7d28f3, accessed 2026-10-18 (tested) | tag lock, no reuse, editable notes, release attestation |
| ENG.10 | T1 | GitHub REST docs "Actions permissions" (https://docs.github.com/en/rest/actions/permissions) and github/rest-api-description | accessed 2026-10-18 (tested) | `sha_pinning_required`, workflow permission fields |
| ENG.11 | T1 | actions/create-github-app-token `action.yml` | v3.3.0 (64ab729a6294) | inputs |
| ENG.12 | T1 | actions/attest `action.yml`, README; attest-build-provenance README | v4.2.2 | checksums input, wrapper |
| ENG.13 | T1 | Launchpad published binaries, resolute amd64 (https://api.launchpad.net/devel/ubuntu) | accessed 2026-10-18 (tested) | Valgrind, cppcheck, qemu, cross GCC |
| ENG.14 | T1 | vhelin/wla-dx releases, `LICENSE`, `master` (https://github.com/vhelin/wla-dx) | v10.7; 7ae11df1657d; accessed 2026-10-18 | fixture tool version, GPL-2.0-or-later |
| ENG.15 | T1 | silentbicycle/theft `LICENSE`, `src/theft_run.c` | 62e093d9e33b (v0.4.5) | licence, POSIX use |
| ENG.16 | T1 | CMake manuals `cmake-presets(7)`, `ctest(1)` (https://cmake.org/cmake/help/latest/) | CMake 4.4.4 (released 2026-10-02) | presets v11 `jobs` |
| ENG.17 | T1 | muhtari `.planning/preparation/ENGINEERING.md`, `.github/workflows/*.yml` pins; nesturbator `CMakePresets.json`, `.clang-format`, workflows, ruleset, `tests/cmake/global_symbols.cmake` | muhtari working tree 2026-10-18; nesturbator 3200aa868371 | ancestor text, sibling practice |
| ENG.18 | T1 | nesturbator Actions runs 37084642541, 38676328559, 38415960004, 38432194730, job 115428512765; live settings via `gh api` | accessed 2026-10-15 (by muhtari) | measurements |
| ENG.19 | T1 | gabbaboy `cmake/ExpectedTests.cmake`, workflows, PRs #10–#29 | b7a7f46cd2f7 | inventory, first-tag lessons |
| ENG.20 | T1 | glueyneo `.planning/preparation/QUALITY-PERFORMANCE-AND-CI.md` | 313976dfeba3 | tails, budgets |
| ENG.21 | T1 | lattice_stripe workflows and `.github/ISSUE_TEMPLATE/` | 66379aeb38cf | bot merges, forms |
| ENG.22 | T1 | OpenGSD core `workflows/ship.md` | 1.17.0 | ship title, `[ci skip]` |
| ENG.23 | T1 | local runs: `gh api` and Launchpad queries 2026-10-18; clang-tidy and Apple clang 21 on nesturbator `src/` 2026-10-15; secret-read guard | 2026-10-15, 2026-10-18 | (tested) results |
| ENG.24 | T1 | GCC Warning Options (https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html) | accessed 2026-10-15 | warning flags |
| ENG.25 | T1 | lattice_stripe history: `release.yml`, `drift.yml`, `pr-title.yml` commits; tags v1.7.3–v1.7.12; runs 30474364540, 30474374555, 30474391526, 38228990990 | accessed 2026-10-15 | dropped runs, silent reporter, title scopes, drift |
| ENG.26 | T1 | exifcleaner-node `release.yml` (`a88f57b73636`, `a44704fd528d`); exifcleaner-electron publish fixes | accessed 2026-10-15 | tagged-commit build, draft by ID |
| ENG.27 | T1 | GitHub Docs: workflow concurrency; workflow syntax `defaults.run.shell` (https://docs.github.com/en/actions) | accessed 2026-10-15 | pending runs cancelled; `bash -e` without `pipefail` |
| ENG.28 | T1 | `.planning/LESSONS.md` (cogmaster; inbound from muhtari, nesturbator, gabbaboy) | 2026-10-18 | sibling lessons XFER-058…XFER-056, GB-CI-004/005, GB-GSD-020, GB-CGB-002 |
