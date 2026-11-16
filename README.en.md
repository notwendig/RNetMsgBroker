# RNetMsgBroker

[Deutsch](README.md)

RNetMsgBroker is a Qt Core shared library that decodes CAN/R-Net messages using editable definitions in `R-Net.json`. The repository also includes a console test application. The [German README](README.md) contains the version history, output format, build commands, and JSON schema details.

## Build and test

Use the CMake commands in the [German build section](README.md#build), then run the documented tests in [Test](README.md#test). The `deliver` target creates a versioned source archive. Validate changes to `R-Net.json` before relying on decoded fields; the catalog is a research artifact and may be incomplete.

## Safety and licensing

Decoded traffic must not be treated as a validated controller state. Do not transmit to a wheelchair bus based on decoder output alone. See [SECURITY.md](SECURITY.md), [LICENSE](LICENSE), and [LICENSE_TODO.md](LICENSE_TODO.md) for reporting and rights limitations.
