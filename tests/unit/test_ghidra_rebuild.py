from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.ghidra import apply_wiz8_signature_fixes as signature_fixes
from wiz8decomp.ghidra import rebuild


def test_rebuild_runs_one_ordered_replay_and_writes_timing_report(
    tmp_path: Path, monkeypatch
) -> None:
    calls: list[str] = []
    settings = SimpleNamespace(repo_dir=tmp_path, build_dir=tmp_path / "build")
    function_map = tmp_path / "evidence/reviewed/wiz8/functions.csv"
    function_map.parent.mkdir(parents=True)
    function_map.write_text("unused", encoding="utf-8")

    monkeypatch.setattr(rebuild, "resolve_program_name", lambda _settings, selector: selector)
    monkeypatch.setattr(
        rebuild,
        "import_programs",
        lambda *_args, **_kwargs: calls.append("import"),
    )
    monkeypatch.setattr(
        rebuild,
        "apply_observation_evidence",
        lambda *_args, **_kwargs: calls.append("observations"),
    )
    monkeypatch.setattr(
        rebuild,
        "apply_function_map",
        lambda *_args, **_kwargs: calls.append("functions"),
    )
    monkeypatch.setattr(rebuild, "apply_zlib_model", lambda *_args, **_kwargs: calls.append("zlib"))
    monkeypatch.setattr(rebuild, "apply_sgp_model", lambda *_args, **_kwargs: calls.append("sgp"))
    monkeypatch.setattr(
        rebuild, "apply_wiz8_format_model", lambda *_args, **_kwargs: calls.append("formats")
    )
    monkeypatch.setattr(
        rebuild, "apply_wiz8_class_model", lambda *_args, **_kwargs: calls.append("classes")
    )
    monkeypatch.setattr(
        rebuild,
        "apply_wiz8_signature_fixes",
        lambda *_args, **_kwargs: calls.append("signatures"),
    )
    monkeypatch.setattr(
        rebuild,
        "apply_eh_frame_types",
        lambda *_args, **_kwargs: calls.append("eh-frame-types"),
    )
    monkeypatch.setattr(
        rebuild,
        "apply_class_candidates",
        lambda *_args, **_kwargs: calls.append("candidates"),
    )
    monkeypatch.setattr(
        rebuild,
        "apply_provenance",
        lambda *_args, **_kwargs: calls.append("provenance"),
    )
    monkeypatch.setattr(
        rebuild,
        "validate_reviewed_replay",
        lambda *_args, **_kwargs: calls.append("validate") or {"ok": True, "failure_count": 0},
    )

    report = rebuild.rebuild_program(settings, "canonical")

    assert calls == [
        "import",
        "functions",
        "zlib",
        "sgp",
        "formats",
        "classes",
        "signatures",
        "provenance",
        "observations",
        "eh-frame-types",
        "candidates",
        "validate",
    ]
    assert report["ok"] is True
    assert [phase["name"] for phase in report["phases"]] == [
        "fresh_import_and_auto_analysis",
        "reviewed_function_catalog",
        "zlib_model",
        "sgp_model",
        "wiz8_format_model",
        "reviewed_class_model",
        "reviewed_signatures",
        "reviewed_provenance",
        "canonical_neutral_observations",
        "eh_frame_types",
        "candidate_class_observations",
        "validation",
    ]
    assert (settings.build_dir / "reports/ghidra-replay/canonical.json").is_file()


def test_wiz8_signature_wrapper_preserves_replay_materialization_flag(monkeypatch) -> None:
    calls: list[bool] = []

    def record_apply(*_args, materialize: bool = True, **_kwargs):
        calls.append(materialize)
        return {}

    monkeypatch.setattr(signature_fixes, "apply_reviewed_signatures", record_apply)

    signature_fixes.apply_wiz8_signature_fixes(SimpleNamespace(), "canonical", materialize=False)

    assert calls == [False]
