import json
from pathlib import Path

from wiz8decomp.config import Settings
from wiz8decomp.ghidra.session import _runtime_path, _WarmSession, _worker_identity


def settings(tmp_path: Path, repo: Path) -> Settings:
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": str(tmp_path / "ghidra"),
            "WIZ8_INPUT_DIR": str(tmp_path / "inputs"),
            "WIZ8_WORK_DIR": str(tmp_path / "work"),
            "WIZ8_GHIDRA_PROJECT_DIR": str(repo / "ghidra-project"),
            "repo_dir": repo,
        }
    )


def test_warm_runtime_identity_is_checkout_and_selector_scoped(tmp_path: Path) -> None:
    first = settings(tmp_path / "one", tmp_path / "one" / "repo")
    second = settings(tmp_path / "two", tmp_path / "two" / "repo")
    first_path, first_token = _runtime_path(first, "wiz8")
    second_path, second_token = _runtime_path(second, "wiz8")
    assert first_path != second_path
    assert first_token != second_token
    assert _worker_identity(first, "wiz8") != _worker_identity(second, "wiz8")


def test_incompatible_or_missing_runtime_metadata_is_cold(tmp_path: Path) -> None:
    current = settings(tmp_path, tmp_path / "repo")
    session = _WarmSession(current, "wiz8")
    session.runtime_path.parent.mkdir(parents=True)
    session.runtime_path.write_text('{"identity": {"repo_dir": "other"}}')
    assert session._metadata() is None


def test_live_worker_is_reused_without_starting_another_process(
    tmp_path: Path, monkeypatch
) -> None:
    current = settings(tmp_path, tmp_path / "repo")
    session = _WarmSession(current, "wiz8")
    session.runtime_path.parent.mkdir(parents=True)
    session.socket_path.touch()
    session.runtime_path.write_text(
        json.dumps(
            {
                "identity": _worker_identity(current, "wiz8"),
                "pid": 123,
                "socket": str(session.socket_path),
            }
        )
    )
    monkeypatch.setattr(session, "_metadata", lambda: {"pid": 123})
    monkeypatch.setattr(session, "_start", lambda: (_ for _ in ()).throw(AssertionError()))

    class Connection:
        def __enter__(self):
            return self

        def __exit__(self, *_args):
            return None

        def settimeout(self, _value):
            pass

        def connect(self, _path):
            pass

        def sendall(self, _data):
            pass

        def recv(self, _size):
            return b'{"results": [], "transport": "pyghidra-warm"}\n'

    monkeypatch.setattr("socket.socket", lambda *_args, **_kwargs: Connection())
    assert session.request({"queries": []})[1] == "pyghidra-warm"


def test_deleting_runtime_state_does_not_delete_project_evidence(tmp_path: Path) -> None:
    evidence = tmp_path / "ghidra-project" / "function.gzf"
    evidence.parent.mkdir(parents=True)
    evidence.write_text("reviewed")
    cache = tmp_path / "build" / "ghidra"
    cache.mkdir(parents=True)
    (cache / "warm-session.json").write_text("runtime")
    for path in cache.iterdir():
        path.unlink()
    assert evidence.read_text() == "reviewed"
