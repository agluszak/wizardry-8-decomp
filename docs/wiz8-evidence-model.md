# Wizardry evidence and provenance model

Names in this repository come from sources of very different authority. An exact-matching released
SGP source function and a Cosmic Forge editor label are both "a name", but only one of them is
evidence about Sir-Tech's original code. This document fixes the vocabulary that keeps the
difference visible in every tracked artifact.

## The three primary oracles are not interchangeable

* **SGP is a source oracle.** It can give exact original shared-source names, types,
  translation-unit boundaries, and sometimes exact bodies.
* **The demo is an official cross-build binary oracle.** It helps recover function boundaries,
  class layouts, historical implementations, and source ownership. It rarely supplies names.
* **Cosmic Forge is a semantic and patching oracle.** It knows formats, offsets, field meanings,
  executable addresses, and runtime structures, but most of its names are not Sir-Tech's original
  names.

The demo's asymmetry against the other alternate builds is part of the model: demo differences are
Sir-Tech development history, while 1.261 and 1.28 differences are third-party modifications.
`config/analysis/cross-build-oracles.csv` already records that role per build.

## Four independent axes

A reviewed identity answers four separate questions. Collapsing them is the mistake this model
exists to prevent.

| Column | Question | Example |
| --- | --- | --- |
| `owner` | Which codebase does the *body* belong to? | `sgp-shared`, `wiz8-engine-code`, `msvc6-runtime`, `fan-patch` |
| `name_origin` | Where did the *name* come from? | `sgp-source`, `fan-patch-signature` |
| `authority` | May the name be trusted as Sir-Tech's original? | `source-backed` |
| `confidence` | How certain is the *match* that produced the row? | `exact`, `strong`, `candidate` |

`owner` and `confidence` are already in use across `evidence/reviewed/*/functions.csv` and keep their
current meanings. `name_origin` and `authority` are the additions.

Two further columns apply where a layout rather than a name is at stake:

* `layout_origin` — where a structure or field layout came from, e.g. `sgp-source`,
  `cosmic-forge-parser`, `inferred-from-accesses`;
* `confirmed_by` — the independent source that corroborated the interpretation, e.g.
  `canonical-consumer`, `demo-cross-build`, `differential-edit`.

## `name_origin` vocabulary

`name_origin` is a `|`-joined set drawn from this closed list. Order is not significant.

| Token | Meaning |
| --- | --- |
| `original-source` | A released or recovered original source file compiles to this body. |
| `sgp-source` | Specifically the pinned SGP oracle tree, behind a Wizardry build branch. |
| `original-export` | A decorated export or import preserves the exact ABI name. |
| `original-runtime-string` | The program itself names the entity at runtime, e.g. class registration. |
| `original-source-path` | A `__FILE__`, assertion, or logging path assigns translation-unit ownership. |
| `official-demo` | Evidence carried by the official demo build. |
| `official-cross-build` | Evidence carried by another official build, including protected retail. |
| `fan-patch-signature` | A verified fan-patch signature database, currently CFAgent. |
| `cosmic-forge` | The Cosmic Forge editor, its parsers, resources, or the author's posts. |
| `descriptive` | We named it from observed behaviour. No external source claims this name. |

`sgp-source` is deliberately narrower than `original-source`: it also asserts the licence and
vendored non-commercial handling described in
[docs/libraries/sgp-source-oracle.md](libraries/sgp-source-oracle.md).

## `authority` vocabulary and its ceiling rule

| Token | Rank | Meaning |
| --- | ---: | --- |
| `source-backed` | 4 | An original source body proves the name. |
| `abi-backed` | 3 | An original decorated symbol proves the name. |
| `string-backed` | 2 | An original string in the program supports the name or its owning unit. |
| `external-semantic` | 1 | A third party assigned a meaningful name; Sir-Tech may have called it anything. |
| `descriptive` | 0 | We assigned the name. |

Each `name_origin` token has a ceiling. `authority` is the maximum ceiling over the row's origins,
never higher:

| `name_origin` token | Ceiling |
| --- | --- |
| `original-source`, `sgp-source` | `source-backed` |
| `original-export` | `abi-backed` |
| `original-runtime-string`, `original-source-path` | `string-backed` |
| `official-demo`, `official-cross-build` | `descriptive` |
| `fan-patch-signature`, `cosmic-forge` | `external-semantic` |
| `descriptive` | `descriptive` |

The official builds sit at `descriptive` on purpose. They are *boundary* oracles: they prove that a
function exists, where it starts, which vtable slot it occupies, and how it changed over time, but
they do not by themselves name it. When the demo does supply a name, it does so by retaining a
diagnostic string that retail dropped — and then the row carries both tokens, so the string origin
supplies the authority and `official-demo` records which build the string came from:

```text
name_origin = original-source-path|official-demo
authority   = string-backed
```

This makes the invariant mechanically checkable rather than a matter of reviewer discipline.

## Promotion

A name is promoted out of `external-semantic` only when a second, independent source supports it.
Corroboration is recorded by adding the second origin token, never by editing the authority alone.
Concretely, a CFAgent name becomes source-backed only once the SGP compile actually emits that body;
a Cosmic Forge field name becomes trustworthy only once a canonical consumer reads the field that
way.

The reverse never happens: adding `fan-patch-signature` or `cosmic-forge` to a row cannot raise its
authority, because their ceiling is below every original-evidence token.

