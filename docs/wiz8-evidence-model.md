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
`evidence/reviewed/cross-build/oracles.csv` already records that role per build.

The input-container investigation found no source, debug-symbol or project-file artifacts in the
inspected corpus; this finding does not establish their absence from uninspected releases.

## Provenance fields

Reviewed claims separate `owner`, `name_origin`, `authority`, and `confidence`.
The vocabulary and authority ceilings are defined and validated by
[`provenance.py`](../tools/wiz8decomp/provenance.py). Source/PDB projection is
analysis annotation, not independent confirmation of the recovered model.
