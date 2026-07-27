from wiz8decomp.reports.class_family import collect_family, render_family


def _write(function: str, site: str, offset: str, vtable: str) -> dict[str, str]:
    return {
        "program": "wiz8",
        "site": site,
        "function_start": function,
        "object_offset": offset,
        "vtable": vtable,
    }


def _table(address: str, slots: str) -> dict[str, str]:
    return {"program": "wiz8", "address": address, "kind": "vftable", "slot_count": slots}


def test_family_follows_writers_that_install_more_than_one_table() -> None:
    """A constructor installing a base then a derived table joins the two."""

    writes = [
        # ctor: base table then its own
        _write("00400100", "00400110", "0x0", "00500000"),
        _write("00400100", "00400120", "0x0", "00500040"),
        # dtor of the derived: subobject at +0x60, then the base's table
        _write("00400200", "00400210", "0x60", "00500000"),
        _write("00400200", "00400220", "0x0", "00500020"),
        # unrelated class, reachable from nothing above
        _write("00400300", "00400310", "0x0", "00509999"),
    ]
    tables = [_table(a, "4") for a in ("00500000", "00500020", "00500040", "00509999")]

    family = collect_family(writes, tables, [], "00500040")
    assert [item["vtable"] for item in family["tables"]] == [
        "00500000",
        "00500020",
        "00500040",
    ]
    assert "00509999" not in {item["vtable"] for item in family["tables"]}
    # Writers are attributed per table, which is what separates a base's
    # installer from its derived one.
    by_table = {item["vtable"]: item["written_by"] for item in family["tables"]}
    assert by_table["00500040"] == ["00400100"]
    assert by_table["00500000"] == ["00400100", "00400200"]


def test_family_seeded_anywhere_reaches_the_same_members() -> None:
    writes = [
        _write("00400100", "00400110", "0x0", "00500000"),
        _write("00400100", "00400120", "0x0", "00500040"),
    ]
    tables = [_table("00500000", "1"), _table("00500040", "2")]
    seeded_low = collect_family(writes, tables, [], "00500000")
    seeded_high = collect_family(writes, tables, [], "00500040")
    assert {item["vtable"] for item in seeded_low["tables"]} == {
        item["vtable"] for item in seeded_high["tables"]
    }


def test_rendering_keeps_the_object_offset_out_of_rich_markup() -> None:
    """The offset must survive printing.

    The first version wrote the offset as `[this+0x60]`, and the CLI prints
    through Rich, which read the brackets as a markup tag and dropped the
    offset entirely -- leaving a report whose whole point had vanished.
    """

    writes = [_write("00400200", "00400210", "0x60", "00500000")]
    rendered = render_family(
        collect_family(writes, [_table("00500000", "1")], [], "00500000")
    )
    assert "this+0x60" in rendered
    assert "[" not in rendered and "]" not in rendered
