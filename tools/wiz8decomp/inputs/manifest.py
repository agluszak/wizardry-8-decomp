from __future__ import annotations

from typing import Any, Literal

from pydantic import BaseModel, ConfigDict, Field


class Evidence(BaseModel):
    model_config = ConfigDict(extra="forbid")
    kind: str
    value: str


class InputRecord(BaseModel):
    model_config = ConfigDict(extra="forbid")
    relative_path: str
    size: int
    sha256: str
    detected_type: str
    extension_type_mismatch: bool = False
    container_technology: str | None = None
    installer_technology: str | None = None
    configured_role: str | None = None
    role_source: Literal["local-config", "unassigned"] = "unassigned"
    extraction_tool: str | None = None
    extraction_tool_version: str | None = None
    evidence: list[Evidence] = Field(default_factory=list)


class InputManifest(BaseModel):
    model_config = ConfigDict(extra="forbid", populate_by_name=True)
    schema_id: Literal["wiz8.input-manifest"] = Field("wiz8.input-manifest", alias="schema")
    format_version: int = 1
    input_root_label: str = "WIZ8_INPUT_DIR"
    files: list[InputRecord]
    ambiguities: list[dict[str, Any]] = Field(default_factory=list)
