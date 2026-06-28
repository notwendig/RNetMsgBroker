#!/usr/bin/env bash
set -euo pipefail

# Repository smoke test:
# 1. validate R-Net.json
# 2. build project if CMake/Qt are available
# 3. create CSV from bundled test data
# 4. check that no decoder UNKNOWN appears in CSV
# 5. create the delivery ZIP with CMake target deliver

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

python3 -m json.tool R-Net.json >/dev/null
echo "JSON_OK"

python3 - <<'PY'
import json
from pathlib import Path
forbidden = {"fi"+"eds", "des"+"cipt", "big_endi"+"en", "pfa"+"se"}
data = json.loads(Path("R-Net.json").read_text(encoding="utf-8"))
found = []
def walk(obj, path="$"):
    if isinstance(obj, dict):
        for k, v in obj.items():
            if k in forbidden:
                found.append(f"{path}.{k}")
            walk(v, f"{path}.{k}")
    elif isinstance(obj, list):
        for i, v in enumerate(obj):
            walk(v, f"{path}[{i}]")
walk(data)
if found:
    raise SystemExit("Forbidden JSON keys found:\n" + "\n".join(found))
print("JSON_KEYS_OK")
PY

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

cmake --build "$ROOT_DIR/build" --target deliver
DELIVER_ZIP="$ROOT_DIR/build/RNetMsgBroker_v$(head -1 VERSION.txt).zip"
test -s "$DELIVER_ZIP"
echo "DELIVER_OK $DELIVER_ZIP"
