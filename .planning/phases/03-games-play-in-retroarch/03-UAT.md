---
status: complete
phase: 03-games-play-in-retroarch
source:
  - 03-01-SUMMARY.md
  - 03-02-SUMMARY.md
  - 03-03-SUMMARY.md
  - 03-04-SUMMARY.md
  - 03-05-SUMMARY.md
  - 03-06-SUMMARY.md
  - 03-07-SUMMARY.md
  - 03-08-SUMMARY.md
  - 03-09-SUMMARY.md
  - 03-10-SUMMARY.md
  - 03-11-SUMMARY.md
  - 03-12-SUMMARY.md
  - 03-13-SUMMARY.md
  - 03-14-SUMMARY.md
  - 03-15-SUMMARY.md
  - 03-16-SUMMARY.md
  - 03-17-SUMMARY.md
  - 03-18-SUMMARY.md
started: 2026-10-20T23:23:18Z
updated: 2026-10-20T23:26:20.138Z
runtime: Codex
model: GPT-6
---

## Current Test

[testing complete]

## Execution Policy

AGENTS rule 6 assigns verification and visual judgment to the agent. All 87 structured
coverage entries classify as automated, with no classification errors or human judgment.
The all-covered confirmation is performed by the agent against fresh checks and artifacts;
no owner confirmation is substituted or recorded. No identity-bound or hardware-only item applies.

## Tests

### 1. Owned CPU-executed SMS tiles, display-off backdrop and immutable completed lines
expected: Owned CPU-executed SMS tiles, display-off backdrop and immutable completed lines
result: pass
source: automated
coverage_id: sms-background-tracer
requirement: GAME-01
summary: 03-01-SUMMARY.md
verification: vdp.background; fixtures.boot_to_play; fixtures.boot_to_play.wrong_hash

### 2. First-boundary press, hold, release and second press with independent counter and scroll values
expected: First-boundary press, hold, release and second press with independent counter and scroll values
result: pass
source: automated
coverage_id: absolute-input-tracer
requirement: GAME-04
summary: 03-01-SUMMARY.md
verification: abi.api_v3; io.absolute_input; runner.input; fixtures.boot_to_play.input; fixtures.boot_to_play.mailbox

### 3. In-memory content, fresh input, descriptor claims and independently correct callback pixels equal to the runner
expected: In-memory content, fresh input, descriptor claims and independently correct callback pixels equal to the runner
result: pass
source: automated
coverage_id: production-libretro-tracer
requirement: GAME-07
summary: 03-01-SUMMARY.md
verification: libretro.host.content; libretro.info_claims

### 4. Existing source policy, core ABI and metadata gates retain their named fault detection after consolidation
expected: Existing source policy, core ABI and metadata gates retain their named fault detection after consolidation
result: pass
source: automated
coverage_id: repository-check-consolidation
requirement: GAME-01
summary: 03-02-SUMMARY.md
verification: cmake --workflow --preset ci; cmake --workflow --preset hygiene

### 5. Physical check lines stay strictly below one tenth of core lines and unknown implementations reject
expected: Physical check lines stay strictly below one tenth of core lines and unknown implementations reject
result: pass
source: automated
coverage_id: strict-repository-budget
requirement: GAME-01
summary: 03-02-SUMMARY.md
verification: docs.budgets; docs.budgets.ratio_below; docs.budgets.ratio_equal; docs.budgets.unclassified; docs.budgets.helper; docs.budgets.helper_unclassified

### 6. Complete RetroArch helper tree remains within 300 lines and rejects a combined 301-line fixture
expected: Complete RetroArch helper tree remains within 300 lines and rejects a combined 301-line fixture
result: pass
source: automated
coverage_id: hosted-helper-accounting
requirement: GAME-07
summary: 03-02-SUMMARY.md
verification: policy.e2e_budget; policy.e2e_budget.self_test

### 7. Released RetroArch boots the owned SMS game and receives press, hold, release and a second press through frontend replay callbacks, producing counter and scroll two
expected: Released RetroArch boots the owned SMS game and receives press, hold, release and a second press through frontend replay callbacks, producing counter and scroll two
result: pass
source: automated
coverage_id: hosted-sms-input-scroll
requirement: GAME-07
summary: 03-03-SUMMARY.md
verification: https://github.com/szTheory/cogmaster/actions/runs/39480735490

### 8. Baseline and changed screenshots equal runner pixels and the independent coordinate oracle; wrong scroll and a changed pixel both reject
expected: Baseline and changed screenshots equal runner pixels and the independent coordinate oracle; wrong scroll and a changed pixel both reject
result: pass
source: automated
coverage_id: independent-screenshot-semantics
requirement: GAME-07
summary: 03-03-SUMMARY.md
verification: libretro.screenshot.oracle; libretro.screenshot.oracle_control; libretro.screenshot.pixel_control; https://github.com/szTheory/cogmaster/actions/runs/39480735490

### 9. Production and replay modules run the same rich frontend sequence and independently expected pixels; production still refuses save states
expected: Production and replay modules run the same rich frontend sequence and independently expected pixels; production still refuses save states
result: pass
source: automated
coverage_id: unchanged-production-callbacks
requirement: GAME-04
summary: 03-03-SUMMARY.md
verification: libretro.host.content; libretro.host.replay; libretro.host.xrgb8888

### 10. All hosted helpers plus the shared oracle total 300 physical lines and combined 301-line controls reject in CI and hygiene
expected: All hosted helpers plus the shared oracle total 300 physical lines and combined 301-line controls reject in CI and hygiene
result: pass
source: automated
coverage_id: complete-hosted-helper-budget
requirement: GAME-07
summary: 03-03-SUMMARY.md
verification: policy.e2e_budget; policy.e2e_budget.self_test; policy.e2e_budget.shared_helper

