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
# A data identity is a link-time table or vtable address. Ghidra prints the
# store as `target = &NAME;`, so the right-hand side is a single symbol.
_IDENTITY_STORE = re.compile(r"^&([A-Za-z_$][\w$]*(?:::[A-Za-z_$][\w$]*)*)$")
_IDENTITY_NAME = re.compile(r"vftable|vtable|^PTR_|^DAT_|^PAIRED_DATA_|table", re.IGNORECASE)
_CONTROL_FLOW = {
    "if": re.compile(r"(?<![\w.])if\s*\("),
    "while": re.compile(r"(?<![\w.])while\s*\("),
    "return": re.compile(r"(?<![\w.])return\b"),
    "switch": re.compile(r"(?<![\w.])switch\s*\("),
    "case": re.compile(r"(?<![\w.])case\b"),
    "goto": re.compile(r"(?<![\w.])goto\b"),
}
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


def _body_extent(ranges: list[list[str]] | None) -> int:
    """Byte extent of a census body's ranges, or zero when it has none."""
    if not ranges:
        return 0
    low = min(int(start, 16) for start, _end in ranges)
    high = max(int(end, 16) for _start, end in ranges)
    return high - low


def same_unit_helper_calls(
    row: dict[str, Any],
    target_bodies: dict[tuple[str, str], dict[str, Any]],
    unit_of: dict[int, str],
) -> list[tuple[str, str, str]]:
    """Calls the rebuild makes to a same-unit helper and the retail does not.

    A helper whose definition the compiler can see is a poor inlining candidate,
    so such a call is either a body the retail carries at the call site or a
    different helper the rebuilt source reaches for. Carrying the body leaves the
    helper's own direct calls at the caller, and the rebuild's call to the helper
    is what puts them on its other side, so a helper whose callees the retail
    caller already names is a carried body and is not a source difference. A
    helper that calls nothing leaves no such trace, and the size the retail body
    gained over the rebuilt one is the only thing left to read it by. What is
    left names neither, and only the rebuilt source can be read for it. Neither
    label is a claim about the authored spelling.
    """
    unit = unit_of.get(int(row["address"], 16))
    if not unit:
        return []
    orig_calls = {call["identity"] for call in row["orig"]["calls"] or []}
    growth = _body_extent(row["orig"].get("body_ranges")) - _body_extent(
        row["recomp"].get("body_ranges")
    )
    observations = []
    for call in row["recomp"]["calls"] or []:
        identity = call["identity"]
        body = target_bodies.get(("orig", identity))
        if identity in orig_calls or body is None:
            continue
        if unit_of.get(int(identity.removeprefix("pair:"), 16)) != unit:
            continue
        nested = {nested_call["identity"] for nested_call in body["calls"] or []}
        names_helper_callees = bool(nested) and nested <= orig_calls
        grew_by_helper = not nested and growth >= _body_extent(body.get("body_ranges")) // 2
        if names_helper_callees or grew_by_helper:
            shape = "retail-carried-body"
        else:
            shape = "helper-identity"
        observations.append((shape, identity, call["name"]))
    return sorted(observations)


_INFERRED_SIGNATURE_SOURCES = frozenset({"ANALYSIS", "DEFAULT"})


def _signature_inferred(callee: dict) -> str | None:
    """Which side of a callee width disagreement is only Ghidra's inference.

    Ghidra derives a callee's return storage and parameter widths from the code
    it decompiled, and the two images do not render the same source the same
    way: a `bool` function whose returns are `return 0;` and `return 1;` can come
    out `AL:1` on one side and `EAX:4` on the other without either binary
    disagreeing. Where either side's private signature carries `ANALYSIS` or
    `DEFAULT` provenance the width is the decompiler's, not a declaration, and
    the report says which side so the observation is not read as an ABI defect.

    `GetItemSpell` and `W8PathingService::TestWaypointSpan` are the two read by
    hand: both retail epilogues are `XOR EAX,EAX` followed by `MOV AL,...` or
    `SETZ AL`, so both return one byte and our declared `int` and `unsigned
    char` are correct.
    """
    sides = []
    for side in ("orig", "recomp"):
        source = callee["private_comparison"][side].get("signature_source")
        if source in _INFERRED_SIGNATURE_SOURCES:
            sides.append(f"{side}:{source}")
    return ",".join(sides) or None


def _retail_trace_short(address: int, traced: dict[int, dict[str, int]]) -> bool:
    """Whether the retail's private trace of this root covered far less than the rebuild's.

    Ghidra sometimes copies the receiver into a stack local and works from the
    local, and the rooted value-flow walk does not follow a store into a stack
    slot, so the retail side of the field census can stop after a couple of
    accesses while the rebuild keeps the receiver in a register and records
    dozens. An empty retail width list in that case is a truncated trace, not an
    absent access, and must not be read as a field claim.
    """
    counts = traced.get(address)
    if not counts:
        return False
    retail, rebuild = counts["orig"], counts["recomp"]
    return retail * 2 < rebuild


