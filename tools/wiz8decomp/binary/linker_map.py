"""The single owner of VC6 linker MAP parsing and address resolution.

A MAP file carries three related inventories: the public symbol table, the
section table, and the per-source line records. Runtime crash analysis and the
GDB debugger both need to turn an address into a symbol, its displacement and
its source line, so they share this module instead of carrying two parsers.

Resolution is section-aware when the MAP carries a section table (the real
build does) and falls back to symbol adjacency for the minimal fixtures some
tests use. Demangling is batched through `binary.demangle`; a name that is not
decorated, or that the demangler rejects, keeps its decorated spelling.
"""

from __future__ import annotations

import re
from bisect import bisect_right
from dataclasses import dataclass, replace
from pathlib import Path

from .demangle import DemanglerMissing, demangle

PUBLIC_RE = re.compile(
    r"^\s+(?P<segment>[0-9A-Fa-f]+):(?P<offset>[0-9A-Fa-f]+)\s+(?P<symbol>\S+)\s+"
    r"(?P<address>[0-9A-Fa-f]{8})(?:\s+(?P<flag>\w))?\s+(?P<object>.+?)\s*$"
)
SECTION_RE = re.compile(
    r"^\s+(?P<segment>[0-9a-fA-F]{4}):(?P<offset>[0-9a-fA-F]{8})\s+"
    r"(?P<length>[0-9a-fA-F]{8})H\s+\S+\s+\S+\s*$"
)
LINE_HEADER_RE = re.compile(r"^Line numbers for .*\((?P<source>.+)\) segment ")
LINE_RE = re.compile(r"(?P<line>[0-9]+)\s+(?P<segment>[0-9a-fA-F]{4}):(?P<offset>[0-9a-fA-F]{8})")
FRAME_RE = re.compile(r"^#(\d+)\s+(0x[0-9A-Fa-f]+)")

MAX_UNBOUNDED_FUNCTION_BYTES = 0x1000


@dataclass(frozen=True)
class MapSymbol:
    segment: int
    offset: int
    address: int
    decorated_name: str
    object_name: str
    is_function: bool


@dataclass(frozen=True)
class MapSection:
    segment: int
    start: int
    end: int


@dataclass(frozen=True)
class SourceLine:
    address: int
    segment: int
    source: str
    line: int


@dataclass(frozen=True)
class SymbolResolution:
    address: int
    symbol: MapSymbol | None
    confidence: str
    reason: str
    displacement: int = 0
    location: str = ""
    name: str | None = None

    @property
    def owner(self) -> str:
        return self.symbol.object_name if self.symbol is not None else ""

    def format(self) -> str:
        if self.symbol is None:
            return f"{self.address:08x}: <unresolved:{self.confidence}> {self.reason}"
        name = self.name or self.symbol.decorated_name
        return (
            f"{self.address:08x}: {name}+0x{self.displacement:x} "
            f"[{self.symbol.object_name}]{self.location}"
        )


def demangle_names(names: list[str]) -> dict[str, str]:
    """Map decorated names to signatures, tolerating a missing demangler."""
    unique = sorted({name for name in names if name.startswith("?")})
    if not unique:
        return {}
    try:
        return demangle(unique)
    except (DemanglerMissing, RuntimeError):
        return {}


