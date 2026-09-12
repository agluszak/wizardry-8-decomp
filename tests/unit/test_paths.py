from __future__ import annotations

from pathlib import Path

from wiz8decomp.paths import compile_database_relative


def test_compile_database_relative_accepts_docker_and_local_prefixes(tmp_path: Path) -> None:
    repository = tmp_path / "checkout"
    source = repository / "src/wiz8/Combat.cpp"
    source.parent.mkdir(parents=True)
    source.write_text("", encoding="utf-8")

    assert compile_database_relative("/repo/src/wiz8/Combat.cpp", repository) == (
        "src/wiz8/Combat.cpp"
    )
    assert compile_database_relative(str(source), repository) == "src/wiz8/Combat.cpp"
    assert compile_database_relative("src/wiz8/Combat.cpp", repository) == "src/wiz8/Combat.cpp"
    assert compile_database_relative("/zlib/adler32.c", repository) is None
    assert compile_database_relative("/infozip/unzip.c", repository) is None