### 11. All sixteen planar indices, all sixty-four SMS colours, tile bit eight, separate and combined flips, palette choice and priority-zero semantics
expected: All sixteen planar indices, all sixty-four SMS colours, tile bit eight, separate and combined flips, palette choice and priority-zero semantics
result: pass
source: automated
coverage_id: background-flags-and-colours
requirement: GAME-01
summary: 03-04-SUMMARY.md
verification: vdp.background.tiles; vdp.background.priority

### 12. Horizontal and vertical wrap, fine-scroll backdrop, source-column vertical lock, top horizontal lock, left mask and display-off output
expected: Horizontal and vertical wrap, fine-scroll backdrop, source-column vertical lock, top horizontal lock, left mask and display-off output
result: pass
source: automated
coverage_id: scrolling-and-empty-fields
requirement: GAME-01
summary: 03-04-SUMMARY.md
verification: vdp.background.scroll; vdp.background.locks; vdp.background.empty

### 13. Stamped CRAM, VRAM and backdrop writes preserve emitted dots; line and frame latches and partial-line state restore deterministically
expected: Stamped CRAM, VRAM and backdrop writes preserve emitted dots; line and frame latches and partial-line state restore deterministically
result: pass
source: automated
coverage_id: immutable-raster-intervals
requirement: GAME-01
summary: 03-04-SUMMARY.md
verification: vdp.background.intervals; vdp.background.latches; vdp.background.state_interval

### 14. SMS1 appends as value three; R2 through R6 address gates, console boot, reset and matching-profile states are checked independently
expected: SMS1 appends as value three; R2 through R6 address gates, console boot, reset and matching-profile states are checked independently
result: pass
source: automated
coverage_id: sms1-shared-profile
requirement: GAME-01
summary: 03-04-SUMMARY.md
verification: abi.api_v4; vdp.background.sms1_masks; vdp.background.sms1_state; core.load.profiles; io.open_bus

### 15. Unsupported legacy and invalid text modes produce backdrop and a VDP fault while preserving bounded public frame delivery
expected: Unsupported legacy and invalid text modes produce backdrop and a VDP fault while preserving bounded public frame delivery
result: pass
source: automated
coverage_id: bounded-unsupported-video
requirement: GAME-01
summary: 03-04-SUMMARY.md
verification: vdp.background.unsupported

### 16. Public frame pixels equal the independent fixture oracle and runner little-endian native hash, with original card controls retained
expected: Public frame pixels equal the independent fixture oracle and runner little-endian native hash, with original card controls retained
result: pass
source: automated
coverage_id: native-field-identity
requirement: GAME-01
summary: 03-04-SUMMARY.md
verification: fixtures.boot_to_play; fixtures.boot_to_play.input; fixtures.boot_to_play.wrong_hash; libretro.host.content; runner.hash.sms; runner.hash.gg

### 17. Independent 8x8 and tile-even 8x16 shapes, Y wrap and terminator, EC shift, clipped edges and two-axis zoom through the live full-field API
expected: Independent 8x8 and tile-even 8x16 shapes, Y wrap and terminator, EC shift, clipped edges and two-axis zoom through the live full-field API
result: pass
source: automated
coverage_id: sprite-shapes-and-clipping
requirement: GAME-01
summary: 03-05-SUMMARY.md
verification: vdp.sprites.shapes; vdp.sprites.wrap_clip; vdp.sprites.zoom

### 18. Earlier opaque SAT slot wins, transparent earlier dots reveal later slots, background priority and left mask work, SMS1 counts two/four/six/eight use the selected count-dependent rule
expected: Earlier opaque SAT slot wins, transparent earlier dots reveal later slots, background priority and left mask work, SMS1 counts two/four/six/eight use the selected count-dependent rule
result: pass
source: automated
coverage_id: sprite-order-and-zoom-slots
requirement: GAME-01
summary: 03-05-SUMMARY.md
verification: vdp.sprites.priority; vdp.sprites.sms1_zoom; vdp.sprites.gg_slots

### 19. Full 64-entry scan retains eight slots, fixed-stamp write hammer cannot rescan or repaint, pattern mutations preserve emitted dots and selected slots survive partial-line state restoration
expected: Full 64-entry scan retains eight slots, fixed-stamp write hammer cannot rescan or repaint, pattern mutations preserve emitted dots and selected slots survive partial-line state restoration
result: pass
source: automated
coverage_id: bounded-saved-line-work
requirement: GAME-01
summary: 03-05-SUMMARY.md
verification: vdp.sprites.intervals; vdp.sprites.bounds_state; core.state.record_sweep

### 20. Touching edges do not collide, shared opaque dots do, reads before/at/after an edge observe and clear flags, later overlaps reassert, and frame IRQ and control latch release on read
expected: Touching edges do not collide, shared opaque dots do, reads before/at/after an edge observe and clear flags, later overlaps reassert, and frame IRQ and control latch release on read
result: pass
source: automated
coverage_id: active-dot-status
requirement: GAME-02
summary: 03-05-SUMMARY.md
verification: vdp.sprites.collision; vdp.sprites.sms1_collision; vdp.status

### 21. Eight Y matches do not overflow, nine do independently of opacity and X; display-off, masked, clipped, transparent and inactive-dot controls exclude collision
expected: Eight Y matches do not overflow, nine do independently of opacity and X; display-off, masked, clipped, transparent and inactive-dot controls exclude collision
result: pass
source: automated
coverage_id: overflow-and-exclusions
requirement: GAME-02
summary: 03-05-SUMMARY.md
verification: vdp.sprites.overflow; vdp.sprites.inactive

