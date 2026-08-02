package wiz8.recovery;

/** Rendering codec for compiler-owned symbol identities, never decompiler text. */
final class SpecialNames {
	private SpecialNames() {
	}

	static String normalize(String name) {
		String stripped = name.replace("'", "").replace("`", "");
		while (stripped.startsWith("_")) {
			stripped = stripped.substring(1);
		}
		if (stripped.startsWith("operator_new")) {
			return "operator new" + stripped.substring("operator_new".length());
		}
		if (stripped.startsWith("operator_delete")) {
			return "operator delete" + stripped.substring("operator_delete".length());
		}
		String[] encoded = {
			"scalar_deleting_destructor", "vector_deleting_destructor",
			"vbase_destructor", "default_constructor_closure",
			"vector_constructor_iterator", "vector_destructor_iterator",
			"array_unwind", "local_static_guard"
		};
		for (String prefix : encoded) {
			if (stripped.startsWith(prefix)) {
				return prefix.replace('_', ' ') + stripped.substring(prefix.length());
			}
		}
		return stripped.trim();
	}

	static String decorate(String name) {
		String normalized = normalize(name);
		int adjustor = normalized.indexOf("adjustor{");
		if (adjustor >= 0) {
			return "`" + normalized.substring(0, adjustor).stripTrailing() +
				"'`" + normalized.substring(adjustor).strip() + "'";
		}
		return "`" + normalized + "'";
	}
}
