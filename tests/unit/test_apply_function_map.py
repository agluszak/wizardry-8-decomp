import csv
import io
from pathlib import Path

import pytest
from wiz8decomp.evidence.schema import schema_for
from wiz8decomp.ghidra.apply_function_map import load_function_identities

HEADER = "address,provisional_name,owner,confidence,name_origin,authority,evidence\n"


def _write_functions(path: Path, text: str) -> None:
    rows = list(csv.DictReader(io.StringIO(text)))
    columns = schema_for("functions.csv").columns
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=columns)
        writer.writeheader()
        for row in rows:
            row.setdefault("program", "unknown")
            writer.writerow(row)


def test_load_function_identities_keeps_only_reviewed_names(tmp_path: Path) -> None:
    path = tmp_path / "functions.csv"
    _write_functions(
        path,
        HEADER + "10001020,accepted,library,high,original-source,source-backed,source match\n"
        "10001010,Class::method,adapter,strong,descriptive,descriptive,vtable slot\n"
        "10001030,guess,unknown,candidate,descriptive,descriptive,proximity only\n"
        "10001040,,unknown,exact,descriptive,descriptive,no name yet\n",
    )

    identities = load_function_identities(path)

    assert [(identity.address, identity.name) for identity in identities] == [
        (0x10001010, "Class::method"),
        (0x10001020, "accepted"),
    ]
    assert identities[1].name_origin == ("original-source",)
    assert identities[1].authority == "source-backed"


def test_load_function_identities_rejects_duplicate_addresses(tmp_path: Path) -> None:
    path = tmp_path / "functions.csv"
    _write_functions(
        path,
        HEADER + "10001010,first,library,high,descriptive,descriptive,one\n"
        "10001010,second,library,exact,descriptive,descriptive,two\n",
    )

    with pytest.raises(ValueError, match="duplicate identity"):
        load_function_identities(path)


def test_load_function_identities_rejects_unsupported_authority(tmp_path: Path) -> None:
    path = tmp_path / "functions.csv"
    _write_functions(
        path,
        HEADER + "10001010,Random,shared,exact,fan-patch-signature,source-backed,cfagent seed\n",
    )

    with pytest.raises(ValueError, match="not derivable"):
        load_function_identities(path)


def test_load_function_identities_reads_lower_authority_aliases(tmp_path: Path) -> None:
    path = tmp_path / "functions.csv"
    _write_functions(
        path,
        "address,provisional_name,owner,confidence,name_origin,authority,aliases,evidence\n"
        "0040efa0,Random,sgp-shared,exact,sgp-source|fan-patch-signature,source-backed,"
        "GetRandomNumber,compiled body\n",
    )

    identity = load_function_identities(path)[0]

    assert identity.name == "Random"
    assert identity.aliases == ("GetRandomNumber",)
    assert identity.name_origin == ("fan-patch-signature", "sgp-source")


def test_load_function_identities_rejects_self_alias(tmp_path: Path) -> None:
    path = tmp_path / "functions.csv"
    _write_functions(
        path,
        "address,provisional_name,owner,confidence,name_origin,authority,aliases,evidence\n"
        "0040efa0,Random,sgp-shared,exact,sgp-source,source-backed,Random,compiled body\n",
    )

    with pytest.raises(ValueError, match="listed as its own alias"):
        load_function_identities(path)


def test_load_function_identities_uses_stable_ids_not_evidence_narratives(
    tmp_path: Path,
) -> None:
    path = tmp_path / "functions.csv"
    _write_functions(
        path,
        "program,address,provisional_name,owner,confidence,name_origin,authority,"
        "source_path,evidence\n"
        "wiz8,005e2890,PListIndexOf,wiz8-foundation,exact,descriptive,descriptive,"
        "3D Code\\PList.cpp,a long reconstruction narrative\n",
    )
    (tmp_path / "function-evidence.csv").write_text(
        "evidence_id,program,address,origin,kind,reference,details\n"
        "function-evidence:wiz8:005e2890:reccmp,wiz8,005e2890,reccmp,exact-compile,"
        "match:e0393a26,a much longer explanation\n",
        encoding="utf-8",
    )

    identity = load_function_identities(path)[0]

    assert identity.identity_id == "functions:wiz8:005e2890"
    assert identity.source_unit == "3D Code\\PList.cpp"
    assert identity.evidence_ids == ("function-evidence:wiz8:005e2890:reccmp",)
    assert identity.evidence == "a long reconstruction narrative"


def test_wiz8_zlib_map_covers_library_and_owned_boundary() -> None:
    repository = Path(__file__).resolve().parents[2]
    identities = load_function_identities(repository / "evidence/reviewed/wiz8/functions.csv")
    identities = [
        identity
        for identity in identities
        if identity.owner in {"sgp-compression", "zlib-1.0.4"}
    ]

    assert len(identities) == 51
    assert [identity.name for identity in identities[:5]] == [
        "ZAlloc",
        "ZFree",
        "DecompressInit",
        "Decompress",
        "DecompressFini",
    ]
    assert sum(identity.owner == "zlib-1.0.4" for identity in identities) == 46
