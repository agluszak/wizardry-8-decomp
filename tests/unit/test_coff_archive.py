from __future__ import annotations

import struct

from wiz8decomp.binary.coff_archive import named_iat_archive


def test_named_iat_separates_caller_symbol_from_provider_name() -> None:
    caller = "?srAssertFail@@YAXPBD0J0@Z"
    provider = "?srAssertFail@@YAXPBD0J0ZZ"
    archive = named_iat_archive("SR.dll", caller, provider)
    assert archive.startswith(b"!<arch>\n")
    header = archive[8:68]
    # DLL grouping controls terminator placement.
    assert header[:16].decode("ascii").strip().rstrip("/") == "SR.dll"
    size = int(header[48:58].decode("ascii"))
    assert len(archive) in {68 + size, 68 + size + 1}
    data = archive[68 : 68 + size]
    machine, sections, _, symbols, count, _, _ = struct.unpack_from("<HHLLLHH", data)
    assert machine == 0x14C
    headers = [struct.unpack_from("<8sLLLLLLHHL", data, 20 + i * 40) for i in range(sections)]
    by_name = {header[0].rstrip(b"\0"): header for header in headers}
    assert set(by_name) == {b".idata$4", b".idata$5", b".idata$6"}
    hint = by_name[b".idata$6"]
    assert data[hint[4] + 2 : hint[4] + hint[3]].rstrip(b"\0") == provider.encode()
    strings = data[symbols + count * 18 :]
    assert b"__imp_" + caller.encode() + b"\0" in strings
    assert b"__IMPORT_DESCRIPTOR_SR\0" in strings
    for name in (b".idata$4", b".idata$5"):
        offset, symbol, relocation = struct.unpack_from("<LLH", data, by_name[name][5])
        assert offset == 0
        assert relocation == 7  # IMAGE_REL_I386_DIR32NB, not an absolute pointer.
        assert data[symbols + symbol * 18 : symbols + symbol * 18 + 8] == b".idata$6"