### 22. Frame and line edges, R10 zero/one/FF schedules, writes before/at/after reload, first postactive decrement and blank reload
expected: Frame and line edges, R10 zero/one/FF schedules, writes before/at/after reload, first postactive decrement and blank reload
result: pass
source: automated
coverage_id: irq-edge-and-reload-ordering
requirement: GAME-02
summary: 03-06-SUMMARY.md
verification: vdp.timing.irq_edges; vdp.timing.irq_reload; vdp.timing.mode_edges

### 23. Disabled IE preserves pending, enabling or disabling recomputes INT, status clears both sources, owned IM1 loop counts six line interrupts and one frame interrupt, saved pending cannot disagree with CPU INT
expected: Disabled IE preserves pending, enabling or disabling recomputes INT, status clears both sources, owned IM1 loop counts six line interrupts and one frame interrupt, saved pending cannot disagree with CPU INT
result: pass
source: automated
coverage_id: irq-pending-cpu-and-state
requirement: GAME-02
summary: 03-06-SUMMARY.md
verification: vdp.timing.irq_pending; vdp.timing.irq_cpu; vdp.timing.irq_state

### 24. Every entry and discontinuity of all six literal NTSC and PAL height tables at ticks 603, 604 and 605, including NTSC-240 rolling wrap
expected: Every entry and discontinuity of all six literal NTSC and PAL height tables at ticks 603, 604 and 605, including NTSC-240 rolling wrap
result: pass
source: automated
coverage_id: all-v-counter-intervals
requirement: GAME-02
summary: 03-06-SUMMARY.md
verification: vdp.timing.v_ntsc_192; vdp.timing.v_ntsc_224; vdp.timing.v_ntsc_240; vdp.timing.v_pal_192; vdp.timing.v_pal_224; vdp.timing.v_pal_240

### 25. All 171 H values and four phases, SMS active 08 and GG crop 20 anchors, held capture, no recapture on falling or unchanged levels, Japanese readback mixing and native GG absence of TH
expected: All 171 H values and four phases, SMS active 08 and GG crop 20 anchors, held capture, no recapture on falling or unchanged levels, Japanese readback mixing and native GG absence of TH
result: pass
source: automated
coverage_id: translated-h-and-th-capture
requirement: GAME-02
summary: 03-06-SUMMARY.md
verification: vdp.timing.hcounter; vdp.timing.th_capture; io.port3f_region; io.gg_ports; vdp.timing.th_edges

### 26. Counter phase persists across fixed host frame deadlines and state restoration, invalid captured H gap values refuse before apply, reset releases capture
expected: Counter phase persists across fixed host frame deadlines and state restoration, invalid captured H gap values refuse before apply, reset releases capture
result: pass
source: automated
coverage_id: rolling-phase-state-and-deadline
requirement: GAME-02
summary: 03-06-SUMMARY.md
verification: vdp.timing.deadlines; vdp.timing.counter_state; cmake --workflow --preset ci

### 27. NTSC and PAL 192/224/240 fields, final rows, 224/256 map wrap and exact PAL PAR
expected: NTSC and PAL 192/224/240 fields, final rows, 224/256 map wrap and exact PAL PAR
result: pass
source: automated
coverage_id: completed-height-and-aspect
requirement: GAME-01
summary: 03-07-SUMMARY.md
verification: vdp.geometry.heights; vdp.geometry.coordinates

### 28. Every old/new height pair at latch-minus-one, latch and latch-plus-one on both TVs; live palette and backdrop cannot change a completed image
expected: Every old/new height pair at latch-minus-one, latch and latch-plus-one on both TVs; live palette and backdrop cannot change a completed image
result: pass
source: automated
coverage_id: completed-transition-coherence
requirement: GAME-01
summary: 03-07-SUMMARY.md
verification: vdp.geometry.transitions

### 29. Full-field and crop canaries, invalid pitch no-write guarantees, fixed deadlines, allocation counts, hostile writes and validate-before-apply height restoration
expected: Full-field and crop canaries, invalid pitch no-write guarantees, fixed deadlines, allocation counts, hostile writes and validate-before-apply height restoration
result: pass
source: automated
coverage_id: bounded-frame-copy-and-state
requirement: GAME-01
summary: 03-07-SUMMARY.md
verification: vdp.geometry.buffers; vdp.geometry.hostile; vdp.geometry.state

### 30. Six TV/height combinations check every field coordinate for tiles, both sprite sizes, zoom, SAT order, priority, flips, palette, mask, scroll locks and IRQ scroll bands
expected: Six TV/height combinations check every field coordinate for tiles, both sprite sizes, zoom, SAT order, priority, flips, palette, mask, scroll locks and IRQ scroll bands
result: pass
source: automated
coverage_id: guest-rendering-matrix
requirement: GAME-01
summary: 03-07-SUMMARY.md
verification: fixtures.vdp_lines.pixels.ntsc.192 through fixtures.vdp_lines.pixels.pal.240; runner.hashes_file

### 31. Eight numeric mailbox records per TV/height cover line and frame IRQ counts, entry H/V, two status reads and the visible V-counter 3F/40 boundary
expected: Eight numeric mailbox records per TV/height cover line and frame IRQ counts, entry H/V, two status reads and the visible V-counter 3F/40 boundary
result: pass
source: automated
coverage_id: guest-timing-measurements
requirement: GAME-02
summary: 03-07-SUMMARY.md
verification: fixtures.vdp_lines.mailbox.ntsc.192 through fixtures.vdp_lines.mailbox.pal.240

