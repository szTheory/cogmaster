# The test card's pinned frame hashes: SHA-256 of frame 60's crop, rows top to
# bottom, each RGB444 pixel low byte first. Computed from the card's rules by
# an independent script, not by this code; every test reads them from here.
# A changed card changes these only together with the behaviour revision.
set(COGMASTER_TESTCARD_SHA256_SMS e14d77ce564f0b1498f703134e82e6c1fc718a27c94e49b5a35b95a981857ae7)
set(COGMASTER_TESTCARD_SHA256_GG 7404ed4adc74d6ccbe99f0e157fe0db873e78895834799123cbb04495db5e779)
# SHA-256 of the file cogmaster-run --frames 60 --png 60:FILE writes for the SMS
# card: 256 x 192 RGB, each RGB444 channel times 0x11. runner.png_file checks
# the IHDR and decodes every pixel back to the card before comparing it.
set(COGMASTER_TESTCARD_PNG_SHA256_SMS c4d1160ffb76d0d18c6866687250392f54bd685dae4fe222731d516fbe7a5460)
