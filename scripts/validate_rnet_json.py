#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Validate R-Net.json beyond mere JSON syntax.

Checks added for release safety without rejecting the existing catalogue:
- no legacy typo keys (fieds, descipt, big_endien, zyklu),
- IDs/masks/data matchers are parseable,
- duplicate matcher signatures are reported as warnings, not hard failures.

Important: catalogue masks may be wider than a raw 29-bit CAN ID because some
R-Net definitions use full SocketCAN-style masks such as 0xFFFFFFFF. Therefore
this validator checks parseability, not a too-narrow CAN-ID range.
"""
import json
import math
import re
import sys
from pathlib import Path

BAD_KEYS = {"fieds", "descipt", "big_endien", "zyklu"}
NUMERIC_KEYS = {
    "Id-and-mask", "id-and-mask", "id_and_mask", "mask",
    "id-result", "id_result", "result", "value",
    "key-mask", "key_mask", "keyMask",
    "bit_mask", "bit-mask", "offset", "byte", "byte_offset",
    "width", "size", "len", "length", "shift",
}
U64_MAX = (1 << 64) - 1

HEXBYTE_RE = re.compile(r"^(?:0x)?[0-9A-Fa-f]{1,2}(?:[\s,;:-]+(?:0x)?[0-9A-Fa-f]{1,2})*$")


def die(msg):
    print(f"R-Net.json FEHLER: {msg}", file=sys.stderr)
    return 1


def warn(msg):
    print(f"R-Net.json WARNUNG: {msg}", file=sys.stderr)


def parse_num(v, where, max_value=U64_MAX):
    if isinstance(v, bool):
        raise ValueError(f"{where}: bool ist keine Zahl")
    if isinstance(v, int):
        n = v
    elif isinstance(v, float):
        if not math.isfinite(v) or v < 0 or int(v) != v or abs(v) > (1 << 53):
            raise ValueError(f"{where}: JSON-Float ist unsicher; bitte String wie \"0x123\" verwenden")
        n = int(v)
    elif isinstance(v, str):
        s = v.strip()
        if not s:
            raise ValueError(f"{where}: leerer Zahlen-String")
        base = 16 if s.lower().startswith("0x") or any(c in s for c in "abcdefABCDEF") else 10
        n = int(s[2:] if s.lower().startswith("0x") else s, base)
    else:
        raise ValueError(f"{where}: ungeeigneter Zahlentyp {type(v).__name__}")
    if n < 0 or n > max_value:
        raise ValueError(f"{where}: außerhalb 0..0x{max_value:X}")
    return n


def parse_maybe_payload(v, where):
    """Return a stable signature for numeric or byte-string payload matchers."""
    if isinstance(v, list):
        out = []
        for i, item in enumerate(v):
            out.append(parse_num(item, f"{where}[{i}]", 0xFF))
        return tuple(out)
    if isinstance(v, str):
        s = v.strip()
        # Accept strings such as "00 AA BB" as byte payloads instead of one number.
        if HEXBYTE_RE.match(s) and any(sep in s for sep in " ,;:-"):
            parts = [x for x in re.split(r"[\s,;:-]+", s) if x]
            return tuple(parse_num(x, f"{where} byte", 0xFF) for x in parts)
    return parse_num(v, where)


def iter_objects(doc):
    if isinstance(doc, list):
        return doc
    if isinstance(doc, dict):
        for key in ("frames", "definitions", "messages"):
            if isinstance(doc.get(key), list):
                return doc[key]
        return [doc]
    raise ValueError("Top-Level muss Objekt oder Array sein")


def scan_bad_keys(value, path="$"):
    errors = []
    if isinstance(value, dict):
        for k, v in value.items():
            here = f"{path}.{k}"
            if k in BAD_KEYS:
                errors.append(f"{here}: alter Tippfehler-Schlüssel nicht erlaubt")
            errors.extend(scan_bad_keys(v, here))
    elif isinstance(value, list):
        for i, v in enumerate(value):
            errors.extend(scan_bad_keys(v, f"{path}[{i}]"))
    return errors


def data_signature(obj, idx):
    data = obj.get("data", obj.get("data-match", obj.get("data_match", obj.get("payload"))))
    if data is None:
        return None
    if isinstance(data, dict):
        value = data.get("value", data.get("data", data.get("id-result", data.get("result"))))
        mask = data.get("mask", data.get("bit_mask", data.get("bit-mask", "0xFFFFFFFFFFFFFFFF")))
        offset = data.get("offset", data.get("byte", 0))
        width = data.get("width", data.get("size", data.get("len", 0)))
        return (
            parse_maybe_payload(value, f"frames[{idx}].data.value") if value is not None else None,
            parse_maybe_payload(mask, f"frames[{idx}].data.mask"),
            parse_num(offset, f"frames[{idx}].data.offset"),
            parse_num(width, f"frames[{idx}].data.width"),
            bool(data.get("big_endian", data.get("big-endian", False))),
        )
    return (parse_maybe_payload(data, f"frames[{idx}].data"), "implicit-mask", 0, 0, False)


def main(argv):
    path = Path(argv[1]) if len(argv) > 1 else Path("R-Net.json")
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except Exception as e:
        return die(f"kann {path} nicht laden: {e}")

    bad = scan_bad_keys(doc)
    if bad:
        for msg in bad:
            print("R-Net.json FEHLER:", msg, file=sys.stderr)
        return 1

    try:
        frames = iter_objects(doc)
    except Exception as e:
        return die(str(e))

    if not frames:
        return die("keine Definitionen gefunden")

    seen = {}
    errors = []
    warnings = []
    for i, obj in enumerate(frames):
        if not isinstance(obj, dict):
            errors.append(f"frames[{i}]: ist kein Objekt")
            continue
        try:
            mask_v = obj.get("Id-and-mask", obj.get("id-and-mask", obj.get("id_and_mask", obj.get("mask"))))
            if mask_v is None:
                raise ValueError("Pflichtfeld Id-and-mask fehlt")
            # Match masks may legitimately be 0xFFFFFFFF or wider field masks;
            # do not restrict them to the 29-bit raw CAN-ID range.
            id_mask = parse_num(mask_v, f"frames[{i}].Id-and-mask")
            result_v = obj.get("id-result", obj.get("id_result", obj.get("result", obj.get("value", mask_v))))
            id_result = parse_num(result_v, f"frames[{i}].id-result")
            key_mask_v = obj.get("key-mask", obj.get("key_mask", obj.get("keyMask", id_mask)))
            key_mask = parse_num(key_mask_v, f"frames[{i}].key-mask")
            ext = obj.get("extended", obj.get("is_extended", obj.get("isExtended", "any")))
            rtr = obj.get("remote", obj.get("rtr", obj.get("is_remote", "any")))
            sig = (id_mask, id_result, key_mask, str(ext).lower(), str(rtr).lower(), data_signature(obj, i))
            name = obj.get("rnet_name", obj.get("name", f"frames[{i}]"))
            if sig in seen:
                warnings.append(f"frames[{i}] {name}: doppelte Matcher-Signatur wie {seen[sig]}")
            else:
                seen[sig] = f"frames[{i}] {name}"

            for list_name in ("id_parts", "fields"):
                if list_name in obj and not isinstance(obj[list_name], list):
                    errors.append(f"frames[{i}].{list_name}: muss Array sein")
                if isinstance(obj.get(list_name), list):
                    for j, item in enumerate(obj[list_name]):
                        if not isinstance(item, dict):
                            errors.append(f"frames[{i}].{list_name}[{j}]: muss Objekt sein")
                            continue
                        for key, val in item.items():
                            if key in NUMERIC_KEYS:
                                parse_num(val, f"frames[{i}].{list_name}[{j}].{key}")
        except Exception as e:
            errors.append(str(e))

    for w in warnings:
        warn(w)

    if errors:
        for e in errors:
            print("R-Net.json FEHLER:", e, file=sys.stderr)
        return 1

    print(f"R-Net.json OK: {len(frames)} Definitionen, {len(seen)} eindeutige Matcher")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
