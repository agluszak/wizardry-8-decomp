"""Investigation index of existing Ghidriff differences, never equivalence claims."""

from __future__ import annotations

import csv
import hashlib
import json
import re
from collections import Counter, defaultdict
from difflib import SequenceMatcher
from pathlib import Path
from typing import Any

from reccmp.compare.call_census import call_delta

from ..paths import atomic_json

_GENERATED = re.compile(
    r"\b(?:[a-z]*Var\d+|local_[0-9a-f]+|param_\d+|LAB_\w+|DAT_\w+|FUN_[0-9a-f]+)\b"
)
_LITERAL = re.compile(
    r'"(?:[^"\\]|\\.)*"|\b(?:0x[0-9a-f]+|\d+(?:\.\d*)?(?:e[+-]?\d+)?)\b', re.IGNORECASE
)
_TOKEN = re.compile(
    r'"(?:[^"\\]|\\.)*"|\b0x[0-9a-f]+\b|\b\d+(?:\.\d*)?\b|[A-Za-z_$][\w$]*|->|::|==|!=|<=|>=|&&|\|\||[^\s]',
    re.IGNORECASE,
)
_CALL = re.compile(r"\b([A-Za-z_$][\w$:]*)\s*\(")
_TYPES = re.compile(
    r"\b(?:undefined[1248]?|u?int|u?short|u?long|char|byte|bool|float|double|void|signed|unsigned)\b"
)
_SIGNALS = {
    "exception-frame": r"ExceptionList|unwind|try_level|EH_|catch|exception",
    "lifecycle": r"vftable|vtable|~|destructor|constructor|scalar_deleting",
    "container-template": r"W8(?:Growable)?Vector|W8Hash|srArray|template|<[^;]+>::",
    "floating-point": r"\b(?:float|double|float10|ROUND|NAN|SQRT)\b|\d+\.\d+",
    "folding-noop": r"NoOp|compiler_folded|folded_empty",
    "field-offset": r"->|\+\s*0x[0-9a-f]+",
    "field-width": r"\b(?:byte|ushort|undefined[1248])\s*\*",
    "signedness": r"\b(?:unsigned|signed|uint|ushort|sbyte)\b",
    "global-identity": r"\b(?:g_|DAT_|PAIRED_DATA_)\w+",
    "predicate": r"\b(?:if|while)\s*\(|<=|>=|==|!=",
}


def changed_lines(row: dict) -> tuple[list[str], list[str]]:
    # summary.json stores Ghidriff's unified diff. Context/header lines are
    # excluded; the report never recomputes code differences.
    removed, added = [], []
    for line in row["code_diff"]:
        for text in line.splitlines():
            if text.startswith("-") and not text.startswith("---"):
                removed.append(text[1:].strip())
            elif text.startswith("+") and not text.startswith("+++"):
                added.append(text[1:].strip())
    return removed, added


def broad_shape(old: list[str], new: list[str]) -> str:
    def generated(lines):
        return [_GENERATED.sub("GENERATED", line) for line in lines]

    left, right = generated(old), generated(new)
    if not old and not new:
        return "data only"
    if left == right:
        return "generated names only"
    if [_LITERAL.sub("LITERAL", line) for line in left] == [
        _LITERAL.sub("LITERAL", line) for line in right
    ]:
        return "literal or address only"
    if [_TYPES.sub("TYPE", line) for line in left] == [_TYPES.sub("TYPE", line) for line in right]:
        return "types or casts only"
    if all(re.fullmatch(r"[\w *]+;", line) for line in old + new):
        return "declarations only"
    old_calls = [
        m for line in old for m in _CALL.findall(line) if m not in {"if", "while", "for", "switch"}
    ]
    new_calls = [
        m for line in new for m in _CALL.findall(line) if m not in {"if", "while", "for", "switch"}
    ]
    if old_calls != new_calls:
        return "calls differ"
    return "control or statement structure"


