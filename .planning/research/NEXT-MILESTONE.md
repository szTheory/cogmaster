# Next milestone recommendation: v2.2 "Legacy video modes"

Written 2026-11-13 at the v2.2 `/gsd-new-milestone` step, from nine lens reports (product, TMS9918 modes, SG-1000
machine, states and movies, browser/ports/tools, verification and CI, debt, sibling lessons, legal and security) and
one adversarial pass. Status: recommendation, not owner-approved scope. The rolling order lives in
`PRODUCT-ROADMAP.md`; the lessons are `CM-*` rows in `LESSONS.md`.

## Recommendation

v2.2 = Graphics I and II rendering (SEED-006 split), 2 phases plus a rider PR, then v3.0 States and rewind, then v3.1
SG-1000. Phase sketch and out-of-scope list are in `PRODUCT-ROADMAP.md` (Immediate).

## Why this over the others

- **Evidence.** 46 of the 49 remaining default-profile failing rows wait on legacy modes (graphics1 13, graphics2 33,
  no M4=1, text or multicolour row), so one small isolated module (about 150 to 190 lines of C, about 900 to 1,000 with
  tests and docs) addresses 94 percent of what still fails. SEED-006's own trigger has fired.
- **Risk.** Lowest legal and security exposure of any candidate: the MAME `tms9928a`, `sg1000` and `sk1100` files carry
  BSD-3 headers and the SG-1000 hash list is CC0, so no seed, dependency or owner consent is needed; only PROVENANCE
  section 2 needs rows. Existing pins are all M4=1 and the renderer dispatches once per line, so the base-versus-head
  hashes compare proves no pin moved (the v2.1 method).
- **Cost.** About 10 to 40 s more on the be32 lane. No public API change, so the near-full docs stay untouched.

## Strongest case against, and the answer

States (and rewind, run-ahead, achievements memory) are the largest gap for every player: the libretro core returns
serialize size 0 and exports only the battery span. A pure audience view ships them first. They lose on order only: the
trigger has not fired on evidence, the work is bigger (measured state about 317 KB, four open review items, no `fuzz.state`
target, an API bump), and every behaviour revision already refuses older states, so modes first costs no migration.
The order is the owner's to swap at no rework. Second objection: the flipped rows are about 2 to 3 percent of the
ledger on the weakest predicate. Accepted; that is why acceptance rests on owned fixtures and oracles, not the count.

## Facts the adversarial pass verified against the code

- State format 17 is needed: the saved sprite slots hold x, tile and row only, the row is bounded to 0..7 (legacy 16x16
  sprites need 0..15), overflow is tied to count 8, and the VDP struct size is pinned.
- The ledger predicate is `display && distinct >= 2 && !fault` with no input, so a correct static picture stays `fail`
  and the claim is "most leave the unsupported-mode leaf", not "46 pass".
- A currently passing row cannot regress within 600 frames: faults fire only with display on and an unsupported mode.
  The only hazard is the mode-4 dispatch edit.
- Four F-16 rows are `pass`; that they draw only the backdrop in play is an inference, so add a fixture that enters
  Graphics II after frame 600.
- Perf: 122,889 Ir per frame over 49,152 dots is about 2.5 Ir per dot; `docs/perf-fuzz.md` still shows a stale example.
- `SYSTEM_RAM` is not libretro-only: the header has only the one-byte `cogmaster_peek`, so cheat writes need an accessor.

## Open model choices that need in-phase evidence

Graphics II R3 and R4 masks, the sprite Y threshold, collision for colour-0 sprites, R0 bit 3, the text-mode border;
the prep note in `SMS-VDP.md` section 7 disagrees with MAME and must be rewritten in the phase. The TI manual is a scanned
PDF and no permissively licensed TMS9918 test ROM exists, so fixtures are self-authored.

## Missing items the phase plan must cover

Mode-aware backdrop and `completed.backdrop`; keep the Light Phaser "would this fault" predicate in step with the
supported-mode test; transient legacy dots during mode-4 bring-up now draw instead of fault; native Game Gear keeps
faulting and the docs say so. Do not make a MAME screenshot a gate (no BIOS bytes, `mame` not installed here).

## Debt and CI riders

Rider PR before phase 16: false `docs/audio.md:77`, stale `docs/embedding.md` lines, dirty-tree refusal in the library
run path, `curl --retry` on the assembler fetch. Phase 17: prune the dead diagnose vocabulary once, re-run the library
on `main` so the ledger stamp is an ancestor, re-derive timeouts. Other findings: raise `quality.json` floors and an
absolute `ir_per_frame` ceiling, trim `runner.diagnose` on the emulated lanes (3 to 5 min of wall time), and a
`scripts/milestone-clean.sh` check (open PRs, main CI, latest release, clean tree on main) before each milestone-close
docs PR. Doc headroom: `docs/runner.md` about 1 KB, `docs/embedding.md` about 2 KB, so split or prune before any
API-adding milestone.

## Roadmap rationale (after v2.2)

- v3.0 States and rewind: highest player value, mostly built (state visitor, CRC, refusal sweep exist), needs the public
  API, libretro plumbing, `fuzz.state` and a state-size spike. No movie format until a consumer is named.
- v3.1 SG-1000: 209 of 214 pinned entries should boot (190 plain carts plus 19 RAM-window carts), ledger 1,986 to 2,200
  rows; SC-3000 keyboard, tablet and multicarts stay out (no evidence; the "multicart" entries are Dahjee RAM pass-through).
  Mk I RAM size (1 KB or 2 KB) and ROM mirroring are unresolved hardware choices.
- Browser: ship the library as a zero-import wasm module with a hash test; the page belongs in a sibling repo unless
  the owner amends rule 2 and DEC.01. Needs an owner ruling.
- Movies: only with a named consumer; BizHawk import needs an inflate seed and zip-bomb caps, and BizHawk's source must
  not be read (licence history); use the TASVideos format pages.
- Tools: stdlib `ctypes` docs, no PyPI or npm packages. Ports: handhelds marginal (about 36 Ir per Z80 cycle estimate),
  libretro buildbot listing only after save states exist and as an owner hand-off.
- Measured accuracy stays parked until the owner has hardware.

## Primary sources used by the lens agents

MAME `tms9928a.cpp`, `315_5124.cpp`, `sg1000.cpp`, `sk1100.cpp` (BSD-3 headers checked at a pin) and `sg1000.xml` (CC0);
ares `component/video/tms9918` (ISC); libretro documentation; Gearsystem and Genesis Plus GX wiki pages and release
notes (documents only); TASVideos format pages. Several sources could not be verified (SMS Power! was down, the
TMS9918A datasheet is a scan, RetroAchievements pages looped); the lens reports said so and those points are listed above
as open.
