"""Guard the audited SurRender consumer import model.

`SR_DLL_IMPORT` changes MSVC code generation. It is not an ownership marker for
symbols that happen to live in SR.DLL, so import annotations must stay explicit,
reviewed ABI decisions rather than drift as recovery work moves around.
"""

from __future__ import annotations

import re
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[2]
SURRENDER_HEADERS = REPOSITORY / "include" / "surrender"

CLASS_IMPORT_RE = re.compile(r"\bclass\s+SR_DLL_IMPORT\s+([A-Za-z_]\w*)")
COMMENT_RE = re.compile(r"//[^\n]*|/\*.*?\*/", re.DOTALL)

# Audited class-wide imports. Removing one is just as ABI-significant as adding
# one: class dllimport changes implicit special members, vtable emission and call
# shape. Change this set only together with consumer/import or assembly evidence.
AUDITED_CLASS_IMPORTS = {
    "srBSplineFilter",
    "srBellFilter",
    "srBinStream",
    "srBoxFilter",
    "srCamera",
    "srClipPlane",
    "srFilter",
    "srGERD",
    "srMaterial",
    "srMaterialIFace",
    "srMeshModel",
    "srModel",
    "srModelInstance",
    "srModeler",
    "srScene",
    "srTexture",
    "srTextureIFace",
    "srTextureMap",
    "srTimer",
    "srTriangleFilter",
    "srVariableTimer",
}

# srBinIStream now uses member imports and inline lifecycle bodies. Only the
# outstanding srDebugVP recovery may still remove its blanket annotation;
# no new class may be added here merely to make the test pass.
TRANSITIONAL_CLASS_IMPORTS = {
    "srDebugVP",
}

# Provider exports alone are specifically not consumer-import evidence.
PROVIDER_ONLY_CLASSES = {
    "srExponentTable",
    "srFStreamOpener",
    "srTextureFile",
    "srTriangulator",
}

# These standalone provider headers have no known Wizardry/JPEG/ZIP import at
# all. Keep them completely free of the consumer import annotation.
PROVIDER_ONLY_HEADERS = {
    "srBinFStream.h",
    "srBinIAsyncStream.h",
    "srBounder.h",
    "srDebugDD.h",
    "srEnvironmentMapper.h",
    "srExponentTable.h",
    "srMemoryPool.h",
    "srMutex.h",
    "srThread.h",
    "srTextureFile.h",
    "srTriangulator.h",
    "srVideoManager.h",
}

# Mixed headers where only the listed declaration is a known consumer import.
# Count the macro as well as checking the spelling so unrelated imports cannot
# quietly accumulate beside the evidenced one.
AUDITED_MIXED_MEMBER_IMPORTS = {
    "srImporter.h": ("SR_DLL_IMPORT void exportSurface(",),
    "srPixelConvert.h": (
        "static SR_DLL_IMPORT void mapPixelFormat(e_surfaceType type, PixelFormat& format);",
    ),
}

# These accessors are proven header bodies in retail callers. SR.DLL may also
# export an out-of-line identity, but consumers read the field directly.
INLINE_CORE_ACCESSORS = {
    "getMaterial": "material_170",
    "getRegistry": "registry_",
    "getStatisticsManager": "statistics_manager_28",
    "getTimer": "timer_08",
}


def _code(path: Path) -> str:
    return COMMENT_RE.sub("", path.read_text(encoding="utf-8"))


def _class_imports() -> set[str]:
    imports: set[str] = set()
    for header in SURRENDER_HEADERS.glob("*.h"):
        imports.update(CLASS_IMPORT_RE.findall(_code(header)))
    return imports


def test_class_wide_surrender_imports_match_audited_surface() -> None:
    observed = _class_imports()

    provider_only = sorted(observed & PROVIDER_ONLY_CLASSES)
    missing = sorted(AUDITED_CLASS_IMPORTS - observed)
    unexpected = sorted(
        observed - AUDITED_CLASS_IMPORTS - TRANSITIONAL_CLASS_IMPORTS - PROVIDER_ONLY_CLASSES
    )

    errors = []
    if provider_only:
        errors.append("provider-only classes carry SR_DLL_IMPORT: " + ", ".join(provider_only))
    if missing:
        errors.append(
            "audited class-wide imports were removed without updating the ABI audit: "
            + ", ".join(missing)
        )
    if unexpected:
        errors.append(
            "new class-wide imports need consumer/codegen evidence: " + ", ".join(unexpected)
        )

    assert not errors, "\n".join(errors)


