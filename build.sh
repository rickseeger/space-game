#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

# Optional apt hint (non-fatal)
MISSING=()
for pkg in g++ cmake libglfw3-dev libgl1-mesa-dev; do
  dpkg -s "$pkg" &>/dev/null || MISSING+=("$pkg")
done
if ((${#MISSING[@]})); then
  echo "Missing packages: ${MISSING[*]}"
  echo "Install with: sudo apt install ${MISSING[*]}"
  exit 1
fi

BUILD_TYPE="${1:-Release}"
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..
cmake --build . -j"$(nproc)"
echo ""
echo "Built: $ROOT/build/vector_drift"
echo "Run:   $ROOT/build/vector_drift"
