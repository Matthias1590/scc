#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <source-file.c>"
    exit 1
fi

CURRENT_DIR="$(pwd)"
QBE="$CURRENT_DIR"/qbe/qbe

SRC_FILE="$1"
SRC_FOLDER="$(dirname "$SRC_FILE")"
SRC_NAME="$(basename "$SRC_FILE")"

cd "$SRC_FOLDER"

set -e

if [ -z "$2" ]; then
    make -B -C "$CURRENT_DIR" > /dev/null
else
    make -C "$CURRENT_DIR" > /dev/null
fi

"$CURRENT_DIR"/scc "$SRC_NAME" > /dev/null

"$QBE" -o out.s out.qbe > /dev/null
gcc -o out.elf out.s > /dev/null

set +e

./out.elf
