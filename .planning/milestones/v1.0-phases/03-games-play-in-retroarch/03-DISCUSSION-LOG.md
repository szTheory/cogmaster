# Phase 03: Games play in RetroArch - Discussion Log

> Human discussion record only. Research, planning and execution consume 03-CONTEXT.md.
> This is an alternatives/rationale record, not a committed command log or verification report.

**Date:** 2026-10-20
**Phase:** 03-games-play-in-retroarch
**Areas considered:** Picture presentation, reset/input, saves, compatibility reporting.
**Runtime / model:** Codex / GPT-6.

## Owner direction and method

The initial selection offered four concrete areas. The owner selected all, then requested breadth and depth
through relevant stakeholder lenses, primary research where useful, alternatives/tradeoffs/footguns/lessons,
an adversarial pass, a coherent recommendation for each point, and automatic adoption of the recommendations.
They reiterated a preference for smaller, flatter dependencies and permissive reuse over a new package.

No further interview answers are implied. The matrices below record agent recommendations adopted under that
explicit delegation, not choices the owner individually selected from menus. Three read-only agents researched
presentation, input/saves and compatibility; the parent synthesized their results and reconciled conflicts.

Relevant lenses: players, frontend integrators, homebrew developers, TAS/reproducibility, hardware fidelity,
C/embedded portability, rendering/performance, file integrity/security, release/testing cost, and maintenance.
Distributed systems and unrelated language frameworks add no useful design constraint to this instance-owned
C core. Considered multiwriter saves only to define their limit; did not build a coordination system.

## Picture presentation

| Choice | Recommendation adopted | Strongest alternative and tradeoff |
|--------|------------------------|-----------------------------------|
| GG aspect | Preserve released 6:5 PAR; optional square pixels | Square default is familiar but silently changes a released default without new panel evidence |
| Full field | Adapter view, native crop/hash unchanged | Switching to SMS mode also changes emulation, making a viewing option surprising |
| Geometry | Completed-field metadata and standard geometry notifications | Fixed letterbox avoids resizing but changes native output and adds blanks |
| Rendering | Shared profiles and monotonic bounded catch-up | A complete per-dot fetch model costs complexity without a reproduced need |
| Uncertain edges | Retain best-supported constants; research H-counter origin | Moving IRQ to fit an inferred count can tune the wrong coordinate |

**Why:** Players retain a stable picture; developers see native pixels; integrators get consistent callback
dimensions; portability/performance retain fixed memory. Existing tests and host contracts are stronger evidence
for preserving the default than a conflicting preparation recommendation. No change to hash semantics is needed.

**Adversarial pass:** A mode-switching ROM must not move the host deadline. A CRAM-writing loop must not redraw
earlier pixels. Geometry refusal must not hang. Full field can contain unwanted content and must be labelled.
Unknown GG 240-line LCD behavior must remain qualified. SMS1 is a profile, not an independent emulator.

**New evidence:** The original SMS Display Timing thread is accessible again; its later posts explain the
apparent IRQ discrepancy through H-counter origin. It does not establish every revision's behavior. External
core documentation is used for user-visible precedent, never as licensed emulator implementation source.

## Reset and input

| Choice | Recommendation adopted | Strongest alternative and tradeoff |
|--------|------------------------|-----------------------------------|
| Restart | SMS SOFT, GG POWER; battery preserved | Physical switch pulse can be ignored by a game; POWER-all discards SMS work RAM unnecessarily |
| Physical Reset | Separate Select input level where the machine has the switch | Direct CPU reset bypasses hardware-visible behavior |
| Opposites | Independent direction bits | Neutralizing or last-input priority hides host policy or adds ordering state |
| Disconnect | Fresh released state | Retaining the last state causes stuck input and false edges |
| Replay body | Bounded absolute per-frame text | Delta events are concise but missed releases leak across frames; BK2/JSON adds scope/deps |

**Why:** The existing library reset operations and vendored libretro contract make Restart useful while
retaining physical switch semantics for game input. Explicit absolute states are reproducible across runner
and host. Console switches have one source; native GG Start and SMS Pause remain distinct.

**Adversarial pass:** Hold Pause, disconnect/reconnect, opposing directions, port-2 activity, missing callbacks,
invalid high bits and truncated input. Polling the host must not clear guest lag. Device aliases count only
when decoded by that hardware profile. The exact body grammar is recorded in context so planning need not ask.