### 32. Wrong expected value, missing row and one-frame hang are rejected with named nonzero results
expected: Wrong expected value, missing row and one-frame hang are rejected with named nonzero results
result: pass
source: automated
coverage_id: fixture-negative-controls
requirement: GAME-02
summary: 03-07-SUMMARY.md
verification: fixtures.vdp_lines.judge.wrong_value; fixtures.vdp_lines.judge.missing_row; fixtures.vdp_lines.judge.hang

### 33. All 4096 native colours, even-byte latch and odd-byte commit, address changes and wrap, immediate SMS colours and immutable raster intervals
expected: All 4096 native colours, even-byte latch and odd-byte commit, address changes and wrap, immediate SMS colours and immutable raster intervals
result: pass
source: automated
coverage_id: native-gg-palette
requirement: GAME-03
summary: 03-08-SUMMARY.md
verification: vdp.gg.palette; vdp.gg.colours; vdp.gg.intervals

### 34. Exact 192 and 224 crop coordinates, shared pixel identity, frozen native image versus live full field, SOFT retention and POWER recovery
expected: Exact 192 and 224 crop coordinates, shared pixel identity, frozen native image versus live full field, SOFT retention and POWER recovery
result: pass
source: automated
coverage_id: native-lcd-and-full-field
requirement: GAME-03
summary: 03-08-SUMMARY.md
verification: vdp.gg.coordinates; vdp.gg.freeze; vdp.gg.state; core.state.record_sweep

### 35. Previous media sizes select AUTO, impossible modes reject, resolved mode is visible, only selected aliases and native Start qualify, SMS-mode Start routes to Pause
expected: Previous media sizes select AUTO, impossible modes reject, resolved mode is visible, only selected aliases and native Start qualify, SMS-mode Start routes to Pause
result: pass
source: automated
coverage_id: explicit-mode-and-controller-decode
requirement: GAME-04
summary: 03-08-SUMMARY.md
verification: abi.api_v5; vdp.gg.profiles; vdp.gg.ports; runner.load.video_override

### 36. Real GG-header image at boot_to_play.gg, every native LCD pixel and digest checked independently, Start baseline and press/hold/release/repress counters and raw bytes judged
expected: Real GG-header image at boot_to_play.gg, every native LCD pixel and digest checked independently, Start baseline and press/hold/release/repress counters and raw bytes judged
result: pass
source: automated
coverage_id: owned-native-playing-fixture
requirement: GAME-03
summary: 03-08-SUMMARY.md
verification: fixtures.boot_to_play.gg.baseline; fixtures.boot_to_play.gg.start; fixtures.boot_to_play.gg.mailbox.baseline; fixtures.boot_to_play.gg.mailbox.start

### 37. Either pad drives the same independently checked counter, scroll and picture sequence; released card hashes remain unchanged
expected: Either pad drives the same independently checked counter, scroll and picture sequence; released card hashes remain unchanged
result: pass
source: automated
coverage_id: both-sms-pad-playing-paths
requirement: GAME-01
summary: 03-08-SUMMARY.md
verification: fixtures.boot_to_play.input; fixtures.boot_to_play.pad2; runner.hash.sms; runner.hash.gg

### 38. Native 4:3 and square-pixel 10:9 LCD presentation, with selected PAR applied to full fields
expected: Native 4:3 and square-pixel 10:9 LCD presentation, with selected PAR applied to full fields
result: pass
source: automated
coverage_id: gg-presentation
requirement: GAME-03
summary: 03-09-SUMMARY.md
verification: libretro.presentation.xrgb8888 and libretro.presentation.rgb565

### 39. All completed heights, rejected geometry notifications, fixed storage and exact PAL sample totals
expected: All completed heights, rejected geometry notifications, fixed storage and exact PAL sample totals
result: pass
source: automated
coverage_id: completed-geometry-pal
requirement: GAME-03
summary: 03-09-SUMMARY.md
verification: tests/libretro/presentation.h through cmake --workflow --preset ci

### 40. Automatic native GG profile and Start-scripted crop equal runner and independent glyph pixels
expected: Automatic native GG profile and Start-scripted crop equal runner and independent glyph pixels
result: pass
source: automated
coverage_id: native-gg-hosted
requirement: GAME-03
summary: 03-09-SUMMARY.md
verification: libretro.host.content.gg and libretro.host.replay.gg; https://github.com/szTheory/cogmaster/actions/runs/39494627790

### 41. SMS input and scrolling, no-content equality and pixel negative control remain successful
expected: SMS input and scrolling, no-content equality and pixel negative control remain successful
result: pass
source: automated
coverage_id: hosted-controls-preserved
requirement: GAME-07
summary: 03-09-SUMMARY.md
verification: https://github.com/szTheory/cogmaster/actions/runs/39494627790; policy.e2e_budget counts 300 physical lines including the shared oracle

### 42. Every pad bit, simultaneous opposing directions, TR outputs, region echo and physical Reset availability
expected: Every pad bit, simultaneous opposing directions, TR outputs, region echo and physical Reset availability
result: pass
source: automated
coverage_id: independent-pad-pins
requirement: GAME-04
summary: 03-10-SUMMARY.md
verification: io.pad_pins and io.port3f_region; libretro.controls.xrgb8888 and libretro.controls.rgb565

### 43. Held and duplicate inputs, release and repress, disconnected and held reconnect, interleaved instances, state restoration and refusal before apply
expected: Held and duplicate inputs, release and repress, disconnected and held reconnect, interleaved instances, state restoration and refusal before apply
result: pass
source: automated
coverage_id: pause-input-history
requirement: GAME-04
summary: 03-10-SUMMARY.md
verification: io.pause_history and core.state.record_sweep; vdp.pause_nmi covers settled NTSC and PAL last-line ticks

