from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.reports.semantic_debt import _field_flow_triage, _field_observations
from wiz8decomp.source_index import load_source_index


@pytest.mark.integration
def test_field_flow_joins_real_reccmp_v6_owner_usrs_and_class_layout() -> None:
    repository = Path(__file__).resolve().parents[2]
    index_path = repository / "build/source-index.json"
    if not index_path.is_file():
        pytest.skip("run `uv run wiz8 check` to generate the pinned reccmp source index")

    index = load_source_index(repository)
    assert "schema" not in index

    classes = {
        (str(record.get("target") or "").upper(), str(record.get("qualified_name") or "")): record
        for record in index["classes"]
        if record.get("target") and record.get("qualified_name") and record.get("semantic_id")
    }
    actual_use = next(
        use
        for use in index["member_uses"]
        if use.get("owner_identity")
        and use.get("owner")
        and (str(use.get("target") or "").upper(), str(use["owner"])) in classes
    )
    target = str(actual_use["target"]).upper()
    owner_class = classes[(target, str(actual_use["owner"]))]

    # reccmp v6 intentionally keeps the Clang USR and the stable source-model
    # record identity as two different namespaces.
    assert str(actual_use["owner_identity"]).startswith("c:")
    assert actual_use["owner_identity"] != owner_class["semantic_id"]

    observed = _field_observations(index, target)
    observed_use = next(
        field
        for field in observed
        if field["field_identity"] == actual_use["field_identity"]
        and field["owner_identity"] == actual_use["owner_identity"]
    )
    assert observed_use["owner_semantic_id"] == owner_class["semantic_id"]

    # Find a real compiler-owned class whose complete layout extends past the
    # fields that happen to be referenced by recovered code. This is precisely
    # the case where deriving stride from observed MemberExprs would be wrong.
    candidate = None
    for record in index["classes"]:
        record_target = str(record.get("target") or "").upper()
        size = record.get("size")
        semantic_id = str(record.get("semantic_id") or "")
        if record.get("layout_trusted") is not True or not isinstance(size, int) or size <= 0:
            continue
        fields = [
            field
            for field in _field_observations(index, record_target)
            if field["owner_semantic_id"] == semantic_id
            and isinstance(field["offset_bytes"], int)
            and isinstance(field["extent_bytes"], int)
            and field["uses"]
        ]
        if not fields:
            continue
        observed_end = max(field["offset_bytes"] + field["extent_bytes"] for field in fields)
        if observed_end < size:
            candidate = (record_target, record, fields, observed_end)
            break

    assert candidate is not None, "expected one real class with an unobserved layout tail"
    stride_target, stride_class, stride_fields, observed_end = candidate
    first_use = stride_fields[0]["uses"][0]
    class_name = str(stride_class["qualified_name"])
    class_size = int(stride_class["size"])
    assert observed_end < class_size

    identity = SimpleNamespace(
        qualified_name="RealIndexStrideWitness",
        semantic_id=first_use["function_identity"],
        owning_class=None,
        parameter_types=(f"const class {class_name} *",),
        has_this=False,
    )
    flow = {
        "entry": "00401000",
        "root": {"identity": "real-index-root", "role": "argument"},
        "accesses": [
            {
                "kind": "load",
                "site": "00401001",
                "width": 4,
                "effective_address": {
                    "root": "real-index-root",
                    "constant": 0,
                    "terms": [{"stride": class_size}],
                },
            }
        ],
        "completeness": {"status": "complete", "stops": []},
    }
    report = _field_flow_triage(
        repository,
        index,
        stride_target,
        [{"case": "real-index-stride", "flow": flow, "source_parameter_index": 0}],
        identities_by_address={0x401000: (identity,)},
    )

    stride = report["categories"]["actual_pointee_stride"][0]
    assert stride["source_class_semantic_id"] == stride_class["semantic_id"]
    assert stride["source_element_bytes"] == class_size
    assert stride["retail_strides_bytes"] == [class_size]
    assert stride["status"] == "consistent"
