# Cartridge identity and RAM

The loader hashes the complete body after removing a copier header, before padding. The immutable database
contains only full-body SHA-1 identities derived from the three pinned CC0 MAME software lists plus the
owned battery fixture fact. A chip hash from a compound record is never a whole-image identity. Public
identity, board support, persistence knowledge and gameplay compatibility are independent facts.

Mapper allocation and persistent extent are separate. Unknown images retain supported mapper heuristics
but persistence is unknown. Known nonbattery/volatile RAM is not battery storage. Supported Sega battery
sizes are 8, 16 and 32 KiB, with the mapper's documented power-of-two mirroring; the owned fixture selects
32 KiB and initializes it to FF. Its first three bytes hold A5, 5A and an 8-bit continuation count. Mailbox
record 01 reports the previous count in its low byte and the incremented count in its high byte.

## Moldorian disposition

The pinned [MAME Game Gear list](https://github.com/mamedev/mame/blob/1d7a454ed79e97d35d2a9f127ac029ead94711d6/hash/gamegear.xml)
identifies SHA-1 `0dad260b5357b2f61d1452bb73d8321ca53d7262`, battery yes and 24,576 declared bytes.
[SMS Power's game record](https://www.smspower.org/Games/Moldorian-GG) independently lists 24 KB backup.
These are high-confidence identity/declaration facts, not evidence of the physical address-line decode.

A narrow search on 2026-10-20 for Moldorian PCB, SRAM chip and address-line documentation found no
verifiable wiring source. Confidence in a specific physical decode remains insufficient. The metadata
therefore reports known battery and `COGMASTER_GEOMETRY_UNSUPPORTED`, retaining the 24 KiB declaration.
The ordinary Sega mapper remains bounded with its existing power-of-two allocation; no battery span
is authorized for this identity. The declaration never becomes a 24,575 mask, guessed modulo or truncation
of physical RAM. Persistence consumers must refuse this geometry explicitly. The supported owned fixture
provides the battery-continuation path independently. This disposition can change only with board evidence.

## Persistent writes

The Sega RAM window selects a 16 KiB bank with $FFFC bit 2 and enables RAM with bit 3.
For supported 8/16/32 KiB boards, unwired address bits mirror the physical power-of-two RAM
(SMS-CARTRIDGES-AND-PERIPHERALS section 2.1). Codemasters enables its 8 KiB window at
A000–BFFF with bit 7 of the 4000 register (section 2.2). Translation uses physical RAM
geometry first; only an offset in the separately classified persistent extent counts as a
persistent write. Counting same-value writes makes the generation independent of value comparisons.
This software persistence contract does not infer battery presence from mapper enable writes.
