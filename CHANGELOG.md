# Changelog

## 0.2.15

- CMake-Target `deliver` ergänzt.
- `scripts/deliver.py` erzeugt ein reproduzierbares, versioniertes Quell-ZIP.
- `scripts/deliver.sh` ergänzt als einfacher Wrapper für Build + Delivery.
- GitHub-Actions-Workflow erzeugt und veröffentlicht das Delivery-ZIP als Workflow-Artefakt.

## 0.2.14

- `R-Net.json` auf korrekt geschriebene Schlüssel normalisiert (`big_endian`, `description`, `fields`, `phase`).
- Tippfehler-Aliase aus Parser und Dokumentation entfernt.
- `readJson()` verlangt wieder syntaktisch gültiges JSON ohne automatischen Reparaturversuch.
- `scripts/check.sh` prüft `R-Net.json` zusätzlich auf verbotene Tippfehler-Schlüssel.

## 0.2.13

- GitHub-Pflegestand ergänzt: `CHANGELOG.md`, `AUTHORS.md`, `CONTRIBUTING.md`, `SECURITY.md`, Pull-Request-Template und Issue-Template.
- Skripte ergänzt: Build, Check, candump-zu-CSV und Analyse von `phase=unknown.observed`.
- Doku ergänzt: CSV-Format, JSON-Schema, Start-up-Sequenz und Umgang mit unbekannten Frames.
- Öffentliche API und wichtige Parser-/CSV-Funktionen kommentiert.
- Optionales CMake-Install-Target ergänzt.

## 0.2.12

- CSV-Header präzisiert: letzte Spalte heißt `ausgabe`.

## 0.2.11

- `--out` / `-o` schreibt Semikolon-CSV mit stabiler Spaltenstruktur.
- Datei-Ausgabe enthält nur CSV, keine Log-Kopf-/Abschlusszeilen.

## 0.2.10

- `R-Net.json` um `phase`, `quelle`, `senke` ergänzt.
- Serial-Exchange-Sequenzen in Data-/RTR-Definitionen getrennt.
- `--full` zeigt Frame-Metadaten.

## 0.2.9

- `--out <file>` / `-o <file>` ergänzt.

## 0.2.8

- Beobachtete 7-Byte-JSM-Heartbeat-Variante ergänzt.
- Zusätzliche Startup-/RTR-Familien ergänzt.

## 0.2.7

- Frame-Familien aus redragonx/open-rnet `reference/RNET_FRAME_DICTIONARY.md` übernommen.
- Spezifischere Datenmuster gewinnen beim Matching.

## 0.2.6

- `--candump` / `-d` liest candump-Dateien.
- `--full` für candump-Ausgabe.

## 0.2.5

- Standard-Demo ohne Argumente gibt Kurz- und Vollausgabe aus.

## 0.2.4

- `--both` / `-b` ergänzt.

## 0.2.3

- `toString(..., bool full=false)` ergänzt.

## 0.2.2

- `zyklus` auf Frame-Ebene verschoben; alte Feldvarianten bleiben kompatibel.

## 0.2.1 und älter

- Grundstruktur als Qt-Core-Library mit JSON-Katalog und Test-App.
