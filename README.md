# cogmaster

A Sega Master System and Game Gear emulator core in C: a small library, a headless runner (`cogmaster-run`) and a
libretro core for RetroArch. One core covers the Mark III, both Master System revisions, the Japanese FM unit and
the Game Gear.

**Status: experimental.** Cartridge code runs with Mode 4 background video and pad input in the runner and
RetroArch. Games play with sound: the SN76489 (with Game Gear stereo) and, on Japanese Master System content, the
YM2413 FM unit; the core option `FM sound unit` turns that unit off or on. Video coverage is growing. With no content,
the built-in test card still runs.

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

## Play a game

cogmaster ships no BIOS and no game; bring a `.sms` or `.gg` file you own, no BIOS is needed.

In RetroArch: Main Menu → Load Core → **Sega - MS/GG (cogmaster)**, then Load Content and pick your file. Start
Core with no content shows the built-in test card instead; [docs/runner.md](docs/runner.md#test-card) explains
what each part of it checks.

With the runner, download `cogmaster-runner-<os>-<arch>.zip` from the same release, unzip it, and run 600 frames
of your file:

```sh
bin/cogmaster-run --frames 600 --hash game.sms
```

It prints the SHA-256 of the last frame's picture, the same on every platform:

```text
frame 600 sha256 <64 hex digits>
```

Add `--png 600:shot.png` to write the picture to a PNG file. Exit status 77 means the cartridge, its size or the
machine is not supported; see [docs/runner.md](docs/runner.md#exit-status).

With no file, the runner shows the test card, and `--machine gg` picks the Game Gear card:

```sh
bin/cogmaster-run --frames 60 --hash
```

## Documentation

- [docs/runner.md](docs/runner.md): `cogmaster-run` options, output, exit statuses and the test card.
- [docs/embedding.md](docs/embedding.md): using the C library from CMake, pkg-config or `add_subdirectory`.

## Legal

cogmaster ships no Sega BIOS, game or save images; your own dumps stay on your machine
([ASSET_POLICY.md](ASSET_POLICY.md)). Code from other projects enters only under permissive licences with their
notices ([PROVENANCE.md](PROVENANCE.md)). The project is not affiliated with or endorsed by Sega.

## Licence

[MIT](LICENSE). Every release archive also carries [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
