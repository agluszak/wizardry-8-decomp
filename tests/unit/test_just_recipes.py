from pathlib import Path


def test_context_recipe_does_not_consume_options_as_a_program() -> None:
    justfile = Path("Justfile").read_text(encoding="utf-8")
    assert "context selector *args:" in justfile
    assert 'context address program="wiz8"' not in justfile
    assert "report context {{selector}} {{args}}" in justfile
    assert 'build target="match" *args:' not in justfile
