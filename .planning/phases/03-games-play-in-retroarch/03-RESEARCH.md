# Phase 03: Games play in RetroArch - Research

**Researched:** 2026-10-20
**Domain:** SMS/GG mode-4 video, input, cartridge persistence and host conformance
**Confidence:** MEDIUM

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions and Agent's Discretion

### Picture presentation and rendering

- **D-01:** Keep the released default Game Gear PAR 6:5 (160:144 yields DAR 4:3). Offer an explicitly named
  square-pixel presentation option (native DAR 10:9) in the adapter. Neither changes pixels, timing, machine
  selection or hashes. HOSTS §1's square-default recommendation loses to the released behavior documented in
  that same section and ARCHITECTURE §5; there is no new panel measurement justifying a default reversal.
- **D-02:** Native GG crop remains the default; full VDP field is an optional presentation view. Never enter
  SMS compatibility mode merely to expose offscreen pixels. Apply the selected PAR to the displayed extent,
  rather than stretching every extent to 4:3. Label full-field viewing as exposing normally hidden content.
  SMS stays 256×192/224/240, including the hardware mask column; borders and automatic left-column crop wait.
- **D-03:** Keep RGB444 field/crop identity and host palette conversion. Retain the no-content test-card
  oracle unchanged. Report PAL's documented PAR rather than returning NTSC 8:7 for both standards. The field
  buffer is bounded by 256×240; GG 192/224-line crop origins are (48,24)/(48,40). Preserve the documented
  GG 240-line LCD freeze as the best-supported default, confidence-qualified and tested for deterministic,
  bounded delivery. Keep LCD behavior distinct from full-field rendering; do not invent a hardware crop.
- **D-04:** Return consistent dimensions, pitch, crop and aspect for a completed field, even through mode
  changes. Use libretro SET_GEOMETRY inside retro_run when displayed geometry/aspect changes; max storage is
  established at load and never grows. A geometry notification refusal must not stop bounded frame delivery.
  Exact register latch policy is research/planner discretion grounded in SMS-VDP, not an owner question.
- **D-05:** Keep SMS2 default, with SMS1 and GG differences in shared profiles. Expose the in-scope profile
  and GG SMS-mode overrides coherently through media/runner/core options; auto detection uses table/header
  facts. Unsupported TMS/SG modes remain unsupported, not a second renderer added under this phase.
- **D-06:** Keep the settled line renderer and monotonic catch-up before visible mutations. Bound work by
  the field, 64 sprite entries, eight visible sprite slots and emitted segments; a hostile write loop must not
  repeatedly redraw earlier pixels, allocate memory, or postpone the fixed frame deadline. Preserve the
  phase-2 instruction-boundary overshoot guarantee through height/interrupt changes.
- **D-07:** Keep named 607/608 interrupt constants, 600 latch and 604 V-counter defaults unless stronger
  evidence changes the model. The newly accessible final posts of the SMS Display Timing thread reconcile
  the apparent 654 estimate through H-counter origin; research that coordinate before changing an IRQ edge.
  A binary oracle is a locator, never a gate or hardware verdict. Preserve per-frame vscroll and the specific
  SMS1 zoom-slot rule in ARCHITECTURE §13; DEC.44's shorthand "first four" is not a second independent rule.
  Capture assumptions and distinguish tests of our selected model from measurements of real hardware.

### Reset and input behavior

- **D-08:** Separate RetroArch Restart from a physical Reset input. Restart invokes existing SOFT reset on
  SMS and POWER on GG, preserving battery bytes and generation in both cases. Port-1 Select is the physical
  SMS Reset level on profiles that have that switch. It changes the pin, never resets the CPU directly.
  This resolves contradictory HOSTS §1 / ARCHITECTURE §9 wording: libretro.h prefers soft reset, and a Restart
  that only pulses an ignored game input can fail to restart. POWER everywhere is the acceptable alternative
  only if a demonstrated software case needs it. Do not change the library's existing SOFT/POWER contracts.
- **D-09:** Keep D-pad, B=1, A=2 and port-1 Start=SMS Pause/native-GG Start. Console controls have one source
  (port 1); SMS has two independent pads, native GG one local pad. GG SMS compatibility mode maps Start to
  Pause/NMI per its hardware model. Advertise only pad/none devices that actually work in this phase.
- **D-10:** Pass independent direction bits through, including U+D and L+R. SOCD cleaning is a host policy;
  last-input priority would add hidden ordering and state. Poll fresh absolute input once before each frame.
  Missing callbacks, disconnected devices and RETRO_DEVICE_NONE release pins immediately and cannot leave
  stuck buttons. A reconnect does not manufacture a Pause edge from stale host state.
- **D-11:** Core edge-detects Pause; holding it produces one NMI at the settled last-line timing. Native GG
  Start is an active-low port level, not an NMI. Lag credit comes only from actual CPU reads of controller
  ports that the active profile decodes, including valid mirrors; peeks and host polling do not count. Resolve
  GG alias claims against the full-decode model during research instead of crediting undriven reads.
- **D-12:** Runner --input remains the headerless body of the future movie format, with one absolute record
  per frame. Choose this small textual grammar: `p1=00 p2=00 pause=0 reset=0 start=0`. Pad masks are two hex
  digits, bits 0..5 = U,D,L,R,1,2; high bits are invalid. Console fields are 0/1. Accept LF/CRLF and an optional
  final newline; no blank/comment records in the body. Without --input all inputs are released. Require exactly
  --frames records; reject unknown fields/bits, overlong lines, early EOF and surplus records as input errors.
  Use a bounded parser and existing file handling, not JSON, BK2 or a replay dependency. --port1/--port2 accept
  the in-scope pad/none choices; none suppresses that pad. Frame numbering and boundary application must be
  documented with a tiny example and tested at the first press/release. Future movie headers remain deferred.

### Battery identification, import and persistence

- **D-13:** At load, classify physical mapper RAM separately from persistent media using the public pinned
  CC0 hash metadata. Known volatile Sega/Codemasters RAM stays out of BATTERY/SAVE_RAM. Export exactly the
  classified persistent length, fixed pointer and length until unload; never change exposure on first RAM
  enable. Unknown ROMs can still run through supported heuristics, but persistence is explicitly unknown and
  no battery span is guessed. An explicit --save-dir request for unknown persistence is refused before running
  (exit 77 with the reason), avoiding silent loss. Known nonbattery titles need no file. General homebrew
  persistence overrides are deferred until an actual use case needs them.
- **D-14:** The own battery fixture has explicit hash-keyed metadata consumed by the production classification
  path, separate in provenance from CC0 commercial data. Store its reproducible expected identity as a fixture
  fact and verify it against the assembler result; do not introduce a core-build/fixture-build dependency cycle,
  a filename match or a test-only loader bypass. Public CC0 titles absent from the ledger remain identifiable.
