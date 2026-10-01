#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH="$SCRIPT_DIR"
LIBS="$SCRIPT_DIR/../libraries"
BUILD="$SCRIPT_DIR/build_out"
FQBN="esp32:esp32:esp32"
NRFILE="$SCRIPT_DIR/build_nr.txt"
BUILD_H="$SCRIPT_DIR/build.h"

if ! command -v arduino-cli >/dev/null 2>&1; then
    echo "[ERROR] arduino-cli not found in PATH." >&2
    exit 1
fi

if [[ ! -d "$LIBS" ]]; then
    echo "[ERROR] Libraries folder not found: $LIBS" >&2
    exit 1
fi

nr=0
if [[ -f "$NRFILE" ]]; then
    nr="$(tr -dc '0-9' < "$NRFILE")"
    [[ -n "$nr" ]] || nr=0
fi
nr=$((nr + 1))
printf '%s\n' "$nr" > "$NRFILE"
cat > "$BUILD_H" <<EOF
#ifndef BUILD_NR_H
#define BUILD_NR_H

#define BUILD_NR $nr

#endif
EOF

echo
echo "Sketch  : $SKETCH"
echo "Libs    : $LIBS"
echo "Build   : $BUILD"
echo "Build Nr: $nr"
echo "FQBN    : $FQBN"
echo

arduino-cli compile \
    --fqbn "$FQBN" \
    --libraries "$LIBS" \
    --output-dir "$BUILD" \
    "$SKETCH"

echo
echo "Done."
