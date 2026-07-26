# Debug-artifact corpus snapshot

These CSVs are the reviewed output of a complete sweep of every EXE/DLL available in the
configured Wizardry input containers and every container member name. They are tracked because
reproduction requires proprietary, untracked inputs. Container and binary SHA-256 identities are
recorded in the CSVs; any unavailable binary remains an explicit row rather than being omitted.

The producer is `wiz8decomp.debug_artifacts`. Normal runs write the same four CSVs under
`build/reports/debug-artifacts/` and fail when they differ from this snapshot. Refresh only after
reviewing a complete corpus sweep:

```sh
uv run wiz8 debug-artifacts --archive-password '<local-password>' --update-snapshot
```

Encrypted RAR inputs require `unrar` or `unrar-nonfree` on `PATH`. The password is used only for
the temporary extraction and is never written to these reports.

`container-members.csv` records only member names with the requested debug/source/project
extensions. `containers.csv` always has one row per input, including containers with zero hits.
No executable bytes or extracted game data are tracked here.
