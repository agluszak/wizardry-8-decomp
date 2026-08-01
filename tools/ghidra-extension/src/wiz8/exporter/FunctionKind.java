package wiz8.exporter;

import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.symbol.Namespace;

/**
 * Classification of a selected function for source-recovery printing.
 *
 * The classification uses only live Ghidra state: the function's symbol name
 * and its owning namespace. A function counts as class-owned only when its
 * parent namespace is a real {@link GhidraClass}.
 */
public enum FunctionKind {
	ORDINARY,
	CONSTRUCTOR,
	DESTRUCTOR,
	/** Compiler-generated scalar/vector deleting destructor: marker only, never a body. */
	SYNTHETIC_DELETING_DESTRUCTOR;

	public static FunctionKind classify(Function function) {
		Namespace namespace = function.getParentNamespace();
		if (!(namespace instanceof GhidraClass)) {
			return ORDINARY;
		}
		String name = function.getName();
		String normalized = normalizeSpecialName(name);
		if (normalized.equals("scalar deleting destructor") ||
			normalized.equals("vector deleting destructor")) {
			return SYNTHETIC_DELETING_DESTRUCTOR;
		}
		String owner = CallableIdentity.classLeaf(namespace.getName());
		if (CallableIdentity.classLeaf(name).equals(owner)) {
			return CONSTRUCTOR;
		}
		if (name.equals("~" + owner) || name.equals("~" + namespace.getName())) {
			return DESTRUCTOR;
		}
		return ORDINARY;
	}

	/**
	 * Normalize the project's spellings of compiler-generated names. The reviewed
	 * project stores {@code 'scalar_deleting_destructor'}; demangler output can
	 * also produce backtick-quoted, space-separated forms.
	 */
	public static String normalizeSpecialName(String name) {
		String stripped = name.replace("'", "").replace("`", "");
		while (stripped.startsWith("_")) {
			stripped = stripped.substring(1);
		}
		return stripped.replace('_', ' ').trim();
	}

	/** The repository's decorated rendering, e.g. {@code `scalar deleting destructor'}. */
	public static String decorateSpecialName(String name) {
		return "`" + normalizeSpecialName(name) + "'";
	}
}
