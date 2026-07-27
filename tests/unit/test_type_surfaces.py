"""Keep the two type surfaces from disagreeing again.

`config/types/wiz8/*.h` and `include/wiz8/gameplay_boundaries.h` are not rival
copies of one model. The first is the on-disk format catalogue, and most of what
it declares - the SLF directory, save-game records, waypoints - has no
counterpart in the compiled tree at all. The second is what the matching source
actually compiles against. They are complementary, and neither is redundant.

What they must not do is disagree where they overlap, which is exactly what
happened: both declared the 0x297 monster record, under different names, and the
applied-types copy modelled a five-byte attribute array as two separate scalars.
Nothing caught it, because nothing consumes `config/types` - no Python, no
Justfile target, no test. It is read by people.

So this is the consumer. It is deliberately narrow: sizes only, because a size
is the one fact both surfaces state unambiguously and the one whose disagreement
means a record has been repacked underneath somebody.
"""

from __future__ import annotations

import re
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[2]
SIZED_TYPE = re.compile(r"\}\s*(?P<name>\w+);\s*/\*\s*(?P<size>0x[0-9a-fA-F]+)")

# The same record is declared under two names: the applied-types catalogue calls
# it by its database role, the matching source by its recovered name. Recorded
# here rather than inferred, so that renaming either side has to state intent.
ALIASES = {"W8MonsterDatabaseRecord": "W8MonsterRecord"}


def sized_types(path: Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    return {
        match.group("name"): match.group("size").lower()
        for match in SIZED_TYPE.finditer(text)
    }


def source_types() -> dict[str, str]:
    """Every sized type the matching headers declare, wherever they declare it.

    Reading one header would tie this check to today's split. gameplay_boundaries.h
    is being broken up into per-unit headers, and a record moving between them is
    a refactor, not a reason for the surfaces to stop being compared.
    """

    found: dict[str, str] = {}
    for header in sorted((REPOSITORY / "include/wiz8").rglob("*.h")):
        found.update(sized_types(header))
    return found


def config_types() -> dict[str, tuple[str, str]]:
    found: dict[str, tuple[str, str]] = {}
    for header in sorted((REPOSITORY / "config/types/wiz8").glob("*.h")):
        for name, size in sized_types(header).items():
            found[name] = (size, header.name)
    return found


def test_both_surfaces_declare_sizes_this_can_read() -> None:
    # If either surface stops annotating sizes the way this reads them, the
    # comparison below silently compares nothing.
    assert len(config_types()) > 20
    assert len(source_types()) > 15


def test_a_type_declared_in_both_surfaces_has_one_size() -> None:
    config = config_types()
    source = source_types()
    shared = sorted(set(config) & set(source))
    assert shared, "the two surfaces no longer overlap; this check has gone blind"
    disagree = [
        f"{name}: {config[name][1]} says {config[name][0]}, "
        f"the matching headers say {source[name]}"
        for name in shared
        if config[name][0] != source[name]
    ]
    assert not disagree, "type surfaces disagree: " + "; ".join(disagree)


def test_the_monster_record_agrees_across_its_two_names() -> None:
    # The drift that motivated this file. Both names describe one 0x297 record
    # reached by seeking to 4 + index * 0x297, so a size split is a real defect
    # rather than a naming preference.
    config = config_types()
    source = source_types()
    for config_name, source_name in ALIASES.items():
        assert config_name in config, f"{config_name} is no longer declared"
        assert source_name in source, f"{source_name} is no longer declared"
        assert config[config_name][0] == source[source_name], (
            f"{config_name} and {source_name} are the same record but "
            f"{config[config_name][0]} != {source[source_name]}"
        )
