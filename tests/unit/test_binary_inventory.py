from wiz8decomp.binary.inventory import classify_module


def test_classify_surrender_dll_families() -> None:
    expected = {
        "srDD_OpenGL.dll": "renderer",
        "srEXT_AVI.dll": "renderer-extension",
        "srHXImporter.dll": "renderer-importer",
        "srVP_KNI.dll": "renderer-vector-processor",
    }

    for name, classification in expected.items():
        actual, evidence = classify_module({"module_name": name, "relative_path": f"Dll/{name}"})
        assert actual == classification
        assert evidence
