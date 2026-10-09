export BINARY_NAME := rack
# Code: the zero tail of the text segment's last 64 KB page (the text ends at 0x6fd5e8).
export CODE_CAVE_START := 0x6FD600
export CODE_CAVE_END := 0x700000
# Second code cave: RC1 code nothing reaches (0x5394a0..0x53c5cc: its only callers, 0x4ded28 and
# 0x4defb0, are never called or referenced themselves). It holds the walking (src/c/walk.c and
# src/asm/walk_hooks.s, see src/linker_template.ld), written as bin/$(BINARY_NAME)2.bin.
export CODE_CAVE2_START := 0x5394A0
export CODE_CAVE2_END := 0x53C5CC
# Globals and data: a 128 KB bss table at 0x9313a0..0x9513a0 that only dead code uses (RC1's
# live code never reads or writes it; its count at 0x9513a0 is zeroed at boot and stays 0).
# The globals are zero when the game boots and the patch never writes them, so they must be
# .bss only. The data in data/ (sounds, animations) sits around them; see src/patch.txt.
export GLOBALS_START := 0x93C000
export GLOBALS_END := 0x93C100

CC = powerpc64-linux-gnu-gcc
AS = powerpc64-linux-gnu-as
LD = powerpc64-linux-gnu-ld
OBJCOPY = powerpc64-linux-gnu-objcopy

# The C code is 32-bit, which would save the game's non-volatile registers with 32-bit
# stores. Keeping it off r14-r31 means it never saves them; every call into or out of
# the game goes through the 64-bit glue in src/asm/glue.s.
FIXED_REGS = $(foreach r,14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31,-ffixed-r$(r))

# Strafing switch: the #define at the top of src/c/strafe.c. "make" builds what is set there;
# "make BACK_ANIM_ADDR=0x..." overrides it for one build. It decides the C code that is built and
# whether its hook file in src/asm/optional/ goes into the patch. (RC2's acceleration, once the
# switch RC2_ACCELERATION, is always on now: src/c/walk.c.)
strafe_switch = $(shell sed -n 's/^.define $(1)[[:space:]]\{1,\}\([^[:space:]]*\).*/\1/p' src/c/strafe.c)
is_on = $(shell [ $$(($(1))) -ne 0 ] && echo 1)
ifndef BACK_ANIM_ADDR
BACK_ANIM_ADDR := $(call strafe_switch,BACK_ANIM_ADDR)
endif

# The assembly files whose PATCH lines go into the patch (scripts/linker.sh and patch.sh read this).
export HOOK_SRCS := $(wildcard src/asm/*.s) \
        $(if $(call is_on,$(BACK_ANIM_ADDR)),src/asm/optional/back_walk.s)

CFLAGS = -mcpu=cell -mbig -m32 -nostdlib -Os -ffunction-sections -fdata-sections -fno-builtin -fno-common \
         -Isrc/c -fsingle-precision-constant -msdata=none -Wall $(FIXED_REGS) \
         -DBACK_ANIM_ADDR=$(BACK_ANIM_ADDR)
ASFLAGS = -mregnames -mcell -be -a32

OBJS := $(patsubst src/c/%.c,obj/%.o,$(wildcard src/c/*.c)) \
        $(patsubst src/asm/%.s,obj/%.o,$(HOOK_SRCS))

all: clean bin/$(BINARY_NAME).bin bin/$(BINARY_NAME)2.bin patch.txt usage

obj/%.o: src/c/%.c src/c/*.h
	@mkdir -p $(@D) && $(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/asm/%.s
	@mkdir -p $(@D) && $(AS) $(ASFLAGS) -o $@ $<

obj/$(BINARY_NAME).ld: src/linker_template.ld src/symbols.ld
	@mkdir -p $(@D) && ./scripts/linker.sh

obj/$(BINARY_NAME).elf: $(OBJS) obj/$(BINARY_NAME).ld
	@$(LD) -m elf32ppc -T obj/$(BINARY_NAME).ld -o $@ $(OBJS) -Map=$(@:.elf=.map)

bin/$(BINARY_NAME).bin: obj/$(BINARY_NAME).elf
	@mkdir -p $(@D) && $(OBJCOPY) -O binary -j .text -j .rodata $< $@

bin/$(BINARY_NAME)2.bin: obj/$(BINARY_NAME).elf
	@mkdir -p $(@D) && $(OBJCOPY) -O binary -j .text2 $< $@

patch.txt: obj/$(BINARY_NAME).elf src/patch.txt
	@./scripts/patch.sh

usage:
	@echo "Strafing: BACK_ANIM_ADDR=$(BACK_ANIM_ADDR)"
	@echo "Code:$$(stat -c%s bin/$(BINARY_NAME).bin) / $$(($(CODE_CAVE_END) - $(CODE_CAVE_START))) bytes"
	@echo "Code 2:$$(stat -c%s bin/$(BINARY_NAME)2.bin) / $$(($(CODE_CAVE2_END) - $(CODE_CAVE2_START))) bytes"
	@echo "Globals: $$(powerpc64-linux-gnu-size -A obj/$(BINARY_NAME).elf | awk '/^\.bss/ {print $$2}') / $$(($(GLOBALS_END) - $(GLOBALS_START))) bytes"

# Regenerates data/anim.bin and data/sound.bin from tools/vendor (needs Python 3).
data:
	@python3 tools/make_data.py

patch_eboot:
	@python3 -W ignore ./scripts/eboot.py

clean:
	@rm -rf obj bin patch.txt EBOOT.BIN

.PHONY: all usage data patch_eboot clean
