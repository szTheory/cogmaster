---
title: "Prompt coverage"
summary: "Every ask in the owner's kickoff prompt (2026-10-18) mapped to the file and section that answers it, with a status; and where the owner's constraints and preparation's recommendations differ."
read_when: "At kickoff review and at every milestone close; when the owner asks whether something was forgotten."
updated: 2026-10-18
---

# Prompt coverage

Statuses: **Documented** (a home and a settled recommendation), **Partial** (home exists; something remains, named),
**Later** (a seed holds it), **Inapplicable** (with the reason). Homes are preparation file stems with section
numbers; AGENTS rule N; SEED-NNN; "v1.0 phN". Rows are never renumbered or deleted; new asks take the next number.

## 1. Coverage

| # | Owner ask | Home | Status | Remaining / when |
|---|---|---|---|---|
| 1 | A Sega Master System + Game Gear emulator, one core for both, called cogmaster | AGENTS preamble; DECISIONS DEC.01, DEC.04; SMS-SYSTEM 5 | Documented | GG profile lands v1.0 ph3–4 |
| 2 | Learn from gabbaboy, glueyneo, nesturbator, genewhip, muhtari: their .planning, code, commits, pitfalls, what made the fastest progress | `.planning/LESSONS.md` XFER-001–076; GSD-HANDOFF 1; DECISIONS process rows | Documented | re-read at every discuss (AGENTS preamble) |
| 3 | Playstead lessons; homebrew in CI | LESSONS XFER-025, XFER-050; CONFORMANCE 1, 4, 7; HOSTS 4 | Documented | Playstead `sms`/`gg` IDs are a Playstead-side owner task |
| 4 | DNA of mature OSS projects (lattice_stripe, exifcleaner): CI/CD, release, triage | ENGINEERING 4–7, 13; LESSONS XFER-026–033 | Documented | |
| 5 | A reusable core with clean video/audio/input interfaces for other frontends | ARCHITECTURE 1, 4, 5; HOSTS 1, 4, 7 | Documented | |
| 6 | Incredibly fast | ENGINEERING 8, 14; ECOSYSTEM 4; DECISIONS perf rows | Documented | Ir/frame gate armed v1.0 ph1–2 |
| 7 | Correct; address what people dislike about other emulators | ECOSYSTEM 1, 2 (C.NN, G.NN); CONFORMANCE 3–5 | Documented | |
| 8 | Best-practice C, CMake, CI with an adversarial fan-out per decision | ENGINEERING 2–4; BRIEF (fan-out method); DECISIONS | Documented | |
| 9 | Code that is a joy to read; KISS, YAGNI, ponytail.dev ladder | AGENTS Style; ENGINEERING 10 | Documented | |
| 10 | Baselines in every measurable dimension, ratcheted to the point of diminishing returns | ENGINEERING 8 (`quality.json`), CONFORMANCE 5 (ledger ratchet) | Documented | `quality.json` built in the v2.0 tune-up (SEED-007) |
| 11 | Hardware documentation with provenance, primary sources first, a RAG-friendly second brain | BRIEF; README (index); SMS-SYSTEM, SMS-VDP, SMS-AUDIO, SMS-CARTRIDGES-AND-PERIPHERALS, SMSPOWER-EXTRACTS | Documented | hardware facts graduate to `docs/hardware/` as subsystems land |
| 12 | Development velocity as fast as possible | GSD-HANDOFF 1–3; LESSONS XFER-001, 002, 068; DECISIONS DEC.06–08 | Documented | |
| 13 | Telemetry without overhead, optional, for integration and debugging | ARCHITECTURE 8 (counters, hooks compiled twice) | Documented | hooks v5.0 (SEED-004) |
| 14 | Great day-0/1/2 developer experience; easy to adopt | ENGINEERING 11; HOSTS 1, 4; README budget | Documented | |
| 15 | Property-based testing, fuzzing, safe to run hostile ROMs | ENGINEERING 3; CARTRIDGES 6 (bounds table); AGENTS rule 4 | Documented | fuzz targets report-only until green (AGENTS rule 8) |
| 16 | Fast CI and builds | ENGINEERING 4; SEED-007 | Documented | |
| 17 | Ship early and often; every phase a release | AGENTS rule 10; DECISIONS DEC.08; ENGINEERING 5 | Documented | |
| 18 | Always PRs, green main, triage at every step, auto-merge and auto-release with zero human verification where possible | AGENTS rules 6, 10; ENGINEERING 5–7; `scripts/apply-repo-settings.sh` | Documented | |
| 19 | 12-factor secrets: `.env.local` locally, Actions secrets in CI | ENGINEERING 13; `.env.example`; GSD-HANDOFF 7 | Documented | |
| 20 | Rolling near/mid/long roadmap revised at every milestone | `.planning/PRODUCT-ROADMAP.md`; seeds; GSD-HANDOFF 4 | Documented | |
| 21 | Performance stress testing to p99.999 on the critical flows, folded into CI | ENGINEERING 8 (nightly tails, per-PR Ir gate) | Documented | |
| 22 | Recurring CI/CD efficiency audits baked into the process | SEED-007; ENGINEERING 12 | Documented | |
| 23 | Recurring weakest-quality-dimension reviews | SEED-007; ENGINEERING 12 | Documented | |
| 24 | Verification shifted left and automated; only the truly necessary handed to the owner; recorded so it never loops back | AGENTS rule 6; DECISIONS DEC.09; GSD-HANDOFF 3 | Documented | settled, never re-proposed |
| 25 | Stop after each phase so the owner can check and switch models | AGENTS rule 9; GSD-HANDOFF 3, 5, 6; `scripts/hygiene.sh` stop policy | Documented | |
| 26 | Releases the owner can download and try on a Mac through some frontend | HOSTS 3 (RetroArch Metal); README install line | Documented | first release v1.0 ph1 |
| 27 | The owner's physical carts as test input (legal dumps) | ASSET_POLICY 4; CONFORMANCE 5 (local `library` preset) | Documented | |
| 28 | Speedrunning | DECISIONS DEC.04 (exact timing); ECOSYSTEM 3; SEED-002 | Partial | frame-exact movies and lag counter v3.0 |
| 29 | Machine/AI-assisted speedruns and TAS | ECOSYSTEM 3; HOSTS 5; SEED-002, SEED-004 | Later | v3.0 movies, v5.0 tools |
| 30 | AI playthroughs | HOSTS 5 (Gymnasium order); SEED-004 | Later | v5.0 |
| 31 | Portable to low-power devices (DS, PSP, handhelds) | ECOSYSTEM 4; ENGINEERING 9, 14; SEED-005 | Partial | compile lanes v6.0; budgets are a ratchet input from v1.0 |
| 32 | Consistent frame rate, no stutter, low latency | ARCHITECTURE 5 (input at VBlank); HOSTS 1, 3; ENGINEERING 8 | Documented | |
| 33 | Great interface design for integration ergonomics | ARCHITECTURE 4; HOSTS 1, 4 | Documented | |
| 34 | All regions | SMS-SYSTEM 5; DECISIONS; v1.0 ph3 (region profile) | Documented | PAL and Japanese FM over the ledger v2.0 |
| 35 | Different monitor/display types | SMS-VDP 5; HOSTS 7 (host helpers outside the hash) | Documented | |
| 36 | Mappers: maximise and ratchet compatibility | CARTRIDGES 2, 7; CONFORMANCE 5 | Documented | Korean/EEPROM v2.0 (SEED-001) |
| 37 | Easy saves | CARTRIDGES 4; HOSTS 1, 4 | Documented | `.srm` interop v2.0 |
| 38 | ROM-hacker friendly without compromising design | CARTRIDGES 5; HOSTS 5; SEED-004 | Later | v5.0; patching stays in the host until then |
| 39 | Public repo, no PII | AGENTS rule 3; `scripts/hygiene.sh`; ASSET_POLICY | Documented | |
| 40 | MIT or Apache | DECISIONS DEC.02 (MIT); PROVENANCE 1 | Documented | |
| 41 | What people like and dislike about emulators in general | ECOSYSTEM 2 (G.NN) | Documented | |
| 42 | Strangler-fig reuse of permissive code | PROVENANCE 4; AGENTS rule 1 | Documented | |
| 43 | Best-in-class in every quality lens, not a generic emulator | ECOSYSTEM 6; DECISIONS; PRODUCT-ROADMAP | Documented | |
| 44 | Research official, fan and emulator sources | BRIEF tiers; every file's Sources table | Documented | |
| 45 | Clean architecture, no circular dependencies | ARCHITECTURE 3 (modules table); ENGINEERING 10 | Documented | |
| 46 | Compile the research into .md files and give a kickoff command for a clear context window | this directory; GSD-HANDOFF 2 | Documented | |
| 47 | Lessons exchange between sibling projects | LESSONS protocol; AGENTS rule 13 | Documented | |
| 48 | Use subagents for context-window efficiency | BRIEF; GSD `workflow.research` and advisor mode (GSD-HANDOFF 3) | Documented | |
| 49 | Usable inside other frontends (Playstead, libretro) | HOSTS 1, 4 | Documented | |
| 50 | Built with OpenGSD, new project | GSD-HANDOFF; `.planning/config.json` | Documented | |
| 51 | Power efficiency on handhelds and in the browser | ENGINEERING 14; ECOSYSTEM 4; SEED-005, SEED-008 | Partial | measured in the nightly bench from v1.0; device runs v6.0 |
| 52 | Efficient in the browser through WebAssembly | ENGINEERING 14; HOSTS 6; SEED-008 | Partial | wasm32 hash lane v1.0 ph1; the page after v1.0 |
| 53 | Copy and paste over dependencies | AGENTS Style; PROVENANCE 1 | Documented | |
| 54 | Build the right system (SMS/GG, stated at the top of the prompt) | AGENTS preamble; BRIEF; every hardware file | Documented | |
| 55 | Game Gear as a first-class target, not an afterthought (owner decision 2026-10-18: SMS + GG in v1.0) | DECISIONS DEC.04; SMS-VDP 6; SMS-AUDIO 2; CARTRIDGES 1 | Documented | |
| 56 | SG-1000/SC-3000 compatibility (same hardware family) | SEED-006; SMS-VDP 7 | Later | v2.0 scoping |

