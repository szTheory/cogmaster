# cogmaster

A Sega Master System and Game Gear emulator core in C: a small library, a headless runner (`cogmaster-run`) and a
libretro core for RetroArch. One core covers the Mark III, both Master System revisions, the Japanese FM unit and
the Game Gear.

**Status: experimental.** Runs Master System and Game Gear cartridges with sound in RetroArch and the runner and
gives the same picture and audio hashes on every platform. It makes no promise about any single game: most
catalogued titles have no checked result and are listed unknown; the checks are narrow (a game boots, a frame
matches, a value appears), never "playable" or "completable". Counts and rows are in
[docs/compatibility.md](docs/compatibility.md). Not supported: cartridges for the SG-1000, save states, and the
boards the page lists as unsupported. The page covers Master System and Game Gear cartridges only, so
Mark III software is not tracked there.

The core option `FM sound unit` turns the YM2413 FM unit off or on. With no content, the built-in test card still
runs.

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
To start through a BIOS dump of your own instead, see [docs/bios.md](docs/bios.md); the real BIOS checks the cartridge header, so some games that run with the
default lock up under it.

In RetroArch: Main Menu → Load Core → **Sega - MS/GG (cogmaster)**, then Load Content and pick your file. Start
Core with no content shows the built-in test card instead; [docs/runner.md](docs/runner.md#test-card) explains
what each part of it checks.

A European game runs at NTSC speed until you ask for PAL: in RetroArch set **Region and timing (reload content)**
to **Export PAL**; with the runner use `--region pal`. Japanese Master System games get the FM sound unit on their
own; **FM sound unit (reload content)** or `--fm on` or `--fm off` overrides it. What is checked for each setting
is in [docs/compatibility.md](docs/compatibility.md).

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
machine is not supported, or a BIOS you asked for is missing; see [docs/runner.md](docs/runner.md#exit-status).

With no file, the runner shows the test card, and `--machine gg` picks the Game Gear card:

```sh
bin/cogmaster-run --frames 60 --hash
```

## Documentation

- [docs/runner.md](docs/runner.md): `cogmaster-run` options, output, exit statuses and the test card.
- [docs/bios.md](docs/bios.md): starting through a BIOS dump of your own, in the runner and in RetroArch.
- [docs/controllers.md](docs/controllers.md): Sports Pad, Paddle, Light Phaser and 3-D glasses titles, and the controller options.
- [docs/embedding.md](docs/embedding.md): using the C library from CMake, pkg-config or `add_subdirectory`.

## Legal

cogmaster ships no Sega BIOS, game or save images; your own dumps stay on your machine
([ASSET_POLICY.md](ASSET_POLICY.md)). Code from other projects enters only under permissive licences with their
notices ([PROVENANCE.md](PROVENANCE.md)). The project is not affiliated with or endorsed by Sega.

## Licence

[MIT](LICENSE). Every release archive also carries [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
