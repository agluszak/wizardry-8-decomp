"""Live-project commands must share one checkout-scoped Ghidra owner."""

from __future__ import annotations

import subprocess
import threading
import time
from pathlib import Path

import pytest
from wiz8decomp.config import load_settings


@pytest.mark.integration
def test_context_recovery_and_class_report_use_short_lived_owners() -> None:
    settings = load_settings()
    assert settings is not None
    commands = [
        ["just", "context", "0x005CF300"],
        ["just", "recover-explain", "0x005CF250:0x005CF5FF"],
        ["just", "context", "0x005CF580"],
        ["just", "report", "class", "W8DialogInterface"],
    ]
    observed_headless: list[str] = []
    stop = threading.Event()

    def watch_processes() -> None:
        while not stop.is_set():
            for command_line in Path("/proc").glob("[0-9]*/cmdline"):
                try:
                    command = (
                        command_line.read_bytes().replace(b"\0", b" ").decode(errors="replace")
                    )
                except OSError:
                    continue
                if (
                    "analyzeHeadless" in command
                    and " -process " in command
                    and "wizardry8" in command
                ):
                    observed_headless.append(command)
            time.sleep(0.01)

    watcher = threading.Thread(target=watch_processes, daemon=True)
    watcher.start()
    try:
        for command in commands:
            result = subprocess.run(
                command,
                cwd=settings.repo_dir,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
            )
            assert result.returncode == 0, result.stdout
            folded = result.stdout.casefold()
            assert "project lock" not in folded and "project is locked" not in folded
            processes = subprocess.run(
                ["ps", "-eo", "args="], text=True, capture_output=True, check=True
            ).stdout
            assert "wiz8decomp.ghidra.session --worker" not in processes
    finally:
        stop.set()
        watcher.join(timeout=1)
    assert not observed_headless


@pytest.mark.integration
def test_concurrent_commands_serialize_project_ownership() -> None:
    settings = load_settings()
    assert settings is not None
    processes = [
        subprocess.Popen(
            ["just", "context", address],
            cwd=settings.repo_dir,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        for address in ("0x005CF300", "0x005CF580")
    ]
    results = [(process.wait(), process.stdout.read()) for process in processes if process.stdout]
    assert len(results) == 2
    for returncode, output in results:
        assert returncode == 0, output
        assert "project lock" not in output.casefold()
