"""Read source-owned bodies and signatures from the rebuilt VC6 debug data."""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .reconstructed_pdb import Procedure, ProgramDatabase, Signature, TypeStream, load, load_object

GENERATED_MEMBERS = {
    "`scalar deleting destructor'": "scalar_deleting_destructor",
    "`vector deleting destructor'": "vector_deleting_destructor",
}
_TEMPLATE_SPACING = re.compile(r"\s+(?=[*&>])")


@dataclass(frozen=True)
class Body:
    name: str
    length: int
    object_file: str
    signature: Signature | None
    frame_variables: tuple[tuple[str, int, str], ...]

    def shape(self) -> tuple[Any, ...]:
        signature = self.signature
        return (
            self.length,
            signature.spelling(self.name) if signature else "",
            self.frame_variables,
        )


def reviewed_spelling(name: str) -> str:
    """Normalize only VC6's quoted generated-member spelling."""

    for quoted, plain in GENERATED_MEMBERS.items():
        name = name.replace(quoted, plain)
    return _TEMPLATE_SPACING.sub("", name)


def _body(procedure: Procedure, types: TypeStream, object_file: str) -> Body:
    return Body(
        name=reviewed_spelling(procedure.name),
        length=procedure.length,
        object_file=object_file,
        signature=types.signature(procedure.type_index),
        frame_variables=tuple(
            (variable.name, variable.frame_offset, types.name(variable.type_index))
            for variable in procedure.frame_variables
        ),
    )


def bodies_from_pdb(pdb: Path) -> list[Body]:
    database: ProgramDatabase = load(pdb)
    return [_body(procedure, database.types, procedure.module) for procedure in database.procedures]


def bodies_from_objects(build_dir: Path) -> list[Body]:
    bodies: list[Body] = []
    for path in sorted(build_dir.rglob("*.obj")):
        info = load_object(path, name=str(path.relative_to(build_dir)))
        bodies.extend(
            _body(procedure, info.types, info.object_file) for procedure in info.procedures
        )
    return bodies


def index_bodies(bodies: list[Body]) -> tuple[dict[str, Body], dict[str, list[str]]]:
    """Index unique compiler bodies; retain collisions as explicit ambiguity."""

    grouped: dict[str, list[Body]] = {}
    for body in bodies:
        grouped.setdefault(body.name, []).append(body)
    unique: dict[str, Body] = {}
    ambiguous: dict[str, list[str]] = {}
    for name, group in grouped.items():
        if len({body.shape() for body in group}) == 1:
            unique[name] = group[0]
        else:
            ambiguous[name] = sorted(body.object_file for body in group)
    return unique, ambiguous
