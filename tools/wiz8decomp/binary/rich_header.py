from __future__ import annotations

import struct
from pathlib import Path
from typing import Any


def parse_rich_header(path: Path) -> dict[str, Any] | None:
    data = path.read_bytes()[:0x1000]
    rich = data.find(b"Rich")
    if rich < 0 or rich + 8 > len(data):
        return None
    key = struct.unpack_from("<I", data, rich + 4)[0]
    dans_encoded = struct.pack("<I", 0x536E6144 ^ key)
    start = data.rfind(dans_encoded, 0, rich)
    if start < 0:
        return {"xor_key": f"0x{key:08x}", "records": [], "valid": False}
    decoded = bytearray()
    for offset in range(start, rich, 4):
        decoded.extend(struct.pack("<I", struct.unpack_from("<I", data, offset)[0] ^ key))
    records = []
    for offset in range(16, len(decoded), 8):
        if offset + 8 > len(decoded):
            break
        product_build, count = struct.unpack_from("<II", decoded, offset)
        product_id = product_build >> 16
        build = product_build & 0xFFFF
        records.append({"product_id": product_id, "build": build, "count": count})
    return {"xor_key": f"0x{key:08x}", "records": records, "valid": decoded[:4] == b"DanS"}
