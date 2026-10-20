# cogmaster

A Sega Master System and Game Gear emulator core in C: a small library, a headless runner (`cogmaster-run`) and a
libretro core for RetroArch. One core covers the Mark III, both Master System revisions, the Japanese FM unit and
the Game Gear.

**Status: experimental.** Cartridge code runs with Mode 4 background video and pad input in the runner and
RetroArch. Video coverage is growing; audio is silent. With no content, the built-in test card still runs.

## Install in RetroArch (macOS, Apple Silicon)

Paste this line into Terminal. It downloads the core and the release's checksum list, checks the core against
it, and only then puts the core and its information file into RetroArch's folders:

```sh
mkdir -p ~/Library/Application\ Support/RetroArch && cd "$(mktemp -d)" && curl -fsSL -O https://github.com/szTheory/cogmaster/releases/latest/download/cogmaster-libretro-macos-arm64.zip -O https://github.com/szTheory/cogmaster/releases/latest/download/SHA256SUMS && shasum -a 256 -c --ignore-missing SHA256SUMS && tar -xf cogmaster-libretro-macos-arm64.zip -C ~/Library/Application\ Support/RetroArch cores info
```

If the download was damaged or changed, `shasum` reports `FAILED` and nothing is installed.

On other systems, download `cogmaster-libretro-<os>-<arch>.zip` from the
[latest release](https://github.com/szTheory/cogmaster/releases/latest) (`<os>` is `linux`, `macos` or `windows`;
`<arch>` is `x64` or `arm64`), check it against `SHA256SUMS` from the same release, and copy its `cores` and
`info` folders into the folders RetroArch lists under Settings → Directory → Cores and Core Info.

## Requirements

- Linux x64/arm64: glibc 2.17+ for the core, 2.34+ for `cogmaster-run`.
- macOS 10.13+ (Intel) or 11+ (Apple Silicon).
- Windows 10+ x64/arm64, with no runtime install.

## Quick start: RetroArch

1. Main Menu → Load Core → **Sega - MS/GG (cogmaster)**.
2. Main Menu → **Start Core**.
3. The test card appears: a white border, a red band at the top, a green band at the bottom, a blue band on the
   right, a checkerboard on the left and a chart of the console's 64 colours.
   [docs/runner.md](docs/runner.md#test-card) explains what each part checks.

## Quick start: runner

Download `cogmaster-runner-<os>-<arch>.zip` from the same release, unzip it, and run 60 frames:

```sh
bin/cogmaster-run --frames 60 --hash
```

It prints the SHA-256 of the last frame's picture, the same on every platform:

```text
frame 60 sha256 e14d77ce564f0b1498f703134e82e6c1fc718a27c94e49b5a35b95a981857ae7
```

Add `--machine gg` for the Game Gear card, and `--png 60:card.png` to write the picture to a PNG file.

## Documentation

- [docs/runner.md](docs/runner.md): `cogmaster-run` options, output, exit statuses and the test card.
- [docs/embedding.md](docs/embedding.md): using the C library from CMake, pkg-config or `add_subdirectory`.

## Legal

cogmaster ships no Sega BIOS, game or save images; your own dumps stay on your machine
([ASSET_POLICY.md](ASSET_POLICY.md)). Code from other projects enters only under permissive licences with their
notices ([PROVENANCE.md](PROVENANCE.md)). The project is not affiliated with or endorsed by Sega.

## Licence

[MIT](LICENSE). Every release archive also carries [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
