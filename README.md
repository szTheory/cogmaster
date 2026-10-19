# cogmaster

A Sega Master System and Game Gear emulator core in C. It ships as a small MIT-licensed library, a headless
command-line runner (`cogmaster-run`) and a libretro core for RetroArch. One core covers the Mark III, both
Master System revisions, the Japanese FM unit and the Game Gear.

**Status:** preparation complete (2026-10-18); the first release arrives with the first development phase. Until
then this repository holds the design and research pack under `.planning/preparation/`.

## Install in RetroArch (macOS, Apple Silicon)

The install line is published with the first release and verified in CI on every pull request.

## What it will do

- Run Master System and Game Gear cartridges with exact NTSC and PAL timing, the PSG with Game Gear stereo, and
  the YM2413 for the Japanese profile.
- Give identical frame and audio hashes on every platform (Linux, macOS, Windows, x64 and arm64, a 32-bit
  big-endian lane and WebAssembly).
- Stay small, integer-only and allocation-free after load, so it embeds in frontends, handhelds and browsers.
- Expose what speedrunners, TAS authors, homebrew developers and ROM hackers need: deterministic runs, counters,
  a first-fault trace, and later hooks, movies and bindings.

## Legal

No Sega BIOS or game bytes are in this repository or its releases; your own dumps stay on your machine
(`ASSET_POLICY.md`). Third-party code enters only under permissive licences with notices (`PROVENANCE.md`). The
project is not affiliated with or endorsed by Sega.

## Licence

MIT. See `LICENSE`.