### 44. Separate and overlapping pad sequences change counters, CRAM, scroll and frame hashes; only decoded enabled guest reads clear lag
expected: Separate and overlapping pad sequences change counters, CRAM, scroll and frame hashes; only decoded enabled guest reads clear lag
result: pass
source: automated
coverage_id: guest-lag-and-play
requirement: GAME-04
summary: 03-10-SUMMARY.md
verification: io.lag_decode exhaustively checks all low port addresses on four profiles; fixtures.boot_to_play.input, fixtures.boot_to_play.pad2 and fixtures.boot_to_play.both_pads; runner.lag and runner.stop

### 45. Port-1 console ownership, immediate frame-boundary release, held reconnection, physical Reset versus SMS SOFT and GG POWER Restart
expected: Port-1 console ownership, immediate frame-boundary release, held reconnection, physical Reset versus SMS SOFT and GG POWER Restart
result: pass
source: automated
coverage_id: frontend-reset-device-contract
requirement: GAME-04
summary: 03-10-SUMMARY.md
verification: libretro.controls.xrgb8888 and libretro.controls.rgb565

### 46. Native active-low Start is a level, SMS-mode Start queues Pause, and only native 00 plus C0/C1/DC/DD reads qualify
expected: Native active-low Start is a level, SMS-mode Start queues Pause, and only native 00 plus C0/C1/DC/DD reads qualify
result: pass
source: automated
coverage_id: gg-start-and-aliases
requirement: GAME-03
summary: 03-10-SUMMARY.md
verification: io.lag_decode and vdp.gg.ports; libretro.controls.xrgb8888, libretro.controls.rgb565 and fixtures.boot_to_play.gg.start

### 47. Public-pinned CC0 identities, sorted deterministic generation, title aliases, conflicting facts, entities, numeric sizes and compound-record exclusion
expected: Public-pinned CC0 identities, sorted deterministic generation, title aliases, conflicting facts, entities, numeric sizes and compound-record exclusion
result: pass
source: automated
coverage_id: immutable-cartridge-identities
requirement: GAME-06
summary: 03-11-SUMMARY.md
verification: cart.generator and cart.database; Pinned three-file regeneration and byte-for-byte cmp

### 48. Table and header facts resolve AUTO profiles, including GG pin_42 SMS mode, while explicit host settings remain authoritative
expected: Table and header facts resolve AUTO profiles, including GG pin_42 SMS mode, while explicit host settings remain authoritative
result: pass
source: automated
coverage_id: automatic-machine-and-mode
requirement: GAME-03
summary: 03-11-SUMMARY.md
verification: cart.database and abi.api_v3; runner.probe.keys, runner.probe.headerless and runner.probe.unsupported

### 49. Known nonbattery, unknown persistence, supported battery and known unsupported geometry remain independent of mapper allocation and ledger membership
expected: Known nonbattery, unknown persistence, supported battery and known unsupported geometry remain independent of mapper allocation and ledger membership
result: pass
source: automated
coverage_id: persistence-classification
requirement: GAME-05
summary: 03-11-SUMMARY.md
verification: cart.database and cart.generator

### 50. Fixed fixture SHA-1 matches after linking; production classification survives rename and copier header, rejects a changed byte, and executes the guest counter mailbox
expected: Fixed fixture SHA-1 matches after linking; production classification survives rename and copier header, rejects a changed byte, and executes the guest counter mailbox
result: pass
source: automated
coverage_id: owned-battery-production-identity
requirement: GAME-05
summary: 03-11-SUMMARY.md
verification: cart.fixture_identity, cart.fixture_hash and cart.fixture_wrong_hash; Battery image rebuilt from source and checked by the post-link SHA-1 command

### 51. Real 24576-byte declaration is retained with unsupported geometry after the narrow wiring investigation found insufficient physical evidence
expected: Real 24576-byte declaration is retained with unsupported geometry after the narrow wiring investigation found insufficient physical evidence
result: pass
source: automated
coverage_id: moldorian-geometry-disposition
requirement: GAME-05
summary: 03-11-SUMMARY.md
verification: cart.database; docs/hardware/cartridges.md primary-source evidence and confidence disposition

### 52. Exact supported 8/16/32 KiB spans, independent physical aliases, fresh FF, stable pointers through enable, frames and both resets; unknown, none and unsupported expose no span
expected: Exact supported 8/16/32 KiB spans, independent physical aliases, fresh FF, stable pointers through enable, frames and both resets; unknown, none and unsupported expose no span
result: pass
source: automated
coverage_id: fixed-classified-spans
requirement: GAME-05
summary: 03-12-SUMMARY.md
verification: battery.spans and battery.excluded; battery.fixture

### 53. Same-value writes count individually; reads, disabled writes, registers, volatile offsets, resets and imports do not increment; internal states visit and validate generation
expected: Same-value writes count individually; reads, disabled writes, registers, volatile offsets, resets and imports do not increment; internal states visit and validate generation
result: pass
source: automated
coverage_id: mapped-write-generation
requirement: GAME-05
summary: 03-12-SUMMARY.md
verification: battery.generation, battery.excluded and core.state.record_sweep

