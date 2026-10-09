#!/bin/bash
# Writes patch.txt: the header from src/patch.txt, the code, the data lines from src/patch.txt,
# then the hook words. The hooks go last so nothing jumps into the cave before it is written.
set -e

# The hook files: src/asm/*.s, or the list the Makefile passes (it adds the optional ones switched on).
HOOK_SRCS=${HOOK_SRCS:-$(echo src/asm/*.s)}

OBJDUMP=powerpc64-linux-gnu-objdump
OUTPUT="patch.txt"
ELF="obj/${BINARY_NAME}.elf"

head -5 src/patch.txt > "$OUTPUT"
echo "" >> "$OUTPUT"
echo "$CODE_CAVE_START: bin/$BINARY_NAME.bin" >> "$OUTPUT"
echo "$CODE_CAVE2_START: bin/${BINARY_NAME}2.bin" >> "$OUTPUT"
tail -n +6 src/patch.txt >> "$OUTPUT"
echo "" >> "$OUTPUT"
echo "#### Hooks ####" >> "$OUTPUT"

cat $HOOK_SRCS | grep '^PATCH ' | \
while read _ addr name rest; do
    addr=${addr%,}
    name=${name%,}
    INSN=$($OBJDUMP -s -j ".hook.$name" "$ELF" 2>/dev/null | grep -A1 "Contents of section" | tail -1 | awk '{print $2}')
    if [ -z "$INSN" ]; then
        echo "no word for hook $name" >&2
        exit 1
    fi
    echo "$addr: 0x$INSN" >> "$OUTPUT"
done
