from __future__ import annotations

import logging
import os
import shlex
import shutil
import subprocess
from collections.abc import Mapping, Sequence
from dataclasses import asdict, dataclass
from datetime import UTC, datetime
from pathlib import Path

from .paths import atomic_json

LOG = logging.getLogger(__name__)


@dataclass(frozen=True)
class CommandResult:
    argv: list[str]
    executable: str
    cwd: str
    exit_status: int
    stdout: str
    stderr: str
    timestamp_utc: str

    @property
    def command(self) -> str:
        return shlex.join(self.argv)


def resolve_executable(name: str) -> str | None:
    found = shutil.which(name)
    return str(Path(found).resolve()) if found else None


def run(
    argv: Sequence[str | os.PathLike[str]],
    *,
    cwd: Path,
    log_path: Path | None = None,
    env: Mapping[str, str] | None = None,
    check: bool = True,
) -> CommandResult:
    args = [os.fspath(arg) for arg in argv]
    executable = resolve_executable(args[0]) or args[0]
    LOG.debug("running %s", shlex.join(args))
    completed = subprocess.run(
        args,
        cwd=cwd,
        env=dict(env) if env else None,
        text=True,
        encoding="utf-8",
        errors="replace",
        capture_output=True,
        check=False,
    )
    result = CommandResult(
        argv=args,
        executable=executable,
        cwd=str(cwd.resolve()),
        exit_status=completed.returncode,
        stdout=completed.stdout,
        stderr=completed.stderr,
        timestamp_utc=datetime.now(UTC).isoformat(),
    )
    if log_path:
        atomic_json(log_path, asdict(result))
    if check and completed.returncode:
        detail = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(f"command failed ({completed.returncode}): {result.command}\n{detail}")
    return result


def tool_version(name: str, args: Sequence[str] = ("--version",)) -> dict[str, str | None]:
    executable = resolve_executable(name)
    if not executable:
        return {"executable": None, "version": None}
    completed = subprocess.run(
        [executable, *args], capture_output=True, text=True, errors="replace", check=False
    )
    text = (completed.stdout + "\n" + completed.stderr).strip().splitlines()
    return {"executable": executable, "version": text[0].strip() if text else "unknown"}
