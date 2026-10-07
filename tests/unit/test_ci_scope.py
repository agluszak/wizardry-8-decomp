from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path

import pytest

_SCRIPT = Path(__file__).resolve().parents[2] / ".github/scripts/classify-ci-changes.py"
_SPEC = spec_from_file_location("ci_scope", _SCRIPT)
assert _SPEC is not None and _SPEC.loader is not None
_MODULE = module_from_spec(_SPEC)
_SPEC.loader.exec_module(_MODULE)


@pytest.mark.parametrize(
    "paths, public, analysis, wiz8_compare, surrender",
    [
        (["src/surrender/node.cpp"], True, False, False, True),
        (["include/surrender/srNode.h"], True, False, True, True),
        (["src/wiz8/engine_code/Monster.cpp"], True, False, True, False),
        (["include/wiz8/layouts/game_status.h"], True, False, True, False),
        (["include/bink.h"], True, False, True, False),
        (["config/reccmp/srext-unzip.csv"], True, False, True, False),
        (["tests/runtime/wiz8_runtime_test.cpp"], True, False, False, False),
        (["tools/wiz8decomp/runtime.py"], True, False, False, False),
        (["tools/wiz8decomp/comparison.py"], True, False, True, True),
        (["cmake/Reccmp.cmake"], True, True, True, True),
        ([".github/workflows/ci.yml"], True, True, True, True),
        (["docs/ci.md", "README.md"], False, False, False, False),
    ],
)
def test_classify(paths, public, analysis, wiz8_compare, surrender) -> None:
    assert _MODULE.classify(paths) == {
        "public": public,
        "analysis": analysis,
        "wiz8_compare": wiz8_compare,
        "surrender": surrender,
    }
