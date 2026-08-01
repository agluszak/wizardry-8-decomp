package wiz8.exporter;

import java.util.Map;

import ghidra.app.decompiler.ClangTypeToken;
import ghidra.program.model.data.DataType;

/**
 * Maps Ghidra's data-type spellings onto the repository's C++ spellings.
 *
 * Ghidra cannot store template syntax, so the reviewed project encodes it:
 * {@code W8GrowableVector[stLight_#]} means {@code W8GrowableVector<stLight*>}.
 * Bracket lists nest and separate arguments with commas; a {@code _#} suffix
 * marks one level of pointer indirection; multi-word primitive names join
 * their words with underscores. Independent of that encoding, Ghidra's own
	 * primitive aliases ({@code uint}, etc.) are spelled out as VC6 C++ types.
	 * Unknown-width storage and {@code float10} are handled structurally by
	 * {@link CxxTypePrinter}; this string codec never invents signedness or a
	 * target representation for them.
 *
 * {@code code} is deliberately not mapped: a surviving {@code code} token
 * marks an indirect call the recognizers declined, and it must stay visible
 * as exactly that.
 */
final class TypeNames {

	private static final Map<String, String> PRIMITIVES = Map.ofEntries(
		Map.entry("uint", "unsigned int"),
		Map.entry("ushort", "unsigned short"),
		Map.entry("uchar", "unsigned char"),
		Map.entry("ulong", "unsigned long"),
		Map.entry("byte", "unsigned char"),
		Map.entry("sbyte", "signed char"),
		Map.entry("longlong", "__int64"),
		Map.entry("ulonglong", "unsigned __int64"),
		Map.entry("unsigned_char", "unsigned char"),
		Map.entry("unsigned_short", "unsigned short"),
		Map.entry("unsigned_int", "unsigned int"),
		Map.entry("unsigned_long", "unsigned long"),
		Map.entry("signed_char", "signed char"));

	private TypeNames() {
	}

	static String map(String text) {
		if (text == null) {
			return null;
		}
		String trimmed = text.trim();
		if (trimmed.isEmpty()) {
			return text;
		}
		return mapType(trimmed);
	}

	/** Preserve a namespace encoded structurally in a demangler data-type path. */
	static String mapToken(ClangTypeToken token) {
		String rendered = map(token.getText());
		String qualified = qualifiedBase(token.getDataType(), rendered);
		return qualified == null ? rendered : qualified;
	}

	static String qualifiedBase(DataType type, String rendered) {
		if (type == null || rendered == null) {
			return null;
		}
		String category = type.getCategoryPath().getPath();
		String prefix = "/Demangler/";
		if (!category.startsWith(prefix) || category.length() == prefix.length()) {
			return null;
		}
		String namespace = category.substring(prefix.length()).replace("/", "::");
		return namespace + "::" + rendered;
	}

	/**
	 * Map a token only when it is one identifier carrying the project's
	 * bracket-template encoding, e.g. a {@code W8GrowableVector[int]}
	 * qualifier the decompiler emits as plain syntax text. Anything else
	 * passes through untouched.
	 */
	static String mapTemplateSpelling(String text) {
		if (text == null || text.isEmpty() || !text.endsWith("]")) {
			return text;
		}
		int open = text.indexOf('[');
		if (open <= 0) {
			return text;
		}
		for (int i = 0; i < open; i++) {
			char c = text.charAt(i);
			if (!Character.isJavaIdentifierPart(c)) {
				return text;
			}
		}
		return mapType(text);
	}

	private static String mapType(String text) {
		return TemplateNameCodec.encoded(text) ? TemplateNameCodec.decode(text)
			: mapPrimitive(text);
	}

	static String mapPrimitive(String name) {
		return PRIMITIVES.getOrDefault(name, name);
	}
}
