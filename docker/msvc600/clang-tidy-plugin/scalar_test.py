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
assert {facts.declarations[row["key"]].name for row in report["predicate32_inventory"]} == {
    "IsIntegerPredicate",
    "operator int",
}
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

# A body for the same-name, same-arity int* overload must not resolve the
# escaped record whose distinct overload has no recovered body.
overload = next(key for key, row in facts.declarations.items() if row.name == "overload_ready")
assert overload in facts.bool_escaped

arrays = {row.name: key for key, row in facts.declarations.items() if row.name.endswith("_pending")}
ready = arrays["notices_pending"]
assert ready in facts.bool_writes and ready in facts.bool_supported
assert ready not in facts.bool_escaped and ready not in facts.bool_invalid
assert arrays["invalid_pending"] in facts.bool_invalid
assert arrays["partial_pending"] in facts.bool_escaped
assert arrays["escaped_pending"] in facts.bool_escaped

assert arrays["shifted_pending"] in facts.bool_escaped
assert arrays["numeric_pending"] in facts.bool_invalid

# Function decay must never connect a callback slot to the function's return
# domain. Actual calls still participate in the scalar copy graph.
callback_keys = {
    row.name: key
    for key, row in facts.declarations.items()
    if row.name in {"ReadCallbackValue", "decay_callback_pointer", "decay_callback_result"}
}
callback = callback_keys["decay_callback_pointer"]
result = callback_keys["decay_callback_result"]
reader = callback_keys["ReadCallbackValue"]
assert not any(flow.target == callback or flow.source == callback for flow in facts.flows)
assert any(flow.target == result and flow.source == reader for flow in facts.flows)
assert any(use.key == callback for use in facts.escapes)
assert any(use.key == reader for use in facts.escapes)
assert not any(row["target"] == callback for row in report["pointer_integer_transports"])
print("scalar facts: cross-TU recovery and bool compatibility fixtures pass")

# New clients consume the same collector facts rather than separate inventories.
by_name = {declaration.name: key for key, declaration in facts.declarations.items()}
domains = integer_report(facts, [])["domain_inventory"]
flags = next(item for item in domains if by_name["fixture_flags"] in item["members"])
status = next(item for item in domains if by_name["fixture_status"] in item["members"])
assert flags["behavior"] == "flags-like"
assert {item["value"] for item in flags["mask_operands"]} == {4, 8}
assert status["value_domain"] == "tri-state-values"
assert status["observed_values"] == [-1, 0, 1]
assert status["complete_value_domain"]

from scalar_facts import anchored_report, write_recovery_patch

pointer_key = by_name["fixture_owner"]
pointer_seed = {
    "key": pointer_key,
    "property": "pointee",
    "value": facts.types[pointer_key][2],
    "basis": {
        "kind": "source-oracle",
        "reference": "synthetic fixture",
        "reason": "test independently established pointer owner",
    },
}
pointer = next(
    item
    for item in anchored_report(facts, [pointer_seed], "pointee")
    if pointer_key in item["members"]
)
assert pointer["status"] == "candidate", pointer
assert pointer["changes"] == [by_name["fixture_storage"]]
mixed_key = by_name["fixture_mixed_owner"]
mixed_seed = {**pointer_seed, "key": mixed_key}
mixed = next(
    item for item in anchored_report(facts, [mixed_seed], "pointee") if mixed_key in item["members"]
)
assert mixed["status"] == "blocked"

nominal_key = by_name["fixture_monster_id"]
nominal_seed = {
    "key": nominal_key,
    "property": "nominal",
    "value": "FixtureMonsterId",
    "role": "ID",
    "basis": pointer_seed["basis"],
}
nominal = next(
    item
    for item in anchored_report(facts, [nominal_seed], "nominal")
    if nominal_key in item["members"]
)
assert nominal["status"] == "candidate", nominal
assert nominal["changes"] == [by_name["fixture_id_copy"]]

callback = next(
    row
    for row in integer_report(facts, [])["callbacks"]
    if row["slot"] == by_name["fixture_callback"]
)
assert callback["status"] == "modeled", callback
assert len(callback["nodes"]) == 2
assert any(
    flow.role == "callback-argument" and facts.declarations[flow.source].name == "callback_argument"
    for flow in facts.flows
)
assert any(
    flow.target == by_name["callback_result"] and flow.source.endswith("::callback-return")
    for flow in facts.flows
)
assert not any(
    flow.target == by_name["fixture_callback"] and flow.source == by_name["FixtureImplementation"]
    for flow in facts.flows
)