def text_fingerprints(old: list[str], new: list[str]) -> set[tuple[str, str, str]]:
    """Exact token deltas in aligned changed lines; presentation evidence only."""
    result = set()
    # Avoid misleading pairings in wholesale rewritten blocks. Structural
    # functions still receive call deltas and descriptive signals.
    if len(old) != len(new) or len(old) > 80:
        return result
    for left, right in zip(old, new):
        a, b = _TOKEN.findall(left), _TOKEN.findall(right)
        changes = [
            op
            for op in SequenceMatcher(None, a, b, autojunk=False).get_opcodes()
            if op[0] != "equal"
        ]
        if len(changes) != 1:
            continue
        _, i, j, k, l = changes[0]
        before, after = " ".join(a[i:j]), " ".join(b[k:l])
        if not before or not after or len(before) + len(after) > 160:
            continue
        if _GENERATED.fullmatch(before) or _GENERATED.fullmatch(after):
            continue
        if _LITERAL.fullmatch(before) and _LITERAL.fullmatch(after):
            kind = "literal"
        elif before in {"<", ">", "<=", ">=", "==", "!="} or after in {
            "<",
            ">",
            "<=",
            ">=",
            "==",
            "!=",
        }:
            kind = "predicate"
        elif _TYPES.fullmatch(before) and _TYPES.fullmatch(after):
            kind = "type-spelling"
        else:
            kind = "token-spelling"
        result.add((kind, before, after))
    return result


