from pathlib import Path

import pytest
from wiz8decomp.source_classes import parse_source_classes, validate_source_classes


def test_source_class_inventory_qualifies_nested_records(tmp_path: Path) -> None:
    source = tmp_path / "include/wiz8/model.h"
    source.parent.mkdir(parents=True)
    source.write_text(
        """
        template <class T> class Vector { virtual void clear(); };
        class SR_DLL_IMPORT Owner : public Vector<int> {
        public:
            class Entry { virtual void visit(); };
            virtual void update(int value);
        };
        void consume(class ParameterOnly* value) {}
        """,
        encoding="utf-8",
    )

    records = {item.qualified_name: item for item in parse_source_classes(tmp_path)}
    assert set(records) == {"Owner", "Owner::Entry", "Vector"}
    assert records["Owner"].bases == ("Vector",)
    assert records["Owner"].methods[0].name == "update"
    assert records["Owner"].methods[0].virtual is True


def test_source_class_inventory_distinguishes_template_specializations(tmp_path: Path) -> None:
    source = tmp_path / "include/surrender/math.h"
    source.parent.mkdir(parents=True)
    source.write_text(
        """
        template <class T> class Vector {};
        template <> class Vector<float> {};
        """,
        encoding="utf-8",
    )

    assert [item.qualified_name for item in parse_source_classes(tmp_path)] == [
        "Vector",
        "Vector<float>",
    ]


def test_source_class_validation_rejects_duplicate_definitions(tmp_path: Path) -> None:
    for relative in ("include/wiz8/model.h", "src/wiz8/model.cpp"):
        source = tmp_path / relative
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_text("class Duplicate {};\n", encoding="utf-8")

    with pytest.raises(ValueError, match="duplicate C\\+\\+ class definitions"):
        validate_source_classes(tmp_path)
