"""Investigation index of existing Ghidriff differences, never equivalence claims."""

from __future__ import annotations

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
_TYPES = re.compile(
    r"\b(?:undefined[1248]?|u?int|u?short|u?long|char|byte|bool|float|double|void|signed|unsigned)\b"
)
# A data identity is a link-time table or vtable address. Ghidra prints the
# store as `target = &NAME;`, so the right-hand side is a single symbol.
_IDENTITY_STORE = re.compile(r"^&([A-Za-z_$][\w$]*(?:::[A-Za-z_$][\w$]*)*)$")
_IDENTITY_NAME = re.compile(r"vftable|vtable|^PTR_|^DAT_|^PAIRED_DATA_|table", re.IGNORECASE)


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


def identity_store_deltas(old: list[str], new: list[str]) -> set[tuple[str, str, str]]:
    """Table/vtable addresses stored on one side's changed lines only.

    A construction-phase or companion table the other side never installs is a
    class-model or layout disagreement, not a text difference. Each image names
    its own tables, so a store the other side makes at the same point in the same
    order under a different name is reported as `renamed` and a store with no
    counterpart position as `missing`. The report records which; it does not
    decide which name or count is correct.
    """

    def stored(lines: list[str]) -> list[str]:
        found: list[str] = []
        for line in lines:
            _, separator, right = line.partition("=")
            if not separator:
                continue
            match = _IDENTITY_STORE.match(right.strip().rstrip(";").strip())
            if match and _IDENTITY_NAME.search(match.group(1)):
                found.append(match.group(1))
        return found

    left, right = stored(old), stored(new)
    result = set()
    for side, own, other in (("retail", left, right), ("rebuild", right, left)):
        for index, name in enumerate(own):
            if name in other:
                continue
            result.add((side, name, "renamed" if index < len(other) else "missing"))
    return result


def _census_row(calls: dict[int, dict], address: int) -> dict | None:
    """The call-census row for a function, or None when either side is unresolved."""
    row = calls.get(address)
    if row is None or any(row.get(side, {}).get("calls") is None for side in ("orig", "recomp")):
        return None
    return row


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
    signature_observations = {}
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
    categories: Counter = Counter()
    inline: Counter = Counter()
    for function in summary["functions"]:
        if function["outcome"] != "differences":
            continue
        address = int(function["orig"], 16)
        old, new = changed_lines(function)
        signals = set()
        call_row = _census_row(calls, address)
        for key in field_callers.get(address, []):
            groups[key].add(address)
            signals.add("field-width" if key[4] and key[5] else "field-offset")
        for identity in signature_callers.get(address, []):
            observation = signature_observations[identity]
            groups[
                (
                    "callee-signature-observation",
                    identity,
                    tuple(observation["private_discrepancies"]),
                )
            ].add(address)
            signals.add("callee-signature")
        if function["data"]:
            signals.add("referenced-data")
            for finding in function["data"]:
                groups[("referenced-data", json.dumps(finding, sort_keys=True))].add(address)
        if function.get("inline_callees"):
            signals.add("inline-retry")
            inline["retried"] += 1
            for callee in function["inline_callees"]:
                groups[("inline-retried-callee", f"pair:{int(callee, 16):#x}")].add(address)
        delta = None
        if call_row is not None:
            delta = call_delta(call_row["orig"]["calls"], call_row["recomp"]["calls"])
            categories[delta["category"]] += 1
            if delta["deltas"]:
                signals.add("call-target")
            for change in delta["deltas"]:
                groups[
                    ("direct-call-delta", tuple(change["retail"]), tuple(change["rebuild"]))
                ].add(address)
        else:
            categories["unavailable-sequence"] += 1
        token_deltas = text_fingerprints(old, new)
        for fingerprint in token_deltas:
            groups[fingerprint].add(address)
        for side, identity, shape in identity_store_deltas(old, new):
            signals.add("data-identity")
            groups[("one-sided-identity-store", side, shape, identity)].add(address)
        rows[address] = {
            **function,
            "marker_kinds": sorted(markers.get(address, {"unknown"})),
            "signals": sorted(signals),
            "token_deltas": sorted(token_deltas),
            "call_delta": delta,
            "call_observation": call_row,
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
        if key[0] == "direct-call-delta":
            callees = {
                identity: callee_names.get(identity) for delta in key[1:] for identity in delta
            }
        elif key[0] in {"callee-signature-observation", "inline-retried-callee"}:
            callees = {key[1]: callee_names.get(key[1])}
        else:
            callees = {}
        clusters.append(
            {
                "key": list(key),
                "functions_count": len(addresses),
                "signature_observation": signature_observations[key[1]]
                if key[0] == "callee-signature-observation"
                else None,
                "field_observation": field_observations.get(key),
                "callees": callees,
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
        "functions_in_repeated_clusters": len(covered),
        "unclassified_differing": len(rows.keys() - covered),
        "clusters": len(clusters),
        "call_categories": dict(categories),
        "inline_retried_differing": inline["retried"],
        "signals": dict(Counter(tag for row in rows.values() for tag in row["signals"])),
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