## The applied name and its aliases

`provisional_name` is the name actually applied to the program; it is the name from the
highest-authority origin on the row. When a lower-authority source calls the same address something
else, that name is kept in the optional `|`-joined `aliases` column rather than discarded. Ghidra
applies `provisional_name` as the function symbol and each alias as a secondary label at the same
address, so the address stays searchable under the name the community already uses.

The reference case is `0x0040EFA0`. CFAgent calls it `GetRandomNumber`. Released SGP `Random.c`
contains the same algorithm as `Random`, behind the Wizardry precompiled-header branch, and its
header explicitly discusses Wizardry using that subsystem. The compile settled it: released
`Random.c` emits, at `/O2 /Ob2 /MD /G5`, a body whose relocation-masked hash is exactly the one at
`0x0040EFA0` in every unprotected build. The accepted row is:

```csv
address,provisional_name,owner,confidence,name_origin,authority,aliases,evidence
0040efa0,Random,sgp-shared,exact,sgp-source|fan-patch-signature,source-backed,GetRandomNumber,...
00506280,GetFact,wiz8-game-state,strong,fan-patch-signature,external-semantic,,...
```

`GetRandomNumber` is retained as a fan-patch alias unless independent Wizardry evidence shows that
Sir-Tech renamed it. Had the compile refuted the identity, the `sgp-source` token would simply never
have been added and the row would have stayed at `external-semantic` — the ceiling rule makes that
the only way to express the difference. See
[docs/libraries/sgp-source-oracle.md](libraries/sgp-source-oracle.md).

## Authority ranking of the full source list

This is the ordering used when two sources disagree.

| Evidence source | Names | Class/function boundaries | Authority |
| --- | --- | --- | --- |
| Actual PDB, MAP, OBJ or LIB files | Exact | Exact | Highest, if found |
| Exact-matching released SGP source | Exact shared-source names | Exact translation units, functions, types | Very high |
| Decorated `SR.DLL` and renderer exports/imports | Exact original ABI names and signatures | Exact exported method/class boundaries | Very high |
| CodeView/PDB path records | Exact project and source paths | Translation-unit ownership, not individual names | High |
| `__FILE__`, assertion and logging strings | Exact source filenames and line numbers | Strong function ownership | High |
| Game runtime-class registration strings | Usually exact class names and IDs | Strong class identity | High |
| Constructor vptr writes and vtables | No intrinsic name | Exact polymorphic boundaries and slot counts | High |
| Demo and official alternate builds | Usually no direct names | Strong cross-build boundary evidence | High |
| CFAgent signature names | Semantic external names | Verified target addresses | Medium-high |
| Cosmic Forge editor | Semantic field/format names | Parser and data boundaries | Medium |
| Community patches and documentation | Semantic names | Patch-site boundaries | Medium |
| Pure decompiler inference | Descriptive only | Heuristic | Low until corroborated |

Note that boundary authority and name authority diverge sharply in the middle of this table. The
demo is "High" for boundaries and effectively absent for names; CodeView paths are "High" for
ownership and give no function names at all. The `name_origin`/`authority` pair only governs names;
boundary decisions are recorded in the cross-build model instead.

## Why `Wiz8.exe` forces this

The canonical executable contains no MSVC RTTI type-descriptor strings, so there are zero exact
class names to export from it. Every local class identity therefore rests on constructors,
destructors, vtables, source paths and behaviour, as recorded in
[docs/wiz8-source-model.md](wiz8-source-model.md). In that setting an unmarked plausible name is
indistinguishable from a recovered one, which is exactly the failure this taxonomy prevents.

## Applying it

`name_origin` and `authority` are required on every row of every reviewed identity CSV under
`evidence/reviewed/*/functions.csv`, validated against the closed vocabularies and the ceiling rule by
`tests/unit/test_provenance.py`. `wiz8decomp.provenance` owns the vocabulary; the function-map
loader rejects any row that violates it, and `apply-functions` writes the origin, authority and
aliases into the Ghidra plate comment so the provenance is visible while reading the program.

Generated candidate artifacts under `build/` are not covered: they are heuristic output, and
promoting a candidate into the reviewed layer is what triggers the provenance decision.
`config/analysis/cross-build-map.csv` is also out of scope — it records boundary mappings, not
names, and keeps its own `automated_classification`/`review_decision` vocabulary.

### FID matches

FID is the one generator that emits *names*, so it states its provenance up front rather than
leaving each match to be classified by hand later. A seed's build provenance already records how it
was produced, and that determines what its symbol names are worth:

| Seed `source_kind` | `name_origin` | Authority |
| --- | --- | --- |
| `precompiled-archive` | `original-export` | `abi-backed` |
| `cmake-object-library` | `original-source` | `source-backed` |
| none recorded | `descriptive` | `descriptive` |

A precompiled archive carries the original library's COFF symbol table and a source-built object
carries names the pinned source declares; both are original evidence, and neither is something we
invented. A match with no recorded seed provenance — the `srs` database — degrades to `descriptive`
instead of inheriting the authority of the database it happens to live in. This is separate from
`_is_authoritative_fid_name`, which rejects Ghidra defaults and compiler-local labels; that filter
decides whether a name exists at all, while this decides what it proves.

Current authority counts are generated from the canonical catalogs rather than copied into this
document. Multiple sources for one address are rows in `function-evidence.csv`, not duplicate
function identities.
