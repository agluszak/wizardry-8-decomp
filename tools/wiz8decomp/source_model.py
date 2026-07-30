"""Compatibility view over reccmp's generated compiler-backed source index."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any


class SourceModelError(ValueError):
    """The generated source index is absent or internally inconsistent."""


@dataclass(frozen=True)
class SourceFunction:
    address: int
    kind: str
    file: str
    line: int
    name: str
    semantic_id: str | None
    semantic_kind: str | None
    calling_convention: str | None
    return_type: str
    parameter_types: tuple[str, ...]
    owning_class: str | None
    is_virtual: bool

    @property
    def prototype(self) -> str:
        """Render compiler-owned types for reports, never for ABI synchronization."""

        if self.semantic_id is None:
            return ""
        parameters = ", ".join(self.parameter_types) or "void"
        prefix = f"{self.return_type} " if self.return_type else ""
        return f"{prefix}{self.name}({parameters})"


@dataclass(frozen=True)
class SourceModel:
    functions: dict[int, SourceFunction]


def load_source_index(repository: Path) -> dict[str, Any]:
    path = repository / "build/source-index.json"
    if not path.is_file():
        raise SourceModelError(
            f"{path} is missing; run `just lint` then `wiz8 analyze source-index`"
        )
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("schema") != "reccmp-source-index-v1":
        raise SourceModelError(f"{path} has an unsupported source-index schema")
    return document


def target_for_program(program_name: str) -> str:
    """Return the source marker target owned by a configured Ghidra program."""

    return "SURRENDER" if "--sr--" in program_name else "WIZ8"


def build_source_model(repository: Path, target: str = "WIZ8") -> SourceModel:
    source_stem = {"WIZ8": "wiz8", "SURRENDER": "surrender"}.get(target.upper())
    if source_stem is None:
        raise SourceModelError(f"unsupported source-model target: {target}")
    source_prefixes = (f"src/{source_stem}/", f"include/{source_stem}/")
    functions: dict[int, SourceFunction] = {}
    folded: dict[int, bool] = {}
    for marker in load_source_index(repository)["markers"]:
        if not marker["source_file"].startswith(source_prefixes):
            continue
        declaration = marker.get("declaration")
        name = declaration["qualified_name"] if declaration else marker.get("marker_name")
        if not name:
            raise SourceModelError(
                f"{marker['source_file']}:{marker['line']}: marker has no semantic identity"
            )
        address = int(marker["address"])
        if address in functions:
            current_folded = bool(marker.get("folded"))
            previous_folded = folded[address]
            if current_folded and not previous_folded:
                continue
            if not current_folded and previous_folded:
                pass
            else:
                raise SourceModelError(
                    f"{target.upper()} 0x{address:08x} has more than one source owner"
                )
        functions[address] = SourceFunction(
            address=address,
            kind=marker["marker_kind"],
            file=marker["source_file"],
            line=int(marker["line"]),
            name=name,
            semantic_id=declaration.get("semantic_id") if declaration else None,
            semantic_kind=declaration.get("semantic_kind") if declaration else None,
            calling_convention=declaration.get("calling_convention") if declaration else None,
            return_type=declaration.get("return_type", "") if declaration else "",
            parameter_types=tuple(declaration.get("parameter_types", ())) if declaration else (),
            owning_class=declaration.get("owning_class") if declaration else None,
            is_virtual=bool(declaration.get("is_virtual")) if declaration else False,
        )
        folded[address] = bool(marker.get("folded"))
    return SourceModel(functions=dict(sorted(functions.items())))


def validate_source_index(repository: Path) -> dict[str, int]:
    """Return compiler-backed ownership counts after loading all marker bindings."""

    document = load_source_index(repository)
    models = {target: build_source_model(repository, target) for target in ("WIZ8", "SURRENDER")}
    classes = document["classes"]
    class_ids = [item["semantic_id"] for item in classes]
    if len(class_ids) != len(set(class_ids)):
        raise SourceModelError("compiler-backed source index contains duplicate class definitions")
    return {
        "functions": sum(len(model.functions) for model in models.values()),
        "wiz8_functions": len(models["WIZ8"].functions),
        "surrender_functions": len(models["SURRENDER"].functions),
        "classes": len(classes),
        "vtable_classes": sum(item.get("vtable_address") is not None for item in classes),
    }
