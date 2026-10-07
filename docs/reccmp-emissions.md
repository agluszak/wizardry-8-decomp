# Compiler emission metadata

Authored source carries `FUNCTION`, `GLOBAL` and `VTABLE` annotations. Compiler
special members, deleting destructors, initializers, thunks and template
instantiations are binary entities identified outside authored C++.

`tools/wiz8decomp/emissions.py` generates disposable reccmp data sources under
`build/generated/reccmp/`. `wiz8 build` and `wiz8 check` generate them
automatically before reccmp consumes them.

The reviewed source of truth is
`evidence/observations/compiler-emissions.csv`, plus narrow reviewed corrections in
`config/reccmp/emission_overrides.csv`. The generator does not inspect the rebuilt
PDB, infer symbols from function order, correlate bodies, or maintain a second
recovery database.

`source_files` records source associations used for changed-file/template
selection; it does not claim an original translation unit. `symbol` records an
independently established linker identity. `recomp_selector` and
`selector_is_symbol` are available for reviewed overrides when an exact rebuilt
selector is known.

Generated CSVs are deterministic build products. Missing or stale output is simply
rewritten from the reviewed inventory. Library identities remain owned by the
corresponding checked-in reccmp metadata files rather than authored source.
