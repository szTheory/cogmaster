# Stack

**Project:** cogmaster · **Researched:** 2026-10-18 · Details and pins: `.planning/preparation/ENGINEERING.md`.

| Layer | Choice | Why |
|---|---|---|
| Language | C17, `CMAKE_C_EXTENSIONS OFF`; public header also valid C++ | Embeds in any host, console SDK or WASM |
| Build | CMake (≥ 3.25 for embedders; presets v11 locally and in CI), Ninja, CTest; one entrypoint `cmake --workflow --preset <lane>` | No script to keep in step with CI |
| Runtime dependencies | None; the core links only `memcpy`/`memmove`/`memset`/`memcmp` | Portability and determinism |
| Vendored | `libretro.h` (MIT) only | The libretro ABI |
| Test harness | Plain CTest executables, an in-repo check header and a small seeded property harness | No framework dependency |
| Fixture tools (pinned; the `ci` preset fetches them on any host) | WLA-DX v10.7 built from its source tarball (SHA-256 pinned) for own assembly fixtures; SingleStepTests z80 sample committed, full set fetched nightly; qemu-user + PowerPC GCC for the big-endian lane; wasi-sdk + wasmtime for the wasm32 lane; Valgrind for instructions per frame | Reproducible test inputs with no committed third-party ROM bytes |
| Formatting and analysis | clang-format 21, clang-tidy analyzer checks | One version locally and in CI |
| CI/CD | GitHub Actions (Ubuntu 26.04, macOS 15, Windows 2025/11-arm), release-please 17.6.1 via its v5.0.0 action, a GitHub App token, artifact attestations | Automatic, verifiable releases |
| Hosts | RetroArch 1.22.2 (Metal build) on the owner's Mac; the runner for automation and Playstead | No GUI in this repository |