- **D-15:** Verify the claimed 24 KiB entry against the pinned list, then model physical bank decoding and
  persistent length independently. Existing `ram_size - 1` masking assumes powers of two: assigning 24576 to
  that field is invalid. Exact exports do not authorize truncating physical RAM or inventing modulo decoding.
  Research settles the mapping from documented board facts; cover supported sizes/banks with independent tests.
- **D-16:** Raw import: missing file means fresh battery; shorter files (including empty) pad with $FF;
  oversized files reject without applying bytes (runner exit 4). Read at most the expected length plus one.
  Other I/O errors fail with exit 1. Exact-size wrong-game data cannot be detected by a raw format: preserve
  that format and use a wrong-save control that produces a different continuation, not a fake validation claim.
- **D-17:** Runner saves to an existing --save-dir as `<ROM stem>.srm`, consistently with CARTRIDGES §4 and
  RetroArch. Resolve HOSTS §4's deferred Playstead `.sav` example in favor of .srm for this runner. No automatic
  dual-extension import or hash namespace. Explain same-stem collision behavior and use separate save dirs.
  Reject --save-dir without content. The libretro adapter exports memory; the frontend owns file I/O.
- **D-18:** Increment the 64-bit generation on each mapped persistent write, including same-value writes.
  Reads, RAM-enable writes, volatile writes and import do not increment it. Snapshot after import; flush only
  dirty generations and acknowledge persistence only after successful replacement. Reset preserves bytes and
  generation. API additions need actual consumers/tests under the existing size-tag and version rules.
- **D-19:** Persist with a uniquely and exclusively created temporary in the destination directory, checked
  full write/flush/close and platform replacement semantics. Never delete/truncate the last good target first;
  clean up only this process's temp. Use POSIX/native Windows facilities, not a filesystem package. Finish the
  bounded frame after SIGTERM/SIGINT, then flush outside the signal handler; exit 0 only on success, exit 1 on
  failure. Test first create, overwrite and failure preservation. Request synchronization where supported, but
  distinguish atomic visibility from universal power-loss durability. Concurrent writers to one save are not a
  supported multiwriter protocol; no locks, journal, backup rotation or daemon is added in this phase.

### Compatibility reporting, fixtures and verification

- **D-20:** A result asserts its predicate at a frame cap/profile/revision. `boots@F` is a strict boot smoke
  observation, never a playable/completable claim. Keep its display/distinct-frame/first-fault observations
  explicit; a static screen may fail this heuristic without being a broken game. Remove the instantaneous
  "CPU not HALTed" rejection: phase 2 proves healthy interrupt-driven HALT loops. Do not replace it with an
  invented general stall detector. Fault records stay diagnostic; a failed strict smoke check is not proof
  of a gameplay defect. No sound claim before phase 4.
- **D-21:** Keep required `tests/compat/ledger.csv` and its nine logical columns. Use conventional quoted CSV
  for titles containing commas/quotes, with doubled embedded quotes. The pinned CC0 titles demonstrate that
  CONFORMANCE §5's no-comma rule conflicts with verbatim titles. Share a small parser under the CMake/POSIX-sh
  repository-check rules; handle semicolons without treating them as CMake list separators. No CSV dependency.
- **D-22:** Ratchet identity includes ROM, resolved machine/profile, region/TV, board/device configuration,
  input-script identity and normalized predicate/frame/address parameters. Expected hashes are protected
  values, not part of identity: changing `frame@F=H` must not create a new unprotected case. Protect script
  content as well as its path. Deleted passing cases, duplicate normalized keys, changed expectations and
  profile substitutions cannot bypass comparison. Preserve the dated behavior-revision/expiry exception
  mechanism and protected base-branch judge. Keep these in the existing ratchet, not a new governance service.
- **D-23:** The local library run enumerates every candidate regular image in COGMASTER_ROM_DIR and attempts
  supported images with bounded stepping, even when absent from the CC0 list. Classification/unsupported
  refusals and malformed/I/O cases stay distinct. Without an accepted expectation report unknown plus local
  observations; never make current output its own golden hash. Zero evaluated images must say zero coverage.
  This resolves GAME-06's "every image" versus CONFORMANCE §5's matches-only registration: public matching
  controls export eligibility, not local execution. All enumeration and private diagnostics stay local.
- **D-24:** Only public CC0 identities/titles and allowed predicate facts/date/core revision may enter the
  public ledger through the explicit update path. Unknown names/digests, paths, screenshots, RAM/save bytes
  and commercial traces never enter commits, CI artifacts or evidence. Keep local results outside tracked
  files; never upload the whole library build directory. CI tests this preset using a temporary fixture dir.
- **D-25:** pass/fail describe evaluated predicates; unknown lacks an accepted evaluation/expectation;
  unsupported names the unavailable board/device/profile; infrastructure_error names a failed runner/judge.
  Do not call all refusals game failures or treat table membership as a support guarantee. Generated public
  docs show predicate/fixture counts and their limits. Keep .info experimental through v2.0.
- **D-26:** Extend the named WLA-DX fixtures and mailbox, with on-screen own glyphs inside the GG window.
  boot_to_play proves independent no-input/press/held/repeated-edge cases for both SMS pads and GG Start;
  input changes counter, scroll and frame. vdp_lines covers specified rendering features, heights, counters,
  flags and scroll bands. battery proves fresh-process continuation after exit/SIGTERM, missing/wrong controls.
  Expectations derive from hardware documents and independent calculations, not just two paths to the same bug.
- **D-27:** Reuse/simplify the existing RetroArch screenshot driver before adding content/input/GG coverage:
  tests/retroarch is already 293 of 300 allowed lines. Keep richer semantics in the in-process host and fixtures;
  the hosted session must still genuinely deliver input to the loaded game and compare its scrolled frame.
  Test-only input injection must not become the installed adapter's input source. Keep existing test-card
  coverage while sharing the driver. No stateful shell harness or expanded e2e framework.
- **D-28:** Existing inventory/negative-control, zero-allocation, endian/hash and bounded-run checks carry
  forward. New behavior is tested in ci; one-time facts get command/exit/run URL, never committed command logs.
  Repository-check code stays below one tenth of src lines; consolidate checks rather than waive that budget.
  The representative performance workload becomes boot_to_play; arming the perf gate remains the separate
  post-merge CI follow-up. Review precedes verification; one GSD step ends here.

### Agent's Discretion

