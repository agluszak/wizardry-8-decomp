from types import SimpleNamespace

from wiz8decomp import reccmp_data
from wiz8decomp.reccmp_data import render_wiz8_data_source


def test_source_identity_wins_when_projecting_reviewed_evidence(tmp_path, monkeypatch) -> None:
    boundary = tmp_path / "config/reccmp/wiz8-gameplay-boundaries.csv"
    boundary.parent.mkdir(parents=True)
    boundary.write_text(
        "address,size,symbol,owner,confidence,relocation_masked_sha256,evidence\n"
        "00401000,16,OldName,wiz8,strong,,fixture\n"
        "00402000,8,LibraryName,msvc6-runtime,exact,,fixture\n",
        encoding="utf-8",
    )
    model = SimpleNamespace(
        functions={
            0x00401000: SimpleNamespace(address=0x00401000, name="SourceName", kind="FUNCTION")
        }
    )
    monkeypatch.setattr(reccmp_data, "build_source_model", lambda _repository: model)
    monkeypatch.setattr(
        reccmp_data,
        "load_claims",
        lambda _repository: [
            {
                "entity_kind": "function",
                "predicate": "accepted-identity",
                "entity_key": "00401000",
                "value": "ClaimName",
                "origin": "descriptive",
            }
        ],
    )

    assert render_wiz8_data_source(tmp_path).splitlines() == [
        "address|symbol|size|type",
        "00401000|SourceName|16|function",
        "00402000|LibraryName|8|library",
    ]