class LinkerMap:
    """One parsed linker MAP: symbols, sections and source lines."""

    def __init__(
        self,
        symbols: list[MapSymbol],
        sections: list[MapSection],
        source_lines: list[SourceLine],
    ) -> None:
        self.symbols = sorted(symbols, key=lambda symbol: symbol.address)
        self.addresses = [symbol.address for symbol in self.symbols]
        self.sections = sections
        self.source_lines = sorted(source_lines, key=lambda line: line.address)
        self.segment_bases: dict[int, int] = {}
        for symbol in self.symbols:
            self.segment_bases.setdefault(symbol.segment, symbol.address - symbol.offset)
        self._sections_by_segment: dict[int, list[MapSection]] = {}
        for section in sections:
            self._sections_by_segment.setdefault(section.segment, []).append(section)

    @classmethod
    def read(cls, path: Path) -> LinkerMap:
        symbols: list[MapSymbol] = []
        sections: list[MapSection] = []
        source_lines: list[SourceLine] = []
        if not path.is_file():
            return cls(symbols, sections, source_lines)
        lines = path.read_text(encoding="cp1252", errors="replace").splitlines()
        for line in lines:
            if match := PUBLIC_RE.match(line):
                symbols.append(
                    MapSymbol(
                        segment=int(match.group("segment"), 16),
                        offset=int(match.group("offset"), 16),
                        address=int(match.group("address"), 16),
                        decorated_name=match.group("symbol"),
                        object_name=match.group("object").strip(),
                        is_function=match.group("flag") == "f",
                    )
                )
            elif match := SECTION_RE.match(line):
                start = int(match.group("offset"), 16)
                sections.append(
                    MapSection(
                        segment=int(match.group("segment"), 16),
                        start=start,
                        end=start + int(match.group("length"), 16),
                    )
                )
        segment_bases: dict[int, int] = {}
        for symbol in sorted(symbols, key=lambda item: item.address):
            segment_bases.setdefault(symbol.segment, symbol.address - symbol.offset)
        source: str | None = None
        for line in lines:
            if header := LINE_HEADER_RE.match(line):
                source = header.group("source").replace("Z:\\repo\\", "").replace("\\", "/")
                continue
            if source is None:
                continue
            for match in LINE_RE.finditer(line):
                segment = int(match.group("segment"), 16)
                base = segment_bases.get(segment)
                if base is not None:
                    source_lines.append(
                        SourceLine(
                            address=base + int(match.group("offset"), 16),
                            segment=segment,
                            source=source,
                            line=int(match.group("line")),
                        )
                    )
        return cls(symbols, sections, source_lines)

    def _section_containing(self, symbol: MapSymbol) -> MapSection | None:
        for section in self._sections_by_segment.get(symbol.segment, []):
            if section.start <= symbol.offset < section.end:
                return section
        return None

    def resolve(self, address: int) -> SymbolResolution:
        if not self.addresses or address < self.addresses[0]:
            return SymbolResolution(address, None, "unresolved", "before first public symbol")
        index = bisect_right(self.addresses, address) - 1
        symbol = self.symbols[index]
        if not symbol.is_function:
            return SymbolResolution(address, None, "unresolved", "nearest public is data")
        next_symbol = self.symbols[index + 1] if index + 1 < len(self.symbols) else None
        unbounded = next_symbol is None or next_symbol.segment != symbol.segment
        if (
            next_symbol is not None
            and next_symbol.segment == symbol.segment
            and address >= next_symbol.address
        ):
            return SymbolResolution(address, None, "unresolved", "address crosses symbol extent")
        if unbounded:
            section = self._section_containing(symbol)
            if section is not None:
                base = self.segment_bases.get(symbol.segment, symbol.address - symbol.offset)
                if address >= base + section.end:
                    return SymbolResolution(address, None, "unresolved", "address crosses section")
            else:
                displacement = address - symbol.address
                if displacement >= MAX_UNBOUNDED_FUNCTION_BYTES:
                    return SymbolResolution(
                        address,
                        None,
                        "low",
                        f"unbounded function displacement 0x{displacement:x}",
                    )
        return SymbolResolution(address, symbol, "high", "within function extent")

    def lookup(self, address: int) -> MapSymbol | None:
        resolution = self.resolve(address)
        return resolution.symbol if resolution.confidence == "high" else None

    def source_location(self, symbol: MapSymbol, address: int) -> str:
        eligible = [
            entry
            for entry in self.source_lines
            if entry.segment == symbol.segment and symbol.address <= entry.address <= address
        ]
        if not eligible:
            return ""
        entry = eligible[-1]
        return f" {entry.source}:{entry.line}"

    def resolve_many(self, addresses: list[int]) -> list[SymbolResolution | None]:
        """Resolve addresses with displacement, source line and demangled name."""
        resolutions = [self.resolve(address) for address in addresses]
        demangled = demangle_names(
            [resolution.symbol.decorated_name for resolution in resolutions if resolution.symbol]
        )
        results: list[SymbolResolution | None] = []
        for address, resolution in zip(addresses, resolutions):
            symbol = resolution.symbol
            if symbol is None:
                results.append(None)
                continue
            results.append(
                replace(
                    resolution,
                    displacement=address - symbol.address,
                    location=self.source_location(symbol, address),
                    name=demangled.get(symbol.decorated_name) or symbol.decorated_name,
                )
            )
        return results

    def find_decorated(self, decorated_name: str) -> MapSymbol | None:
        for symbol in self.symbols:
            if symbol.decorated_name == decorated_name:
                return symbol
        return None