## Saves

| Choice | Recommendation adopted | Strongest alternative and tradeoff |
|--------|------------------------|-----------------------------------|
| Media exposure | Classify once at load; exact stable persistent span | Always exposing 32 KiB is easy but mislabels volatile RAM and guesses battery hardware |
| Unknown persistence | Run normally; refuse an explicit save request with a reason | Silent no-op loses user progress; a generic override adds API/policy without current need |
| Own fixture | Hash-keyed fixture facts through production classification | Filename/test-only bypass does not exercise real identification |
| Imports | Short pads $FF; oversized rejects before apply | Truncation accepts another game's/format's bytes without evidence of intended interop |
| Filename | ROM stem plus .srm in supplied existing directory | .sav follows a deferred Playstead example but differs from cartridge/RetroArch convention |
| Persistence | Exclusive same-directory temp, checked replacement outside signal handler | Direct truncate-write destroys the last good file; fixed .tmp paths collide |

**Why:** Players keep raw files usable by hosts; integrators get fixed memory lifetime; the core stays free
of files and clocks. Exact export length is separate from physical allocation. Known nonbattery titles need
no save. Same-stem titles need separate directories; no speculative namespace or sidecar format is added.

**Adversarial pass:** Existing mapper masks assume power-of-two allocation; the claimed 24 KiB entry must not
be wired into that mask. Test bank/size behavior after research. Same-size wrong saves cannot be authenticated
without metadata and must not be falsely called validated. A partial write, flush/close or replacement failure
must report failure; SIGTERM is not success if saving failed. Atomic replacement is not a universal power-loss
guarantee. No locks/journals are needed for the supported single-writer save-directory usage.

## Compatibility reporting and evidence

| Choice | Recommendation adopted | Strongest alternative and tradeoff |
|--------|------------------------|-----------------------------------|
| Claims | Specific predicates; boot smoke differs from proven fixture play | A commercial playable percentage overstates boot observations |
| CSV | Keep required path and columns, support quoted fields | Restricted CSV contradicts verbatim CC0 titles; TSV contradicts explicit GAME-06 path |
| Ratchet | Stable normalized identity, protected values/content/deletions | Hash-bearing predicate keys and script paths alone permit expectation replacement |
| Local library | Attempt every supported candidate, keep private observations local | Matches-only execution misses the requirement; publishing unknown identities violates privacy |
| Host e2e | Simplify/reuse existing driver, in-process semantics for breadth | Another orchestration layer exceeds the 300-line budget and repeats past sibling failures |

**Why:** The public ledger remains useful and honest, private images remain private, and regressions cannot
be hidden by changing a key. The production core and small fixtures carry confidence; governance does not
become the product. Existing tests/retroarch already consumes 293 of 300 allowed lines.

**New evidence:** Public pinned CC0 SMS descriptions contain commas. The no-comma CSV schema therefore
cannot preserve those titles. The existing phase-2 HALT tests establish that sampled HALT is not a valid
boot-failure test. The ratchet's hash-in-key issue follows directly from its documented predicate grammar.

**Adversarial pass:** Duplicate keys, deleted passing rows, changed scripts, altered expected digests, changed
profiles, zero evaluated images, static valid screens, self-declared ROM verdicts and whole-build artifact
uploads. Local attempts on unlisted inputs must not leak their identities through public update/report paths.
No-input/press/held/repeated-edge fixtures and wrong/missing save controls distinguish actual behavior.

## Discretion and deferred work

The owner delegated exact implementation structure and source-grounded hardware follow-up. No implementation
or plan was produced during this discuss step. Context records concrete defaults, the observations that would
justify changing uncertain models, scoped tests, and the preparation conflicts to reconcile in the phase PR.

Sound, BIOS/peripherals/extra boards, save interop measurements, public states/movie headers, hooks and per-dot
hardware accuracy retain their existing later phases. Unknown-homebrew save overrides and multiwriter saves
remain unrequested additions. The post-merge CI todo stays a separate ci: PR; it was not folded into this phase.

Sources and code anchors are in 03-CONTEXT.md's canonical references. Primary docs were read; prohibited
emulator source was not opened or copied. No new dependency, owner hardware task or release ceremony is added.
