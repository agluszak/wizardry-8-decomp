from pathlib import Path
from types import SimpleNamespace

import pytest
from reccmp.compare.csv import csv_parse
from reccmp.types import EntityType, ImageId
from wiz8decomp.emissions import (
    INVENTORY,
    OUTPUT,
    OVERRIDES,
    Emission,
    derive_emissions,
    emission_inventory,
    generate_emissions,
    read_emissions,
    render_emissions,
)


def _inventory(repository: Path, text: str) -> None:
    path = repository / INVENTORY
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("target|address|symbol|name|type|source_files\n" + text)


def _entity(symbol: str):
    return SimpleNamespace(
        entity_type=EntityType.FUNCTION,
        fact=lambda side, field: symbol if side == ImageId.RECOMP and field == "symbol" else None,
    )


def _engine(monkeypatch, *, matches=None, slots=()):
    entities = {recomp: _entity(symbol) for _, recomp, symbol in slots}
    monkeypatch.setattr(
        "reccmp.compare.vtables.compare_vtable",
        lambda *_args: SimpleNamespace(
            slots=[SimpleNamespace(orig_raw=orig, recomp_raw=recomp) for orig, recomp, _ in slots]
        ),
    )
    return SimpleNamespace(
        db=None,
        orig_bin=None,
        recomp_bin=None,
        get_vtables=lambda: [None] if slots else [],
        get_match=(matches or {}).get,
        get=lambda side, address: entities.get(address),
    )


def test_generate_preserves_reccmp_identity_without_cpp_or_pdb(tmp_path: Path) -> None:
    _inventory(
        tmp_path,
        "WIZ8|004218b0|??_GW8IntervalGate@@UAEPAXI@Z||synthetic|include/wiz8/gate.h\n"
        "SURRENDER|10001000||Vector<int>::Grow|template|include/surrender/vector.h\n",
    )
    result = generate_emissions(tmp_path)
    assert result["emissions"] == 2
    rows = list(csv_parse((tmp_path / OUTPUT / "wiz8-emissions.csv").read_text()))
    assert rows == [
        (0x4218B0, {"symbol": "??_GW8IntervalGate@@UAEPAXI@Z", "type": EntityType.FUNCTION})
    ]
    sr = list(csv_parse((tmp_path / OUTPUT / "surrender-emissions.csv").read_text()))
    assert sr == [(0x10001000, {"name": "Vector<int>::Grow", "type": EntityType.FUNCTION})]
    assert not (tmp_path / "src").exists()
    before = (tmp_path / OUTPUT / "wiz8-emissions.csv").read_bytes()
    generate_emissions(tmp_path)
    assert (tmp_path / OUTPUT / "wiz8-emissions.csv").read_bytes() == before


def test_override_is_explicit_and_retains_file_selection_metadata(tmp_path: Path) -> None:
    _inventory(tmp_path, "WIZ8|00401000||Unknown::~Unknown|synthetic|src/wiz8/owner.cpp\n")
    path = tmp_path / OVERRIDES
    path.parent.mkdir(parents=True)
    path.write_text(
        "target|address|symbol|name|type\nWIZ8|00401000|??_GKnown@@UAEPAXI@Z||synthetic\n"
    )
    [row] = emission_inventory(tmp_path, "WIZ8")
    assert row.symbol == "??_GKnown@@UAEPAXI@Z"
    assert row.source_files == ("src/wiz8/owner.cpp",)


@pytest.mark.parametrize("identity", ["||synthetic", "|Name|function"])
def test_invalid_inventory_is_rejected(identity: str) -> None:
    with pytest.raises(ValueError):
        read_emissions("target|address|symbol|name|type\nWIZ8|00401000|" + identity + "\n")


def test_duplicate_address_is_rejected_even_when_the_name_agrees() -> None:
    with pytest.raises(ValueError, match="duplicate"):
        read_emissions(
            "target|address|symbol|name|type\n" + "WIZ8|00401000||Grow<int>|template\n" * 2
        )


def test_existing_catalog_pair_supplies_exact_pdb_symbol(monkeypatch) -> None:
    symbol = "?Grow@?$Vector@H@@QAEHH@Z"
    row = Emission("WIZ8", 0x401000, "", "Vector<int>::Grow", "template")
    engine = _engine(monkeypatch, matches={row.address: _entity(symbol)})
    [derived] = derive_emissions([row], engine)
    assert derived.recomp_selector == symbol
    assert derived.selector_is_symbol
    assert derived.symbol == row.symbol
    assert derived.name == row.name


def test_vtable_association_uses_raw_deleting_destructor_symbol(monkeypatch) -> None:
    symbol = "??_GKnown@@UAEPAXI@Z"
    row = Emission("WIZ8", 0x401000, "", "Known::`scalar deleting destructor'", "synthetic")
    engine = _engine(monkeypatch, slots=[(row.address, 0x501000, symbol)])
    assert derive_emissions([row], engine)[0].recomp_selector == symbol


@pytest.mark.parametrize(
    "slots",
    [
        [(0x401000, 0x501000, "??_GA@@UAEPAXI@Z"), (0x401000, 0x501010, "??_GB@@UAEPAXI@Z")],
        [(0x401000, 0x501000, "??_GA@@UAEPAXI@Z"), (0x401010, 0x501000, "??_GA@@UAEPAXI@Z")],
        [(0x401000, 0x501000, "?Draw@A@@QAEXXZ")],
    ],
)
def test_ambiguous_folded_or_ordinary_slots_do_not_invent_identity(monkeypatch, slots) -> None:
    row = Emission("WIZ8", 0x401000, "", "Unknown::~Unknown", "synthetic")
    assert derive_emissions([row], _engine(monkeypatch, slots=slots)) == [row]