### 54. Empty, one-byte, short and exact imports copy and FF-pad only the persistent extent; oversize, SIZE_MAX and invalid arguments preserve the entire state; repeat imports are generation-neutral
expected: Empty, one-byte, short and exact imports copy and FF-pad only the persistent extent; oversize, SIZE_MAX and invalid arguments preserve the entire state; repeat imports are generation-neutral
result: pass
source: automated
coverage_id: bounded-transactional-import
requirement: GAME-05
summary: 03-12-SUMMARY.md
verification: battery.import and abi.api_v7; fuzz.regress.save under CI and Apple Clang ASan plus UBSan

### 55. Production-classified owned fixture accepts two same-size raw saves and reports their independently expected previous and next counters
expected: Production-classified owned fixture accepts two same-size raw saves and reports their independently expected previous and next counters
result: pass
source: automated
coverage_id: raw-wrong-data-continuation
requirement: GAME-05
summary: 03-12-SUMMARY.md
verification: battery.continuation

### 56. Existing nightly fuzz framework includes raw save import with edge lengths, all persistence classes, exact bytes, FF suffixes and refusal equality
expected: Existing nightly fuzz framework includes raw save import with edge lengths, all persistence classes, exact bytes, FF suffixes and refusal equality
result: pass
source: automated
coverage_id: save-fuzz-entry
requirement: GAME-05
summary: 03-12-SUMMARY.md
verification: fuzz.libfuzzer.save with installed libFuzzer and Apple Clang sanitizer runtime

### 57. Missing, empty, short and exact saves import with FF padding; oversize exits 4 unchanged; directory and read failures exit 1; unknown and unsupported persistence refuse before execution
expected: Missing, empty, short and exact saves import with FF padding; oversize exits 4 unchanged; directory and read failures exit 1; unknown and unsupported persistence refuse before execution
result: pass
source: automated
coverage_id: bounded-raw-import
requirement: GAME-05
summary: 03-13-SUMMARY.md
verification: runner.save.import, runner.save.imports and runner.save.excluded

### 58. First creation and overwrite preserve exact battery bytes, clean generations do no replacement, and spaces and Unicode retain the intended stem and target
expected: First creation and overwrite preserve exact battery bytes, clean generations do no replacement, and spaces and Unicode retain the intended stem and target
result: pass
source: automated
coverage_id: native-dirty-replacement
requirement: GAME-05
summary: 03-13-SUMMARY.md
verification: runner.save.replacement and runner.save.import

### 59. Create, short-write, flush, sync, close and replace faults preserve the previous target or missing target, retain dirty acknowledgement and permit retry without touching unrelated temporaries
expected: Create, short-write, flush, sync, close and replace faults preserve the previous target or missing target, retain dirty acknowledgement and permit retry without touching unrelated temporaries
result: pass
source: automated
coverage_id: failed-save-preservation
requirement: GAME-05
summary: 03-13-SUMMARY.md
verification: runner.save.failures and runner.save.native_error; runner.save.process.create, write, flush, sync, close and replace

### 60. SIGTERM and SIGINT finish admitted frames and flush outside handlers, clean signals skip replacement, failed saves exit 1, and abrupt interruption leaves the old target and complete private temporary
expected: SIGTERM and SIGINT finish admitted frames and flush outside handlers, clean signals skip replacement, failed saves exit 1, and abrupt interruption leaves the old target and complete private temporary
result: pass
source: automated
coverage_id: signal-and-interruption
requirement: GAME-05
summary: 03-13-SUMMARY.md
verification: runner.save.process.term, int, int_write, normal_write and interrupt; runner.save.clean_sigint and runner.save.clean_sigterm

### 61. Separate children persist and continue 0 to 1 to 2 after normal exit, SIGTERM and SIGINT, checking full 32768-byte saves and no leaked temporary
expected: Separate children persist and continue 0 to 1 to 2 after normal exit, SIGTERM and SIGINT, checking full 32768-byte saves and no leaked temporary
result: pass
source: automated
coverage_id: fresh-process-continuation
requirement: GAME-05
summary: 03-14-SUMMARY.md
verification: runner.save.continuation.normal, runner.save.continuation.term and runner.save.continuation.int

### 62. Missing save reports 0100, a same-size save with counter 41 reports 2A29, and oversize exits 4 without changing the target digest
expected: Missing save reports 0100, a same-size save with counter 41 reports 2A29, and oversize exits 4 without changing the target digest
result: pass
source: automated
coverage_id: independent-save-controls
requirement: GAME-05
summary: 03-14-SUMMARY.md
verification: runner.save.continuation.normal, runner.save.continuation.term and runner.save.continuation.int

### 63. Real classified SAVE_RAM exists before enable, remains stable through frames and Restart, continues restored bytes across unload and reload, and invalidates callbacks outside the loaded lifetime
expected: Real classified SAVE_RAM exists before enable, remains stable through frames and Restart, continues restored bytes across unload and reload, and invalidates callbacks outside the loaded lifetime
result: pass
source: automated
coverage_id: frontend-save-memory
requirement: GAME-05
summary: 03-14-SUMMARY.md
verification: libretro.battery.sms2 and libretro.battery.gg; battery.excluded and mapper.codemasters.ram

### 64. SOFT and POWER preserve pointer, length, all bytes and generation on SMS2 and native GG; import and direct restoration are generation-neutral and guest continuation increments once
expected: SOFT and POWER preserve pointer, length, all bytes and generation on SMS2 and native GG; import and direct restoration are generation-neutral and guest continuation increments once
result: pass
source: automated
coverage_id: real-battery-reset-preservation
requirement: GAME-04
summary: 03-14-SUMMARY.md
verification: battery.continuation, libretro.battery.sms2 and libretro.battery.gg