def _unresolved_call_shape(delta: dict) -> str:
    """How a call delta with an unpaired callee still differs, from reccmp's own opcodes.

    `call_delta` files any function with an unpaired callee under
    `unresolved-target` and returns before it can say which side has the extra
    call, because a callee missing from the manifest may be ICF'd away or simply
    not paired yet, and a delta across one is not trustworthy. That is the
    right call for the delta, but it leaves the category content-free: a
    function whose only interesting difference is one call lands in the same
    bucket as one with no difference at all.

    So this reads the removed and added call counts straight out of the opcodes
    reccmp already computed and does not re-derive the classification. It is a
    triage order over a population the category deliberately refuses to judge,
    not a second opinion about whether the delta is real.
    """
    deltas = delta.get("deltas") or []
    if not deltas:
        return "unresolved-no-call-delta"
    removed = sum(len(entry["retail"]) for entry in deltas)
    added = sum(len(entry["rebuild"]) for entry in deltas)
    if added == 0:
        return "unresolved-retail-only-calls"
    if removed == 0:
        return "unresolved-rebuild-only-calls"
    return "unresolved-calls-both-sides"


def _census_row(calls: dict[int, dict], address: int) -> dict | None:
    """The call-census row for a function, or None when either side is unresolved."""
    row = calls.get(address)
    if row is None or any(row.get(side, {}).get("calls") is None for side in ("orig", "recomp")):
        return None
    return row


