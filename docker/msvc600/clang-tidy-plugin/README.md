# Wizardry clang-tidy plugin

`uv run wiz8 lint` loads two project-specific checks into Clang 21:

- `wiz8-redundant-scalar-cast` flags redundant scalar conversions.
- `wiz8-project-record-reinterpret-cast` rejects reinterpretation between modeled project record types.

The host selects changed lines with `WIZ8_REDUNDANT_CAST_LINES`; `*` selects the
whole tree. Clang-tidy runs directly, without a Python wrapper or recovery fact
collector. Compiler diagnostics and the stock checks in `.clang-tidy` run alongside
these checks.

The analysis image builds the plugin and smoke-tests its diagnostics from
`docker/msvc600/clang-tidy-plugin/CMakeLists.txt`.
