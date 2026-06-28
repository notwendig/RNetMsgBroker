#!/usr/bin/env bash
set -euo pipefail

# Repository smoke test:
# 1. validate R-Net.json
# 2. build project if CMake/Qt are available
# 3. create CSV from bundled test data
# 4. check that no decoder UNKNOWN appears in CSV

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

python3 -m json.tool R-Net.json >/dev/null
echo "JSON_OK"

if command -v cmake >/dev/null 2>&1; then
  scripts/build.sh
else
  echo "cmake nicht gefunden: Build übersprungen" >&2
  exit 4
fi

BIN="$ROOT_DIR/build/RNetMsgBrokerTest"
if [[ ! -x "$BIN" ]]; then
  BIN="$ROOT_DIR/build/Desktop_Debug/RNetMsgBrokerTest"
fi

OUT="$ROOT_DIR/testdata/candump_sample.csv"
"$BIN" -d "$ROOT_DIR/testdata/candump_sample.txt" --full -o "$OUT"

test -s "$OUT"
if grep -q 'UNKNOWN ' "$OUT"; then
  echo "Decoder UNKNOWN in $OUT gefunden" >&2
  grep 'UNKNOWN ' "$OUT" >&2
  exit 5
fi

echo "CSV_OK $OUT"
python3 scripts/list_unknown_observed.py "$OUT" || true
