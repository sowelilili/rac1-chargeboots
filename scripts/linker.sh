#!/bin/bash
set -e

OUTPUT="obj/${BINARY_NAME}.ld"

cp src/linker_template.ld "$OUTPUT"

sed -i "1a CODE_CAVE_START = $CODE_CAVE_START;\nCODE_CAVE_END = $CODE_CAVE_END;" "$OUTPUT"
sed -i "1a CODE_CAVE2_START = $CODE_CAVE2_START;\nCODE_CAVE2_END = $CODE_CAVE2_END;" "$OUTPUT"
sed -i "1a GLOBALS_START = $GLOBALS_START;\nGLOBALS_END = $GLOBALS_END;" "$OUTPUT"

sed -i '/\/\* SYMBOLS_GO_HERE \*\//r src/symbols.ld' "$OUTPUT"
sed -i '/\/\* SYMBOLS_GO_HERE \*\//d' "$OUTPUT"

# Every "PATCH addr, name, instruction" line in the hook files becomes a section at that address:
# src/asm/*.s, or the list the Makefile passes in HOOK_SRCS (it adds the optional ones switched on).
HOOK_SRCS=${HOOK_SRCS:-$(echo src/asm/*.s)}
HOOKS=$(mktemp)
cat $HOOK_SRCS | grep '^PATCH ' | \
    awk '{addr=$2; name=$3; gsub(/,/, "", addr); gsub(/,/, "", name); print "    .hook." name " " addr " : { *(.hook." name ") }"}' > "$HOOKS"
sed -i '/\/\* HOOKS_GO_HERE \*\//r '"$HOOKS" "$OUTPUT"
sed -i '/\/\* HOOKS_GO_HERE \*\//d' "$OUTPUT"
rm "$HOOKS"
