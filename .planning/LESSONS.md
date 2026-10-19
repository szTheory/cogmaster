---
title: "Lesson exchange"
summary: "Lessons imported from the owner's sibling emulators (hypotheses until reproduced here) and the template for exporting this project's lessons."
read_when: "At every discuss step, at each phase or milestone boundary, when a sibling lesson might apply, or when the owner asks to export lessons."
updated: 2026-10-18
---

# Lesson exchange

Rules that came from these lessons live in `AGENTS.md` and `.planning/preparation/DECISIONS.md`, because agents
read those files; this ledger records where each rule came from and whether it has been reproduced here.
AGENTS.md says to re-read this file at every discuss step: a sibling planning on the same day as an export still
missed one (XFER-059), so reading at kickoff alone is not enough.

## Protocol

- **Inbound.** A sibling's lesson is a hypothesis until its mechanism is shown to apply here. Record the source
  project and revision, whether it was implemented or only planned there, how this project differs, and the
  local decision. Supersede entries; do not erase them.
- **Outbound.** When the owner says "export lessons", produce one pasteable block per lesson in the template
  below. Exclude paths, identities, private ROM names and hashes, credentials and transcripts. Never imply that
  another console shares a Master System or Game Gear timing behaviour.
- **IDs.** `CM-<AREA>-<NNN>` for lessons learned here (areas: GSD, CI, REL, TEST, ARCH, PERF, AUDIO, VIDEO, HOST,
  LEGAL, DOCS); `XFER-NNN` for imports. IDs are unique; a test (`policy.lessons_unique`, planned for the v2.0
  tune-up) rejects duplicates, because a sibling's ledger reused two IDs.
- **Status vocabulary.** observed → reproduced → adopted → superseded (or rejected, with the reason).

## Template

```
Lesson <ID> (<date>, <project>@<revision>)
Symptom and reproduction: …
Root cause / invariant: …
Fix and evidence (command, exit status, run URL): …
Applies when: …
Does not establish: …
Suggested check: …
Sources: …
Status: observed | reproduced | adopted | superseded
```

## Imported lessons, first batch (2026-10-18, preparation; as imported by muhtari on 2026-10-15)

These 33 were imported by muhtari from nesturbator, gabbaboy, glueyneo, playstead, lattice_stripe and exifcleaner.
muhtari's own first-week lessons MU-* follow in the second batch. The source column names the project the
lesson came from; muhtari `.planning/LESSONS.md` at `952fce9701175c291f4dab78d26205172cc29d8d` carries the
original evidence columns.

