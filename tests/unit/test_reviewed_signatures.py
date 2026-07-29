from pathlib import Path

from wiz8decomp.evidence.signatures import load_reviewed_signatures


def test_reviewed_wiz8_signatures_are_canonical_records() -> None:
    repository = Path(__file__).resolve().parents[2]
    signatures = load_reviewed_signatures(repository, "wiz8")

    by_address = {item.address: item for item in signatures}
    assert by_address[0x004ADDF0].this_type == "W8GrowableVector<int> *"
    assert by_address[0x005222D0].parameters == (
        ("character_index", "int"),
        ("item", "void *"),
        ("origin", "unsigned char *"),
        ("slot", "unsigned short *"),
    )
    assert {
        0x005E22C0,
        0x005E2370,
        0x005E23E0,
        0x005E2440,
        0x005E2480,
        0x005E2530,
        0x005E26B0,
        0x005E26E0,
        0x005E27C0,
        0x005E2870,
        0x005E2890,
        0x005E2900,
        0x005E29A0,
        0x005E2A00,
        0x005E2A60,
        0x005E2AA0,
        0x005E2B50,
        0x005E2B80,
        0x005E2C70,
        0x005E2C80,
        0x005E2CC0,
    } <= set(by_address)
    assert by_address[0x005E2890].return_type == "int"
    assert by_address[0x005E2890].parameters == (
        ("ppl", "W8PList *"),
        ("pEntry", "void *"),
    )
    assert by_address[0x004E5840].return_type == "W8MonsterInfo *"
    assert by_address[0x004E5840].parameters == (
        ("caller_line", "int"),
        ("caller_file", "char *"),
        ("location_id", "int"),
        ("assert_on_failure", "unsigned char"),
    )
    assert by_address[0x004E58B0].return_type == "W8MonsterDatabaseRecord *"
    assert by_address[0x004E5950].return_type == "Monster *"
    assert by_address[0x004E5990].return_type == "float"
    assert by_address[0x004E5990].parameters == (("monster_info", "W8MonsterInfo *"),)
    assert by_address[0x004E5AA0].return_type == "W8MonsterInfo *"
    assert by_address[0x004E5AA0].parameters == (("reset_iterator", "unsigned char"),)
    assert by_address[0x004E5AF0].return_type == "int"
    assert by_address[0x004E5AF0].parameters == (("monster_info", "W8MonsterInfo *"),)
    assert by_address[0x004E5B50].return_type == "int"
    assert by_address[0x004E5B50].parameters == (("monster_species", "unsigned int"),)
    assert by_address[0x004E5C00].parameters == (("forced_cleanup", "unsigned char"),)
    assert by_address[0x004E5E50].return_type == "W8MonsterInfo *"
    assert by_address[0x004EA310].parameters == (("forced_cleanup", "unsigned char"),)
    assert by_address[0x004E5E50].parameters == (("monster_species", "unsigned int"),)
    assert by_address[0x004E5EA0].parameters == ()
    assert by_address[0x004E6020].parameters == (
        ("monster_info", "W8MonsterInfo *"),
        ("control_state", "int"),
    )
    assert by_address[0x004E6780].return_type == "unsigned int"
    assert by_address[0x004E6780].parameters == (("record", "W8MonsterDatabaseRecord *"),)
    assert by_address[0x004E68C0].return_type == "unsigned char"
    assert by_address[0x004E68C0].parameters == ()
    assert by_address[0x00514BE0].parameters == (("handle", "int"), ("item", "void *"))
    assert by_address[0x00514C80].return_type == "void *"
    assert by_address[0x004F88F0].parameters[0] == (
        "output_items",
        "W8GrowableVector<W8WorldItem *> *",
    )
    assert by_address[0x004ADDF0].this_type == "W8GrowableVector<int> *"
