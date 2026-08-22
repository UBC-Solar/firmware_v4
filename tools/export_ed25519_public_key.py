#!/usr/bin/env python3
"""Convert an Ed25519 PEM public key into a generated C header."""

import argparse
import base64
from pathlib import Path


ED25519_SUBJECT_PUBLIC_KEY_INFO_PREFIX = bytes.fromhex("302a300506032b6570032100")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    pem = args.input.read_text(encoding="ascii")
    lines = [line.strip() for line in pem.splitlines() if not line.startswith("-----")]
    try:
        der = base64.b64decode("".join(lines), validate=True)
    except ValueError as exc:
        raise SystemExit(f"OTA public key is not valid PEM: {exc}") from exc
    if not der.startswith(ED25519_SUBJECT_PUBLIC_KEY_INFO_PREFIX) or len(der) != 44:
        raise SystemExit("OTA public key must be an Ed25519 SubjectPublicKeyInfo PEM key")
    raw = der[len(ED25519_SUBJECT_PUBLIC_KEY_INFO_PREFIX):]
    values = ", ".join(f"0x{byte:02X}" for byte in raw)
    content = f"""#ifndef SUNLITE_OTA_PUBLIC_KEY_H
#define SUNLITE_OTA_PUBLIC_KEY_H

#include <stdint.h>

static const uint8_t sunlite_ota_public_key[32] = {{
    {values}
}};

#endif /* SUNLITE_OTA_PUBLIC_KEY_H */
"""
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(content, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