| ID | Lesson | Source | Local disposition |
|---|---|---|---|
| XFER-001 | Pre-write every artifact GSD would ask for and dry-run the roadmapper on a copy; the kickoff then resumes at roadmap creation. | nesturbator | adopted (this preparation; GSD-HANDOFF) |
| XFER-002 | Phase 1 is a ROM-free walking skeleton that ships the whole release pipeline; every later phase ends in a release by merging. | nesturbator | adopted (v1.0 phase 1; AGENTS rule 10) |
| XFER-003 | Requirements written as the command that proves them make verification mechanical. | nesturbator | adopted (REQUIREMENTS) |
| XFER-004 | Verification is automation-first; the owner is asked only for consent or irreducible judgement. | nesturbator; gabbaboy | adopted (AGENTS rule 6) |
| XFER-005 | Implement the whole CPU against exhaustive public vectors before any game; never grow it opcode by opcode from a game trace. | glueyneo | adopted (v1.0 phase 2: SingleStepTests z80) |
| XFER-006 | Governance machinery must stay proportional; per-file rights digests, prose validators and threat registers consumed a sibling's milestone. | glueyneo | adopted (AGENTS rule 7) |
| XFER-007 | Review warnings about test code that grow into requirements stalled a milestone; keep real-host e2e to a screenshot compare and a save round trip. | nesturbator | adopted, strengthened (AGENTS rule 7: ≤ 300 lines, tested; see XFER-044) |
| XFER-008 | A release PR opened with the default Actions token needs maintainer approval and blocks; mint a GitHub App token for every bot merge. | gabbaboy | adopted (ENGINEERING 5, owner task) |
| XFER-009 | Aligned machine-edited version comments broke formatting and silently blocked releases; keep machine-edited lines out of aligned blocks and check the release PR at every phase close. | nesturbator | adopted (`version.txt` only) |
| XFER-010 | Merges after a phase's verification make it stale and force override closeouts; decide the closeout policy up front. | nesturbator | adopted (GSD-HANDOFF 3) |
| XFER-011 | Tests that skip themselves hid a broken GUI path; RetroArch aborts inside the agent sandbox, so the hosted job is the evidence. | nesturbator | adopted (no self-skip; hosted e2e) |
| XFER-012 | Timeouts guessed instead of measured cancelled green runs; use twice the slowest recent run with its run ID. | nesturbator | adopted (ENGINEERING 4) |
| XFER-013 | A CTest regex with `--no-tests=error` does not prove each named test ran; keep an exact executed-test inventory and make negative controls assert the specific failure. | gabbaboy | adopted (ENGINEERING 3) |
| XFER-014 | A globally ignored ROM extension silently dropped new fixtures; exempt the fixture directory and count tracked files. | gabbaboy | adopted (`.gitignore` exemption) |
| XFER-015 | Agents read AGENTS.md and the plan, not the lessons file; a lesson becomes a control only as a rule plus a failing check. | gabbaboy | adopted (this file's preamble; AGENTS) |
| XFER-016 | Prove playability with one integrated host session: boot, a control that changes play, moving picture, audible sound. | nesturbator, gabbaboy | adopted (v1.0 phase 3 and 4) |
| XFER-017 | Diagnose a stalled guest at its first divergence with a bounded trace; more cycles prove nothing. | nesturbator | adopted, strengthened (first-fault capture in the runner from phase 1; XFER-058) |
| XFER-018 | Confidence-qualified models: implement the best-supported model, label what is unmeasured, stop gap loops that lack new evidence. | gabbaboy | adopted (DECISIONS) |
| XFER-019 | A repository's licence is not the licence of every byte in a built ROM (SDK runtime, fonts, compiler libraries). | genewhip | adopted (ASSET_POLICY; fixtures are pure assembly) |
| XFER-020 | A cited licence was wrong at the pinned commit; check the LICENSE at the exact pin. | nesturbator | adopted (PROVENANCE) |
| XFER-021 | CI that runs on every branch push and on the pull request doubles cost; push triggers only `main`. | gabbaboy | adopted (ENGINEERING 4) |
| XFER-022 | A README that doubles as the CI manual grew to 114 KB and buried the install line; split user and maintainer docs with budgets. | nesturbator | adopted (AGENTS rule 5) |
| XFER-023 | GSD's config writer merges the owner's global defaults; write config with an empty `GSD_HOME`. | GSD 1.17.0 | adopted (preparation) |
| XFER-024 | GSD's ship step pushes a CI-skip commit to the PR branch; squash messages must come from the PR title and body. | GSD 1.17.0 | adopted (`scripts/apply-repo-settings.sh`) |
| XFER-025 | Homebrew whose licence is unclear makes a poor CI fixture; self-authored test ROMs, designed so the oracle cannot be fooled, were cheaper and safer; a gate that exists but never executes is the real hazard. | playstead | adopted (CONFORMANCE; ASSET_POLICY) |
| XFER-026 | A newly queued run cancels the pending one in its concurrency group even with `cancel-in-progress: false`; CI must build release-please's `sha`. | lattice_stripe | adopted (ENGINEERING 4, 5) |
| XFER-027 | A publish job must be safe to re-run: `GET releases/tags/{tag}` omits drafts; a full re-run skips publish once the release PR is tagged. | lattice_stripe, exifcleaner | adopted, strengthened (XFER-039: address drafts by ID) |
| XFER-028 | A scheduled reporter that tested a piped `$?` under `bash -e` without `pipefail` stayed green while its report listed failures. | lattice_stripe | adopted (ENGINEERING 7) |
| XFER-029 | Pipeline fixes titled `fix(release):` cut ten patch releases in one day; a title lint outside the required check let bad scopes reach `main`. | lattice_stripe | adopted (`scripts/check-title.sh` in the required check) |
| XFER-030 | Check each release as a user would: download from `releases/latest/download/`, verify `SHA256SUMS`, pin the attestation to the release workflow. | lattice_stripe, exifcleaner | adopted (ENGINEERING 5) |
| XFER-031 | A live setting that no check covered made every Dependabot auto-merge run fail; a drift check must cover every setting it applies. | lattice_stripe | adopted (`apply-repo-settings.sh --check`) |
| XFER-032 | GitHub Actions updates are mostly majors, so a Dependabot group that includes majors never qualifies for patch/minor auto-merge. | lattice_stripe | adopted (ENGINEERING 7) |
| XFER-033 | An executable-only phase-completion gate grew to about 3,400 lines with its verifier and tests. | exifcleaner | rejected: AGENTS rules 6–8 keep the policy without the machinery |

## Imported lessons, second batch (2026-10-18, preparation; muhtari's own week, read at `952fce9701175c`)

| ID | Lesson | Source | Local disposition |
|---|---|---|---|
| XFER-034 | GSD's config writer merges `$GSD_HOME/.gsd/defaults.json`; a global yolo mode and web gates leak into a public `config.json`. Write it with `GSD_HOME` pointing at an empty directory. | muhtari MU-GSD-001 | adopted (preparation; GSD-HANDOFF 5) |
| XFER-035 | `/gsd-ship` titles PRs "Phase N: name" and pushes a `[ci skip]` commit; squash title = PR title, body = PR body; retitle as a Conventional Commit. | muhtari MU-GSD-002 | adopted (AGENTS rule 10) |
| XFER-036 | `/gsd-verify-work` after a passing execute is redundant and can start gap planning; run it only for `human_needed`. | muhtari MU-GSD-003 | adopted (GSD-HANDOFF 3) |
| XFER-037 | `branching_strategy: phase` forks only at execute; branch before the first committing step and let a pre-commit hook refuse `main`. | muhtari MU-GSD-006 | adopted (AGENTS rule 10; hygiene) |
| XFER-038 | Dry-run the roadmap step on a copy before the real kickoff. | muhtari MU-GSD-007 | adopted (GSD-HANDOFF 8) |
| XFER-039 | A fresh draft release is missing from `gh release list` and lookups read only 100 releases; address drafts by release ID with `gh api`, use `--paginate`. | muhtari phase 1 (01-17, 01-18) | adopted (ENGINEERING 5) |
| XFER-040 | release-please 17.6.0: `initial-version` applies only while the manifest says `0.0.0`; `feat` bumps minor before 1.0 only with `bump-patch-for-minor-pre-major: false`; use a `Release-As:` footer. | muhtari MU-REL-001 | adopted (ENGINEERING 5) |
| XFER-041 | Events caused by `GITHUB_TOKEN` start no workflow runs; Dependabot-triggered runs see only Dependabot secrets; store the App key as a Dependabot secret too. | muhtari MU-REL-002 | adopted (owner task) |
| XFER-042 | A `.planning`-only PR ran the whole matrix; skip heavy jobs with a `changes` job while the aggregator still reports; never a trigger-level `paths` filter. | muhtari MU-CI-002 | adopted (ENGINEERING 4) |
| XFER-043 | Repository settings drift from their docs; keep settings as a script with a `--check` mode run at each tune-up. | muhtari MU-CI-003 | adopted (`scripts/apply-repo-settings.sh`) |
| XFER-044 | A real-host e2e phase added about 10,250 test lines and changed nothing under `src/` (one shell test alone 4,197 lines). The ≤ 300-line rule needs a CI check, not a sentence. | nesturbator phase 16 (branch `gsd/phase-16-host-evidence-and-roadmap`) | adopted (AGENTS rule 7; a CTest counts `tests/retroarch/` lines) |
| XFER-045 | Agents invent attempt quotas, "formal slots" and once-only ceremonies to stop loops, and the quotas become the work; one sibling retired the scheme, another is inside it now. | glueyneo METHODOLOGY (retired 2026-10-16); nesturbator STATE | adopted (AGENTS rule 8) |
| XFER-046 | A requirement that needs a *published* release cannot pass before merge and kept a phase open with a "publication gap" plan. | nesturbator EVID-04 | adopted (AGENTS rule 8; confirmed at ship instead) |
| XFER-047 | A milestone scoped as evidence plus governance shipped no release in five days; every milestone names a behaviour a player can see. | nesturbator v4 | adopted (PRODUCT-ROADMAP) |
| XFER-048 | Eight phases of work merged without a release because release was deferred to a milestone-end phase, which then grew ledgers, receipts and a fuzz campaign; the owner stopped it by hand. | gabbaboy v0.2 phase 15 | adopted (AGENTS rule 10: every phase releases; the release PR is never held) |
| XFER-049 | A fuzz *harness* assertion (wrong profile) blocked a release; a failing harness is a test bug to triage; fuzz targets are report-only until green for N runs. | gabbaboy `tests/fuzz_core.c` | adopted (AGENTS rule 8; ENGINEERING 3) |
| XFER-050 | No rights-clear game fixture was ever admitted for a hardware milestone, so 128 plans produced no game evidence; choose or author the fixture before the milestone starts. | gabbaboy D-101 | adopted (CONFORMANCE: SMS and GG fixtures named at kickoff) |
| XFER-051 | Perf regressions were found only at milestone end; arm the per-PR instruction-count gate in phase 1–2. | gabbaboy 15-05; nesturbator v2 | adopted (ENGINEERING 8; DECISIONS) |
| XFER-052 | Release qualification that required a first-time fork contributor and an independent approver halted a workstream for a solo maintainer. | glueyneo workstream 1 | adopted (AGENTS rule 8) |
| XFER-053 | On OpenGSD 1.15 a stale `gaps_found` report routed back to gap planning without re-verifying; run one fresh verifier before any gaps plan. | glueyneo 2026-10-04 note | observed (re-check on 1.17.0 at the first `gaps_found`) |
| XFER-054 | A permission denial was treated as a verification result; write evidence from the command that produced it, from the repository root, with no `cd` chains. | glueyneo METHODOLOGY; muhtari 01-01 | adopted (AGENTS rule 11) |
| XFER-055 | Tooling grew to 1.4× the emulator and no game booted; the last fault's address was never captured. Put first-fault capture (PC, address, width, device) in the runner from phase 1. | glueyneo sizes and MVS attempt | adopted (ARCHITECTURE; HOSTS 4) |
| XFER-056 | 8 of 13 phase-1 plans were fixture and toolchain provenance (patched MAME builds, an expired signing key halting Binutils). Pin the assembler tarball by SHA-256; keep signature chains off the critical path. | genewhip phase 1; gabbaboy GB-CGB-004 | adopted (ENGINEERING 2; SMSPOWER-EXTRACTS 7) |
| XFER-057 | Preparation pinned a sibling commit that existed only on an unpushed branch; six plans re-fetched seeds. Pin only commits the public API answers 200 for; fetch with `gh api`, never from a local checkout. | muhtari phase 1 (01-01…01-20 deviations) | adopted (AGENTS rule 12) |
| XFER-058 | Plan acceptance text hard-coded tool output that changed (CTest 4 summary line, CMake 4.4 ConfigVersion text, wrapped FATAL_ERROR). Accept on exit status and tolerant regexes. | muhtari phase 1 | adopted (AGENTS "GSD summaries"; ENGINEERING 3) |
| XFER-059 | A sibling planned to grow its CPU by instruction family from ROM demands two weeks after XFER-005 was exported; lessons were not re-read at discuss. | genewhip 02-CONTEXT D-32 | adopted (AGENTS preamble: re-read at every discuss) |
| XFER-060 | CMake 4 leaves `CMAKE_OSX_SYSROOT` empty so Homebrew clang-tidy misses `string.h`; Apple clang lowers `memset` to `_bzero`, breaking a freestanding allowlist. | muhtari 01-11, 01-01 | adopted (ENGINEERING 2) |
| XFER-061 | `file(WRITE)` writes CRLF on Windows so hash files differed with equal values; use `file(CONFIGURE … NEWLINE_STYLE UNIX)`. | muhtari 01-14 | adopted (ENGINEERING 3) |
| XFER-062 | A script that exits on first match under `pipefail` kills the upstream `gh api` with SIGPIPE; read all input, then decide. | muhtari 01-14, 01-15 | adopted (ENGINEERING 4) |
| XFER-063 | `[skip ci]` in a PR *title* becomes the squash subject and suppresses CI on `main`; the title check scans title and body. | muhtari 01-15 | adopted (`scripts/check-title.sh`) |
| XFER-064 | A code-review fix round after verification changed 25 files including the release workflow; the rehearsal predated the fixes and is not a required check. Review before verification; rehearsal re-runs on release-input changes and joins `CI required`. | muhtari 01-VERIFICATION | adopted (AGENTS rule 7; ENGINEERING 5) |
| XFER-065 | Phase-1 leaves a tail of CI follow-ups (arming the perf gate needs post-merge evidence; promoting a report-only lane). Budget one `ci:` PR right after the first merge. | muhtari todos | adopted (GSD-HANDOFF 3) |
| XFER-066 | A CPU phase planned as 16 strictly sequential waves; opcode groups and devices that do not share an inventory can run in parallel waves. | muhtari phase 2 plans | observed (plan phase 2 with parallel waves where inventories differ; gabbaboy D-056 keeps shared-inventory plans sequential) |
| XFER-067 | Native failures came from the shell and the host, not the emulator (dash noclobber exit codes, timing bounds on ASan hosts); no stateful shell harness around RetroArch, and CI shell scripts run under dash. | nesturbator 16-22 | adopted (ENGINEERING 4; HOSTS 3) |
| XFER-068 | Owner attention is the scarce resource across five parallel projects: a sibling sat 14 days at a phase boundary. Keep phases short and self-contained, each a release. | genewhip log | adopted (coarse granularity, four phases per milestone) |
| XFER-069 | A lesson ledger reused IDs; keep a uniqueness check. | gabbaboy LESSONS | adopted (this file's protocol) |
| XFER-070 | Instance-owned machine on an integer master timeline with device deadlines; bounded stepping with stop reasons; RTC/time injected by the host and a CTest proving no host clock; every claim scoped to build, model and corpus; suites declare their exclusion count; deferred registers fail closed. | gabbaboy D-009, D-010, D-012/063, D-015, D-041, D-068, D-073 | adopted (ARCHITECTURE; DECISIONS) |
| XFER-071 | Under PIC a file-scope `const` table of function pointers lands in `.data.rel.ro` and fails a no-mutable-globals check; dispatch with `switch`. | muhtari MU-ARCH-001 | adopted (DECISIONS) |
| XFER-072 | A permissive licence today does not clear a repository's history or its embedded firmware; audit licence history and per-file editors at the pin. | muhtari MU-LEGAL-001 | adopted (PROVENANCE 2) |
| XFER-073 | The `theft` property-testing library needs `fork`; a ~60-line seeded splitmix64 harness covers the need on every platform. | muhtari MU-TEST-001 | adopted (ENGINEERING 3) |
| XFER-074 | GSD's secret-read guard blocks agents from `.env.local` and key files; secret setup is an owner command. | muhtari MU-GSD-008 | adopted (GSD-HANDOFF 7) |
| XFER-075 | GitHub's App manifest flow makes the release App one owner command: a local page posts the manifest, the conversion response carries the key, a helper pipes it into `gh secret set` for Actions and Dependabot. | muhtari MU-REL-004 | adopted (GSD-HANDOFF 7) |
| XFER-076 | A hygiene rule that scans for a header signature must not contain the signature itself as text, or every copy of the script matches. | muhtari hygiene (ATARI7800 compared as hex) | adopted (`scripts/hygiene.sh` compares `TMR SEGA` as hex) |
| XFER-077 | Hosted ASan outgrew job and per-test timeouts; scale sanitizer timeouts in one pass after registration, per CMake directory. | gabbaboy GB-CI-004, GB-CI-005 | adopted (ENGINEERING 3: `policy.timeout_scaled`) |
| XFER-078 | CMake embeds the git revision at configure time; reconfigure after committing or the build reports a stale revision. | gabbaboy GB-GSD-020 | adopted (ENGINEERING 2: `build.revision_matches_head`) |
| XFER-079 | A failing negative control leaks under LSan, which replaces its exit code; run controls with leak detection off and free before failing. | gabbaboy GB-CGB-002 | adopted (ENGINEERING 3: `expect_fail.cmake`) |

## Lessons learned here

None yet. The first export is offered at the v1.0 phase 1 close.
