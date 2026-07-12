#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

if ! command -v doxygen >/dev/null 2>&1; then
    echo "FEHLER: doxygen ist nicht installiert." >&2
    echo "Fedora: sudo dnf install -y doxygen graphviz" >&2
    exit 127
fi

doxygen Doxyfile

echo "Doxygen HTML: $(pwd)/docs/api/html/index.html"
