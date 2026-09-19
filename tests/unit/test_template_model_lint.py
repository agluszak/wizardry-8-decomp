from pathlib import Path

import pytest
from wiz8decomp.template_model_lint import TemplateModelError, validate_template_model


def _write(tmp_path: Path, relative: str, source: str) -> None:
    path = tmp_path / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(source, encoding="utf-8")


def test_rejects_concrete_member_specialization_from_template_emission(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "include/wiz8/vector.h",
        """
template <class T> class W8GrowableVector {};

template <>
inline W8GrowableVector<int>::W8GrowableVector(
    const W8GrowableVector<int>& other)
{
}
""",
    )

    with pytest.raises(TemplateModelError, match="explicit-specialization"):
        validate_template_model(tmp_path)


@pytest.mark.parametrize(
    "source",
    [
        "template <> void srFlags<int>::set(int bit, int on) {}\n",
        "template class W8GrowableVector<int>;\n",
        "extern template class W8GrowableVector<int>;\n",
        "template void W8GrowableVector<int>::Grow(int);\n",
    ],
)
def test_rejects_explicit_specialization_and_instantiation(
    tmp_path: Path, source: str
) -> None:
    _write(tmp_path, "src/wiz8/example.cpp", source)

    with pytest.raises(TemplateModelError):
        validate_template_model(tmp_path)


def test_primary_template_definition_is_allowed(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "include/wiz8/example.h",
        """
template <class T> class Example {
public:
    void set(T value);
};

template <class T> void Example<T>::set(T value)
{
    stored = value;
}
""",
    )

    assert validate_template_model(tmp_path)["ok"] is True


def test_comments_and_strings_do_not_trigger_the_gate(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/wiz8/example.cpp",
        """
// template <> void Fake<int>::f();
const char* text = "template class Fake<int>;";
""",
    )

    assert validate_template_model(tmp_path)["ok"] is True


def test_reviewed_srrender_lifecycle_helper_is_the_only_builtin_exception(
    tmp_path: Path,
) -> None:
    _write(
        tmp_path,
        "include/surrender/srTypeRegistry.h",
        """
template <bool Register> struct srInstanceLifecycle {};

template <>
struct srInstanceLifecycle<false> {
};
""",
    )

    assert validate_template_model(tmp_path)["ok"] is True

    _write(
        tmp_path,
        "include/surrender/other.h",
        """
template <class T> struct Other {};

template <>
struct Other<int> {
};
""",
    )
    with pytest.raises(TemplateModelError, match="include/surrender/other.h"):
        validate_template_model(tmp_path)