def control_flow_excess(old: list[str], new: list[str]) -> tuple[tuple[str, int, int], ...]:
    """Decisions, loops and exits the retail recovered and the rebuild did not.

    A rebuilt function that reaches the retail's result with fewer branches,
    loops or returns is missing a decision, an early exit or a retry, and that
    survives in the changed lines whatever Ghidra called the variables, so the
    count is a triage order rather than a claim. A decompiler that recovers the
    same control flow through a different shape reports a surplus too: the six
    functions read by hand from this signal all turned out to be the decompiler
    merging or splitting a nested block, which is why the report records the
    per-keyword counts and not a verdict.
    """
    counts = []
    for keyword, pattern in _CONTROL_FLOW.items():
        retail = sum(1 for line in old if pattern.search(line))
        rebuild = sum(1 for line in new if pattern.search(line))
        if retail > rebuild:
            counts.append((keyword, retail, rebuild))
    return tuple(counts)


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
    unit_of = {
        int(marker["address"]): marker["source_file"]
        for marker in index["markers"]
        if marker["target"] == summary["target"]
    }
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
    # A comparison run does not produce field-uses.json by itself, so this stays
    # empty rather than unbound when the census is absent; the report must run
    # against a bare compare --changed directory.
    traced: dict[int, dict[str, int]] = {}
    field_path = directory / "field-uses.json"
    if field_path.is_file():
        fields = json.loads(field_path.read_text())
        if fields["comparison_inputs"] != summary["inputs"]:
            raise ValueError("field census and comparison use different inputs")
        # How much of each function's root the private trace reached. A decompiler
        # that launders the receiver through a stack local leaves the retail side
        # with far fewer traced accesses than the rebuild, and an empty width list
        # then means "the trace stopped" rather than "the retail never touched
        # the member". Recorded so a width claim is not read as an absence.
        for row in fields["private_access_observations"]:
            counts = {
                side: len(fact["flow"]["accesses"])
                for side, fact in row["sides"].items()
                if fact.get("flow") is not None
            }
            if len(counts) == 2:
                traced[int(row["function"], 16)] = counts
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
    unresolved_shapes = Counter()
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
        call_row = _census_row(calls, address)
        helper_calls = (
            same_unit_helper_calls(call_row, target_bodies, unit_of) if call_row is not None else []
        )
        for key in field_callers.get(address, []):
            groups[key].add(address)
            if helper_calls:
                # The rebuild reaches those members through the helper the retail
                # carries, so neither the census nor the text can tell a missing
                # member from a call. Recorded, not claimed.
                continue
            if _retail_trace_short(address, traced):
                # The retail's own trace of this function's root reached far less
                # than the rebuild's, so an empty retail width list is a truncated
                # trace rather than an absent access.
                signals.add("field-retail-coverage-limited")
                continue
            signals.add("field-width" if key[4] and key[5] else "field-offset")
        if (helper_calls or _retail_trace_short(address, traced)) and field_callers.get(address):
            signals.discard("field-offset")
            signals.discard("field-width")
            signals.add("field-through-helper" if helper_calls else "field-untraced")
        for identity in signature_callers.get(address, []):
            observation = signature_observations[identity]
            groups[
                (
                    "callee-signature-observation",
                    identity,
                    tuple(observation["private_discrepancies"]),
                )
            ].add(address)
            inferred = _signature_inferred(observation)
            if inferred:
                signals.add(f"signature-inferred-{inferred.replace(':', '-')}")
            else:
                signals.add("signature-abi")
        if any(re.match(r"^[\w *]+\([^;]*\)$", line) for line in old + new):
            # A changed line that reads as a declaration. This matches casts and
            # locals as readily as signatures, so it is recorded on its own
            # signal rather than counted as a callee-signature fact.
            signals.add("declaration-shaped-line")
        if shape in {"generated names only", "declarations only"}:
            signals.add("representation")
        if function["data"]:
            signals.add("referenced-data")
            for finding in function["data"]:
                groups[("referenced-data", json.dumps(finding, sort_keys=True))].add(address)
        if len(old) + len(new) > 100:
            signals.add("large-structural")
        if old != new and Counter(old) == Counter(new):
            signals.add("statement-order")
        delta = None
        if call_row is not None:
            delta = call_delta(call_row["orig"]["calls"], call_row["recomp"]["calls"])
            categories[delta["category"]] += 1
            if delta["category"] == "unresolved-target":
                unresolved_shapes[_unresolved_call_shape(delta)] += 1
            if delta["deltas"]:
                signals.add("call-target")
            elif shape == "calls differ":
                signals.add("call-arguments")
            delta["helper_expansion_candidates"] = []
            for helper_side, expanded_side in (("orig", "recomp"), ("recomp", "orig")):
                helper_sequence = [call["identity"] for call in call_row[helper_side]["calls"]]
                expanded_sequence = [call["identity"] for call in call_row[expanded_side]["calls"]]
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
                        # The helper identity belongs in the key: one shared
                        # body sequence otherwise groups unrelated helpers that
                        # happen to call the same callees in the same order.
                        groups[
                            (
                                "helper-direct-call-expansion",
                                helper_side,
                                identity,
                                tuple(body_sequence),
                            )
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
        if call_row is not None:
            for call_shape, identity, name in helper_calls:
                signals.add("same-unit-helper-call")
                groups[("same-unit-helper-call", call_shape, identity, name)].add(address)
        for fingerprint in text_fingerprints(old, new):
            groups[fingerprint].add(address)
        for side, identity, shape in identity_store_deltas(old, new):
            signals.add("data-identity")
            groups[("one-sided-identity-store", side, shape, identity)].add(address)
        excess = control_flow_excess(old, new)
        if excess:
            signals.add("retail-control-flow")
            groups[("retail-control-flow", tuple(word for word, _, _ in excess))].add(address)
        rows[address] = {
            **function,
            "marker_kinds": sorted(markers.get(address, {"unknown"})),
            "broad_shape": shape,
            "historical_bucket": historical.get(address),
            "signals": sorted(signals),
            "call_delta": delta,
            "call_observation": call_row,
            "control_flow_excess": control_flow_excess(old, new),
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
        elif key[0] == "callee-signature-observation":
            callees = {key[1]: callee_names.get(key[1])}
        elif key[0] == "helper-direct-call-expansion":
            callees = {key[2]: callee_names.get(key[2])}
        elif key[0] == "same-unit-helper-call":
            callees = {key[2]: key[3]}
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
        # `unresolved-target` is one bucket for every function with an unpaired
        # callee, which hides which side has the extra call. These counts say so
        # without re-deciding whether the delta is trustworthy.
        "unresolved_call_shapes": dict(unresolved_shapes),
        "one_sided_identity_stores": sum(
            cluster["key"][0] == "one-sided-identity-store" for cluster in clusters
        ),
        "helper_expansion_candidates": sum(
            bool(row["call_delta"] and row["call_delta"].get("helper_expansion_candidates"))
            for row in rows.values()
        ),
        "same_unit_helper_call_clusters": sum(
            cluster["key"][0] == "same-unit-helper-call" for cluster in clusters
        ),
        "same_unit_helper_calls": sum(
            "same-unit-helper-call" in row["signals"] for row in rows.values()
        ),
        "retail_control_flow_candidates": sum(
            bool(row["control_flow_excess"]) for row in rows.values()
        ),
        "field_observations_through_a_helper": sum(
            "field-through-helper" in row["signals"] for row in rows.values()
        ),
        "field_observations_with_a_truncated_retail_trace": sum(
            "field-untraced" in row["signals"] for row in rows.values()
        ),
        "callee_signature_observations": sum(
            any(name.startswith("signature-inferred-") for name in row["signals"])
            or "signature-abi" in row["signals"]
            for row in rows.values()
        ),
        "callee_signature_observations_from_inference": sum(
            any(name.startswith("signature-inferred-") for name in row["signals"])
            for row in rows.values()
        ),
        "functions_with_a_declaration_shaped_changed_line": sum(
            "declaration-shaped-line" in row["signals"] for row in rows.values()
        ),
        "signature_inference_sides": dict(
            Counter(
                name
                for row in rows.values()
                for name in row["signals"]
                if name.startswith("signature-inferred-")
            )
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
