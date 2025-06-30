#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <file>"
  exit 1
fi

file="$1"

# Make a backup in case you need it (comment out if you don't want a .bak file)
#cp -- "$file" "${file}.bak"

# Delete lines starting with '==' and update the file in-place
sed -i '/^==/d' -- "$file"
