from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.reports.semantic_debt import _field_flow_triage, _field_observations


def _use(
    *,
    target: str = "WIZ8",
    owner: str = "W8First",
    identity: str = "record:W8First::field@include/wiz8/model.h:12:0",
    name: str = "unknown_04",
    function: str = "?Read@W8First@@QAEHXZ",
) -> dict:
    return {
        "field_identity": identity,
        "owner_identity": f"c:@S@{owner}",
        "owner_status": "resolved",
        "owner": owner,
        "name": name,
        "declaration_file": "include/wiz8/model.h",
        "declaration_line": 12,
        "declared_type": "int",
        "offset_bits": 32,
        "extent_bits": 32,
        "offset_bytes": 4,
        "extent_bytes": 4,
        "function_id": function,
        "function_identity": function,
        "function": f"{owner}::Read",
        "use_file": "src/wiz8/model.cpp",
        "use_line": 30,
        "use_column": 5,
        "operations": ["read"],
        "array_indices": [],
        "conversions": [],
        "target": target,
    }


def _class(
    name: str,
    *,
    target: str = "WIZ8",
    size: int = 16,
    alignment: int = 4,
    bases: tuple[str, ...] = (),
    base_offsets: tuple[dict, ...] = (),
    layout_trusted: bool = True,
) -> dict:
    return {
        "semantic_id": f"record:{name}",
        "qualified_name": name,
        "target": target,
        "size": size,
        "alignment": alignment,
        "bases": list(bases),
        "base_offsets": list(base_offsets),
        "layout_trusted": layout_trusted,
    }


def test_same_bare_field_name_stays_separate_by_owner_and_target() -> None:
    index = {
        "member_uses": [
            _use(),
            _use(
                owner="W8Second",
                identity="record:W8Second::field@include/wiz8/other.h:12:0",
                function="?Read@W8Second@@QAEHXZ",
            ),
            _use(
                target="SURRENDER",
                owner="srRenderer",
                identity="record:srRenderer::field@include/surrender/renderer.h:12:0",
                function="?Read@srRenderer@@QAEHXZ",
            ),
        ],
        "classes": [
            _class("W8First"),
            _class("W8Second"),
            _class("srRenderer", target="SURRENDER"),
        ],
    }

    wiz8 = _field_observations(index, "WIZ8")
    surrender = _field_observations(index, "SURRENDER")

    assert len(wiz8) == 2
    assert {row["owner_identity"] for row in wiz8} == {
        "c:@S@W8First",
        "c:@S@W8Second",
    }
    assert {row["owner_semantic_id"] for row in wiz8} == {
        "record:W8First",
        "record:W8Second",
    }
    assert len(surrender) == 1
    assert surrender[0]["target"] == "SURRENDER"


def test_field_observations_are_compiler_rows_not_comment_name_hits() -> None:
    # A comment such as `// this->unknown_04` has no compiler MemberExpr row.
    assert _field_observations({"member_uses": []}, "WIZ8") == []


def test_missing_owner_identity_remains_explicit() -> None:
    use = _use()
    use["owner_identity"] = ""
    use["owner_status"] = "unknown"

    rows = _field_observations({"member_uses": [use]}, "WIZ8")

    assert len(rows) == 1
    assert rows[0]["owner_identity"] is None
    assert rows[0]["owner_status"] == "unknown"


def test_header_owner_keeps_unchanged_consumer_location() -> None:
    row = _field_observations({"member_uses": [_use()]}, "WIZ8")[0]

    assert row["declaration_file"] == "include/wiz8/model.h"
    assert row["consumers"] == [("?Read@W8First@@QAEHXZ", "W8First::Read", "src/wiz8/model.cpp")]


