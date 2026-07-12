# Safety and validation notes

This project decodes wheelchair R-Net/CAN frames. Treat all live CAN transmit
paths as safety-critical. For analysis on a real wheelchair bus prefer listen-only
capture.

The broker catalogue is intentionally JSON-driven. Run before release:

```bash
python3 -m json.tool R-Net.json >/dev/null
python3 scripts/validate_rnet_json.py R-Net.json
./scripts/check.sh
```

Use quoted strings for masks and IDs, for example `"0x56700"`. JSON numeric
values are stored as doubles inside Qt JSON and can lose precision for large
integers.
