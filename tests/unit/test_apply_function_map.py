from pathlib import Path

import pytest
from wiz8decomp.ghidra.apply_function_map import load_function_identities


def test_load_function_identities_keeps_only_reviewed_names(tmp_path: Path) -> None:
    path = tmp_path / "functions.csv"
    path.write_text(
        "address,provisional_name,owner,confidence,evidence\n"
        "10001020,accepted,library,high,source match\n"
        "10001010,Class::method,adapter,strong,vtable slot\n"
        "10001030,guess,unknown,candidate,proximity only\n"
        "10001040,,unknown,exact,no name yet\n",
        encoding="utf-8",
    )

    identities = load_function_identities(path)

    assert [(identity.address, identity.name) for identity in identities] == [
        (0x10001010, "Class::method"),
        (0x10001020, "accepted"),
    ]


def test_load_function_identities_rejects_duplicate_addresses(tmp_path: Path) -> None:
    path = tmp_path / "functions.csv"
    path.write_text(
        "address,provisional_name,owner,confidence,evidence\n"
        "10001010,first,library,high,one\n"
        "10001010,second,library,exact,two\n",
        encoding="utf-8",
    )

    with pytest.raises(ValueError, match="duplicate accepted function addresses"):
        load_function_identities(path)


def test_wiz8_zlib_map_covers_library_and_owned_boundary() -> None:
    repository = Path(__file__).resolve().parents[2]
    identities = load_function_identities(repository / "config/analysis/functions/wiz8-zlib.csv")

    assert len(identities) == 51
    assert [identity.name for identity in identities[:5]] == [
        "wiz8_zalloc",
        "wiz8_zfree",
        "wiz8_inflate_create",
        "wiz8_inflate_read",
        "wiz8_inflate_destroy",
    ]
    assert sum(identity.owner == "zlib-1.0.4" for identity in identities) == 46
