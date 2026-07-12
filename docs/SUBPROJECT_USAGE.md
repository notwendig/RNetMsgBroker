# RNetMsgBroker als CMake-Subproject

`RNetMsgBroker` kann eigenständig gebaut oder per `add_subdirectory()` in eine größere Qt-Anwendung eingebunden werden.

Empfohlene Subproject-Optionen im Parent-Projekt:

```cmake
set(RNETMSGBROKER_BUILD_TESTAPP ON CACHE BOOL "Build broker smoke test" FORCE)
set(RNETMSGBROKER_BUILD_TRANSLATIONS OFF CACHE BOOL "No lupdate/lrelease in parent build" FORCE)
set(RNETMSGBROKER_INSTALL OFF CACHE BOOL "Parent controls install" FORCE)
set(RNETMSGBROKER_DELIVER OFF CACHE BOOL "Parent controls packaging" FORCE)
add_subdirectory(external/RNetMsgBroker)
```

Der Parent linkt danach gegen `RNetMsgBroker` oder `RNetMsgBroker::RNetMsgBroker`.