### 65. Independent frame and RAM expectations pass while altered values exit 3; malformed and conflicting inputs exit 2, unavailable profiles 77 and missing files 1
expected: Independent frame and RAM expectations pass while altered values exit 3; malformed and conflicting inputs exit 2, unavailable profiles 77 and missing files 1
result: pass
source: automated
coverage_id: accepted-predicates
requirement: GAME-06
summary: 03-15-SUMMARY.md
verification: runner.expect.predicates

### 66. Interrupt-driven HALT with display and distinct frames passes; enabled static display and first-fault controls fail with observations, without gameplay or sound claims
expected: Interrupt-driven HALT with display and distinct frames passes; enabled static display and first-fault controls fail with observations, without gameplay or sound claims
result: pass
source: automated
coverage_id: honest-boot-smoke
requirement: GAME-06
summary: 03-15-SUMMARY.md
verification: runner.expect.predicates; abi.api_v8

### 67. Quoted commas, doubled quotes, semicolons and CRLF round-trip; malformed fields, empty records, duplicate aliases and byte/record/field/row bounds reject; permutations are stable
expected: Quoted commas, doubled quotes, semicolons and CRLF round-trip; malformed fields, empty records, duplicate aliases and byte/record/field/row bounds reject; permutations are stable
result: pass
source: automated
coverage_id: quoted-csv-normalization
requirement: GAME-06
summary: 03-15-SUMMARY.md
verification: compat.csv

### 68. Four accepted predicates across two admitted own fixtures execute once each with pinned bytes and the resolved board checked; CC0 metadata stays unknown
expected: Four accepted predicates across two admitted own fixtures execute once each with pinned bytes and the resolved board checked; CC0 metadata stays unknown
result: pass
source: automated
coverage_id: executable-fixture-ledger
requirement: GAME-06
summary: 03-15-SUMMARY.md
verification: compat.fixtures

### 69. Generated status, predicate and fixture counts distinguish evaluated and unknown coverage, header-only means zero, and stale documentation is caught independently
expected: Generated status, predicate and fixture counts distinguish evaluated and unknown coverage, header-only means zero, and stale documentation is caught independently
result: pass
source: automated
coverage_id: generated-honest-counts
requirement: GAME-06
summary: 03-15-SUMMARY.md
verification: policy.compat_doc_fresh and policy.compat_doc_fresh.self_test; compat.csv

### 70. Deleted passes, changed frame hashes or RAM bytes, duplicate aliases and substituted profile, board, script or device facts reject independently
expected: Deleted passes, changed frame hashes or RAM bytes, duplicate aliases and substituted profile, board, script or device facts reject independently
result: pass
source: automated
coverage_id: protected-normalized-passes
requirement: GAME-06
summary: 03-16-SUMMARY.md
verification: compat.ratchet

### 71. Numeric and hex aliases preserve identity while adjacent frames, addresses and configurations remain distinct; hostile titles and evidence remain data
expected: Numeric and hex aliases preserve identity while adjacent frames, addresses and configurations remain distinct; hostile titles and evidence remain data
result: pass
source: automated
coverage_id: identity-adjacency
requirement: GAME-06
summary: 03-16-SUMMARY.md
verification: compat.ratchet and compat.csv

### 72. Unique old predicates permit scoped revisions through inclusive expiry; expiry boundaries, future dates, invalid dates, unrelated predicates and missing or wrong revision bumps reject
expected: Unique old predicates permit scoped revisions through inclusive expiry; expiry boundaries, future dates, invalid dates, unrelated predicates and missing or wrong revision bumps reject
result: pass
source: automated
coverage_id: dated-exceptions
requirement: GAME-06
summary: 03-16-SUMMARY.md
verification: compat.ratchet

### 73. Candidate helper and judge replacement cannot weaken a copied base comparison; missing, extra or changed fresh fixture results require ledger agreement in both directions
expected: Candidate helper and judge replacement cannot weaken a copied base comparison; missing, extra or changed fresh fixture results require ledger agreement in both directions
result: pass
source: automated
coverage_id: trusted-fresh-comparison
requirement: GAME-06
summary: 03-16-SUMMARY.md
verification: compat.trusted_judge and compat.fixtures; policy.ci_required

### 74. Flat regular candidates including hidden names and punctuation are probed and attempted while unrelated files, directories and symlinks are excluded
expected: Flat regular candidates including hidden names and punctuation are probed and attempted while unrelated files, directories and symlinks are excluded
result: pass
source: automated
coverage_id: local-library-enumeration
requirement: GAME-06
summary: 03-17-SUMMARY.md
verification: library.synthetic

### 75. Accepted fixtures, unknown observations, unsupported profiles, malformed input and infrastructure failures remain distinct with explicit empty-library zero coverage
expected: Accepted fixtures, unknown observations, unsupported profiles, malformed input and infrastructure failures remain distinct with explicit empty-library zero coverage
result: pass
source: automated
coverage_id: honest-local-statuses
requirement: GAME-06
summary: 03-17-SUMMARY.md
verification: library.synthetic

### 76. The real library workflow runs an isolated empty fixture directory, missing directories fail configure, and a separate synthetic repository proves literal dotenv parsing
expected: The real library workflow runs an isolated empty fixture directory, missing directories fail configure, and a separate synthetic repository proves literal dotenv parsing
result: pass
source: automated
coverage_id: local-preset-and-dotenv
requirement: GAME-06
summary: 03-17-SUMMARY.md
verification: library.synthetic

### 77. Canonical CC0 titles round-trip while private labels and evidence, unknown identities, fixture changes, new goldens and changed profiles cannot change public cases
expected: Canonical CC0 titles round-trip while private labels and evidence, unknown identities, fixture changes, new goldens and changed profiles cannot change public cases
result: pass
source: automated
coverage_id: filtered-public-updates
requirement: GAME-06
summary: 03-17-SUMMARY.md
verification: compat.public_update; compat.ratchet

