# RNetMsgBroker

## Version

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
- `R-Net.json` mit bekannten R-Net-Frame-Familien aus dem QtRNetAnalyzer-Stand `chatgpt` plus editierbaren Beispielen
- Frame-Metadatum `zyklus` wird auf Definitionsebene gelesen
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
- bei `frames[]`: `zyklus`, `zyklu`, `cycle`, `cyclus`, `period`, `intervall` oder `interval`
- bei `fields[]`: `einheit` oder `unit`; `descipt`, `descript`, `description`, `desc` oder `beschreibung`
- Kompatibilität: `fields[].zyklu` und `fields[].zyklus` werden noch gelesen, aber neue Dateien sollen `frames[].zyklus` verwenden

`...` als Platzhalter ist weiterhin kein JSON-Inhalt und muss entfernt werden.

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

Andere JSON-Datei:

```bash
./build/RNetMsgBrokerTest --json /pfad/R-Net.json --id 0x56710 --data 00aabb
```
