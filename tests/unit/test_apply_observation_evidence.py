from __future__ import annotations

from wiz8decomp.ghidra.apply_observation_evidence import merge_observation_comment


def test_observation_comment_is_idempotent_and_preserves_other_text() -> None:
    first = merge_observation_comment("reviewed note", "assertion", "A.cpp:10\nassert(value)")
    second = merge_observation_comment(
        first, "assertion", r"C:\Projects\Wizardry 8\A.cpp:11" + "\nassert(other)"
    )

    assert second.startswith("reviewed note")
    assert second.count("[wiz8 observation:assertion:begin]") == 1
    assert "A.cpp:10" not in second
    assert r"C:\Projects\Wizardry 8\A.cpp:11" in second
