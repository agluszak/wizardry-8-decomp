from pathlib import Path

from wiz8decomp.linkage_lint import c_linkage_violations
from wiz8decomp.source_text import mask_cpp_noise, source_files


def test_mask_preserves_offsets_and_ignores_comment_tokens_in_literals():
    text = 'int first; /* }\nextern "C" */\nconst char* s = R"tag( // "\n/* } */ )tag";\nint last;'
    masked = mask_cpp_noise(text)
    assert len(masked) == len(text)
    assert [i for i, c in enumerate(masked) if c == "\n"] == [
        i for i, c in enumerate(text) if c == "\n"
    ]
    assert masked.index("int last") == text.index("int last")
    assert "extern" not in masked and "}" not in masked
    assert 'R"tag(' in mask_cpp_noise(text, strings=False)


def test_shared_walk_and_linkage_check_ignore_comments_and_strings(tmp_path: Path):
    root = tmp_path / "include/wiz8"
    root.mkdir(parents=True)
    (root / "test.H").write_text(
        '/* extern "C" int fake; */\n'
        'const char* s = R"(extern "C" int fake;)";\n'
        'extern "C" int actual;\n'
    )
    assert source_files(tmp_path) == [root / "test.H"]
    assert c_linkage_violations(tmp_path) == [{"file": "include/wiz8/test.H", "line": 3}]
