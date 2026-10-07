from pathlib import Path

import pytest
from reccmp.compare.csv import csv_parse
from reccmp.types import EntityType
from wiz8decomp.emissions import (
    INVENTORY,
    OUTPUT,
    OVERRIDES,
    Emission,
    emission_inventory,
    generate_emissions,
    read_emissions,
    render_emissions,
)


def _inventory(repository: Path, text: str) -> None:
    path = repository / INVENTORY
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("target|address|symbol|name|type|source_files\n" + text)


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
    before = (tmp_path / OUTPUT / "wiz8-emissions.csv").stat().st_mtime_ns
    generate_emissions(tmp_path)
    assert (tmp_path / OUTPUT / "wiz8-emissions.csv").stat().st_mtime_ns == before


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


def test_render_order_and_name_only_identity_are_stable() -> None:
    rows = [
        Emission("WIZ8", 0x401010, "", "B<int>::Grow", "template"),
        Emission("WIZ8", 0x401000, "", "A<int>::Grow", "template"),
    ]
    assert [address for address, _ in csv_parse(render_emissions(rows))] == [0x401000, 0x401010]


def test_display_and_recomp_selector_survive_csv_generation() -> None:
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


def test_scoped_generation_preserves_other_target_output(tmp_path: Path) -> None:
    _inventory(tmp_path, "WIZ8|00401000||Grow<int>|template|src/wiz8/owner.cpp\n")
    generate_emissions(tmp_path)
    path = tmp_path / OUTPUT / "surrender-emissions.csv"
    path.write_text("previous output")
    generate_emissions(tmp_path, targets=("WIZ8",))
    assert path.read_text() == "previous output"


@pytest.mark.parametrize("missing", [True, False])
def test_generation_repairs_missing_or_corrupt_output(tmp_path: Path, missing: bool) -> None:
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


def test_compiler_data_emissions_keep_data_kind_and_exact_symbol() -> None:
    symbol = "??_8Stream@@7B@"
    [row] = read_emissions(
        "target|address|symbol|name|type\nSURRENDER|10076000|" + symbol + "|Stream vbtable|global\n"
    )
    [(address, facts)] = csv_parse(render_emissions([row]))
    assert address == row.address
    assert facts["type"] == EntityType.DATA
    assert facts["symbol"] == symbol
