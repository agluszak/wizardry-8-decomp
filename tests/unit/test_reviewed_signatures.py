from pathlib import Path

from wiz8decomp.ghidra.reviewed_signatures import load_reviewed_signatures


def test_reviewed_wiz8_signatures_are_canonical_records() -> None:
    repository = Path(__file__).resolve().parents[2]
    signatures = load_reviewed_signatures(repository, "wiz8")

    assert [item.address for item in signatures] == [0x005222D0, 0x005E2890]
    assert signatures[0].parameters == (
        ("character_index", "int"),
        ("item", "void *"),
        ("origin", "unsigned char *"),
        ("slot", "unsigned short *"),
    )
    assert signatures[1].return_type == "int"
    assert signatures[1].parameters == (("list", "void *"), ("target", "void *"))

    apply_script = (
        repository / "tools/wiz8decomp/ghidra/apply_wiz8_signature_fixes.py"
    ).read_text(encoding="utf-8")
    assert "0x005E2890" not in apply_script.upper()
    assert "SIGNATURE_FIXES" not in apply_script
