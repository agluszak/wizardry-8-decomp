package wiz8.exporter;

/** Rendering codec for compiler-owned symbol identities, never decompiler text. */
final class SpecialNames {
	private SpecialNames() {
	}

	static String normalize(String name) {
		String stripped = name.replace("'", "").replace("`", "");
		while (stripped.startsWith("_")) {
			stripped = stripped.substring(1);
		}
		return stripped.replace('_', ' ').trim();
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
