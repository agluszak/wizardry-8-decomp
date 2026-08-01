from pathlib import Path

import pytest
from wiz8decomp.source_policy import SourcePolicyError, validate_source_policy


def _write_source(repository: Path, source: str) -> None:
    path = repository / "src/wiz8/example.cpp"
    path.parent.mkdir(parents=True)
    path.write_text(source, encoding="utf-8")


@pytest.mark.parametrize(
    "declaration",
    (
        "void Example::InstallVtable() {}",
        "void Example::install_vtable() {}",
        "void Example_ScalarDeletingDestructor(Example*) {}",
        "void scalar_deleting_destructor(Example*) {}",
        "void vector_deleting_destructor(Example*) {}",
        "void* g_vtable_005ecdb0;",
    ),
)
def test_source_policy_rejects_manual_compiler_artifacts(tmp_path: Path, declaration: str) -> None:
    _write_source(tmp_path, declaration)

    with pytest.raises(SourcePolicyError):
        validate_source_policy(tmp_path)


def test_source_policy_allows_compiler_generated_wrapper_evidence(tmp_path: Path) -> None:
    _write_source(
        tmp_path,
        """class Example {
public:
    virtual ~Example();
};

Example::~Example() {}

// SYNTHETIC: WIZ8 0x00401020
// Example::`scalar deleting destructor'
""",
    )

    assert validate_source_policy(tmp_path)["ok"] is True


def test_source_policy_requires_synthetic_marker_for_scalar_wrapper(
    tmp_path: Path,
) -> None:
    _write_source(
        tmp_path,
        """// FUNCTION: WIZ8 0x00401020
// Example::`scalar deleting destructor'
""",
    )

    with pytest.raises(SourcePolicyError, match="SYNTHETIC"):
        validate_source_policy(tmp_path)


def test_source_policy_requires_synthetic_marker_for_vector_wrapper(
    tmp_path: Path,
) -> None:
    _write_source(
        tmp_path,
        """// FUNCTION: WIZ8 0x00401030
// Example::`vector deleting destructor'`adjustor{4}'
""",
    )

    with pytest.raises(SourcePolicyError, match="SYNTHETIC"):
        validate_source_policy(tmp_path)
