# RNetMsgBroker Doxygen-Dokumentation {#index}

@brief Qt-Core-Bibliothek zum Dekodieren von R-Net/CAN-Frames über ein editierbares JSON-Katalogformat.

## Zweck

RNetMsgBroker trennt drei Aufgaben:

1. **Katalog laden**: `R-Net.json` wird in interne Definitionen übersetzt.
2. **Frame matchen**: Ein CAN-Frame wird gegen ID-Masken, STD/EXT/RTR-Flags und optionale Payload-Matcher geprüft.
3. **Frame rendern**: Das beste Match wird als kurzer oder vollständiger Text ausgegeben.

Die Bibliothek greift **nicht** selbst auf CAN-Hardware zu. Sie bekommt fertige Frames von einer Anwendung, einem Testprogramm oder einer Simulation.

## Wichtigste Klassen

- @ref RNetMsgBroker: lädt den JSON-Katalog und dekodiert `RNetMsgBroker::CanMsg`.
- @ref CanFrame: einfache Transportstruktur mit Kanal-, Richtung- und Zeitstempelinformation.
- @ref RNetFrame: Wrapper für QtRNetAnalyzer, der @ref CanFrame mit einem gemeinsamen @ref RNetMsgBroker-Katalog verbindet.

## Typischer API-Ablauf

```cpp
RNetMsgBroker broker;
QString error;
if (!broker.readJson(QStringLiteral("R-Net.json"), &error)) {
    qWarning().noquote() << error;
    return;
}

RNetMsgBroker::CanMsg msg;
msg.canId = 0x0C000400u;
msg.data = QByteArray::fromHex("0B00000000000000");
msg.extended = true;
msg.remote = false;

qInfo().noquote() << broker.toString(msg, false); // kompakt
qInfo().noquote() << broker.toString(msg, true);  // vollständig mit Metadaten
```

## Matching-Regeln

Jede Definition aus dem Katalog wird geprüft. Ein Match liegt vor, wenn:

- `(msg.canId & Id-and-mask) == id-result`,
- ein gesetztes `extended`-Flag zum Frameformat passt,
- ein gesetztes `remote`/`rtr`-Flag zum RTR-Status passt,
- ein optionaler `data`-/`payload`-Matcher passt.

Wenn mehrere Definitionen passen, gewinnt die Definition mit der höchsten Spezifität. Der Score berücksichtigt:

- Anzahl gesetzter Bits in `Id-and-mask`,
- explizite STD/EXT/RTR-Einschränkungen,
- aktive Payload-Matcher,
- Anzahl dekodierter Felder.

## JSON-Hinweise

Neue Definitionen sollten die kanonischen Schlüssel aus `docs/RNET_JSON_SCHEMA.md` verwenden:

- `Id-and-mask`
- `id-result`
- `key-mask`
- `rnet_name`
- `frametype`
- `phase`
- `quelle`
- `senke`
- `zyklus`
- `id_parts`
- `fields`
- `data`

Historische Alias-Schlüssel werden noch gelesen, damit ältere Kataloge funktionieren. Neue Tippfehler-Schlüssel wie `fieds`, `descipt`, `big_endien` oder `zyklu` sollten nicht mehr verwendet werden; `scripts/validate_rnet_json.py` prüft genau darauf.

## CLI-Testprogramm

`RNetMsgBrokerTest` ist ein kleines Kommandozeilenwerkzeug:

```bash
RNetMsgBrokerTest --libversion
RNetMsgBrokerTest --id 0x0C000400 --data 0B00000000000000 --extended --both
RNetMsgBrokerTest --candump testdata/candump_sample.txt --full
RNetMsgBrokerTest --candump testdata/candump_sample.txt --full --out testdata/candump_sample.csv
```

Die CSV-Ausgabe ist semikolongetrennt und für LibreOffice/Calc mit deutscher Locale geeignet.

## Sicherheitsgrenze

Diese Dokumentation beschreibt ausschließlich Dekodierung, Analyse und Testausgabe. Das Senden oder Injizieren von R-Net-/Rollstuhl-CAN-Frames ist nicht Teil der API und muss getrennt sicherheitstechnisch bewertet werden.
