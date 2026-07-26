from __future__ import annotations

import hashlib
import os
import re
import tempfile
from pathlib import Path

from dotenv import load_dotenv
from pydantic import BaseModel, ConfigDict, Field, field_validator

REQUIRED_GHIDRA_VERSION = "12.1.2"
REQUIRED_GHIDRA_RELEASE = "PUBLIC"
REQUIRED_PYGHIDRA_VERSION = "3.1.0"


class GhidraAgentIdMissing(RuntimeError):
    """No agent identity is configured, so a project cannot be isolated."""


def ghidra_agent_id() -> str:
    """The identity that separates one agent's Ghidra project from another's.

    There is deliberately no fallback. A default would silently place two
    checkouts in the same per-agent project root, which is exactly the
    collision this identity exists to prevent: the second agent then contends
    for - or blocks on - a lock it has no reason to touch. Failing here is
    loud, immediate, and fixed by one line in .env.
    """

    raw = os.environ.get("WIZ8_GHIDRA_AGENT_ID") or os.environ.get("CODEX_THREAD_ID") or ""
    cleaned = re.sub(r"[^A-Za-z0-9_.-]+", "-", raw).strip("-.")
    if not cleaned:
        raise GhidraAgentIdMissing(
            "no Ghidra agent identity: set WIZ8_GHIDRA_AGENT_ID in .env to something "
            "unique to this checkout, for example its directory name. Without it two "
            "checkouts share one per-agent project and collide on its lock."
        )
    return cleaned[:96]


def ghidra_agent_token(work_dir: Path) -> str:
    identity = f"{work_dir.resolve()}\0{ghidra_agent_id()}"
    return hashlib.sha256(identity.encode("utf-8")).hexdigest()[:16]


def repository_root() -> Path:
    return Path(__file__).resolve().parents[2]


class Settings(BaseModel):
    model_config = ConfigDict(frozen=True)

    ghidra_install_dir: Path = Field(alias="GHIDRA_INSTALL_DIR")
    input_dir: Path = Field(alias="WIZ8_INPUT_DIR")
    work_dir: Path = Field(alias="WIZ8_WORK_DIR")
    ghidra_project_dir_override: Path | None = Field(default=None, alias="WIZ8_GHIDRA_PROJECT_DIR")
    ghidra_runtime_dir_override: Path | None = Field(default=None, alias="WIZ8_GHIDRA_RUNTIME_DIR")
    repo_dir: Path = Field(default_factory=repository_root)

    @field_validator(
        "ghidra_install_dir",
        "input_dir",
        "work_dir",
        "ghidra_project_dir_override",
        "ghidra_runtime_dir_override",
        mode="before",
    )
    @classmethod
    def absolute_path(cls, value: object) -> Path | None:
        if value is None:
            return value
        path = Path(str(value)).expanduser()
        if not path.is_absolute():
            raise ValueError(f"must be an absolute path: {path}")
        return path.resolve()

    @property
    def build_dir(self) -> Path:
        return self.repo_dir / "build"

    @property
    def project_dir(self) -> Path:
        return self.ghidra_project_dir_override or self.work_dir / "ghidra"

    @property
    def project_name(self) -> str:
        return "wizardry8"

    @property
    def ghidra_runtime_dir(self) -> Path:
        return self.ghidra_runtime_dir_override or (
            Path(tempfile.gettempdir()) / f"wiz8-ghidra-{ghidra_agent_token(self.work_dir)}"
        )


def load_settings(*, require: bool = True) -> Settings | None:
    load_dotenv(repository_root() / ".env", override=False)
    required_keys = ("GHIDRA_INSTALL_DIR", "WIZ8_INPUT_DIR", "WIZ8_WORK_DIR")
    missing = [key for key in required_keys if not os.environ.get(key)]
    if missing:
        if require:
            raise ValueError("missing environment variables: " + ", ".join(missing))
        return None
    optional_keys = ("WIZ8_GHIDRA_PROJECT_DIR", "WIZ8_GHIDRA_RUNTIME_DIR")
    values = {key: os.environ[key] for key in required_keys}
    values.update({key: os.environ[key] for key in optional_keys if os.environ.get(key)})
    return Settings.model_validate(values)


def ghidra_version(install_dir: Path) -> tuple[str | None, str | None]:
    properties = install_dir / "Ghidra" / "application.properties"
    if not properties.is_file():
        return None, None
    values: dict[str, str] = {}
    for line in properties.read_text(encoding="utf-8", errors="replace").splitlines():
        if "=" in line and not line.lstrip().startswith("#"):
            key, value = line.split("=", 1)
            values[key.strip()] = value.strip()
    return values.get("application.version"), values.get("application.release.name")
