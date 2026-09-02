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


class CommandFailure(RuntimeError):
    def __init__(
        self, result: CommandResult, diagnostics: list[str], log_path: Path | None
    ) -> None:
        lines = [f"command failed ({result.exit_status}): {result.command}", *diagnostics]
        if log_path is not None:
            lines.append(f"log: {log_path}")
        super().__init__("\n".join(lines))
        self.result = result
        self.log_path = log_path


def actionable_diagnostics(stdout: str, stderr: str, *, limit: int = 20) -> list[str]:
    """Keep compiler/test diagnostics and discard repetitive build-tool unwinding."""

    combined = f"{stdout}\n{stderr}".strip()
    useful: list[str] = []
    seen: set[str] = set()
    markers = (
        "error C",
        "fatal error",
        " error:",
        "FAILED:",
        "AssertionError",
        "E   ",
        "failed,",
    )
    for raw in combined.splitlines():
        line = raw.strip()
        if line and any(marker in raw for marker in markers) and line not in seen:
            seen.add(line)
            useful.append(line[-500:])
            if len(useful) == limit:
                break
    if useful:
        return useful
    tail = [line.strip() for line in combined.splitlines() if line.strip()][-10:]
    return tail or ["command produced no diagnostics"]


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
        raise CommandFailure(
            result,
            actionable_diagnostics(result.stdout, result.stderr),
            log_path,
        )
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
