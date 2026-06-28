# R-Net JSON catalogue

`R-Net.json` is a practical catalogue, not a strict formal schema. The broker reads it tolerantly so the file can also be used during reverse engineering.

## Recommended frame fields

```json
{
  "Id-and-mask": "0x7FF",
  "id-result": "0x00E",
  "key-mask": "0x7FF",
  "rnet_name": "RNetJsmSerialHeartbeat",
  "frametype": "serial",
  "phase": "startup.serial_auth",
  "quelle": "jsm",
  "senke": "pm",
  "summary": "JSM serial heartbeat during startup.",
  "zyklus": "50ms",
  "id_parts": [],
  "fields": []
}
```

## Meaning of phase / quelle / senke

- `phase`: logical protocol phase, for example `startup.serial_auth`, `startup.config_discovery`, `runtime.normal_operation`, `unknown.observed`.
- `quelle`: likely source. This is inferred from known R-Net conventions and the current trace; candump itself does not prove the sender.
- `senke`: likely target or interested receiver.

## Unknown but observed

Use this for frames that match a repeated pattern but whose meaning is not proven:

```json
"phase": "unknown.observed"
```

This is different from a real decoder `UNKNOWN` output. It means the frame is recognized but not fully understood.
