package wiz8.recovery;

import java.util.LinkedHashMap;
import java.util.Map;

import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionTag;

/** Single parser for reviewed function metadata stored in Ghidra tags. */
final class ReviewedFunctionMetadata {
	private static final String ROLE_PREFIX = "wiz8:role:";
	private static final String SOURCE_PREFIX = "wiz8:source:";
	private static final String ORIGIN_PREFIX = "wiz8:origin:";
	private static final Map<String, EmissionKind> ROLE_TAGS = new LinkedHashMap<>();
	private static final Map<String, SourceKind> SOURCE_TAGS = new LinkedHashMap<>();

	static {
		for (EmissionKind kind : EmissionKind.values()) {
			ROLE_TAGS.put(kebab(kind.name()), kind);
		}
		for (SourceKind kind : SourceKind.values()) {
			SOURCE_TAGS.put(kebab(kind.name()), kind);
		}
	}

	private ReviewedFunctionMetadata() { }

	static Metadata read(Function function) {
		SourceKind source = null;
		EmissionKind emission = null;
		Origin origin = null;
		for (FunctionTag tag : function.getTags()) {
			String name = tag.getName();
			if (name.startsWith(ROLE_PREFIX)) {
				emission = unique(emission, ROLE_TAGS.get(name.substring(ROLE_PREFIX.length())),
					"function role", name, function);
			}
			else if (name.startsWith(SOURCE_PREFIX)) {
				source = unique(source, SOURCE_TAGS.get(name.substring(SOURCE_PREFIX.length())),
					"source role", name, function);
			}
			else if (name.startsWith(ORIGIN_PREFIX)) {
				Origin parsed = parseOrigin(name.substring(ORIGIN_PREFIX.length()));
				origin = unique(origin, parsed, "function origin", name, function);
			}
		}
		return new Metadata(source, emission, origin == null ? Origin.UNKNOWN : origin);
	}

	private static <T> T unique(T current, T parsed, String kind, String tag,
			Function function) {
		if (parsed == null) {
			throw new IllegalStateException("unknown " + kind + " tag " + tag +
				" on " + function.getName(true));
		}
		if (current != null && !current.equals(parsed)) {
			throw new IllegalStateException("conflicting " + kind + " tags on " +
				function.getName(true));
		}
		return parsed;
	}

	private static Origin parseOrigin(String value) {
		return switch (value) {
			case "first-party" -> Origin.FIRST_PARTY;
			case "library" -> Origin.LIBRARY;
			case "surrender" -> Origin.SURRENDER;
			case "sgp" -> Origin.SGP;
			case "msvc-crt" -> Origin.MSVC_RUNTIME;
			case "zlib" -> Origin.ZLIB;
			case "jpeg" -> Origin.JPEG;
			case "info-zip" -> Origin.INFO_ZIP;
			case "win32", "directx", "mfc", "platform" -> Origin.PLATFORM;
			case "import" -> Origin.IMPORT;
			case "unknown" -> Origin.UNKNOWN;
			default -> null;
		};
	}

	private static String kebab(String name) {
		return name.toLowerCase().replace('_', '-');
	}

	record Metadata(SourceKind source, EmissionKind emission, Origin origin) { }
}
