from __future__ import annotations

import os
from pathlib import Path

from dotenv import load_dotenv
from pydantic import BaseModel, ConfigDict, Field, field_validator


REQUIRED_GHIDRA_VERSION = "12.1.2"
REQUIRED_GHIDRA_RELEASE = "PUBLIC"
REQUIRED_PYGHIDRA_VERSION = "3.1.0"


def repository_root() -> Path:
    return Path(__file__).resolve().parents[2]


class Settings(BaseModel):
    model_config = ConfigDict(frozen=True)

    ghidra_install_dir: Path = Field(alias="GHIDRA_INSTALL_DIR")
    input_dir: Path = Field(alias="WIZ8_INPUT_DIR")
    work_dir: Path = Field(alias="WIZ8_WORK_DIR")
    repo_dir: Path = Field(default_factory=repository_root)

    @field_validator("ghidra_install_dir", "input_dir", "work_dir", mode="before")
    @classmethod
    def absolute_path(cls, value: object) -> Path:
        path = Path(str(value)).expanduser()
        if not path.is_absolute():
            raise ValueError(f"must be an absolute path: {path}")
        return path.resolve()

    @property
    def build_dir(self) -> Path:
        return self.repo_dir / "build"

    @property
    def project_dir(self) -> Path:
        return self.work_dir / "ghidra"

    @property
    def project_name(self) -> str:
        return "wizardry8"


def load_settings(*, require: bool = True) -> Settings | None:
    load_dotenv(repository_root() / ".env", override=False)
    keys = ("GHIDRA_INSTALL_DIR", "WIZ8_INPUT_DIR", "WIZ8_WORK_DIR")
    missing = [key for key in keys if not os.environ.get(key)]
    if missing:
        if require:
            raise ValueError("missing environment variables: " + ", ".join(missing))
        return None
    return Settings.model_validate({key: os.environ[key] for key in keys})


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