def test_provider_only_headers_have_no_consumer_import_annotations() -> None:
    offenders = []
    for filename in sorted(PROVIDER_ONLY_HEADERS):
        if "SR_DLL_IMPORT" in _code(SURRENDER_HEADERS / filename):
            offenders.append(filename)

    assert not offenders, (
        "provider-only headers carry consumer SR_DLL_IMPORT annotations: " + ", ".join(offenders)
    )


def test_mixed_headers_match_audited_member_import_surface() -> None:
    errors = []
    for filename, expected in AUDITED_MIXED_MEMBER_IMPORTS.items():
        text = _code(SURRENDER_HEADERS / filename)
        observed_count = text.count("SR_DLL_IMPORT")
        if observed_count != len(expected):
            errors.append(
                f"{filename}: {observed_count} SR_DLL_IMPORT uses, expected {len(expected)}"
            )
        for declaration in expected:
            if declaration not in text:
                errors.append(f"{filename}: missing audited import {declaration}")

    assert not errors, "\n".join(errors)


def test_fstream_opener_stays_provider_only() -> None:
    text = _code(SURRENDER_HEADERS / "srIStreamOpener.h")
    marker = "class srFStreamOpener"
    assert marker in text
    fstream_declaration = text.split(marker, 1)[1]
    assert "SR_DLL_IMPORT" not in fstream_declaration, (
        "srFStreamOpener has no known consumer imports; keep its declaration provider-only"
    )


IMPORTED_FUNCTION_RE = re.compile(
    r"^SR_DLL_IMPORT\s+[\w:<>,&*\s]+?\b(operator[^\s(]*|[A-Za-z_]\w*)\s*\(", re.MULTILINE
)
FRIEND_RE = re.compile(r"\bfriend\s+(SR_DLL_IMPORT\s+)?[\w:<>,&*\s]+?\b(operator[^\s(]*)\s*\(")


def test_repeated_declarations_do_not_contradict_import_decoration() -> None:
    """A redeclared import must keep SR_DLL_IMPORT on every declaration.

    MSVC reports the mismatch as C4273 and clang as -Winconsistent-dllimport;
    both assume dllexport on the undecorated repeat, silently flipping the
    provider build's linkage for that symbol. The practical check covers the
    observed shape: a namespace-scope SR_DLL_IMPORT free function redeclared
    as a class friend without the attribute.
    """
    errors = []
    for header in sorted(SURRENDER_HEADERS.glob("*.h")):
        text = _code(header)
        imported = {match.group(1) for match in IMPORTED_FUNCTION_RE.finditer(text)}
        for match in FRIEND_RE.finditer(text):
            if match.group(1) is None and match.group(2) in imported:
                errors.append(
                    f"{header.name}: friend redeclaration of {match.group(2)} "
                    "drops SR_DLL_IMPORT from an imported function"
                )

    assert not errors, "\n".join(errors)


def test_sr_core_proven_inline_accessors_stay_header_visible() -> None:
    header = _code(SURRENDER_HEADERS / "srCore.h")

    for method, member in INLINE_CORE_ACCESSORS.items():
        body = re.compile(
            rf"\b{method}\s*\(\s*\)\s*const\s*\{{\s*return\s+{member}\s*;\s*\}}",
            re.DOTALL,
        )
        imported = re.compile(rf"SR_DLL_IMPORT[^;{{}}]*\b{method}\s*\(")
        assert body.search(header), (
            f"srCore::{method} is a proven retail header accessor for {member}; "
            "do not turn it back into an out-of-line declaration"
        )
        assert not imported.search(header), (
            f"srCore::{method} must not be SR_DLL_IMPORT; retail callers read {member} directly"
        )
