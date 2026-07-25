from wiz8decomp.ghidra.fid import _is_authoritative_fid_name


def test_fid_name_filter_rejects_unstable_coff_local_labels() -> None:
    assert not _is_authoritative_fid_name("$L264")
    assert not _is_authoritative_fid_name("$L16506")
    assert _is_authoritative_fid_name("__alldiv")
    assert _is_authoritative_fid_name("??_M@YGXPAXIHP6EX0@Z@Z")
