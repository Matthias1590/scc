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

make -C "$CURRENT_DIR"
"$CURRENT_DIR"/scc "$SRC_NAME"

"$QBE" -o out.s out.qbe
gcc -o out.out out.s

set +e

./out.out