- Research and plan the smallest module/API split, mapper metadata generation, native replacement helpers,
  exact option keys consistent with established ones, and fixture decomposition. No new runtime dependency
  is justified by the discussion. Reuse runner hashes, fixture includes, frame conversion and signal handling.
- Resolve measured hardware details (GG aliases, 24 KiB board, H-counter origin, height/latch edge cases)
  through cited research/tests. A source conflict is not a request for owner verification or a license to
  keep changing constants until another emulator agrees. Record confidence and retain best-supported defaults.
- Reconcile stale preparation prose listed above during this phase's PR. These decisions clarify that prose;
  they do not reopen delivery policy, CPU vectors, machine architecture or later milestone boundaries.

### Deferred Ideas (OUT OF SCOPE)

## Deferred Ideas

- Phase 4 audio and OPLL; v2 BIOS, peripherals, extra boards and binary-measured save interop; v3 public states
  and versioned movies; v4 measured per-dot accuracy; v5 hooks/debuggers/importers. No new phase is proposed.
- Generic unknown-homebrew persistence overrides, save namespaces/sidecars and multiwriter coordination require
  a demonstrated need. Existing unknown images still run without a false persistence guarantee.

### Reviewed Todos (not folded)
- `.planning/todos/pending/post-merge-ci-follow-ups.md` remains its separate ci: PR: perf arming, nightly reporter,
  Dependabot and wasm promotion are already decided there. No release or discuss dependency is invented here.
</user_constraints>

## Summary

Implement a shared, bounded mode-4 renderer over the existing cycle-stamped bus, then widen the same path to sprites, height changes, native GG, input, classified battery persistence and compatibility reporting. Keep one frame deadline and preserve the no-content oracle. The current adapter still refuses content and ignores controller devices; content loading is therefore part of the first tracer, not a late integration assumption. [VERIFIED: src/frame.c:35-36; libretro/libretro.c:197-208,298-332]

