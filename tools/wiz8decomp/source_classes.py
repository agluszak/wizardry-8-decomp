"""Generate and validate the C++ class-declaration model.

The source owns declaration semantics.  This module deliberately derives its
inventory from C++ rather than introducing another editable class catalogue.
It is a conservative structural parser: it recognizes record definitions,
their direct bases, and direct method declarations while ignoring comments,
strings, and method bodies.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from .markers import iter_source_files
from .source_model import build_source_model

_TOKEN = re.compile(r"[A-Za-z_]\w*|::|[^\s]")
_IDENTIFIER = re.compile(r"[A-Za-z_]\w*\Z")
_METHOD = re.compile(r"(~?[A-Za-z_]\w*)\s*\(")
_CLASS_KEYWORDS = frozenset(
    {
        "__declspec",
        "alignas",
        "final",
        "SR_DLL_IMPORT",
        "SR_DLL_EXPORT",
    }
)


@dataclass(frozen=True)
class SourceMethod:
    name: str
    virtual: bool


@dataclass(frozen=True)
class SourceClass:
    name: str
    qualified_name: str
    file: str
    line: int
    bases: tuple[str, ...]
    methods: tuple[SourceMethod, ...]


@dataclass(frozen=True)
class _Token:
    text: str
    offset: int


def _mask_non_code(text: str) -> str:
    """Replace comments and literals with spaces while preserving newlines."""

    output = list(text)
    index = 0
    state = "code"
    while index < len(text):
        current = text[index]
        following = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if current == "/" and following == "/":
                output[index] = output[index + 1] = " "
                index += 2
                state = "line-comment"
                continue
            if current == "/" and following == "*":
                output[index] = output[index + 1] = " "
                index += 2
                state = "block-comment"
                continue
            if current == '"':
                output[index] = " "
                state = "string"
            elif current == "'":
                output[index] = " "
                state = "character"
        elif state == "line-comment":
            if current == "\n":
                state = "code"
            else:
                output[index] = " "
        elif state == "block-comment":
            if current == "*" and following == "/":
                output[index] = output[index + 1] = " "
                index += 2
                state = "code"
                continue
            if current != "\n":
                output[index] = " "
        else:
            if current == "\\" and following:
                output[index] = " "
                if following != "\n":
                    output[index + 1] = " "
                index += 2
                continue
            if (state == "string" and current == '"') or (state == "character" and current == "'"):
                state = "code"
            if current != "\n":
                output[index] = " "
        index += 1
    return "".join(output)


def _record_name(tokens: list[_Token]) -> str:
    """Return the declared name from tokens between class/struct and ':'/'{}'."""

    first_angle = next((index for index, token in enumerate(tokens) if token.text == "<"), None)
    if first_angle is not None:
        prefix = [token.text for token in tokens[:first_angle] if _IDENTIFIER.fullmatch(token.text)]
        if prefix:
            depth = 0
            suffix: list[str] = []
            for token in tokens[first_angle:]:
                suffix.append(token.text)
                if token.text == "<":
                    depth += 1
                elif token.text == ">":
                    depth -= 1
                    if depth == 0:
                        break
            return prefix[-1] + "".join(suffix)

    depth = 0
    candidates: list[str] = []
    skip_attribute = False
    for token in tokens:
        value = token.text
        if value == "(" and (skip_attribute or depth):
            depth += 1
            continue
        if value == "(" and candidates and candidates[-1] in {"__declspec", "alignas"}:
            candidates.pop()
            skip_attribute = True
            depth = 1
            continue
        if value == ")" and depth:
            depth -= 1
            if depth == 0:
                skip_attribute = False
            continue
        if depth:
            continue
        if _IDENTIFIER.fullmatch(value) and value not in _CLASS_KEYWORDS:
            candidates.append(value)
    return candidates[-1] if candidates else ""


def _base_names(tokens: list[_Token]) -> tuple[str, ...]:
    names: list[str] = []
    current: list[str] = []
    angle_depth = 0
    for token in tokens:
        if token.text == "<":
            angle_depth += 1
        elif token.text == ">" and angle_depth:
            angle_depth -= 1
        if token.text == "," and angle_depth == 0:
            names.extend(_base_name(current))
            current = []
        else:
            current.append(token.text)
    names.extend(_base_name(current))
    return tuple(names)


def _base_name(tokens: list[str]) -> list[str]:
    ignored = {"public", "protected", "private", "virtual"}
    if "<" in tokens:
        angle = tokens.index("<")
        prefix = [
            value
            for value in tokens[:angle]
            if _IDENTIFIER.fullmatch(value) and value not in ignored
        ]
        return [prefix[-1]] if prefix else []
    identifiers = [
        value for value in tokens if _IDENTIFIER.fullmatch(value) and value not in ignored
    ]
    return [identifiers[-1]] if identifiers else []


def _methods(body: str) -> tuple[SourceMethod, ...]:
    methods: dict[str, bool] = {}
    segment: list[str] = []
    depth = 0
    for character in body:
        if character == "{" and depth == 0:
            head = "".join(segment)
            _add_method(methods, head)
            segment = []
            depth = 1
        elif character == "{" and depth:
            depth += 1
        elif character == "}" and depth:
            depth -= 1
        elif depth == 0 and character == ";":
            _add_method(methods, "".join(segment))
            segment = []
        elif depth == 0:
            segment.append(character)
    return tuple(SourceMethod(name, virtual) for name, virtual in sorted(methods.items()))


def _add_method(methods: dict[str, bool], declaration: str) -> None:
    match = _METHOD.search(declaration)
    if match is None:
        return
    name = match.group(1)
    is_virtual = bool(re.search(r"\bvirtual\b", declaration))
    methods[name] = methods.get(name, False) or is_virtual


def parse_source_classes(repository: Path) -> tuple[SourceClass, ...]:
    roots = [repository / "src/wiz8", repository / "include/wiz8", repository / "include/surrender"]
    records: list[SourceClass] = []
    for path in iter_source_files(roots):
        text = _mask_non_code(path.read_text(encoding="utf-8", errors="ignore"))
        tokens = [_Token(match.group(), match.start()) for match in _TOKEN.finditer(text)]
        template_parameters: set[int] = set()
        for token_index, item in enumerate(tokens[:-1]):
            if item.text != "template" or tokens[token_index + 1].text != "<":
                continue
            depth = 0
            for parameter_index in range(token_index + 1, len(tokens)):
                if tokens[parameter_index].text == "<":
                    depth += 1
                elif tokens[parameter_index].text == ">":
                    depth -= 1
                    if depth == 0:
                        template_parameters.update(range(token_index + 1, parameter_index + 1))
                        break
        brace_stack: list[str | None] = []
        pending: tuple[str, int, tuple[str, ...], int] | None = None
        index = 0
        paren_depth = 0
        while index < len(tokens):
            token = tokens[index]
            if (
                token.text in {"class", "struct"}
                and index not in template_parameters
                and paren_depth == 0
            ):
                end = index + 1
                paren_depth = angle_depth = 0
                colon = None
                while end < len(tokens):
                    value = tokens[end].text
                    if value == "(":
                        paren_depth += 1
                    elif value == ")" and paren_depth:
                        paren_depth -= 1
                    elif value == "<":
                        angle_depth += 1
                    elif value == ">" and angle_depth:
                        angle_depth -= 1
                    elif value == ":" and paren_depth == angle_depth == 0 and colon is None:
                        colon = end
                    elif value in {"{", ";"} and paren_depth == angle_depth == 0:
                        break
                    end += 1
                if end < len(tokens) and tokens[end].text == "{":
                    name_end = colon if colon is not None else end
                    name = _record_name(tokens[index + 1 : name_end])
                    if name:
                        bases = _base_names(tokens[colon + 1 : end]) if colon is not None else ()
                        pending = (name, token.offset, bases, end)
                        index = end
                        token = tokens[index]
            if token.text == "{":
                if pending is not None and pending[3] == index:
                    name, offset, bases, _ = pending
                    parents = [scope for scope in brace_stack if scope]
                    qualified = "::".join([*parents, name])
                    close = _matching_brace(tokens, index)
                    body_start = token.offset + 1
                    body_end = tokens[close].offset
                    records.append(
                        SourceClass(
                            name=name,
                            qualified_name=qualified,
                            file=path.relative_to(repository).as_posix(),
                            line=text.count("\n", 0, offset) + 1,
                            bases=bases,
                            methods=_methods(text[body_start:body_end]),
                        )
                    )
                    brace_stack.append(name)
                    pending = None
                else:
                    brace_stack.append(None)
            elif token.text == "}" and brace_stack:
                brace_stack.pop()
            if token.text == "(":
                paren_depth += 1
            elif token.text == ")" and paren_depth:
                paren_depth -= 1
            index += 1
    return tuple(sorted(records, key=lambda item: (item.qualified_name, item.file, item.line)))


def _matching_brace(tokens: list[_Token], opening: int) -> int:
    depth = 0
    for index in range(opening, len(tokens)):
        if tokens[index].text == "{":
            depth += 1
        elif tokens[index].text == "}":
            depth -= 1
            if depth == 0:
                return index
    raise ValueError("unterminated class definition")


def validate_source_classes(repository: Path) -> dict[str, int]:
    from .evidence.classes import load_reviewed_class_model

    classes = parse_source_classes(repository)
    definitions: dict[str, list[SourceClass]] = {}
    for source_class in classes:
        definitions.setdefault(source_class.qualified_name, []).append(source_class)
    duplicates = {name: items for name, items in definitions.items() if len(items) > 1}
    if duplicates:
        details = []
        for name, items in sorted(duplicates.items()):
            locations = ", ".join(f"{item.file}:{item.line}" for item in items)
            details.append(f"{name} ({locations})")
        raise ValueError("duplicate C++ class definitions: " + "; ".join(details))

    source_model = build_source_model(repository)
    by_name = {item.name: item for item in classes}
    nonvirtual: list[str] = []
    reviewed = load_reviewed_class_model(repository, "wiz8")
    slot_addresses = {slot.target for slot in reviewed.slots}
    for address, function in source_model.functions.items():
        if address not in slot_addresses or "::" not in function.name:
            continue
        class_name, method_name = function.name.rsplit("::", 1)
        if method_name == "scalar_deleting_destructor":
            continue
        source_class = by_name.get(class_name)
        if source_class is None:
            continue
        declaration = next(
            (item for item in source_class.methods if item.name == method_name), None
        )
        if declaration is not None and not declaration.virtual:
            nonvirtual.append(f"0x{address:08x} {function.name}")
    header_classes = sum(1 for item in classes if item.file.startswith("include/"))
    inheritance_edges = sum(len(item.bases) for item in classes)
    virtual_methods = sum(sum(method.virtual for method in item.methods) for item in classes)
    reviewed_names = {item.name for item in reviewed.classes}
    source_names = set(by_name)
    return {
        "classes": len(classes),
        "header_classes": header_classes,
        "inheritance_edges": inheritance_edges,
        "virtual_methods": virtual_methods,
        "reviewed_classes": len(reviewed_names),
        "reviewed_source_definitions": len(reviewed_names & source_names),
        "reviewed_nonvirtual_targets": len(nonvirtual),
    }