# Native spans include the canonical header and the out-of-line definition.
root = Path(__file__).resolve().parent
patch = Path(sys.argv[1]) / "recovery.patch"
result = write_recovery_patch(facts, [seed], root, patch)
assert set(result["changed_declarations"]) == chain, result
assert "unsigned int ReadDuration()" in patch.read_text()
print("shared constraints: flags, sentinels, pointers, IDs, callbacks and source patches pass")

for claims, expected in (
    ([pointer_seed], {by_name["fixture_storage"]}),
    ([nominal_seed], {by_name["fixture_id_copy"]}),
):
    result = write_recovery_patch(facts, claims, root, patch)
    assert set(result["changed_declarations"]) == expected, result
print("pointer and nominal proposals produce concrete source patches")

# An implicit object argument may expose inherited storage to unavailable code.
receiver_storage = by_name["receiver_storage"]
assert any(
    use.key == receiver_storage and use.detail == "aggregate storage argument"
    for use in facts.escapes
)
assert {Path(filename).name for filename in facts.translation_units} == {
    "scalar_test.cpp",
    "scalar_test_second.cpp",
}

from scalar_facts import structural_report

inventory = structural_report(facts)
arrays = {a["name"]: a for a in inventory["arrays"]}
assert arrays["fixture_array_values"]["extent"] == 3
assert arrays["fixture_text"]["text_initializer"]
element = arrays["fixture_callback_table"]["element_node"]
assert any(slot == element for slot, *_ in facts.callback_bindings)
assert any("callback" in slot and slot != element for slot, *_ in facts.callback_bindings)
assert any(
    r["name"] == "FixturePackedRecord" and r["packing_changes_size"] for r in inventory["records"]
)
print("array elements, callback tables, text initializers and packed record observations pass")

assert not any(
    flow.source.startswith("::callback-") or flow.target.startswith("::callback-")
    for flow in facts.flows
)

# Declarator components protect exact callback typedef atoms and array extents.
assert any(component == "callback-return" for _, component, *_ in facts.spans)
assert any(component == "callback-param#0" for _, component, *_ in facts.spans)
assert any(component == "array-extent" for _, component, *_ in facts.spans)
result = write_recovery_patch(facts, [enum_seed], root, patch)
assert {facts.declarations[key].name for key in result["changed_declarations"]} == {
    "ReadMode",
    "mode",
    "g_fixture_mode",
}, result
assert "W8FixtureMode ReadMode" in patch.read_text()
print("existing enum domains generate source patches; callback and extent components collected")

# A callback implementation, typedef return and typedef parameter form one
# source-complete constraint chain; signedness edits preserve long spelling.
callback_seed = {
    **seed,
    "key": by_name["FixturePatchImplementation"],
    "covered_uses": [],
}
result = write_recovery_patch(facts, [callback_seed], root, patch)
assert len(result["changed_declarations"]) == 4, result
assert "typedef unsigned long (*FixturePatchCallback)(unsigned long);" in patch.read_text()
assert "unsigned long FixturePatchImplementation(unsigned long value)" in patch.read_text()
print("callback signatures and all implementation redeclarations patch together")

# Export/source importers use the same native declaration identity as reccmp,
# including void-return functions which have no scalar return D node.
signatures = {row[0]: row for row in facts.signatures.values()}
assert signatures["ReadDuration"][1:4] == (0, 0, False)
assert signatures["StoreDuration"][1:4] == (1, 0, False)
assert signatures["StoreDuration"][4]

# Same physical template member location is not the same typed entity.
for member in ("storage_data", "storage_count"):
    keys = [key for key, row in facts.declarations.items() if row.name == member]
    assert len(keys) == 2, (member, keys)
    assert not set(keys).intersection(facts.inconsistent), (member, facts.inconsistent)
data = [key for key, row in facts.declarations.items() if row.name == "storage_data"]
assert {facts.types[key][0] for key in data} == {"int *", "unsigned long *"}
from scalar_facts import flow_components

assert not any(set(data).issubset(component) for component in flow_components(facts))
print("template specializations retain distinct, cross-TU-stable declaration identities")
