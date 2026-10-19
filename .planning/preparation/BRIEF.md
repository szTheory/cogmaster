---
title: "Preparation writing brief"
summary: "The conventions every preparation file follows, the licence rules for research, and the fan-out method; read by every agent that writes or revises a file in this directory."
read_when: "Before writing or revising any file in .planning/preparation/."
updated: 2026-10-18
---

# Preparation writing brief

cogmaster is a **Sega Master System + Game Gear** emulator core in C (one core; the Game Gear is a machine
profile). Also in the family: Mark III, the Japanese SMS with YM2413 FM, SG-1000/SC-3000 (compatibility mode is
a later seed). It is NOT any other console; another console appears only as labelled precedent and never lends
timing. The owner's sibling projects (muhtari = Atari 7800, nesturbator = NES, gabbaboy = Game Boy,
glueyneo = Neo Geo, genewhip = Genesis, playstead = frontend) are precedent for process and engineering only.

## File conventions (identical to muhtari's preparation pack)
- Frontmatter: `title`, `summary`, `read_when`, `updated` (ISO date). Then a **Where to look** table
  (`| Question | Section |`), then numbered sections (`## 1. …`, subsections `### 1.2`).
- Every factual claim cites `[PFX.NN]` resolving to a **Sources** table at the end of the same file:
  `| ID | Tier | Source | Pin or access date | Supports |`. Prefixes: SYS (SMS-SYSTEM), VDP (SMS-VDP),
  AUD (SMS-AUDIO), CART (SMS-CARTRIDGES-AND-PERIPHERALS), ECO (SMS-ECOSYSTEM-AND-USERS), SP (SMSPOWER-EXTRACTS),
  CF (CONFORMANCE), HI (HOSTS-AND-INTEGRATION), ENG (ENGINEERING), ARCH (ARCHITECTURE), GH (GSD-HANDOFF),
  PV (PROVENANCE.md), AP (ASSET_POLICY.md). `DEC.NN` is a row of DECISIONS.md.
- **Tiers.** T1 primary: official Sega documents, Zilog/TI/Yamaha datasheets, schematics, die/RTL analysis,
  measurements made here, a project's own repository for what that project does. T2 curated community reference
  (SMS Power! wiki and documents by named authors such as Charles MacDonald, Sean Young, Maxim; Sega Retro
  technical pages). T3 one implementation's behaviour as evidence of hardware (permissively licensed emulator
  source, FPGA RTL). T4 forum posts and issue trackers. A tier describes the source for the claim it supports.
- **Pins.** A 12-character commit SHA, a release tag, a wiki revision, or "accessed YYYY-MM-DD".
- **Markers.** (tested) was run here; (untested) was not; (derived) is inferred from cited facts;
  (unverified) comes from recall or was not fetched. Never present recall as a cited fact.
- **Choices.** Each gives a **Recommendation**, the **Strongest alternative**, and **Would change if**: the
  observation that would flip it. Settle every decision point: the GSD phases follow these without re-asking.
- **IDs** use a dot (`SYS.07`), never GSD's `D-NN` form. Prose is in our own words with a citation; no verbatim
  copying of documents longer than a short phrase.
- Hygiene: no e-mail addresses, no home-directory paths, no ROM names from the owner's private library, no
  credentials. Say "the owner" not a name.

## Licence rules for research (PROVENANCE)
- May be **opened and read, and seeded with notice**: ares (ISC) — `ares/ms/*`, `ares/component/processor/z80/*`;
  MAME files headed `license:BSD-3-Clause` (e.g. `src/devices/video/315_5124.cpp`, `src/mame/sega/sms*.cpp`,
  `src/devices/sound/sn76496.cpp`, `ymfm`); MAME `hash/sms.xml`, `hash/gamegear.xml`, `hash/sg1000.xml` (CC0;
  verify at pin); SingleStepTests/z80 (MIT); the owner's muhtari and nesturbator (MIT, same author).
- **Never opened** (facts about them come only from their docs, issues, changelogs and released binaries):
  Genesis Plus GX (non-commercial), Gearsystem (GPL-3), SMS Plus (GPL), MEKA, blastem (GPL), BizHawk cores that
  are GPL, jt89 and VM2413 inside the MiSTer SMS core (GPL), Emulicious (closed). MiSTer `SMS_MiSTer` has no
  LICENSE file: facts only, with the T80 core (BSD) noted.
- Check a licence at the exact pin (LICENSE file and per-file headers); report what you checked.

## Fan-out method
For each decision point, consider breadth and depth through every relevant stakeholder lens (emulator author,
hardware engineer, C/embedded engineer, performance engineer, security/fuzzing, product, speedrunner/TAS, ROM
hacker, homebrew developer, frontend integrator, handheld/browser porter), with pros, cons, trade-offs,
examples, patterns, anti-patterns, footguns and lessons from other emulators and ecosystems. Research online;
prefer primary sources. Run an adversarial pass on your own recommendation. Then synthesise ONE coherent
recommendation per point. Record everything high-signal with provenance so the file works as a RAG source.

## Size and style
Each file 200–350 lines, dense, scannable, no padding. Tables over prose where rows are parallel. Sentences
short. No em-dashes. Every number that matters is in a table with its source.
