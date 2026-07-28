from __future__ import annotations

import hashlib
import struct
from pathlib import Path


def metadata_normalized_pe_hash(path: Path) -> str:
    """Hash a PE after zeroing timestamp, checksum, and certificate-directory fields."""
    data = bytearray(path.read_bytes())
    if data[:2] != b"MZ" or len(data) < 0x40:
        return hashlib.sha256(data).hexdigest()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if pe + 0x98 > len(data) or data[pe : pe + 4] != b"PE\0\0":
        return hashlib.sha256(data).hexdigest()
    data[pe + 8 : pe + 12] = b"\0" * 4
    optional = pe + 24
    magic = struct.unpack_from("<H", data, optional)[0]
    checksum = optional + 64
    if checksum + 4 <= len(data):
        data[checksum : checksum + 4] = b"\0" * 4
    directory = optional + (96 if magic == 0x10B else 112)
    security = directory + 8 * 4
    if security + 8 <= len(data):
        data[security : security + 8] = b"\0" * 8
    return hashlib.sha256(data).hexdigest()


def bytes_entropy(data: bytes) -> float:
    if not data:
        return 0.0
    import math

    counts = [0] * 256
    for byte in data:
        counts[byte] += 1
    return -sum((count / len(data)) * math.log2(count / len(data)) for count in counts if count)
