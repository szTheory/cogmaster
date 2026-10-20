; The memory map and Sega header every fixture shares: one 32 KiB bank in
; slot 0, and the header at $7FF0 with its checksum over $0000-$7FEF, the
; range a ROM size code of $C names (SMS-CARTRIDGES section 1.2). The size is
; set, never left to the assembler, so the image size is deliberate.

; A fixture with another map defines FIXTURE_MAP and its own map first.
.IFNDEF FIXTURE_MAP
.MEMORYMAP
DEFAULTSLOT 0
SLOT 0 START $0000 SIZE $8000
.ENDME

.ROMBANKMAP
BANKSTOTAL 1
BANKSIZE $8000
BANKS 1
.ENDRO
.ENDIF

.SMSHEADER
PRODUCTCODE 0, 0, 0
VERSION 0
REGIONCODE 4
RESERVEDSPACE 0, 0
ROMSIZE $C
.ENDSMS

.COMPUTESMSCHECKSUM
