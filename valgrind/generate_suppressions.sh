#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <your_binary> [args…]"
  exit 1
fi

binary="$1"; shift

# Run Valgrind, merging in your old suppressions,
# generate *all* new suppressions, and disable the
# 1 000-error cutoff:
valgrind \
  --tool=memcheck \
  --leak-check=full \
  --show-leak-kinds=all \
  --gen-suppressions=all \
  --track-origins=yes \
  --error-limit=no \
  --suppressions=valgrind.supp \
  "$binary" "$@" 2>&1 \
| sed -n '/^{$/,/^}$/p' > new.supp

echo "New suppression blocks captured in new.supp"