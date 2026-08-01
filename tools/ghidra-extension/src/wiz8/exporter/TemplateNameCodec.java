package wiz8.exporter;

import java.util.ArrayList;
import java.util.List;

/** The one parser for the repository's Ghidra template-name encoding. */
final class TemplateNameCodec {

	private TemplateNameCodec() {
	}

	static boolean encoded(String text) {
		return text != null && text.indexOf('[') > 0 && text.endsWith("]");
	}

	static String decode(String text) {
		int open = text.indexOf('[');
		if (open <= 0 || !text.endsWith("]")) {
			return TypeNames.mapPrimitive(text);
		}
		List<String> arguments = split(text.substring(open + 1, text.length() - 1));
		StringBuilder mapped = new StringBuilder(TypeNames.mapPrimitive(text.substring(0, open)))
				.append('<');
		for (int i = 0; i < arguments.size(); i++) {
			if (i > 0) {
				mapped.append(',');
			}
			mapped.append(decodeArgument(arguments.get(i).trim()));
		}
		return mapped.append('>').toString();
	}

	private static String decodeArgument(String text) {
		if (text.endsWith("_#")) {
			return decodeArgument(text.substring(0, text.length() - 2)) + "*";
		}
		return encoded(text) ? decode(text) : TypeNames.mapPrimitive(text);
	}

	private static List<String> split(String text) {
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
