"""Exercise the actual cross-TU collector during the plugin image build."""

import sys
from pathlib import Path

from scalar_facts import integer_report, read_scalar_facts

facts = read_scalar_facts(Path(sys.argv[1]))
chain = {
    key
    for key, declaration in facts.declarations.items()
    if declaration.name in {"duration", "stored_duration", "ReadDuration", "value"}
    or declaration.name.startswith("StoreDuration::")
}
assert len(chain) == 5, chain
assert len([flow for flow in facts.flows if flow.source in chain and flow.target in chain]) == 4
field = next(
    key for key, declaration in facts.declarations.items() if declaration.name == "duration"
)
# Synthetic reviewed evidence tests propagation, without claiming that fixture
# instructions or source types establish a retail source-model fact.
seed = {
    "key": field,
    "property": "signedness",
    "value": "unsigned",
    "basis": {
        "kind": "retail",
        "reference": "synthetic fixture",
        "reason": "test unsigned propagation",
        "mnemonic": "jnc",
        "value_width": 32,
        "operand_width": 32,
    },
    "covered_uses": [
        {"file": use.file, "line": use.line, "operation": use.detail}
        for use in facts.operations
        if use.key == field
    ],
}
report = integer_report(facts, [seed])
component = next(item for item in report["components"] if field in item["members"])
property_ = component["properties"]["signedness"]
assert property_["status"] == "candidate", property_
assert set(property_["changes"]) == chain, property_
assert len(report["predicate32_inventory"]) == 1
enum_key = next(
    key for key, declaration in facts.declarations.items() if declaration.name == "ModeReady"
)
enum_seed = {
    "key": enum_key,
    "property": "domain",
    "value": "enum:W8FixtureMode",
    "basis": {
        "kind": "source-oracle",
        "reference": "synthetic fixture",
        "reason": "test independently established enum identity",
    },
}
enum_report = integer_report(facts, [enum_seed])
enum_component = next(item for item in enum_report["components"] if enum_key in item["members"])
enum_property = enum_component["properties"]["domain"]
assert enum_property["status"] == "candidate", enum_property
assert {facts.declarations[key].name for key in enum_property["changes"]} == {
    "ReadMode",
    "mode",
    "g_fixture_mode",
}
assert not any(declaration.name == "member" for declaration in facts.declarations.values())
assert (
    next(
        declaration.domain
        for declaration in facts.declarations.values()
        if declaration.name == "known_member"
    )
    == "pointer"
)
assert any(
    facts.declarations[flow.target].name == "GetMethodDuration"
    and facts.declarations[flow.source].name == "method_duration"
    and flow.role == "return"
    for flow in facts.flows
)
assert next(
    declaration.byte_candidate
    for declaration in facts.declarations.values()
    if declaration.name == "ReadByteAlias"
)
print("scalar facts: cross-TU recovery and bool compatibility fixtures pass")
