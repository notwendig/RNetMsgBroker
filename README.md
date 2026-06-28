# RNetMsgBroker

## Version

0.2.13 — GitHub-Pflegestand: Skripte, Doku, CONTRIBUTING/SECURITY/Issue-Templates und zusätzliche Kommentare ergänzt.

0.2.12 — CSV-Spaltenüberschrift präzisiert: die letzte Spalte heißt jetzt `ausgabe`.
0.2.11 — Test-App: `--out`/`-o` schreibt jetzt Semikolon-CSV mit Spalten `phase`, `quelle`, `senke`, `typ` und korrekt gequoteter Dekodierausgabe.

0.2.10 — `R-Net.json` um Frame-Metadaten `phase`, `quelle`, `senke` ergänzt. Serial-Exchange-Sequenzen haben getrennte `data`/`rtr`-Definitionen, damit Quelle/Senke stimmt. Der Broker liest zusätzlich die Aliasnamen `pfase`, `src` und `sink`/`dst`/`destination`/`target`; `--full` gibt die neuen Metadaten mit aus.

0.2.9 — Test-App: Option `--out <file>` / `-o <file>` ergänzt; normale Ausgabe kann nun direkt in eine Datei geschrieben werden.
0.2.8 — Diagnose candump.txt: 7-Byte-JSM-Heartbeat 03C30F0F ergänzt; beobachtete 0C1803xx- und 1E84..1E87-RTR-Familien als Teildefinitionen ergänzt.

0.2.7 — `R-Net.json` um Frame-Familien aus `redragonx/open-rnet/reference/RNET_FRAME_DICTIONARY.md` erweitert. Bei mehreren Treffern gewinnt das spezifischere Datenmuster.

0.2.6 — Test-App liest mit `--candump` oder `-d` candump-Dateien. `--full` schaltet dabei die Vollausgabe ein. Kurzoption `-d` ist jetzt candump; Einzelpayload bleibt `--data`.

0.2.5 — Standard-Demo ohne Argumente gibt jetzt beide Varianten aus: Kurzform (`full=false`) und Vollform (`full=true`).

0.2.4 — Test-App ergänzt `--both`, damit Kurzform (`full=false`) und Vollform (`full=true`) direkt nacheinander geprüft werden können.

0.2.3 — `toString()` hat den optionalen Parameter `bool full=false`. Kurzform bleibt unverändert; `full=true` ergänzt Frame-/Feld-Metadaten wie `zyklus`, `frametype`, `descipt`, Masken und Daten.

0.2.2 — `zyklu` wurde aus `fields[]` nach `frames[].zyklus` verschoben. `fields[]` lesen weiterhin `einheit`/`unit` und `descipt`/`description`; alte JSON-Dateien mit `fields[].zyklu` bleiben kompatibel.


Eigenständige Qt-Core-Shared-Library zum Dekodieren von R-Net/CAN-Botschaften aus einer editierbaren `R-Net.json`.

## Enthalten

- `RNetMsgBroker` als Shared Library
- `readJson(jsonFilename)` zum Laden von Frame-Definitionen
- interne Ablage als `QMultiMap<id-result, Definition>`
- `add(json)` und `del(json)` für Runtime-Erweiterung/-Löschung
- `toString(CanMsg)` und `toString(canId, data, extended, remote)`
- `RNetMsgBrokerTest` als kleine Konsolen-Test-App
- `R-Net.json` mit bekannten R-Net-Frame-Familien aus dem QtRNetAnalyzer-Stand `chatgpt`, editierbaren Beispielen und ergänzten Definitionen aus `redragonx/open-rnet/reference/RNET_FRAME_DICTIONARY.md`
- Frame-Metadaten `phase`, `quelle`, `senke` und `zyklus` werden auf Definitionsebene gelesen
- Feld-Metadaten `einheit`/`unit` und `descipt`/`description` werden eingelesen und intern mitgespeichert
- alte Dateien mit `fields[].zyklu` oder `fields[].zyklus` bleiben lesbar

## Ausgabeformat

`toString()` liefert bewusst eine kurze Broker-/Log-Zeile:

```text
RnetMsg_xyz (Modul=1); field_1=0xaa; field_2=0xbb
```

Dabei kommen `id_parts` aus der CAN-ID und `fields` aus dem Datenfeld hinter `#`.

## JSON-Schema, tolerant gelesen

Normales gültiges JSON:

```json
{
  "Id-and-mask": "0x56700",
  "id-result": "0x56700",
  "key-mask": "0xFFF0F",
  "rnet_name": "RnetMsg_xyz",
  "frametype": "example",
  "phase": "test.example",
  "quelle": "test",
  "senke": "test",
  "zyklus": "20ms",
  "id_parts": [
    { "name": "Modul", "bit_mask": "0xf0", "shift": 4, "big_endien": false }
  ],
  "fields": [
    {
      "name": "field_1",
      "bit_mask": "0x00ff00",
      "shift": 8,
      "big_endien": false,
      "einheit": "",
      "descipt": "oberes Beispiel-Datenbyte"
    },
    {
      "name": "field_2",
      "bit_mask": "0x0000ff",
      "shift": 0,
      "big_endien": false,
      "einheit": "",
      "descipt": "unteres Beispiel-Datenbyte"
    }
  ]
}
```

