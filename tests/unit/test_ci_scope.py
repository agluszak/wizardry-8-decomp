from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path


_SCRIPT = Path(__file__).resolve().parents[2] / ".github/scripts/classify-ci-changes.py"
_SPEC = spec_from_file_location("ci_scope", _SCRIPT)
assert _SPEC is not None and _SPEC.loader is not None
_MODULE = module_from_spec(_SPEC)
_SPEC.loader.exec_module(_MODULE)


def test_surrender_body_does_not_trigger_wiz8_runtime_lane() -> None:
    result = _MODULE.classify(["src/surrender/node.cpp"])
    assert result == {
        "public": True,
        "analysis": False,
        "wiz8": False,
        "surrender": True,
    }


def test_surrender_header_triggers_provider_and_consumer_lanes() -> None:
    result = _MODULE.classify(["include/surrender/srNode.h"])
    assert result["wiz8"] is True
    assert result["surrender"] is True


def test_wiz8_body_does_not_trigger_surrender_lane() -> None:
    result = _MODULE.classify(["src/wiz8/engine_code/Monster.cpp"])
    assert result["wiz8"] is True
    assert result["surrender"] is False


def test_shared_build_infrastructure_triggers_all_heavy_lanes() -> None:
    result = _MODULE.classify(["cmake/Reccmp.cmake"])
    assert result == {
        "public": True,
        "analysis": True,
        "wiz8": True,
        "surrender": True,
    }


def test_documentation_only_change_skips_heavy_ci() -> None:
    result = _MODULE.classify(["docs/ci.md", "README.md"])
    assert result == {
        "public": False,
        "analysis": False,
        "wiz8": False,
        "surrender": False,
    }


def test_dot_github_change_triggers_all_heavy_lanes() -> None:
    result = _MODULE.classify([".github/workflows/ci.yml"])
    assert result == {
        "public": True,
        "analysis": True,
        "wiz8": True,
        "surrender": True,
    }


def test_wiz8_header_triggers_wiz8_lane() -> None:
    result = _MODULE.classify(["include/wiz8/layouts/game_status.h"])
    assert result["wiz8"] is True
    assert result["surrender"] is False


def test_recovery_fixture_triggers_analysis_lane() -> None:
    result = _MODULE.classify(["tools/recovery-fixture/CMakeLists.txt"])
    assert result["analysis"] is True


def test_bink_header_triggers_wiz8_lane() -> None:
    result = _MODULE.classify(["include/bink.h"])
    assert result["wiz8"] is True