### 78. Complete shared parser and public-update dependencies fit the strict repository ratio and existing hosted code budget with exact executed inventories
expected: Complete shared parser and public-update dependencies fit the strict repository ratio and existing hosted code budget with exact executed inventories
result: pass
source: automated
coverage_id: proportional-checks
requirement: GAME-06
summary: 03-17-SUMMARY.md
verification: docs.budgets; policy.e2e_budget

### 79. Complete playing crops and fixture images compare identically on all eight existing platforms
expected: Complete playing crops and fixture images compare identically on all eight existing platforms
result: pass
source: automated
coverage_id: playing-portability
requirement: GAME-01
summary: 03-18-SUMMARY.md
verification: runner.hashes_file and CI hash-equality in run 39511836104; Eight downloaded hashes.txt files compared with tests/cmake/hashes.cmake

### 80. Existing IRQ, counter, TH capture, status-clear and mailbox checks remain green across portable lanes
expected: Existing IRQ, counter, TH capture, status-clear and mailbox checks remain green across portable lanes
result: pass
source: automated
coverage_id: selected-timing-model
requirement: GAME-02
summary: 03-18-SUMMARY.md
verification: tests/vdp/test_timing.c and tests/diag/expected.csv through ci and wasm32

### 81. Native GG Start and crop plus extended-height crop/freeze and GG SMS-mode output remain deterministic
expected: Native GG Start and crop plus extended-height crop/freeze and GG SMS-mode output remain deterministic
result: pass
source: automated
coverage_id: native-and-sms-mode-gg
requirement: GAME-03
summary: 03-18-SUMMARY.md
verification: vdp.gg.freeze and vdp.gg.state; CI RetroArch Game Gear native crop screenshot equality in run 39511836104

### 82. Both SMS pads, GG Start, region profiles, reset semantics and guest-read lag accounting retain their owned-fixture proofs
expected: Both SMS pads, GG Start, region profiles, reset semantics and guest-read lag accounting retain their owned-fixture proofs
result: pass
source: automated
coverage_id: independent-input
requirement: GAME-04
summary: 03-18-SUMMARY.md
verification: io.absolute_input, io.lag_decode, libretro.controls.rgb565 and libretro.controls.xrgb8888; Scripted playing hash rows and independent fixture oracles

### 83. Atomic save replacement, collision preservation, failure cleanup and fresh-process continuation run on native Windows and portable WASI
expected: Atomic save replacement, collision preservation, failure cleanup and fresh-process continuation run on native Windows and portable WASI
result: pass
source: automated
coverage_id: native-and-wasi-persistence
requirement: GAME-05
summary: 03-18-SUMMARY.md
verification: runner.save replacement, failure and continuation cases in run 39511836104

### 84. Quoted predicates, fresh fixture results, protected comparison and private library classification retain exact executed inventories
expected: Quoted predicates, fresh fixture results, protected comparison and private library classification retain exact executed inventories
result: pass
source: automated
coverage_id: compatibility-and-library
requirement: GAME-06
summary: 03-18-SUMMARY.md
verification: compat.fixtures, compat.ratchet, compat.trusted_judge and library.synthetic; CI protected compatibility comparison on dd40d2f in run 39511836104

### 85. The released frontend delivers input and scrolling output matching runner and independent SMS/GG pixel expectations
expected: The released frontend delivers input and scrolling output matching runner and independent SMS/GG pixel expectations
result: pass
source: automated
coverage_id: released-frontend-play
requirement: GAME-07
summary: 03-18-SUMMARY.md
verification: CI RetroArch SMS input and scrolling screenshot equality in run 39511836104; Agent inspection of final owned input.png and gg-input.png artifacts

### 86. Loaded content allocates nothing after load, preserves an interleaved neighbour, refuses invalid video state and stays within fixed deadlines and sprite bounds
expected: Loaded content allocates nothing after load, preserves an interleaved neighbour, refuses invalid video state and stays within fixed deadlines and sprite bounds
result: pass
source: automated
coverage_id: bounded-loaded-execution
requirement: GAME-01
summary: 03-18-SUMMARY.md
verification: core.no_alloc_after_load and core.state.record_sweep; fuzz.regress.run, both nofp lanes and ASan in run 39511836104

### 87. The playing benchmark reaches an independent expected frame while repository, hosted-helper and documentation budgets remain strict
expected: The playing benchmark reaches an independent expected frame while repository, hosted-helper and documentation budgets remain strict
result: pass
source: automated
coverage_id: representative-workload-and-budgets
requirement: GAME-01
summary: 03-18-SUMMARY.md
verification: perf.playing and CI perf control in run 39511836104; docs.budgets, policy.e2e_budget and hygiene

### 88. Agent confirmation of complete playing behavior
expected: Fresh hosted CI and owned screenshot inspection confirm the integrated playing behavior on the reviewed implementation.
result: pass
source: agent
verification: CI run 39514577638 on 8d9298db1a7c26b254b7fa076780e646eff0f631; all six native builds, be32, wasm32, hash equality, sanitizer, nofp and released RetroArch passed. Eight downloaded hash files contain 66 identical playing rows each. Agent viewed baseline.png, input.png, gg-baseline.png and gg-input.png from the owned screenshot artifact and confirmed coherent native dimensions and changed scroll/colour-counter patterns.
run_url: https://github.com/szTheory/cogmaster/actions/runs/39514577638

## Summary

total: 88
passed: 88
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps

None.

