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
        "surrender": True,
    }


def test_surrender_header_triggers_provider_and_wiz8_comparison() -> None:
    result = _MODULE.classify(["include/surrender/srNode.h"])
    assert result["wiz8_compare"] is True
    assert result["surrender"] is True


def test_wiz8_body_triggers_comparison_not_surrender() -> None:
    result = _MODULE.classify(["src/wiz8/engine_code/Monster.cpp"])
    assert result["wiz8_compare"] is True
    assert result["surrender"] is False


def test_runtime_harness_change_only_triggers_public_checks() -> None:
    result = _MODULE.classify(["tests/runtime/wiz8_runtime_test.cpp"])
    assert result == {
        "public": True,
        "analysis": False,
        "wiz8_compare": False,
        "surrender": False,
    }


def test_reccmp_config_change_triggers_comparison() -> None:
    result = _MODULE.classify(["config/reccmp/srext-unzip.csv"])
    assert result["wiz8_compare"] is True


def test_shared_build_infrastructure_triggers_all_heavy_lanes() -> None:
    result = _MODULE.classify(["cmake/Reccmp.cmake"])
    assert result == {
        "public": True,
        "analysis": True,
        "wiz8_compare": True,
        "surrender": True,
    }


def test_documentation_only_change_skips_heavy_ci() -> None:
    result = _MODULE.classify(["docs/ci.md", "README.md"])
    assert result == {
        "public": False,
        "analysis": False,
        "wiz8_compare": False,
        "surrender": False,
    }


def test_dot_github_change_triggers_all_heavy_lanes() -> None:
    result = _MODULE.classify([".github/workflows/ci.yml"])
    assert result == {
        "public": True,
        "analysis": True,
        "wiz8_compare": True,
        "surrender": True,
    }


def test_wiz8_header_triggers_comparison() -> None:
    result = _MODULE.classify(["include/wiz8/layouts/game_status.h"])
    assert result["wiz8_compare"] is True
    assert result["surrender"] is False


def test_bink_header_triggers_wiz8_comparison() -> None:
    result = _MODULE.classify(["include/bink.h"])
    assert result["wiz8_compare"] is True


def test_runtime_tooling_change_only_triggers_public_checks() -> None:
    result = _MODULE.classify(["tools/wiz8decomp/runtime.py"])
    assert result == {
        "public": True,
        "analysis": False,
        "wiz8_compare": False,
        "surrender": False,
    }


def test_comparison_tooling_change_triggers_comparison_only() -> None:
    result = _MODULE.classify(["tools/wiz8decomp/comparison.py"])
    assert result["wiz8_compare"] is True
    assert result["surrender"] is True
