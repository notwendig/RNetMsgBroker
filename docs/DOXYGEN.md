# Doxygen für RNetMsgBroker

## Erzeugen

```bash
sudo dnf install -y doxygen graphviz
./scripts/build_doxygen.sh
```

Die HTML-Dokumentation liegt danach hier:

```text
docs/api/html/index.html
```

Graphviz ist optional. Die mitgelieferte `Doxyfile` setzt `HAVE_DOT = NO`, damit die Dokumentation auch ohne `dot` erzeugt werden kann.

## CMake-Ziel

Wenn Doxygen installiert ist, kann auch das CMake-Ziel verwendet werden:

```bash
cmake -S . -B build/Desktop_Debug -DRNETMSGBROKER_BUILD_DOCS=ON
cmake --build build/Desktop_Debug --target docs
```

## Umfang

Die Doxygen-Konfiguration dokumentiert öffentliche API, private interne Parse-Strukturen und statische Hilfsfunktionen. Das ist für dieses Projekt absichtlich so gewählt, weil die JSON-Matching-Logik wichtig für Wartung und Tests ist.
