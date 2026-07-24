from __future__ import annotations

import struct
from pathlib import Path

from wiz8decomp.binary.rich_header import parse_rich_header


def test_decodes_rich_records(tmp_path: Path) -> None:
    key = 0x11223344
    decoded = bytearray(b"DanS" + b"\0" * 12)
    decoded.extend(struct.pack("<II", (4 << 16) | 8168, 3))
    encoded = bytearray()
    for offset in range(0, len(decoded), 4):
        encoded.extend(struct.pack("<I", struct.unpack_from("<I", decoded, offset)[0] ^ key))
    path = tmp_path / "rich.bin"
    path.write_bytes(b"MZ" + b"\0" * 30 + encoded + b"Rich" + struct.pack("<I", key))
    result = parse_rich_header(path)
    assert result is not None and result["valid"]
    assert result["records"] == [{"product_id": 4, "build": 8168, "count": 3}]