## 2. Owner constraints vs preparation's choices

| Owner required | Preparation chose | Why |
|---|---|---|
| MIT or Apache | MIT | same as the siblings; compatible with every frontend |
| "Whatever is the best GUI for Mac" to try releases | No GUI in this repository; RetroArch (Metal build) is the frontend, Playstead later | sibling rule that core repositories own no GUI; one install line |
| Make it work for speedrunning, TAS, AI, handhelds, browser | Horizons by reach and dependency: players and homebrew devs first (v1.0–v2.0), states/movies (v3.0), accuracy (v4.0), tools (v5.0), everywhere (v6.0); the browser seed can jump the queue after v1.0 | each later horizon depends on the earlier one; every phase still ships |
| Research and sanity-check upstream as much as possible | A preparation pack plus a fact-check and fresh-eyes review before kickoff; GSD's own researcher still runs before every phase | both; preparation is not a substitute for per-phase research |
| Stop after each phase | One GSD step per session, enforced by AGENTS rule 9 and a hygiene check | stronger than asked, because chaining flags leak from global defaults |

## 3. Keeping this file true
Refresh at kickoff review and at each milestone close. A new owner ask takes the next number. A row moves from
Later to Documented only when its seed is promoted and the work is planned. Requirement traceability in
`REQUIREMENTS.md` does not replace this check: it covers requirements, this covers asks.
