from pathlib import Path

import pytest
from wiz8decomp.linkage_lint import CLinkageGateError, validate_c_linkage


@pytest.mark.parametrize("header", ["sgp_bridge.h", "engine_code/GameData.h"])
def test_product_header_has_no_implicit_c_linkage_exception(tmp_path: Path, header: str) -> None:
    path = tmp_path / "include/wiz8" / header
    path.parent.mkdir(parents=True)
    path.write_text('extern "C" float MoveTimer(int action);\n')

    with pytest.raises(CLinkageGateError, match="without a C-LINKAGE marker"):
        validate_c_linkage(tmp_path)


def test_marked_external_abi_boundary_is_allowed(tmp_path: Path) -> None:
    path = tmp_path / "include/wiz8/callback.h"
    path.parent.mkdir(parents=True)
    path.write_text('extern "C" void Callback(); // C-LINKAGE: external C ABI callback\n')

    assert validate_c_linkage(tmp_path)["ok"]