def test_wrapper_use_remains_a_local_observation_not_resolved_caller_debt() -> None:
    wrapper = _use(function="?State@W8First@@QAEHXZ")
    wrapper["function_identity"] = wrapper["function_id"]
    wrapper["function"] = "W8First::State"
    wrapper["conversions"] = [{"kind": "BitCast", "destination_type": "OtherRecord *"}]

    rows = _field_observations({"member_uses": [wrapper]}, "WIZ8")

    assert rows[0]["uses"][0]["function"] == "W8First::State"
    assert rows[0]["uses"][0]["conversions"] == [
        {"kind": "BitCast", "destination_type": "OtherRecord *"}
    ]
    assert "resolved_conversion" not in rows[0]


def test_default_filter_selector_four_fits_the_fifth_source_map_entry() -> None:
    use = _use(
        target="SURRENDER",
        owner="srGERD",
        identity="record:srGERD::field@include/surrender/srGERD.h:873:0",
        name="mag_filter_map_1f6c_",
        function="?setTextureDefaultMagFilter@srGERD@@QAEXW4e_filter@srTextureIFace@@@Z",
    )
    use.update(
        declared_type="unsigned long[5]",
        offset_bytes=0x1F6C,
        extent_bytes=20,
        operations=["array-index", "read-write"],
        array_indices=[{"constant": True, "value": "4"}],
    )
    flow = {
        "entry": "10018650",
        "function": {"entry": "10018650", "name": "setTextureDefaultMagFilter"},
        "program": {"name": "sr.dll", "binary_sha256": "retail"},
        "profile": {"name": "analysis"},
        "root": {
            "requested": "this",
            "identity": "10018650:this",
            "kind": "parameter",
            "role": "receiver",
            "type": "srGERD *",
            "type_origin": "model-derived HighFunction prototype",
        },
        "accesses": [
            {
                "kind": "store",
                "site": "1001869d",
                "width": 4,
                "effective_address": {
                    "root": "10018650:this",
                    "constant": 0x1F7C,
                    "terms": [],
                },
            }
        ],
        "completeness": {"status": "complete", "stops": []},
    }
    identity = SimpleNamespace(
        qualified_name="srGERD::setTextureDefaultMagFilter",
        semantic_id=use["function_identity"],
        owning_class="srGERD",
        parameter_types=("enum srTextureIFace::e_filter",),
        has_this=True,
    )

    report = _field_flow_triage(
        Path("."),
        {
            "member_uses": [use],
            "classes": [_class("srGERD", target="SURRENDER", size=0x1F80)],
        },
        "SURRENDER",
        [{"case": "S01", "flow": flow}],
        identities_by_address={0x10018650: (identity,)},
    )

    selector = report["selector_observations"][0]
    assert selector["selector"] == 4
    assert selector["array_count"] == 5
    assert selector["retail_store_offset"] == "0x1f7c"
    assert selector["in_bounds"] is True
    assert report["categories"]["known_selector_outside_declared_array"] == []


def test_selector_outside_source_extent_stays_a_contradiction() -> None:
    use = _use(
        target="SURRENDER",
        owner="srGERD",
        identity="record:srGERD::field@include/surrender/srGERD.h:873:0",
        name="mag_filter_map_1f6c_",
        function="?setTextureDefaultMagFilter@srGERD@@QAEXW4e_filter@srTextureIFace@@@Z",
    )
    use.update(
        declared_type="unsigned long[4]",
        offset_bytes=0x1F6C,
        extent_bytes=16,
        operations=["array-index"],
        array_indices=[{"constant": True, "value": "4"}],
    )
    flow = {
        "entry": "10018650",
        "function": {"entry": "10018650", "name": "setTextureDefaultMagFilter"},
        "root": {
            "identity": "10018650:this",
            "role": "receiver",
            "type_origin": "model-derived HighFunction prototype",
        },
        "accesses": [
            {
                "kind": "store",
                "site": "1001869d",
                "width": 4,
                "effective_address": {
                    "root": "10018650:this",
                    "constant": 0x1F7C,
                    "terms": [],
                },
            }
        ],
        "completeness": {"status": "complete", "stops": []},
    }
    identity = SimpleNamespace(
        qualified_name="srGERD::setTextureDefaultMagFilter",
        semantic_id=use["function_identity"],
        owning_class="srGERD",
        parameter_types=("enum srTextureIFace::e_filter",),
        has_this=True,
    )

    report = _field_flow_triage(
        Path("."),
        {
            "member_uses": [use],
            "classes": [_class("srGERD", target="SURRENDER", size=0x1F80)],
        },
        "SURRENDER",
        [{"case": "S01", "flow": flow}],
        identities_by_address={0x10018650: (identity,)},
    )

    contradiction = report["categories"]["known_selector_outside_declared_array"]
    assert len(contradiction) == 1
    assert contradiction[0]["selector"] == 4
    assert contradiction[0]["array_count"] == 4
    assert contradiction[0]["status"] == "contradiction"


