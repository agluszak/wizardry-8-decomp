from pathlib import Path

from wiz8decomp.extract import archives


def test_extract_inno_passes_repeated_include_filters(tmp_path: Path, monkeypatch) -> None:
    source = tmp_path / "setup.exe"
    source.write_bytes(b"installer")
    destination = tmp_path / "out"
    commands = []

    monkeypatch.setattr(archives.shutil, "which", lambda _name: "/usr/bin/innoextract")
    monkeypatch.setattr(
        archives,
        "run",
        lambda command, **_kwargs: commands.append(command) or object(),
    )

    archives.extract_inno(
        source,
        destination,
        log_path=tmp_path / "extract.json",
        includes=("Wiz8.exe", "sr.dll"),
    )

    assert commands == [
        [
            "innoextract",
            "--output-dir",
            destination,
            "--include",
            "Wiz8.exe",
            "--include",
            "sr.dll",
            source,
        ]
    ]
