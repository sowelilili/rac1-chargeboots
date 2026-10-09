#!/usr/bin/env python3
"""Writes data/sound.bin, data/anim.bin and data/back_anim.bin: RC2's charge sounds, charge
animation and backwards walk animation.

All three come from Joe-GH-18/RAC-1-Mods (vendored in vendor/RAC-1-Mods, Apache 2.0; the data itself
is Ratchet & Clank 2's). Upstream loads them through extra ELF segments; RaCMAN writes into the
running game, so they go into a 128 KB bss table (0x9313a0..0x9513a0) that only dead code uses:

    0x931400  sound.bin      upstream's two sound definitions (+0 loop, +0x20 end), then at +0x40
                             upstream's cut-down RC2 level 5 sound bank, unchanged
    0x93C000                 (the mod's globals, 0x100 bytes; see the Makefile)
    0x940000  anim.bin       Ratchet animation 139 (the charge), frame pointers relocated for here
    0x945000  back_anim.bin  Ratchet animation 0x14 (the backwards walk), relocated likewise

The addresses must match src/patch.txt, ANIM_ADDR / SOUND_ADDR in src/c/charge.c and
BACK_ANIM_ADDR in src/c/strafe.c.

    python3 tools/make_data.py      (from the mod folder; no packages needed)
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
MOD = os.path.dirname(HERE)
ASSETS = os.path.join(HERE, "vendor", "RAC-1-Mods", "assets")

TABLE = (0x9313A0, 0x9513A0)            # the unused bss table
GLOBALS = (0x93C000, 0x93C100)
SOUND_ADDR = 0x931400
ANIM_ADDR = 0x940000
BACK_ANIM_ADDR = 0x945000

# Upstream's sound definitions (32 bytes: +0x18 loop type, +0x1a sound index in the bank, +0x1c bank
# handle at runtime): level 5's sounds 24 (the loop) and 20 (the end).
SOUND_DEFS = (bytes.fromhex("000000004280000000000000000004cc000000000000000001000018" "00000000"),
              bytes.fromhex("00000000428000000000000000000200000000000000000000000014" "00000000"))
SOUND_BANK_OFF = 0x40                   # must be 16-byte aligned


def animation(name, vaddr):
    """An RC2 Ratchet animation with absolute frame pointers, as RC1's level loader leaves them."""
    anim = bytearray(open(os.path.join(ASSETS, name), "rb").read())
    for i in range(anim[0x10]):
        off = 0x1C + 4 * i
        struct.pack_into(">I", anim, off, struct.unpack_from(">I", anim, off)[0] + vaddr)
    anim[0x13] = 0                      # RC1 animations have 0 here, RC2's 0xff
    return bytes(anim)


def sounds():
    head = SOUND_DEFS[0] + SOUND_DEFS[1]
    bank = open(os.path.join(ASSETS, "rc2_charge_sounds.bin"), "rb").read()
    return head + bytes(SOUND_BANK_OFF - len(head)) + bank


def main():
    blobs = [("sound.bin", SOUND_ADDR, sounds()),
             ("anim.bin", ANIM_ADDR, animation("rc2_anim139.bin", ANIM_ADDR)),
             ("back_anim.bin", BACK_ANIM_ADDR, animation("rc2_anim20.bin", BACK_ANIM_ADDR))]
    ranges = sorted([(addr, addr + len(data), name) for name, addr, data in blobs] + [(*GLOBALS, "globals")])
    for (start, end, name), (next_start, _, next_name) in zip(ranges, ranges[1:] + [(TABLE[1], 0, "end of table")]):
        if start < TABLE[0] or end > next_start:
            sys.exit(f"{name} ({start:#x}..{end:#x}) overlaps {next_name} or leaves the table")
    os.makedirs(os.path.join(MOD, "data"), exist_ok=True)
    for name, addr, data in blobs:
        with open(os.path.join(MOD, "data", name), "wb") as f:
            f.write(data)
        print(f"{name:14} {len(data):6} bytes at {addr:#x}..{addr + len(data):#x}")


if __name__ == "__main__":
    main()
