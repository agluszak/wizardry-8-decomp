import hashlib
from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.sgp_oracle import BuildText, CoffFunction, _evaluate, classify_body, sweep_sgp_units


def test_sgp_sweep_rejects_unknown_units() -> None:
    with pytest.raises(RuntimeError, match="unknown SGP unit"):
        sweep_sgp_units(SimpleNamespace(), ["unknown"])


def test_relocation_masked_matcher_uses_the_five_way_vocabulary() -> None:
    function = CoffFunction("Example", b"\x55\x8b\xec\xe8\0\0\0\0\xc3", (4,))
    assert classify_body(function, function.body, near_threshold=0.75)["classification"] == "exact"
    relocated = b"\x55\x8b\xec\xe8\x12\x34\x56\x78\xc3"
    assert (
        classify_body(function, relocated, near_threshold=0.75)["classification"]
        == "relocation-equivalent"
    )
    assert (
        classify_body(function, relocated + b"padding" + relocated, near_threshold=0.75)[
            "classification"
        ]
        == "ambiguous-generic"
    )
    near = b"\x55\x8b\xec\xe8\x12\x34\x56\x78\x90"
    assert (
        classify_body(function, near, near_threshold=0.75)["classification"]
        == "near-source-with-wiz8-modifications"
    )
    assert (
        classify_body(function, b"\xcc" * len(function.body), near_threshold=0.75)["classification"]
        == "absent-or-stripped"
    )


def test_source_functions_with_the_same_masked_body_remain_ambiguous() -> None:
    first = CoffFunction("First", b"\xe8\0\0\0\0\xc3", (1,))
    second = CoffFunction("Second", b"\xe8\x11\x22\x33\x44\xc3", (1,))
    build = BuildText("test", "0" * 64, 0x400000, b"\xe8\xaa\xbb\xcc\xdd\xc3", None)
    rows = _evaluate([(("/O2",), [first, second])], [build], 0.75)
    assert {row["classification"] for row in rows} == {"ambiguous-generic"}
    assert {row["address"] for row in rows} == {""}
    assert {row["hit_count"] for row in rows} == {1}


def test_sgp_evaluation_rejects_a_flag_search_matrix() -> None:
    matching = CoffFunction("Example", b"\xc3", ())
    different = CoffFunction("Example", b"\x90\xc3", ())
    build = BuildText("test", "0" * 64, 0x400000, b"\xc3", None)
    with pytest.raises(RuntimeError, match="exactly one settled project profile"):
        _evaluate([(("/O1",), [matching]), (("/O2",), [different])], [build], 0.75, ("/O2",))


def test_vendored_sgp_source_retains_license_and_pinned_units() -> None:
    source = Path(__file__).resolve().parents[2] / "third_party/sfi-sgp/sgp"
    expected = {
        "LibraryDataBase.c": "2e8fa434e31ff65477eff35740eb3bfe7afd7cdac40dd9c2a67d281fcd9d6bab",
        "DbMan.c": "0d42da2c0be7e9c0f2935e0b5b619257997228a8f753aa5e0021793bd90b7d20",
        "SFI Source Code license agreement.txt": "f78ace6a6cfd40cb1b49de2e5fd4a113ebc58cab4864e4a4e5fffd428005c7fd",
    }
    for relative, digest in expected.items():
        assert hashlib.sha256((source / relative).read_bytes()).hexdigest() == digest
    assert not [path for path in source.iterdir() if path.suffix.casefold() == ".lib"]