Three findings change implementation details without reopening the context: GG controller aliases are specifically C0/C1, not arbitrary Cx mirrors; H-counter zero is offset from the first SMS active dot; and the pinned Moldorian 24 KiB declaration is real metadata but is insufficient evidence of physical RAM decoding. Keep its identity/classification, investigate the board narrowly, and refuse unsupported persistence geometry rather than install a guessed mask/modulo mapping. [CITED: https://www.smspower.org/Development/PeripheralPorts] [CITED: https://www.smspower.org/forums/8161-SMSDisplayTiming] [VERIFIED: pinned MAME hash/gamegear.xml:4901-4915]

**Primary recommendation:** lead with a production SMS boot/input/scroll/frame path through library, runner and in-process libretro; schedule the real RetroArch input/screenshot proof before broadening the renderer. All remaining work expands that tested path. [VERIFIED: .planning/phases/03-games-play-in-retroarch/03-CONTEXT.md, D-26–D-28]

## Architectural Responsibility Map

| Capability | Primary owner | Secondary owner | Rationale and provenance |
|---|---|---|---|
| Beam, deadlines, stamped access | Core bus | CPU | Retain instruction-boundary stepping and fixed frame origin; VDP adds events, never resets the frame deadline. [VERIFIED: src/bus.c:21-40,77-126] |
| Rendering, CRAM, sprite flags, latches | Core VDP | Frame copy | Catch up before a visible mutation; snapshot completed geometry alongside pixels. [VERIFIED: .planning/preparation/ARCHITECTURE.md §§2,5,12–13] |
| Pads, Pause/Start, lag credit | Core I/O | Runner/libretro supply absolute input | Host poll is not a guest port read; only successful decode earns lag credit. [VERIFIED: 03-CONTEXT.md D-09–D-12] |
| Cartridge identity, RAM layout, persistence knowledge | Core loader/metadata | Own build-time generator | Same classification for all hosts, independent of ledger membership. [VERIFIED: 03-CONTEXT.md D-13–D-15] |
| Save import, raw span and generation | Core | Runner/libretro consume span | Validate length before mutation; no core filesystem work. [VERIFIED: 03-CONTEXT.md D-16,D-18; .planning/preparation/ARCHITECTURE.md §7] |
| Atomic file replacement, signals | Runner/native platform helper | Core finishes frame | Frontend owns libretro save files. [VERIFIED: 03-CONTEXT.md D-17,D-19] |
| PAR, optional full field, host RGB conversion | Adapter | Core reports completed geometry | Presentation never changes emulation or native hashes. [VERIFIED: 03-CONTEXT.md D-01–D-04] |
| Ledger, local enumeration, docs, ratchet | CMake/POSIX sh tooling | Runner predicates | Keep private execution local and public updates explicitly filtered. [VERIFIED: 03-CONTEXT.md D-20–D-25] |

## Project Constraints (from AGENTS.md)

- Implement library, headless runner and libretro core only; no device/window backend.
- Own Z80/VDP/mapper/ports code from documents. Read external emulator source only after a permissive per-file licence header; never open GPL/LGPL/non-commercial/unlicensed emulator source. Keep notices for reused expression and CC0 metadata provenance.
- No ROM/BIOS/save bytes except manifested fixtures; no personal identity or paths. Public pins must answer successfully through GitHub API. Noreply commits, hygiene hooks, phase branch and PR workflow.
- Integer-only core, state per instance, no wall clock, no allocations after load, deterministic output and bounded run calls for hostile programs.
- Every behavior change has a CI test and contemporaneous docs. README remains short/user-facing; hardware behavior belongs in docs/hardware and embedding prose, not header growth.
- Automation-first verification; the agent judges documentation, images and hosts. Owner checks only identity-bound consent or their hardware. Evidence is command, exit status and run URL; do not commit logs.
- Repository checks use CMake -P or POSIX sh and remain below one tenth of src lines from this phase. Hosted e2e code remains at most 300 lines and counted. Every gate states its prevented fault. Consolidate checks to meet budgets.
- At most two code-review fix rounds and one gap round; review precedes verification. Tool/test review findings become seeds. No quotas, ceremonial qualification or new release gates.
- One GSD step, then stop; no auto/chain commands or hygiene bypasses. Record runtime/model in STATE. Every phase releases; ship confirms head CI and green release PR.
- Plan summaries need requirements-completed and block-style coverage entries containing id, description, requirement, verification kind/ref/status and human_judgment. Acceptance checks use exit status and tolerant regexes.
- Small modules, fixed-width types, plain control flow, cogmaster_ public prefix, module prefixes internally, source-cited hardware comments and no planning IDs in code/tests/user docs.
- Follow the remove/reuse/standard/platform/minimum-code ladder; prefer small permissive seeds to dependencies. Imported lessons remain hypotheses until reproduced; lesson export uses the existing template.

All directives above are direct constraints, not research proposals. [VERIFIED: AGENTS.md:1-97]

<phase_requirements>
## Phase Requirements

| ID | Required behavior | Research support / independent acceptance |
|---|---|---|
| GAME-01 | Mode-4 name table, 8×8/8×16 sprites, zoom, priority, flips, mask, locks and 192/224/240 heights; fixture hashes equal across platforms | Pixel-coordinate and planar-bit calculations plus independent fixture measurements; preserve all existing test-card hashes and add playing-frame hashes to lane comparison. |
| GAME-02 | Dot-exact frame/line interrupts, H/V counters and status flags; mailbox matches expected.csv; status reads clear flags | Edge-minus-one/edge/edge-plus-one bus tests, all six V-counter sequences, H-latch tests and fixture handler measurements. |
| GAME-03 | Native GG RGB444 crop, ports, Start and SMS compatibility; hosted crop equality | CRAM even/odd latch tests, crop/field consistency, metadata/override mode tests and shared hosted driver. |
| GAME-04 | Two pads, Pause and regions/TV; input changes counter/scroll/frame; no read is lag | Absolute frame input and decode-only credit; separate no-input/press/held/release/repeated-edge fixtures for both SMS pads and GG Start. |
| GAME-05 | Raw battery span/generation, atomic runner exit/SIGTERM save, fresh-process continuation and differing controls | Classified metadata, mapper bank alias tests, validate-before-apply imports, generation tests and first-create/overwrite/failure-preservation process tests. |
| GAME-06 | Nine-column ledger/docs, ratchet, local all-images library preset, experimental info | Small quoted CSV parser and canonical predicate identity; base-branch judge; temporary fixture directory checks supported, unknown, refused, malformed, empty and I/O paths. |
| GAME-07 | One hosted SMS session boots, receives input, scrolls and matches runner within e2e budget | Share existing driver, deliver input through frontend callbacks, compare changed and released frames; keep semantics primarily in-process. |
</phase_requirements>

Requirement descriptions summarized from REQUIREMENTS.md:35-41; context D-01–D-28 resolves preparation conflicts. [VERIFIED: .planning/REQUIREMENTS.md:35-41]

## Standard Stack

| Component | Selected stack / pin | Purpose and confidence |
|---|---|---|
| Runtime | Existing C17 core, C standard library host code, CMake/CTest | No new runtime package. [VERIFIED: CMakeLists.txt; .planning/preparation/ENGINEERING.md §2] |
| Fixtures | Existing WLA-DX v10.7, commit 91c52b1f4ef3cc8ba3c0638f7536539579af6a9f | Tool binary only; existing SHA-256 tarball verification and build. API pin returned success this session. [VERIFIED: pins.txt; gh api repos/vhelin/wla-dx/commits/91c52b1f4ef3] |
| Metadata | MAME 1d7a454ed79e97d35d2a9f127ac029ead94711d6; three hash lists | Each downloaded file begins with license:CC0-1.0; API pin returned success. Generate compact immutable facts, no runtime XML parser. [VERIFIED: gh api repos/mamedev/mame/commits/1d7a454ed79e; hash/sms.xml:1-4; hash/gamegear.xml:1-4; hash/sg1000.xml:1-4] |
| Host | Existing pinned libretro header and RetroArch 1.22.2 DMG | Reuse existing ABI and hosted CI, without version upgrades in this phase. [VERIFIED: pins.txt; libretro/libretro.h:1532-1554,7675-7680] |
| Repository checks | CMake -P and POSIX sh | Share CSV normalization/parser and existing failure driver; no dependency. [VERIFIED: AGENTS.md rule 7; tests/cmake/expect_fail.cmake] |
| Native persistence | POSIX exclusive creation/fsync/rename; Windows exclusive CreateFile/FlushFileBuffers and native replacement | Keep platform code outside core. [CITED: https://pubs.opengroup.org/onlinepubs/9799919799/functions/rename.html] [CITED: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew] |

Package installation/legitimacy audit is not applicable: recommend no registry packages. New build-time metadata generation should be an own small tool using the repository's existing CMake/C facilities, not a new XML package. [VERIFIED: 03-CONTEXT.md, Agent's Discretion; AGENTS.md rule 7]

## Architecture Patterns

### Data flow

~~~mermaid
flowchart LR
    ROM[Caller ROM bytes] --> Probe[Strip copier header and hash lookup]
    Probe --> Profile[Resolve board / machine / region / GG mode]
    Profile --> CPU[CPU + fixed frame deadline]
    Input[Absolute host frame input] --> Pins[Pin levels and Pause edge]
    Pins --> CPU
    CPU --> Decode[Stamped memory / port decode]
    Decode --> VDP[Catch up monotonically + apply mutation]
    VDP --> Field[Completed RGB444 field + geometry]
    Field --> Runner[Native crop hash / PNG]
    Field --> Adapter[PAR / crop / RGB conversion]
    Adapter --> RA[RetroArch callback]
    Decode --> RAM[Physical cart RAM]
    RAM --> Battery[Classified persistent span + generation]
    Battery --> Save[Runner atomic replacement or frontend persistence]
~~~

Diagram is a recommended decomposition of the locked architecture, not a claim that every stage exists. [VERIFIED: .planning/preparation/ARCHITECTURE.md §§2–7; 03-CONTEXT.md D-01–D-19]

### Current seams and required changes

| Existing seam | What research found | Plan action |
|---|---|---|
| VDP timing/ports | Renderer absent; mode bits stored but fixed 192-line behavior; CRAM is byte store. [VERIFIED: src/vdp.c:1-4,127-138,203-214] | Keep timing/port ownership; split renderer into a small companion module if needed to respect module size. Add line state, rendering cursor, sprite slots, GG latch and counters. |
| Frame/info | Loaded fields are never drawn; crop and height fixed; PAL rate exists but PAL PAR falls through to NTSC. [VERIFIED: src/frame.c:11-28,35-36,81-105; src/instance.c:208-238] | Introduce one completed-frame descriptor used by info/frame and host conversion. Height cannot be read live independently from the pixels returned. |
| Bus | Fixed origin/deadline and precise stamps already exist. [VERIFIED: src/bus.c:21-40,77-126] | Add latch/counter/line-interrupt/render events without resetting origin on mode writes. |
| I/O | Buttons always released; GG/full decode rejects C0/C1; no host input yet. [VERIFIED: src/io.c:58-100] | Add absolute input state, decoded read tracking, pin mixing and a Pause edge latch. Share exact decode with lag accounting. |
| Mapper/cart | Physical allocation and persistence are not separated; power-of-two masks used on read and write. [VERIFIED: src/internal.h:120-132; src/mapper.c:86-94,122-138] | Add metadata knowledge/persistent length separate from physical RAM size and mapper address translation. |
| State visitor | Existing timing/profile consistency checks; new fields must participate in validate/apply. [VERIFIED: src/state.c:80-107,110-122] | Visit/validate rendering cursor, latches, input edge/lag state and generation as appropriate; invalidate derived render buffers safely if reconstructed. Do not add public save states. |
| Runner | Own hash helpers, file loading, signals, mailbox and 256×240 pixel storage. [VERIFIED: runner/main.c:75-133,245-291; runner/stop.c] | Add bounded input, predicates and native save helper with actual consumers; reuse hash and PNG conversion. |
| Adapter | Content refused; input polling has no consumer; all devices ignored; save spans absent. [VERIFIED: libretro/libretro.c:197-208,232-265,298-332,356-367] | Content load from game.data, controller/options descriptors, callbacks → absolute core input, bounded video/silence and stable memory exports. |

Public additions must append rather than renumber. Source-of-truth examples read this session: "COGMASTER_API_VERSION 2", "COGMASTER_ABI_VERSION 1", "COGMASTER_BEHAVIOUR_REVISION 1", "COGMASTER_STATE_FORMAT_VERSION 2"; existing machine values "COGMASTER_MACHINE_AUTO 0u", "COGMASTER_MACHINE_SMS2 1u", "COGMASTER_MACHINE_GG 2u"; reset values "COGMASTER_RESET_SOFT 0u", "COGMASTER_RESET_POWER 1u". Do not assume SMS1 or input/memory APIs already exist. Append a size-tagged input and the battery/counter surfaces their actual consumers need, updating ABI assertions and legacy-caller tests once. [VERIFIED: include/cogmaster.h:14-17,30-32,46-47,87-105,145-155]

### Renderer and timing model

Use a line-local cursor that only advances. Each visible interval reads current CRAM/VRAM before mutation, emits RGB444 once and updates collision at emitted dots. Scan at most 64 SAT entries for a following line and retain at most eight visible slots. Off-window GG sprites still consume slots; overflow depends on a ninth Y match, not on its opaque pixels. Keep sprite pattern data available for later writes without rescanning earlier pixels. [VERIFIED: .planning/preparation/SMS-VDP.md §§3,9; .planning/preparation/ARCHITECTURE.md §12; 03-CONTEXT.md D-06]

Use the selected 600 hscroll/display/next-line latch, 604 V-counter change and 607/608 frame/line IRQ edges. Those are model constants with different evidentiary strength, not four equivalent hardware measurements. Event-at-or-before-access ordering makes a status read at a flag edge see and clear the flag; /INT must be recomputed immediately on flag clear or IE writes. Keep line pending distinct from status bits. [VERIFIED: src/vdp.c:15-18,40-55,88-93,147-156; .planning/preparation/SMS-VDP.md §4; 03-CONTEXT.md D-07]

**H-counter reconciliation:** the 2008 final posts explain GG LCD begins at HC=$20; subtracting the 48-dot crop puts SMS active dot zero at HC=$08. The official diagram gives the $00–$93 then $E9–$FF sequence. Therefore convert active-origin ticks to counter-origin ticks before table lookup and TH capture; moving IRQ to 654 is unjustified. This is a documented coordinate inference, with sub-dot phase confidence lower than the IRQ measurement. Test active dot 0 → $08 and GG left edge → $20, wrap/jump boundaries and held TH-latched values; retain named 607/608 IRQ constants. [CITED: https://www.smspower.org/forums/8161-SMSDisplayTiming] [CITED: https://www.smspower.org/Development/GGOfficialDocs]

A concrete derived counter model is u=(tick+32) mod 684, then u<592 ? u/4 : (u+340)/4, with integer division. This translates 16 pixels = 32 ticks, retains the documented sequence and gives $F4 near the selected IRQ edge. Record it as a selected model, not a new scope-backed phase measurement. An implementation test must compute expectations from counter intervals/coordinate anchors rather than calling this production formula. [CITED: https://www.smspower.org/Development/GGOfficialDocs] [CITED: https://www.smspower.org/forums/8161-SMSDisplayTiming]

Latch vertical scroll per frame under the existing selected model. For SMS1 horizontal zoom use wide(i) iff i+4<count, not a fixed first-four rule. Keep unsupported mode combinations explicitly unsupported and maintain bounded delivery. Snapshot height/crop at a documented frame boundary; mid-frame height writes must not retroactively relabel completed rows or change the deadline. Native GG 240-line LCD freeze remains power-reset state distinct from a full-field view, using the last valid LCD image and stable crop metadata. [VERIFIED: .planning/preparation/ARCHITECTURE.md §§2,13; 03-CONTEXT.md D-03–D-07]

### GG decode, pins and presentation

GG controller ports decode DC/DD plus the specific C0/C1 aliases; other controller-range reads stay undriven. Two independent SMS Power pages agree on those aliases. Their broader port-mirror claims differ from preparation's blanket full-decode model: limit this phase's correction to evidence-backed cases, documenting broader uncertainty instead of broadening every decode. Lag credit must reuse the successful controller decode and I/O-enable check. Native GG port 00 is Start/region and qualifies; its SMS-mode 00 read is not a controller read. [CITED: https://www.smspower.org/Development/PeripheralPorts] [CITED: https://www.smspower.org/Development/IOPortMapGG]

Read low-active pads through TL/TR and direction pins, mixing outputs from I/O control rather than simply ANDing a host mask into a fixed byte. Both pads are independent on SMS; native GG local-pad only. Pause edge is core-owned and holding it creates one NMI at the selected last-line timing. Native GG Start is a level; GG SMS-mode Start routes to Pause. Missing callbacks/none release absolute levels. Store/reconcile edge history so reconnect cannot synthesize a press. [VERIFIED: .planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md §3; 03-CONTEXT.md D-08–D-12]

Keep GG PAR 6:5 default and offer square pixels as presentation only. PAL SMS PAR comes from the documented 2950000:2128137 ratio. Max geometry is allocated at load; call SET_GEOMETRY from retro_run when presented dimensions/aspect change, then send the frame even when the call returns false. PAL silence needs a larger capacity: current "LR_AUDIO_CAPACITY 802u" only fits NTSC; PAL rational delivery needs up to 966 stereo frames at 48 kHz. This is necessary pacing integration before sound, not audio emulation. [VERIFIED: libretro/libretro.c:20-23,174-194,214-228; .planning/preparation/ARCHITECTURE.md §5; libretro/libretro.h:1532-1554]

### Metadata and physical persistence

The downloaded CC0 files contain 653 SMS, 823 GG and 214 SG entries; the GG list includes 52 pin_42 SMS-mode records. Retain all three settled inputs, including unsupported entries as classification facts, without claiming their machine/board is supported. The XML has dataarea/rom layouts with multiple chip hashes, so a per-chip SHA-1 is not automatically a whole-image key. Emit verified whole-image keys only for contiguous full-body ROM records; retain/audit compound records separately rather than invent a concatenated digest without bytes. [VERIFIED: pinned MAME hash/sms.xml; hash/gamegear.xml; hash/sg1000.xml, parsed with Python ElementTree this session]

Generator tests must cover duplicate identical facts, conflicting identities, unknown slot/device, numeric decimal/hex sizes, XML entities in titles, battery yes versus absent/ambiguous batt, pin_42, ROM-area size and single/multipart records. Generated sorted read-only data and binary search keep runtime bounded. Add fixture metadata separately using reproducible assembler identity; check its expected SHA-1 against the built fixture without making the core depend on fixture assembly. [VERIFIED: .planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md §§1–2; 03-CONTEXT.md D-13–D-15; recommendation from inspected XML structure]

**Moldorian finding:** the pin explicitly says description "Moldorian - Hikari to Yami no Sister (Japan)", battery "yes", ROM SHA-1 "0dad260b5357b2f61d1452bb73d8321ca53d7262" and RAM size "24576". SMS Power also reports 24KB backup. These verify declared persistent length, not wiring. [VERIFIED: pinned MAME hash/gamegear.xml:4901-4915] [CITED: https://www.smspower.org/Games/Moldorian-GG]

**Physical decoding remains unresolved.** The permitted BSD-3 MAME mapper begins with a non-power-of-two RAM-mirroring TODO and uses modulo; that is not board evidence. Its historical fix for Shining Force II changed the same old 24576 declaration to 32768 with an explicit KM62256 32K×8 chip comment, illustrating why metadata sizes cannot establish wiring for Moldorian. Do not apply that other title's board as Moldorian's board. [VERIFIED: mamedev/mame 1d7a454ed79e, src/devices/bus/sega8/rom.cpp:1-8,408-426; gh api repos/mamedev/mame/commits/6f1b0f5e9c0f, hash/gamegear.xml patch]

Plan a narrow evidence task at the metadata/mapper seam: seek documented Moldorian PCB/chip/address-line facts; if found, encode physical decode and persistent extent separately with independent bank/alias tests. If not, retain the known identity/declared length but return an explicit unsupported-persistence-geometry refusal when persistence is requested; do not create a misleading battery span or claim this board supported. Unknown persistence remains distinct from known-but-unsupported geometry. Continue ordinary supported mapper play where bounded/safe, and cover GAME-05 completely with the own supported battery fixture. No owner hardware check is required and no extra phase is invented. [VERIFIED: 03-CONTEXT.md D-13–D-15; research disposition for unresolved board evidence]

### Saves and atomic replacement

Import at most expected+1 bytes into temporary host storage, distinguish missing from I/O failure and reject oversize before touching core memory; pad short data with FF. Count each CPU mapped persistent write, including same-value writes, exactly once; volatile writes/register enables/import never count. Only an actual successful replacement advances the runner's acknowledged generation. Reset preserves bytes/generation. [VERIFIED: 03-CONTEXT.md D-16–D-19]

POSIX rename of an existing non-directory target keeps a name referring to old or new data during replacement. Use an exclusive same-directory temporary, checked full write, flush, synchronization where supported and checked close, then rename; directory synchronization affects crash persistence. Windows ReplaceFile requires an existing target and its WRITE_THROUGH flag is unsupported; first creation needs a suitable native move. Explicitly test both first creation and overwrite, sharing the smallest native helper and cleaning only its own temporary. Do not claim universal power-loss durability. [CITED: https://pubs.opengroup.org/onlinepubs/9799919799/functions/rename.html] [CITED: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew] [VERIFIED: 03-CONTEXT.md D-19]

### Compatibility and private-library execution

The promised ledger/ratchet is not yet an implementation to reuse; repository inspection found the preparation specification and existing CMake/sh checking patterns. Keep its nine columns and add bounded quoted-CSV parsing shared by judge/docs/update. Do not implement CSV by splitting on commas or interpolating fields into CMake lists: semicolons are legal title content. [VERIFIED: .planning/preparation/CONFORMANCE.md §5; inspected scripts and tests file inventories; 03-CONTEXT.md D-21]

Canonical case identity must include resolved profile/region/TV/board/devices, normalized predicate frame/address and script content identity. Keep expected value outside identity; preserve it as protected data. With only nine columns, carry detailed normalized execution facts in the predicate/config grammar and judge's canonical structure rather than adding columns silently. Reject duplicates, deleted passing cases, changed expected hashes, changed script content and profile substitution. Copy the trusted base judge plus dependencies before checking out the head; adding a parser helper must not make the base judge import the untrusted head helper. [VERIFIED: 03-CONTEXT.md D-22; .planning/preparation/CONFORMANCE.md §5]

For boots smoke, track display-enabled observation, distinct completed frames and first fault over the capped run; sampled HALT cannot be a failure condition. Unknown/no golden must not become pass from a newly observed hash. Library enumeration attempts every candidate regular image locally, including metadata misses; report zero evaluated coverage explicitly. Publish only admitted public CC0 identities/titles and predicate facts through the explicit update path. Use synthetic/own fixture temp directories in CI and never upload the library build tree. [VERIFIED: 03-CONTEXT.md D-20,D-23–D-25]

## Don't Hand-Roll

| Problem | Use | Avoid / source |
|---|---|---|
| Image loading, hashing, PNG | Existing runner helpers and palette conversion | A second digest implementation or image dependency. [VERIFIED: runner/main.c:75-181; runner/sha1.c] |
| Save replacement | Native exclusive creation and replacement semantics | Truncate/delete-old-first writes, own locking/journaling package. [VERIFIED: 03-CONTEXT.md D-19] |
| Fixture assembly/mailbox | Existing pinned assembler and includes | C runtime/provenance work or fixture-only loader bypass. [VERIFIED: tests/fixtures/tests.cmake:64-76; 03-CONTEXT.md D-14,D-26] |
| Host screenshot proof | Simplified existing CMake driver and pixel compare | Stateful shell framework or installed test input source. [VERIFIED: 03-CONTEXT.md D-27] |
| CSV | One small shared bounded parser | CSV package or ad hoc comma split at every consumer. [VERIFIED: 03-CONTEXT.md D-21] |
| Counter/tile/mapper oracle | Independent tables, coordinate cases and mailbox calculations | Golden output generated solely by the production implementation. [VERIFIED: 03-CONTEXT.md D-26] |

## Common Pitfalls

| Fault | Prevention / independent check | Source |
|---|---|---|
| Mid-line writes redraw earlier pixels indefinitely | Cursor monotonicity, allocation counter and hostile VRAM/CRAM/register hammer tests; upper bound on emitted field pixels and sprite scans | D-06; ARCHITECTURE §12 |
| Pixels and dimensions describe different modes | Completed descriptor snapshot, height transition before/at/after latch, buffer-canary/pitch/crop tests | D-04 |
| CRAM even byte appears immediately on native GG | Even-write latch, odd-word commit, latch reuse, wrap and SMS-compatible immediate-write cases | SMS-VDP §6 |
| Shared production formula makes both oracle paths wrong | Explicit planar-color examples, coordinate anchors, table discontinuities and hand-derived mailbox counts | D-26 |
| Controller polling grants lag credit or undriven alias reads do | Read tracking after profile decode and enable checks; peek/poll/disabled/unknown-port controls | D-11 |
| New SMS1 value reuses existing GG enum number | Append after source-verified values and exercise old-size callers | include/cogmaster.h:30-32; D-05 |
| A 24 KiB mask silently aliases bank 0 | Separate physical decode/persistent length; unsupported geometry until justified | D-15; inspected mapper.c |
| Save operation reports success after partial write or failed close | Failure injection at full-write/flush/close/replace; old target byte equality and no unrelated-temp deletion | D-19 |
| Wrong raw save is falsely "detected" | Same-length wrong control loads but continuation differs; only oversize is size rejection | D-16 |
| Expected hash change evades ratchet as a new row | Parse expectation separately from key, protect script content, compare deletions/duplicates | D-22 |
| Empty local library becomes a green support claim | Explicit evaluated counts and unknown/refusal/infrastructure distinctions | D-23,D-25 |
| Added test framework consumes implementation budget | Consolidate check code and reuse driver; no padding src or moving counted files elsewhere | AGENTS rule 7; D-27,D-28 |

Pitfall rows are prescriptive implications of the cited locked decisions and inspected code, not additional phase requirements.

## Code Examples

Existing invariant to preserve, quoted verbatim: "#define BUS_ORIGIN_LINE 192u" and "#define BUS_ORIGIN_TICK 607u". A mode change affects VDP events/output, not these frame/deadline coordinates. [VERIFIED: src/bus.c:21-22]

~~~c
#define BUS_ORIGIN_LINE 192u
#define BUS_ORIGIN_TICK 607u
~~~

Existing bug-prone expression, quoted verbatim: "m->cart.ram[(base | (addr & SLOT_MASK)) & (m->cart.ram_size - 1u)]". This works only when physical RAM size has the corresponding power-of-two decode. Keep it for verified supported power-of-two layouts or replace it with an explicit board translation, never substitute persistent length into it. [VERIFIED: src/mapper.c:86-88]

Recommended frame contract, conceptual pseudocode (new design, no asserted existing symbols):

~~~text
finish intervals of the outgoing field using old latches
publish pixels and their completed geometry together
apply newly latched mode to the next field
run_frame still stops at its original fixed deadline + bounded CPU overshoot
~~~

This ordering follows D-04/D-06 and the existing fixed-origin contract; exact mode latch policy is agent discretion with source-cited tests. [VERIFIED: 03-CONTEXT.md D-04,D-06; .planning/preparation/ARCHITECTURE.md §2]

## Test and Integration Plan

Nyquist-generated Validation Architecture and web Security Domain are explicitly disabled: "nyquist_validation": false and "security_enforcement": false. Do not emit a separate web ASVS/validation ceremony; loader/save/input bounds, fuzz replay and sanitizer checks still belong to behavior implementation. [VERIFIED: .planning/config.json:25-28,50-54]

| Slice | Minimal independent checks before expansion |
|---|---|
| Tracer | One known planar tile and backdrop drawn by a self-authored SMS fixture; no-input/press/release yields hand-calculated counter/hscroll and different complete native hashes; same frame through runner and in-process content-loaded libretro; hosted input/scroll screenshot proof follows immediately. |
| Rendering width | Flips, palette, color-0 priority, left mask, locks, SMS1 address masks/zoom slots, sprite Y wrap/terminator, 8×16, 8/9th slot, clipped/off-window occupancy, collision dot/status-read clearing. |
| Timing/height/GG | All table discontinuities for NTSC/PAL heights, IRQ/latch +/-1 ticks, pending IE enable/disable/status races, TH capture, per-frame vscroll, crop offsets, 12-bit commits, freeze/power recovery, geometry refusal. |
| Input | Both independent pads, SOCD bits passed through, pause held/repeated edges, GG Start/native and SMS mode, reset pin versus Restart, disconnect/reconnect, decoded alias/disabled I/O/peek lag controls. |
| Save | Supported physical banks and mirrors independent of exact span, known volatile/no battery/unknown/unsupported geometry; no fixture bypass; pointer/size lifetime; same-value generations; short/empty/exact/oversize raw import; exit/SIGTERM fresh-process continuation; missing/wrong controls; native first-create/overwrite/failure preservation. |
| Compatibility | Quoted commas/quotes/semicolons, malformed record bounds, protected expectation/script/profile/deletion/duplicates, dated exceptions, no sampled-HALT rejection, local every-candidate enumeration, no public/private leakage and zero-coverage reporting. |
| Whole phase | ci/ci-msvc, ASan/nofp, byte-identical fixture-image and playing-frame hashes across existing native/be32/wasm32 lanes; count allocations on loaded playing frames; exact executed inventory and meaningful failure controls; boot_to_play perf workload with report-only arming status unchanged. |

These are recommended checks for GAME-01–GAME-07 under D-26–D-28; integrate them into existing per-subsystem tests.cmake and shared inventories. Quick loop: build the changed targets, then ctest --preset ci -R 'vdp|io|mapper|fixtures|runner|libretro'. Full existing gate: cmake --workflow --preset ci (ci-msvc on Windows). No behavior test is delegated to owner UAT. [VERIFIED: docs/testing.md; .planning/preparation/ENGINEERING.md §3; AGENTS.md rules 5–7]

### Budget findings that affect task order

The actual hosted tree is 293 lines: bitmap decoder 87, compare 54, flip negative control 52, run driver 54 and config 46. Reduce the driver/control duplication before adding content, options or input; preserve the one-pixel negative control and no-content card. The existing budget counts every file under its directory, so moving e2e code to an uncounted helper is not a valid solution. [VERIFIED: tests/cmake/e2e_budget.cmake:1-27; wc -l tests/retroarch/* this session; 03-CONTEXT.md D-27]

Repository checking is a material existing gap: src C/H totals 4,372 lines; tests/cmake totals 1,534 lines, while tests/cmake + scripts + cmake totals 2,547 mixed lines. Not all mixed lines are repository checks (release/installer/build/behavior drivers differ), but the applicable checks must be classified honestly and consolidated substantially to satisfy rule 7. The existing docs budget checker does not enforce the ratio. Do not wait until final verification, exempt existing check code, pad src or merely remove comments to hide this cost; place a proportional consolidation task alongside the first implementation work and maintain one inventory of genuinely counted repository-check files. [VERIFIED: wc -l src/*.c src/*.h tests/cmake/* scripts/*.sh cmake/*.cmake this session; tests/cmake/budgets.cmake; AGENTS.md rule 7]

## Environment Availability

| Dependency | Observation this session | Execution recommendation |
|---|---|---|
| CMake / Ninja / C compiler | CMake 4.4.3, Ninja 1.13.2, Apple clang 21.0.0 | Existing native development/CI preset path. [VERIFIED: command -v and --version probes] |
| GitHub CLI | gh 2.101.0; public MAME/WLA pins resolved successfully | Use pinned public API metadata; no sibling checkout reads. [VERIFIED: gh --version; gh api probes] |
| WLA-DX | Not on PATH; existing configured binary present in ci build | Use CMake-exported executable locations, not find a random PATH assembler. [VERIFIED: cmake/WlaDx.cmake:44-51: "set(wla_dir ${PROJECT_BINARY_DIR}/wla-dx)", "set(COGMASTER_WLA_Z80 ${wla_dir}/build/binaries/wla-z80${wla_exe})", "set(COGMASTER_WLALINK ${wla_dir}/build/binaries/wlalink${wla_exe})"; executable inventory probe] |
| RetroArch | Hosted job already fetches pinned 1.22.2 DMG and runs isolated CMake driver | Keep real-GUI execution on hosted macOS CI; no local success claimed here. [VERIFIED: .github/workflows/ci.yml:307-357; pins.txt] |
| XML/source facts | Three CC0 files fetched to temporary storage; permissive MAME mapper header checked before body | Generate offline immutable data; no network/parser in core. [VERIFIED: gh api content fetches and file headers] |

Fixture output location is created by the existing command "COMMAND ${CMAKE_COMMAND} -E make_directory ${COGMASTER_FIXTURES_DIR}" before assembly/link. Reuse that production build order; proposed fixture identity checks run after assembly rather than making cartdb depend on it. [VERIFIED: tests/fixtures/tests.cmake:64-76]

## Assumptions Log

No unverified factual assumption is promoted to a locked decision. The unresolved Moldorian wiring is explicitly withheld from support; H-counter sub-dot phase and selected VDP latch/freeze models remain confidence-qualified source-based models with tests of our selected behavior. These are not owner confirmation checkpoints. [VERIFIED: 03-CONTEXT.md D-03,D-07,D-15; findings above]

## Open Questions

1. **Moldorian physical SRAM bank/address decoding:** declared length and identity verified; no PCB wiring evidence established. Narrow investigation during mapper implementation, else unsupported persistence geometry with no guessed export. It does not block supported own-fixture continuation or introduce owner work.
2. **H-counter sub-dot phase and hardware latch edges:** official coordinate anchors plus measurements justify the origin correction, not a claim of universal chip-level accuracy. Test the selected phase and keep binary comparisons diagnostic.
3. **Exact dynamic-height latch choice:** choose/document a coherent field boundary model that preserves the fixed deadline and descriptor consistency; exercise every transition. No new hardware truth is claimed by a matching model test.
4. **Repository-check ratio scope/count:** classify code by what it does, consolidate applicable checks and enforce the actual ratio; raw directory totals above are a warning, not a license to redefine checks out of the numerator.

All questions are execution research/engineering judgments already delegated by the context; none requires a routine user question. [VERIFIED: 03-CONTEXT.md, Agent's Discretion; AGENTS.md rules 6–9]

## Sources and Confidence

- **Authoritative local contracts / code, read this session:** AGENTS.md; PROJECT/REQUIREMENTS/ROADMAP/STATE; all phase contexts and phase-2 review disposition; preparation index, decisions, architecture, SMS-VDP, SMS-SYSTEM, cartridge/peripheral, conformance, host, engineering, extracts/assembler, provenance/asset policy, relevant lessons; current core/runner/adapter/fixture and policy code named above.
- **Public pin and CC0 metadata:** [MAME commit](https://github.com/mamedev/mame/commit/1d7a454ed79e97d35d2a9f127ac029ead94711d6), three raw hash lists at that commit; [WLA-DX commit](https://github.com/vhelin/wla-dx/commit/91c52b1f4ef3cc8ba3c0638f7536539579af6a9f). Both API commit checks succeeded; no commercial bytes fetched.
- **Primary hardware accounts:** [Display Timing measurements and May 2008 origin discussion](https://www.smspower.org/forums/8161-SMSDisplayTiming), [Sega GG official documentation transcription](https://www.smspower.org/Development/GGOfficialDocs), [Peripheral Ports](https://www.smspower.org/Development/PeripheralPorts), [GG I/O map](https://www.smspower.org/Development/IOPortMapGG), [Sega mapper documentation](https://www.smspower.org/Development/Mappers).
- **Physical-RAM caution:** [MAME Shining Force II metadata correction](https://github.com/mamedev/mame/commit/6f1b0f5e9c0f), permitted BSD-3 mapper at the pinned commit, Moldorian game data page. None supplies verified Moldorian bank wiring.
- **Host/persistence contracts:** installed MIT libretro.h; [core development](https://docs.libretro.com/development/cores/developing-cores/), [POSIX rename](https://pubs.opengroup.org/onlinepubs/9799919799/functions/rename.html), [Windows ReplaceFile](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew).

**Confidence:** MEDIUM overall. Source readback makes current-code/metadata facts directly verifiable; renderer architecture is strongly grounded in settled contracts; chip subtiming and Moldorian wiring have the limits stated above. The research-plan seam selected jina; no matching callable tool was available, so web lookup/direct authoritative fetch was used. classify-confidence returned MEDIUM for verified websearch/jina/context7 and LOW for unrecognized webfetch/primary/curated provider names; no unsupported HIGH provider claim is used. [VERIFIED: gsd-tools query research-plan and classify-confidence outputs this session]

**Research date:** 2026-10-20. Re-check public pin accessibility and native API contracts at execution; stable hardware documents remain useful until new evidence contradicts the selected model. Graph context is disabled; no project agent skills were configured. [VERIFIED: init.phase-op, graphify status and agent-skills queries; .planning/config.json]
