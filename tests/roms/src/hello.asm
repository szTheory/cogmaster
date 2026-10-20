; The smallest fixture: a valid export header and a jump to itself at the
; entry point. It checks the assembler pin and the header checksum.

.INCLUDE "header.inc"

.BANK 0 SLOT 0
.ORG $0000
start:
    jp start