Dazu passend:

```text
0x56710#00aabb -> RnetMsg_xyz (Modul=1); field_1=0xaa; field_2=0xbb
0x56760#001234 -> RnetMsg_xyz (Modul=6); field_1=0x12; field_2=0x34
```

`readJson()` macht zusätzlich einen toleranten zweiten Parse-Versuch für typische Notiz-Schreibweisen:

- typografische Anführungszeichen `„…“` / `“…”`
- unquotierte Keys wie `{name: ...}`
- hex-Zahlen wie `0xf0`
- bare words wie `name:Modul`
- `//`-Kommentare außerhalb von Strings
- Schreibfehler `fieds` zusätzlich zu `fields`
- bei `frames[]`: `phase` oder `pfase`; `quelle` oder `src`; `senke`, `sink`, `dst`, `destination` oder `target`; außerdem `zyklus`, `zyklu`, `cycle`, `cyclus`, `period`, `intervall` oder `interval`
- bei `fields[]`: `einheit` oder `unit`; `descipt`, `descript`, `description`, `desc` oder `beschreibung`
- Kompatibilität: `fields[].zyklu` und `fields[].zyklus` werden noch gelesen, aber neue Dateien sollen `frames[].zyklus` verwenden

`...` als Platzhalter ist weiterhin kein JSON-Inhalt und muss entfernt werden.

Bei `--full` erscheinen die neuen Frame-Metadaten in der Abschlussklammer, z. B.:

```text
RNetJsmSerialHeartbeat; serial32=0xfc801ecd; serial_tail=0x0; {typ=serial, phase=startup.serial_auth, quelle=jsm, senke=pm, id=0x0000000e, mask=0x000007ff, result=0x0000000e, STD, data=FC 80 1E CD 00 00 00 00}
```


## GitHub-Pflege

Für einen sauberen Repository-Stand sind zusätzlich enthalten:

- `CHANGELOG.md` mit Versionshistorie
- `AUTHORS.md` mit Projekt-/Quellenhinweisen
- `CONTRIBUTING.md` für Änderungen an Code und `R-Net.json`
- `SECURITY.md` mit Sicherheitshinweisen für Rollstuhl-/CAN-Arbeiten
- `.github/workflows/cmake.yml` für einen CMake/Qt6-CI-Build
- `.github/ISSUE_TEMPLATE/bug_report.yml` und `.github/pull_request_template.md`
- `scripts/build.sh`, `scripts/check.sh`, `scripts/candump_to_csv.sh`
- `scripts/list_unknown_observed.py` zum Auffinden fachlich noch unbekannter Frames in CSV-Dateien
- `docs/CSV_FORMAT.md`, `docs/RNET_JSON_SCHEMA.md`, `docs/STARTUP_SEQUENCE.md`, `docs/UNKNOWN_FRAMES.md`

Die Lizenz ist bewusst **nicht automatisch festgelegt**. Vor öffentlicher Veröffentlichung bitte eine passende `LICENSE` ergänzen.

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build -j"$(nproc)"
```

Ohne Ninja:

```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
```

## Test

Automatische Beispiele, jetzt immer mit beiden Varianten `full=false` und `full=true`:

```bash
./build/RNetMsgBrokerTest
```

Ein einzelner Frame:

```bash
./build/RNetMsgBrokerTest --id 0x56710 --data 00aabb
./build/RNetMsgBrokerTest --id 0x56710 --data 00aabb --full
./build/RNetMsgBrokerTest --id 0x56710 --data 00aabb --both
./build/RNetMsgBrokerTest --id 0x56760 --data 001234 --both
```

Candump-Datei lesen:

```bash
./build/RNetMsgBrokerTest --candump candump.txt
./build/RNetMsgBrokerTest -d candump.txt
./build/RNetMsgBrokerTest -d candump.txt --full
./build/RNetMsgBrokerTest -d candump.txt --full --out result.csv
./build/RNetMsgBrokerTest -d candump.txt --full -o result.csv
./build/RNetMsgBrokerTest -d testdata/candump_sample.txt --full
```


Bei `--out`/`-o` wird Semikolon-CSV geschrieben:

```text
zeile;zeit;interface;can_id;format;rtr;dlc;data;name;typ;phase;quelle;senke;ausgabe
```

Die Spalte `ausgabe` ist CSV-gequotet, wenn sie selbst Semikolons enthält. Ohne `--out` bleibt die bisherige lesbare Textausgabe erhalten.

Unterstütztes candump-Format, z. B.:

```text
(1622641700.696733) can0 00C#
(1622641700.717310) can0 00E#FC801ECD00000000
(1622641700.717674) can0 7B3#
```

Andere JSON-Datei:

```bash
./build/RNetMsgBrokerTest --json /pfad/R-Net.json --id 0x56710 --data 00aabb
```
