# Wizardry 8 Ghidra scripts

Ordinary retail inspection uses the checkout-owned ProgramDB through:

```sh
uv run wiz8 ghidra decompile 0x004a5e50 0x004a5f20
uv run wiz8 ghidra asm 0x004a5e50
uv run wiz8 ghidra class W8Monster
```

Inspection writes disposable native C and listing artifacts under `build/ghidra/`.
Use `uv run wiz8 compare` for Ghidriff comparison.

`merge_checkpoint_functions.py` reconciles non-overlapping function analysis from
reviewed GZF checkpoints. Follow the
[checkpoint procedure](../../docs/ghidra-checkpoints.md)
before applying or refreshing analysis.

Retail instructions and accepted original-source evidence outrank inferred types
and decompiler text. Source or PDB projections improve analysis but do not
independently confirm the source that supplied them.
