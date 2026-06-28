# RNetMsgBroker

## Version

0.2.15 — CMake-Build-Target `deliver` ergänzt. Das Ziel erzeugt ein versioniertes ZIP `RNetMsgBroker_v<version>.zip` ohne Build-/Git-/Ausgabedateien.

0.2.14 — `R-Net.json` und Dokumentation auf korrekt geschriebene, strikt gültige JSON-Schlüssel bereinigt. Tippfehler-Aliase werden nicht mehr dokumentiert und nicht mehr als Parser-Aliase akzeptiert.

0.2.13 — GitHub-Pflegestand: Skripte, Doku, CONTRIBUTING/SECURITY/Issue-Templates und zusätzliche Kommentare ergänzt.
0.2.12 — CSV-Spaltenüberschrift präzisiert: die letzte Spalte heißt jetzt `ausgabe`.
0.2.11 — Test-App: `--out`/`-o` schreibt Semikolon-CSV.
0.2.10 — `R-Net.json` um Frame-Metadaten `phase`, `quelle`, `senke` ergänzt.

Eigenständige Qt-Core-Shared-Library zum Dekodieren von R-Net/CAN-Botschaften aus einer editierbaren `R-Net.json`.

## Enthalten

- `RNetMsgBroker` als Shared Library
- `readJson(jsonFilename)` zum Laden von Frame-Definitionen aus syntaktisch gültigem JSON
- interne Ablage als `QMultiMap<id-result, Definition>`
- `add(json)` und `del(json)` für Runtime-Erweiterung/-Löschung
- `toString(CanMsg)` und `toString(canId, data, extended, remote)`
- `RNetMsgBrokerTest` als Konsolen-Test-App
- `R-Net.json` mit bekannten R-Net-Frame-Familien
- Frame-Metadaten `phase`, `quelle`, `senke` und `zyklus`
- Feld-Metadaten `unit`/`einheit` und `description`

## Build und Lieferung

Normaler Build:

```bash
./scripts/build.sh
```

GitHub-/Release-ZIP erzeugen:

```bash
./scripts/deliver.sh
```

Oder direkt über CMake:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target deliver
```

Das Ziel `deliver` erzeugt im Build-Verzeichnis:

```text
build/RNetMsgBroker_v0.2.15.zip
```

Das Archiv enthält nur Projektquellen, Dokumentation, Skripte, Testdaten und GitHub-Dateien. Build-Verzeichnisse, `.git`, CSV-/OUT-/LOG-Dateien und CMake-Zwischendateien werden ausgeschlossen.

## Ausgabeformat

`toString()` liefert bewusst eine kurze Broker-/Log-Zeile:

```text
RnetMsg_xyz (Modul=1); field_1=0xaa; field_2=0xbb
```

Dabei kommen `id_parts` aus der CAN-ID und `fields` aus dem Datenfeld hinter `#`.

## JSON-Schema

`R-Net.json` muss syntaktisch gültiges JSON sein. Keine Kommentare, keine unquotierten Schlüssel, keine `0x`-Zahlen ohne Anführungszeichen und keine absichtlich falsch geschriebenen Schlüssel.

Korrekte Schlüssel:

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
    { "name": "Modul", "bit_mask": "0xf0", "shift": 4, "big_endian": false }
  ],
  "fields": [
    {
      "name": "field_1",
      "bit_mask": "0x00ff00",
      "shift": 8,
      "big_endian": false,
      "einheit": "",
      "description": "oberes Beispiel-Datenbyte"
    },
    {
      "name": "field_2",
      "bit_mask": "0x0000ff",
      "shift": 0,
      "big_endian": false,
      "einheit": "",
      "description": "unteres Beispiel-Datenbyte"
    }
  ]
}
```

`readJson()` akzeptiert nur syntaktisch gültiges JSON. Prüfung:

```bash
python3 -m json.tool R-Net.json >/dev/null && echo JSON_OK
```

Bei `--full` erscheinen die Frame-Metadaten in der Abschlussklammer, z. B.:

```text
RNetJsmSerialHeartbeat; serial32=0xfc801ecd; serial_tail=0x0; {typ=serial, phase=startup.serial_auth, quelle=jsm, senke=pm, id=0x0000000e, mask=0x000007ff, result=0x0000000e, STD, data=FC 80 1E CD 00 00 00 00}
```

## CSV-Ausgabe

```bash
./build/RNetMsgBrokerTest -d ./testdata/candump_sample.txt --full -o ./testdata/candump_sample.csv
```

CSV-Spalten:

```text
zeile;zeit;interface;can_id;format;rtr;dlc;data;name;typ;phase;quelle;senke;ausgabe
```

## GitHub-Pflege

Enthalten sind `CHANGELOG.md`, `AUTHORS.md`, `CONTRIBUTING.md`, `SECURITY.md`, GitHub-Templates, `scripts/build.sh`, `scripts/check.sh`, `scripts/candump_to_csv.sh`, `scripts/list_unknown_observed.py` und die Dokumente unter `docs/`.

Die Lizenz ist bewusst nicht automatisch festgelegt. Vor öffentlicher Veröffentlichung bitte eine passende `LICENSE` ergänzen.

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build -j"$(nproc)"
```

## Test

```bash
./build/RNetMsgBrokerTest --both
./build/RNetMsgBrokerTest -d ./testdata/candump_sample.txt --full -o ./testdata/candump_sample.csv
./scripts/check.sh
```
