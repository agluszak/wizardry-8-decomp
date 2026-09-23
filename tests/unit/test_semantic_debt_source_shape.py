from pathlib import Path

from wiz8decomp.reports.semantic_debt import _source_shaping_directives


def test_source_shaping_directives_are_target_scoped_and_ignore_plain_inline(
    tmp_path: Path,
) -> None:
    wiz8 = tmp_path / "src" / "wiz8"
    surrender = tmp_path / "src" / "surrender"
    wiz8.mkdir(parents=True)
    surrender.mkdir(parents=True)

    (wiz8 / "model.cpp").write_text(
        """
inline int ordinary() { return 1; }
__forceinline int forced() { return 2; }
__declspec(noinline) int blocked() { return 3; }
#pragma optimize("t", on)
int tuned() { return 4; }
""".lstrip(),
        encoding="utf-8",
    )
    (surrender / "renderer.cpp").write_text(
        "__forceinline int provider_only() { return 5; }\n",
        encoding="utf-8",
    )

    rows = _source_shaping_directives(tmp_path, "WIZ8")

    assert [row["kind"] for row in rows] == [
        "forceinline",
        "noinline",
        "optimizer_pragma",
    ]
    assert {row["source_file"] for row in rows} == {"src/wiz8/model.cpp"}
    assert all(row["status"] == "investigation_candidate" for row in rows)