def mismatch_clusters(repository: Path, directory: Path) -> dict[str, Any]:
    from ..source_index import warn_if_source_index_may_be_stale

    index_stale = warn_if_source_index_may_be_stale(repository, "WIZ8")
    summary = json.loads((directory / "summary.json").read_text())
    manifest = json.loads((directory / "manifest.json").read_text())
    digest = hashlib.sha256(json.dumps(manifest, sort_keys=True).encode()).hexdigest()
    if digest != summary["inputs"]["manifest_sha256"]:
        raise ValueError("summary and manifest belong to different comparisons")
    calls_path = directory / "direct-calls.json"
    if not calls_path.is_file():
        raise ValueError(
            "comparison lacks direct-calls.json; generate the catalog call census for these inputs"
        )
    census = json.loads(calls_path.read_text())
    if census["manifest_sha256"] != digest:
        raise ValueError("call census and comparison use different catalogs")
    calls = {int(row["address"], 16): row for row in census["functions"]}
    target_bodies = {(row["image"], row["identity"]): row for row in census["targets"]}
    index = json.loads((repository / "build/source-index.json").read_text())
    declarations = {
        (row["target"], row["semantic_id"], row.get("unit_id")): row
        for row in index["declarations"]
    }
    source_signatures = {}
    for marker in index["markers"]:
        key = marker.get("declaration_key")
        if marker["target"] == summary["target"] and key:
            declaration = declarations.get(tuple(key))
            if declaration:
                source_signatures[int(marker["address"])] = declaration
    symbols = {int(obj["orig"], 16): obj.get("recomp_symbol") for obj in manifest["objects"]}
    markers: dict[int, set[str]] = defaultdict(set)
    for marker in index["markers"]:
        if marker["target"] == summary["target"]:
            markers[int(marker["address"])].add(marker["marker_kind"])
    callee_names = {f"pair:{int(obj['orig'], 16):#x}": obj["name"] for obj in manifest["objects"]}
    callee_names.update(
        {f"{obj['image']}:{int(obj['addr'], 16):#x}": obj["name"] for obj in manifest["unpaired"]}
    )
    # Retain the historical mutually exclusive census as a separate observation.
    # Its labels are not silently promoted to a fresh classification.
    historical = {}
    bucket_file = directory / "remaining-buckets.tsv"
    if bucket_file.is_file():
        with bucket_file.open() as stream:
            historical = {
                int(row["retail address"], 16): row
                for row in csv.DictReader(stream, delimiter="\t")
            }
    signature_observations = {}
    field_observations = {}
    field_callers: dict[int, list[tuple]] = defaultdict(list)
    field_path = directory / "field-uses.json"
    if field_path.is_file():
        fields = json.loads(field_path.read_text())
        if fields["comparison_inputs"] != summary["inputs"]:
            raise ValueError("field census and comparison use different inputs")
        for observation in fields["access_fingerprints"]:
            key = (
                "field-access-observation",
                observation["owner"],
                observation["offset"],
                observation["operation"],
                tuple(observation["retail_observed_widths"]),
                tuple(observation["rebuild_observed_widths"]),
            )
            field_observations[key] = observation
            for member in observation["functions"]:
                field_callers[int(member, 16)].append(key)
    signature_callers: dict[int, list[str]] = defaultdict(list)
    signature_path = directory / "signature-census.json"
    if signature_path.is_file():
        signatures = json.loads(signature_path.read_text())
        if signatures["inputs"] != summary["inputs"]:
            raise ValueError("signature census and comparison use different inputs")
        for callee in signatures["callees"]:
            if not callee["private_discrepancies"]:
                continue
            identity = callee["identity"]
            signature_observations[identity] = callee
            for caller in callee["differing_callers"]:
                signature_callers[int(caller, 16)].append(identity)
    groups: dict[tuple, set[int]] = defaultdict(set)
    rows = {}
    categories = Counter()
    for function in summary["functions"]:
        if function["outcome"] != "differences":
            continue
        address = int(function["orig"], 16)
        old, new = changed_lines(function)
        text = "\n".join(old + new)
        signals = {
            name for name, pattern in _SIGNALS.items() if re.search(pattern, text, re.IGNORECASE)
        }
        shape = broad_shape(old, new)
        for key in field_callers.get(address, []):
            groups[key].add(address)
            signals.add("field-width" if key[4] and key[5] else "field-offset")
        for identity in signature_callers.get(address, []):
            signals.add("signature-abi")
            observation = signature_observations[identity]
            groups[
                (
                    "callee-signature-observation",
                    identity,
                    tuple(observation["private_discrepancies"]),
                )
            ].add(address)
        if shape in {"generated names only", "declarations only"}:
            signals.add("representation")
        if function["data"]:
            signals.add("referenced-data")
            for finding in function["data"]:
                groups[("referenced-data", json.dumps(finding, sort_keys=True))].add(address)
        if any(re.match(r"^[\w *]+\([^;]*\)$", line) for line in old + new):
            signals.add("signature-abi")
        if len(old) + len(new) > 100:
            signals.add("large-structural")
        if old != new and Counter(old) == Counter(new):
            signals.add("statement-order")
        delta = None
        observation = calls.get(address)
        if observation and all(
            observation[side]["calls"] is not None for side in ("orig", "recomp")
        ):
            delta = call_delta(observation["orig"]["calls"], observation["recomp"]["calls"])
            categories[delta["category"]] += 1
            if delta["deltas"]:
                signals.add("call-target")
            elif shape == "calls differ":
                signals.add("call-arguments")
            delta["helper_expansion_candidates"] = []
            for helper_side, expanded_side in (("orig", "recomp"), ("recomp", "orig")):
                helper_sequence = [call["identity"] for call in observation[helper_side]["calls"]]
                expanded_sequence = [
                    call["identity"] for call in observation[expanded_side]["calls"]
                ]
                for position, identity in enumerate(helper_sequence):
                    body = target_bodies.get((helper_side, identity))
                    if body is None or not body["calls"]:
                        continue
                    body_calls = body["calls"]
                    body_sequence = [call["identity"] for call in body_calls]
                    if (
                        helper_sequence[:position] + body_sequence + helper_sequence[position + 1 :]
                        == expanded_sequence
                    ):
                        groups[
                            ("helper-direct-call-expansion", helper_side, tuple(body_sequence))
                        ].add(address)
                        delta["helper_expansion_candidates"].append(
                            {
                                "helper_side": helper_side,
                                "helper": identity,
                                "name": body["name"],
                                "observation": "replacing this call with its native direct-call sequence reproduces the other side's sequence",
                            }
                        )
            for change in delta["deltas"]:
                groups[
                    ("direct-call-delta", tuple(change["retail"]), tuple(change["rebuild"]))
                ].add(address)
        else:
            categories["unavailable-sequence"] += 1
        for fingerprint in text_fingerprints(old, new):
            groups[fingerprint].add(address)
        rows[address] = {
            **function,
            "marker_kinds": sorted(markers.get(address, {"unknown"})),
            "broad_shape": shape,
            "historical_bucket": historical.get(address),
            "signals": sorted(signals),
            "call_delta": delta,
            "call_observation": observation,
            "source_signature": source_signatures.get(address),
            "recomp_decorated_symbol": symbols.get(address),
            "callee_signature_observations": signature_callers.get(address, []),
            "field_access_observations": field_callers.get(address, []),
        }
    clusters = []
    covered = set()
    for key, addresses in groups.items():
        if len(addresses) < 2:
            continue
        covered.update(addresses)
        members = [rows[address] for address in sorted(addresses)]
        clusters.append(
            {
                "key": list(key),
                "functions_count": len(addresses),
                "signature_observation": signature_observations[key[1]]
                if key[0] == "callee-signature-observation"
                else None,
                "field_observation": field_observations.get(key),
                "callees": {
                    identity: callee_names.get(identity)
                    for delta in key[1:]
                    if isinstance(delta, tuple)
                    for identity in delta
                }
                if key[0] == "direct-call-delta"
                else {key[1]: callee_names.get(key[1])}
                if key[0] == "callee-signature-observation"
                else {},
                "representatives": [row["orig"] for row in members[:5]],
                "source_files": sorted({row["source"]["path"] for row in members if row["source"]}),
                "owners": sorted(
                    {row["name"].rsplit("::", 1)[0] for row in members if "::" in row["name"]}
                ),
                "signals": sorted({tag for row in members for tag in row["signals"]}),
                "functions": [row["orig"] for row in members],
            }
        )
    clusters.sort(key=lambda c: (-c["functions_count"], str(c["key"])))
    metrics = {
        **summary["counts"],
        "unpaired_by_marker": dict(
            Counter(
                "+".join(sorted(markers.get(int(row["orig"], 16), {"unknown"})))
                for row in summary["functions"]
                if row["outcome"] == "unpaired"
            )
        ),
        "generated_presentation_candidates": sum(
            "representation" in row["signals"] for row in rows.values()
        ),
        "functions_in_repeated_clusters": len(covered),
        "unclassified_differing": len(rows.keys() - covered),
        "clusters": len(clusters),
        "signature_callee_clusters": sum(
            cluster["key"][0] == "callee-signature-observation" for cluster in clusters
        ),
        "field_access_clusters": sum(
            cluster["key"][0] == "field-access-observation" for cluster in clusters
        ),
        "call_categories": dict(categories),
        "helper_expansion_candidates": sum(
            bool(row["call_delta"] and row["call_delta"].get("helper_expansion_candidates"))
            for row in rows.values()
        ),
        "broad_shapes": dict(Counter(row["broad_shape"] for row in rows.values())),
    }
    output = directory / "mismatch-clusters.json"
    atomic_json(
        output,
        {
            "policy": "Triage observations only; repeated fingerprints are not confirmed root causes or equivalence claims.",
            "inputs": summary["inputs"],
            "source_index_stale": index_stale,
            "call_scope": census["scope"],
            "metrics": metrics,
            "clusters": clusters,
            "functions": list(rows.values()),
        },
    )
    return {
        "report": str(output),
        "metrics": metrics,
        "top_clusters": [
            {
                "key": c["key"],
                "functions_count": c["functions_count"],
                "representatives": c["representatives"],
                "callees": c["callees"],
                "signals": c["signals"],
            }
            for c in clusters[:15]
        ],
    }