def test_stride_join_uses_complete_class_size_not_only_observed_fields() -> None:
    uses = [
        _use(
            owner="srVector3T<float>",
            identity=f"vector3:{name}",
            name=name,
            function="render:poly_normals",
        )
        for name in ("x", "y", "z")
    ]
    for use, offset in zip(uses, (0, 4, 8), strict=True):
        use.update(offset_bytes=offset, extent_bytes=4, declared_type="float")
    identity = SimpleNamespace(
        qualified_name="stMeshModel::RenderTriMeshWithEquations00470380",
        semantic_id="render:poly_normals",
        owning_class="stMeshModel",
        parameter_types=(
            "class srGERD &",
            "const struct TriMesh &",
            "const class srVector3T<float> *",
        ),
        has_this=True,
    )
    flow = {
        "entry": "00470380",
        "function": {"entry": "00470380", "name": "RenderTriMeshWithEquations00470380"},
        "root": {
            "identity": "00470380:poly_normals",
            "requested": "3",
            "role": "argument",
            "type": "srVector4T<float> *",
            "type_origin": "model-derived HighFunction prototype",
        },
        "accesses": [
            {
                "kind": "load",
                "site": f"00470{index}00",
                "width": 4,
                "effective_address": {
                    "root": "00470380:poly_normals",
                    "constant": component,
                    "terms": [{"stride": 16, "index": {"identity": ["register", str(index)]}}],
                },
            }
            for index, component in enumerate((0, 4, 8), 1)
        ],
        "completeness": {"status": "incomplete", "stops": [{"kind": "ambiguous_join"}]},
    }

    report = _field_flow_triage(
        Path("."),
        {
            "member_uses": uses,
            "classes": [_class("srVector3T<float>", size=16)],
        },
        "WIZ8",
        [{"case": "S05", "flow": flow, "source_parameter_index": 2}],
        identities_by_address={0x470380: (identity,)},
    )

    stride = report["categories"]["actual_pointee_stride"][0]
    assert stride["source_element_bytes"] == 16
    assert {field["name"] for field in stride["observed_source_fields"]} == {"x", "y", "z"}
    assert stride["retail_strides_bytes"] == [16]
    assert stride["status"] == "consistent"
    assert stride["ghidra_root_type"] == "srVector4T<float> *"
    assert report["categories"]["unresolved_owner_or_root"][0]["reason"].startswith(
        "the selected rooted flow is incomplete"
    )


