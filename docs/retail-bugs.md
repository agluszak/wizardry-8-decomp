# Retail bugs

Reviewed defect facts have one editable home:
[`evidence/reviewed/wiz8/claims.csv`](../evidence/reviewed/wiz8/claims.csv),
using the `retail-bug` predicate. Each claim binds a retained function entry
to observed behavior and instruction references. This document contains no
separately maintained defect inventory.

Add a reviewed claim
when retail instructions establish a defect; suspicious source or a comparison
disagreement alone is insufficient. Binary behavior does not establish original
local declarations or compiler-storage aliases.

Preservation and explicit compatibility deviations follow the canonical
[source-fidelity policy](source-fidelity.md).
An uninitialized-read suppression needs an `uninit-ok` reason identifying the
retail read and missing assignment. Source comments should only prevent an
accidental behavior change and refer to the reviewed claim when further detail
is needed.
