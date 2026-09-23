from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path

_SCRIPT = Path(__file__).resolve().parents[2] / ".github/scripts/classify-ci-changes.py"
_SPEC = spec_from_file_location("ci_scope", _SCRIPT)
assert _SPEC is not None and _SPEC.loader is not None
_MODULE = module_from_spec(_SPEC)
_SPEC.loader.exec_module(_MODULE)


def test_surrender_body_does_not_trigger_wiz8_lanes() -> None:
    result = _MODULE.classify(["src/surrender/node.cpp"])
    assert result == {
        "public": True,
        "analysis": False,
        "wiz8_compare": False,
        "wiz8_runtime": False,
        "surrender": True,
    }


def test_surrender_header_triggers_provider_and_wiz8_lanes() -> None:
    result = _MODULE.classify(["include/surrender/srNode.h"])
    assert result["wiz8_compare"] is True
    assert result["wiz8_runtime"] is True
    assert result["surrender"] is True


def test_wiz8_body_triggers_comparison_and_runtime_not_surrender() -> None:
    result = _MODULE.classify(["src/wiz8/engine_code/Monster.cpp"])
    assert result["wiz8_compare"] is True
    assert result["wiz8_runtime"] is True
    assert result["surrender"] is False


def test_runtime_harness_change_does_not_trigger_comparison() -> None:
    result = _MODULE.classify(["tests/runtime/wiz8_runtime_test.cpp"])
    assert result["wiz8_runtime"] is True
    assert result["wiz8_compare"] is False
    assert result["surrender"] is False


def test_reccmp_config_change_does_not_trigger_runtime() -> None:
    result = _MODULE.classify(["config/reccmp/wiz8-equivalence-groups.txt"])
    assert result["wiz8_compare"] is True
    assert result["wiz8_runtime"] is False


def test_shared_build_infrastructure_triggers_all_heavy_lanes() -> None:
    result = _MODULE.classify(["cmake/Reccmp.cmake"])
    assert result == {
        "public": True,
        "analysis": True,
        "wiz8_compare": True,
        "wiz8_runtime": True,
        "surrender": True,
    }


def test_documentation_only_change_skips_heavy_ci() -> None:
    result = _MODULE.classify(["docs/ci.md", "README.md"])
    assert result == {
        "public": False,
        "analysis": False,
        "wiz8_compare": False,
        "wiz8_runtime": False,
        "surrender": False,
    }


def test_dot_github_change_triggers_all_heavy_lanes() -> None:
    result = _MODULE.classify([".github/workflows/ci.yml"])
    assert result == {
        "public": True,
        "analysis": True,
        "wiz8_compare": True,
        "wiz8_runtime": True,
        "surrender": True,
    }


def test_wiz8_header_triggers_both_wiz8_lanes() -> None:
    result = _MODULE.classify(["include/wiz8/layouts/game_status.h"])
    assert result["wiz8_compare"] is True
    assert result["wiz8_runtime"] is True
    assert result["surrender"] is False


def test_recovery_fixture_triggers_analysis_lane() -> None:
    result = _MODULE.classify(["tools/recovery-fixture/CMakeLists.txt"])
    assert result["analysis"] is True


def test_bink_header_triggers_both_wiz8_lanes() -> None:
    result = _MODULE.classify(["include/bink.h"])
    assert result["wiz8_compare"] is True
    assert result["wiz8_runtime"] is True


def test_runtime_tooling_change_is_runtime_only() -> None:
    result = _MODULE.classify(["tools/wiz8decomp/runtime.py"])
    assert result["wiz8_runtime"] is True
    assert result["wiz8_compare"] is False


def test_comparison_tooling_change_is_comparison_only() -> None:
    result = _MODULE.classify(["tools/wiz8decomp/comparison.py"])
    assert result["wiz8_compare"] is True
    assert result["wiz8_runtime"] is False


def test_aggregate_gate_requires_wiz8_runtime() -> None:
    workflow = (_SCRIPT.parent.parent / "workflows/ci.yml").read_text(encoding="utf-8")
    aggregate = workflow.split("\n  ci:\n", 1)[1].split("\n  comment-reccmp-status:\n", 1)[0]
    assert (
        "needs: [scope, toolchain, repository, analysis, wiz8, wiz8-runtime, surrender]"
        in aggregate
    )
    assert "WIZ8_RUNTIME_RESULT: ${{ needs['wiz8-runtime'].result }}" in aggregate


def test_reccmp_comment_combines_per_target_lane_status() -> None:
    workflow = (_SCRIPT.parent.parent / "workflows/ci.yml").read_text(encoding="utf-8")
    wiz8 = workflow.split("\n  wiz8:\n", 1)[1].split("\n  wiz8-runtime:\n", 1)[0]
    surrender = workflow.split("\n  surrender:\n", 1)[1].split("\n  ci:\n", 1)[0]
    comment = workflow.split("\n  comment-reccmp-status:\n", 1)[1]

    assert "reccmp-status: ${{ steps.wiz8-checks.outputs.status }}" in wiz8
    assert "reccmp-status: ${{ steps.surrender-status.outputs.status }}" in surrender
    assert "needs: [wiz8, surrender]" in comment
    assert "WIZ8_STATUS: ${{ needs.wiz8.outputs.reccmp-status }}" in comment
    assert "SURRENDER_STATUS: ${{ needs.surrender.outputs.reccmp-status }}" in comment
    assert '"#### Per target"' in comment