def test_stride_stays_visible_when_compiler_layout_is_untrusted() -> None:
    use = _use(owner="W8Vector", identity="vector:x", name="x")
    use.update(offset_bytes=0, extent_bytes=4, declared_type="float")
    identity = SimpleNamespace(
        qualified_name="ReadVector",
        semantic_id=use["function_identity"],
        owning_class=None,
        parameter_types=("const W8Vector *",),
        has_this=False,
    )
    flow = {
        "entry": "00401000",
        "root": {"identity": "vector-root", "role": "argument"},
        "accesses": [
            {
                "kind": "load",
                "site": "00401010",
                "width": 4,
                "effective_address": {
                    "root": "vector-root",
                    "constant": 0,
                    "terms": [{"stride": 16}],
                },
            }
        ],
        "completeness": {"status": "complete", "stops": []},
    }

    report = _field_flow_triage(
        Path("."),
        {
            "member_uses": [use],
            "classes": [_class("W8Vector", layout_trusted=False)],
        },
        "WIZ8",
        [{"case": "untrusted-layout", "flow": flow, "source_parameter_index": 0}],
        identities_by_address={0x401000: (identity,)},
    )

    stride = report["categories"]["actual_pointee_stride"][0]
    assert stride["source_element_bytes"] is None
    assert stride["source_layout_trusted"] is False
    assert stride["retail_strides_bytes"] == [16]
    assert stride["status"] == "source_layout_unavailable"


def test_crossing_and_byte_accesses_remain_candidates_and_receivers_stay_scoped() -> None:
    first = _use(owner="W8First", identity="first:field", name="state")
    first.update(offset_bytes=4, extent_bytes=4, declared_type="unsigned int")
    identity = SimpleNamespace(
        qualified_name="W8First::Read",
        semantic_id=first["function_identity"],
        owning_class="W8First",
        parameter_types=(),
        has_this=True,
    )
    root = "first-root"
    flow = {
        "entry": "00002000",
        "function": {"entry": "00002000", "name": "Read"},
        "root": {"identity": root, "role": "receiver"},
        "accesses": [
            {
                "kind": "load",
                "site": "00002010",
                "width": 1,
                "effective_address": {"root": root, "constant": 6, "terms": []},
            },
            {
                "kind": "load",
                "site": "00002014",
                "width": 4,
                "effective_address": {"root": root, "constant": 6, "terms": []},
            },
        ],
        "completeness": {"status": "complete", "stops": []},
    }
    report = _field_flow_triage(
        Path("."),
        {"member_uses": [first], "classes": [_class("W8First")]},
        "WIZ8",
        [{"case": "receiver-one", "flow": flow}],
        identities_by_address={0x2000: (identity,)},
    )

    assert report["categories"]["byte_access_interpretation"][0]["status"] == (
        "investigation_candidate"
    )
    assert report["categories"]["cross_member_boundary"][0]["status"] == ("investigation_candidate")
    assert report["queries"][0]["root"]["identity"] == root


def test_typed_project_conversion_stays_local_and_cannot_resolve_through_a_wrapper() -> None:
    use = _use(owner="W8First", identity="first:field", name="state")
    use["conversions"] = [
        {
            "kind": "BitCast",
            "source_type": "W8First *",
            "destination_type": "W8Second *",
        }
    ]

    report = _field_flow_triage(
        Path("."),
        {
            "member_uses": [use],
            "classes": [
                _class("W8First"),
                _class("W8Second"),
            ],
        },
        "WIZ8",
        [],
        identities_by_address={},
    )

    conversion = report["categories"]["typed_project_conversion"][0]
    assert conversion["status"] == "local_project_type_conversion_observed"
    assert conversion["resolved_conversion"] is False


def test_field_flow_categories_report_counts_when_examples_are_bounded() -> None:
    uses = []
    for line in range(70):
        use = _use(function=f"?Read{line}@W8First@@QAEHXZ")
        use["use_line"] = line + 1
        use["conversions"] = [
            {
                "kind": "BitCast",
                "source_type": "W8First *",
                "destination_type": "W8Second *",
            }
        ]
        uses.append(use)

    report = _field_flow_triage(
        Path("."),
        {
            "member_uses": uses,
            "classes": [
                _class("W8First"),
                _class("W8Second"),
            ],
        },
        "WIZ8",
        [],
        identities_by_address={},
    )

    conversions = report["categories"]["typed_project_conversion"]
    assert len(conversions) == 64
    assert report["category_summary"]["typed_project_conversion"] == {
        "observed": 70,
        "reported": 64,
        "omitted": 6,
    }
