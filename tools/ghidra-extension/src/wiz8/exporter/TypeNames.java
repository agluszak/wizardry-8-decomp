package wiz8.exporter;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Maps Ghidra's data-type spellings onto the repository's C++ spellings.
 *
 * Ghidra cannot store template syntax, so the reviewed project encodes it:
 * {@code W8GrowableVector[stLight_#]} means {@code W8GrowableVector<stLight*>}.
 * Bracket lists nest and separate arguments with commas; a {@code _#} suffix
 * marks one level of pointer indirection; multi-word primitive names join
 * their words with underscores. Independent of that encoding, Ghidra's own
 * primitive aliases ({@code uint}, {@code undefined4}, {@code float10}, ...)
 * are spelled out as VC6 C++ types.
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
		Map.entry("undefined", "unsigned char"),
		Map.entry("undefined1", "unsigned char"),
		Map.entry("undefined2", "unsigned short"),
		Map.entry("undefined4", "unsigned int"),
		Map.entry("undefined8", "unsigned __int64"),
		Map.entry("float10", "long double"),
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
		if (text.endsWith("_#")) {
			return mapType(text.substring(0, text.length() - 2)) + "*";
		}
		int open = text.indexOf('[');
		if (open > 0 && text.endsWith("]")) {
			String name = text.substring(0, open);
			List<String> arguments = splitTopLevel(text.substring(open + 1, text.length() - 1));
			StringBuilder mapped = new StringBuilder(mapName(name)).append('<');
			for (int i = 0; i < arguments.size(); i++) {
				if (i > 0) {
					mapped.append(", ");
				}
				mapped.append(mapType(arguments.get(i).trim()));
			}
			return mapped.append('>').toString();
		}
		return mapName(text);
	}

	private static String mapName(String name) {
		return PRIMITIVES.getOrDefault(name, name);
	}

	private static List<String> splitTopLevel(String text) {
		List<String> parts = new ArrayList<>();
		int depth = 0;
		int start = 0;
		for (int i = 0; i < text.length(); i++) {
			char c = text.charAt(i);
			if (c == '[') {
				depth++;
			}
			else if (c == ']') {
				depth--;
			}
			else if (c == ',' && depth == 0) {
				parts.add(text.substring(start, i));
				start = i + 1;
			}
		}
		parts.add(text.substring(start));
		return parts;
	}
}
