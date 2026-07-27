import csv
from pathlib import Path

import pytest

from wiz8decomp.ghidra.apply_wiz8_signature_fixes import type_category_paths
from wiz8decomp.ghidra.reviewed_class_model import (
    load_reviewed_class_model,
    parse_pointee,
)
from wiz8decomp.ghidra.validate_replay import expected_pointee_display


def _write(path: Path, fieldnames: list[str], rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def _model_dir(tmp_path: Path, field_rows: list[dict[str, str]]) -> Path:
    directory = tmp_path / "evidence" / "reviewed" / "demo"
    _write(
        directory / "classes.csv",
        [
            "program",
            "class_name",
            "confidence",
            "primary_vtable_id",
            "minimum_size",
        ],
        [
            {
                "program": "demo",
                "class_name": "Node",
                "confidence": "exact",
                "primary_vtable_id": "",
                "minimum_size": "0x8",
            },
            {
                "program": "demo",
                "class_name": "Unsized",
                "confidence": "exact",
                "primary_vtable_id": "",
                "minimum_size": "",
            },
        ],
    )
    _write(
        directory / "fields.csv",
        [
            "program",
            "class_name",
            "offset",
            "size",
            "field_name",
            "data_type",
            "pointee",
            "confidence",
            "evidence_id",
            "description",
        ],
        field_rows,
    )
    _write(
        directory / "vtables.csv",
        [
            "program",
            "vtable_id",
            "class_name",
            "address",
            "subobject_offset",
            "kind",
            "slot_count",
            "confidence",
            "evidence_id",
        ],
        [],
    )
    _write(
        directory / "vtable-slots.csv",
        [
            "program",
            "vtable_id",
            "slot_index",
            "target",
            "slot_name",
            "confidence",
            "evidence_id",
        ],
        [],
    )
    return tmp_path


def _field_row(**overrides: str) -> dict[str, str]:
    row = {
        "program": "demo",
        "class_name": "Node",
        "offset": "0x0",
        "size": "0x4",
        "field_name": "next",
        "data_type": "pointer",
        "pointee": "Node",
        "confidence": "exact",
        "evidence_id": "classes:demo:Node",
        "description": "",
    }
    row.update(overrides)
    return row


def test_parse_pointee_splits_base_and_depth() -> None:
    assert parse_pointee("Node") == ("Node", 0)
    assert parse_pointee("Node *") == ("Node", 1)
    assert parse_pointee("Node**") == ("Node", 2)
    assert parse_pointee(" Node * * ") == ("Node", 2)


def test_expected_pointee_display_adds_the_field_pointer_level() -> None:
    assert expected_pointee_display("Node") == "Node *"
    assert expected_pointee_display("Node *") == "Node * *"


def test_pointee_accepts_self_reference(tmp_path: Path) -> None:
    repo = _model_dir(tmp_path, [_field_row()])
    model = load_reviewed_class_model(repo, "demo")
    assert model.fields[0].pointee == "Node"


def test_pointee_rejects_non_pointer_field(tmp_path: Path) -> None:
    repo = _model_dir(
        tmp_path, [_field_row(data_type="int32", pointee="Node")]
    )
    with pytest.raises(ValueError, match="pointee on non-pointer field"):
        load_reviewed_class_model(repo, "demo")


def test_pointee_rejects_unknown_class(tmp_path: Path) -> None:
    repo = _model_dir(tmp_path, [_field_row(pointee="Ghost")])
    with pytest.raises(ValueError, match="does not name a sized accepted class"):
        load_reviewed_class_model(repo, "demo")


def test_pointee_rejects_unsized_class(tmp_path: Path) -> None:
    repo = _model_dir(tmp_path, [_field_row(pointee="Unsized")])
    with pytest.raises(ValueError, match="does not name a sized accepted class"):
        load_reviewed_class_model(repo, "demo")


def test_signature_type_categories_reach_the_vendored_models() -> None:
    # Reviewed signatures may reference SGP and zlib types; the first-party
    # categories stay first so they shadow the vendored ones deterministically.
    assert type_category_paths("wiz8") == (
        "/wiz8/classes",
        "/wiz8/formats/slf",
        "/wiz8/sgp",
        "/wiz8/zlib_1_0_4",
        "/wiz8/srext_unzip",
    )