def test_explicit_symbol_cannot_be_silently_replaced(monkeypatch) -> None:
    row = Emission("WIZ8", 0x401000, "??_GA@@UAEPAXI@Z", "", "synthetic")
    engine = _engine(monkeypatch, matches={row.address: _entity("??_GB@@UAEPAXI@Z")})
    with pytest.raises(ValueError, match="contradicts explicit"):
        derive_emissions([row], engine)


def test_render_order_and_name_only_identity_are_stable() -> None:
    rows = [
        Emission("WIZ8", 0x401010, "", "B<int>::Grow", "template"),
        Emission("WIZ8", 0x401000, "", "A<int>::Grow", "template"),
    ]
    assert [address for address, _ in csv_parse(render_emissions(rows))] == [0x401000, 0x401010]


def test_display_and_recomp_selector_survive_csv_generation():
    row = Emission(
        "WIZ8",
        0x401000,
        "",
        "srPtr<T>::retained emission",
        "template",
        recomp_selector="??_GKnown@@UAEPAXI@Z",
        selector_is_symbol=True,
    )
    [(address, values)] = csv_parse(render_emissions([row]))
    assert address == row.address
    assert values["name"] == row.name
    assert values["recomp_selector"] == row.recomp_selector
    assert values["selector_is_symbol"] is True
    assert "symbol" not in values


def test_failed_derivation_preserves_last_successful_metadata(tmp_path, monkeypatch):
    _inventory(tmp_path, "WIZ8|00401000||Grow<int>|template|src/wiz8/owner.cpp\n")
    generate_emissions(tmp_path)
    path = tmp_path / OUTPUT / "wiz8-emissions.csv"
    previous = path.read_bytes()

    def fail(*_args, **_kwargs):
        raise RuntimeError("missing rebuilt PDB")

    monkeypatch.setattr("wiz8decomp.comparison.comparison_target", fail)
    with pytest.raises(RuntimeError, match="missing rebuilt PDB"):
        generate_emissions(tmp_path, derive=True)
    assert path.read_bytes() == previous


def test_scoped_generation_preserves_other_target_output(tmp_path):
    _inventory(tmp_path, "WIZ8|00401000||Grow<int>|template|src/wiz8/owner.cpp\n")
    generate_emissions(tmp_path)
    path = tmp_path / OUTPUT / "surrender-emissions.csv"
    path.write_text("previous validated selectors")
    generate_emissions(tmp_path, targets=("WIZ8",))
    assert path.read_text() == "previous validated selectors"


def test_automatic_bootstrap_preserves_explicit_enrichment(tmp_path, monkeypatch):
    from dataclasses import replace

    from reccmp.compare import Compare

    _inventory(tmp_path, "WIZ8|00401000||Known::~Known|synthetic|src/wiz8/owner.cpp\n")
    generate_emissions(tmp_path)
    output = tmp_path / OUTPUT / "wiz8-emissions.csv"
    # Use a real dataclass target substitute so staged paths exercise replacement.
    from dataclasses import make_dataclass

    target = make_dataclass("Target", [("data_sources", list)])([output])
    monkeypatch.setattr("wiz8decomp.comparison.comparison_target", lambda *_a, **_kw: target)
    monkeypatch.setattr(Compare, "from_target", lambda *_: object())
    monkeypatch.setattr(
        "wiz8decomp.emissions.derive_emissions",
        lambda rows, _engine: [
            replace(row, recomp_selector="??_GKnown@@UAEPAXI@Z", selector_is_symbol=True)
            for row in rows
        ],
    )
    generate_emissions(tmp_path, derive=True, targets=("WIZ8",))
    enriched = output.read_bytes()
    generate_emissions(tmp_path)
    assert output.read_bytes() == enriched
    # Inventory changes invalidate the old enrichment and refresh the baseline.
    _inventory(tmp_path, "WIZ8|00402000||Other::~Other|synthetic|src/wiz8/owner.cpp\n")
    generate_emissions(tmp_path)
    assert "00402000" in output.read_text()
    assert "??_GKnown" not in output.read_text()


@pytest.mark.parametrize("missing", [True, False])
def test_bootstrap_repairs_missing_or_corrupt_output(tmp_path, missing):
    _inventory(tmp_path, "WIZ8|00401000||Grow<int>|template|src/wiz8/owner.cpp\n")
    generate_emissions(tmp_path)
    path = tmp_path / OUTPUT / "wiz8-emissions.csv"
    baseline = path.read_bytes()
    if missing:
        path.unlink()
    else:
        path.write_text("corrupt generated metadata")
    generate_emissions(tmp_path)
    assert path.read_bytes() == baseline


def test_compiler_data_emissions_keep_data_kind_and_exact_symbol(monkeypatch):
    symbol = "??_8Stream@@7B@"
    [row] = read_emissions(
        "target|address|symbol|name|type\nSURRENDER|10076000|" + symbol + "|Stream vbtable|global\n"
    )
    [(address, facts)] = csv_parse(render_emissions([row]))
    assert address == row.address
    assert facts["type"] == EntityType.DATA
    assert facts["symbol"] == symbol
    entity = SimpleNamespace(entity_type=EntityType.DATA, fact=lambda *_: symbol)
    [derived] = derive_emissions([row], _engine(monkeypatch, matches={row.address: entity}))
    assert derived.recomp_selector == symbol
    assert derived.selector_is_symbol
