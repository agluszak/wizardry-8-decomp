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
property_ = next(
    item for item in report["integer_components"]["signedness"] if field in item["members"]
)
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
enum_property = next(
    item for item in enum_report["integer_components"]["domain"] if enum_key in item["members"]
)
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

pointer_key = by_name["fixture_storage"]
pointer_seed = {
    "key": pointer_key,
    "property": "pointee",
    "value": facts.types[by_name["fixture_owner"]][2],
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
mixed_key = by_name["fixture_mixed_storage"]
mixed_seed = {**pointer_seed, "key": mixed_key}
mixed = next(
    item for item in anchored_report(facts, [mixed_seed], "pointee") if mixed_key in item["members"]
)
assert mixed["status"] == "blocked"

nominal_key = by_name["fixture_id_copy"]
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

# Character storage, extents and padding are recovered from the same facts.
from scalar_facts import array_report, padding_report

by_name = {declaration.name: key for key, declaration in facts.declarations.items()}
arrays = {row[4]: row for row in facts.arrays}
script = arrays["FixtureSaveRecord::script_name"]
roles = {(kind, role) for owner, kind, role, *_ in facts.character_uses if owner == script[0]}
assert ("char", "api:strcpy#0") in roles, roles
assert ("raw-byte", "api:memcpy#0") in roles, roles
assert ("bytes:64", "api:memcpy#0") in roles, roles
wide = arrays["fixture_wide_text"]
assert any(owner == wide[0] and kind == "wchar_t" for owner, kind, *_ in facts.character_uses), (
    facts.character_uses
)
retail = {
    "kind": "retail",
    "reference": "synthetic fixture",
    "reason": "ANSI consumer and 64-byte serialized extent",
}
script_claims = [
    {
        "key": script[8],
        "property": "character",
        "value": "char",
        "basis": {**retail, "value_width": 8},
    },
    {
        "key": script[8],
        "property": "extent",
        "value": 64,
        "basis": {**retail, "extent_kind": "serialized-extent"},
        "complete_storage_accesses": True,
    },
]
recovered = array_report(facts, script_claims)
character = next(item for item in recovered["character_components"] if script[8] in item["members"])
assert character["status"] == "candidate", character
assert recovered["extents"][0]["status"] == "candidate", recovered["extents"]
result = write_recovery_patch(facts, script_claims, root, patch)
assert "+    char script_name[64];" in patch.read_text(), patch.read_text()
# Without the extent the 16-bit element cannot become a byte element in place.
blocked = array_report(facts, script_claims[:1])["character_components"]
assert next(item for item in blocked if script[8] in item["members"])["status"] == "blocked"

table = arrays["fixture_table"]
table_uses = [use for use in facts.array_uses if use.key == table[0]]
assert {use.detail for use in table_uses} >= {"indexed", "index:24"}
shrink = {
    "key": table[8],
    "property": "extent",
    "value": 25,
    "basis": {
        "kind": "source-oracle",
        "reference": "synthetic fixture",
        "reason": "released table extent",
        "extent_kind": "declaration-contract",
    },
}
assert array_report(facts, [shrink])["extents"][0]["status"] == "blocked"
covered = {
    **shrink,
    "covered_uses": [
        {"file": use.file, "line": use.line, "operation": "indexed"}
        for use in table_uses
        if use.detail == "indexed"
    ],
}
assert array_report(facts, [covered])["extents"][0]["status"] == "candidate"
too_small = {**covered, "value": 24}
assert array_report(facts, [too_small])["extents"][0]["status"] == "blocked"
write_recovery_patch(facts, [covered], root, patch)
assert "+int fixture_table[25];" in patch.read_text(), patch.read_text()

padding = {row["name"]: row for row in padding_report(facts)}
assert padding["padding_1"]["status"] == "candidate", padding
assert "layout" in padding["pad_tail"]["reason"], padding
assert "aggregate" in padding["pad"]["reason"], padding
assert any(use.key.endswith(":field:padded_value") for use in facts.field_references)
result = write_recovery_patch(facts, [], root, patch, padding=True)
assert "-    char padding_1[3];" in patch.read_text(), patch.read_text()
assert "-    char pad_tail;" not in patch.read_text()

exported = by_name["FixtureExported"]
assert facts.linkage[exported] == ("external", "export")
export_seed = {**seed, "key": exported, "covered_uses": []}
report = integer_report(facts, [export_seed])
component = next(
    item for item in report["integer_components"]["signedness"] if exported in item["members"]
)
assert any("dll export" in blocker["reason"] for blocker in component["blockers"]), component
assert report["coverage"]["complete"] is False
print("character storage, extents, padding removal and DLL boundaries pass")
