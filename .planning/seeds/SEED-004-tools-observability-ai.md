---
id: SEED-004
status: dormant
planted: 2026-10-18
planted_during: preparation, before milestone v1.0
trigger_when: states and movies exist (SEED-002) and a homebrew developer, ROM hacker, TAS author or AI user asks for hooks, a debugger view, Python bindings or a Gymnasium environment
scope: medium
---

# SEED-004: Tools — hooks, debugger views, ROM-hacker aids, Python and Gymnasium, Lua on demand (v5.0)

## Why This Matters

The SMS homebrew scene builds against Emulicious's debugger (breakpoints, watchpoints, tile/sprite/palette
viewers, profiler, WLA-DX `.sym` files). ROM hackers want IPS/BPS patching, symbol-aware traces and a memory
API. AI researchers want a vectorised, deterministic environment with RAM peeks. All of it is cheap on a core
whose state is one struct and whose loop can be compiled twice (traced and untraced).

## When to Surface

**Trigger:** SEED-002 shipped and a user ask. The inspection API (peek, counters, first-fault trace) is already
in v1.0; this seed adds the rest in YAGNI order.

## Scope Estimate

**Medium** — one or two phases: hooks (instruction, bus, VDP line, frame) at zero cost when off; `cogmaster-run
--patch` (IPS/BPS, fuzzed); `.sym` loading in traces; Python `ctypes` bindings and a Gymnasium environment;
Lua only when asked.

## Breadcrumbs

- `.planning/preparation/HOSTS-AND-INTEGRATION.md` section 5.
- `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` section 5 (ROM hackers).
- `.planning/preparation/ARCHITECTURE.md` section 8.
