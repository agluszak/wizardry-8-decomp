package wiz8.exporter;

import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionTag;

/** Canonical target and source ownership for one direct call. */
final class CallTargetResolver {
	enum Origin {
		FIRST_PARTY, SURRENDER, SGP, MSVC_RUNTIME, PLATFORM,
		ZLIB, JPEG, INFO_ZIP, IMPORT, UNKNOWN
	}

	record Target(Function referenced, Function canonical, Origin origin,
			boolean importThunk, String evidence) {
	}

	Target resolve(Function referenced) {
		if (referenced == null) {
			return null;
		}
		Function canonical = referenced;
		boolean thunk = false;
		if (referenced.isThunk()) {
			Function resolved = referenced.getThunkedFunction(true);
			if (resolved != null) {
				canonical = resolved;
				thunk = !resolved.equals(referenced);
			}
		}
		Origin tagged = taggedOrigin(referenced);
		if (tagged == Origin.UNKNOWN && canonical != referenced) {
			tagged = taggedOrigin(canonical);
		}
		if (tagged != Origin.UNKNOWN) {
			return new Target(referenced, canonical, tagged, thunk,
				"reviewed Ghidra origin tag");
		}
		if (canonical.isExternal()) {
			String library = canonical.getExternalLocation() == null ? ""
				: canonical.getExternalLocation().getLibraryName();
			return new Target(referenced, canonical, libraryOrigin(library), true,
				"Ghidra external location " + library);
		}
		String identity = (canonical.getParentNamespace().getName(true) + "::" +
			canonical.getName()).toLowerCase();
		Origin origin = identityOrigin(identity);
		return new Target(referenced, canonical, origin, thunk,
			origin == Origin.FIRST_PARTY ? "ordinary program function"
				: "centralized symbol-identity codec");
	}

	private static Origin taggedOrigin(Function function) {
		for (FunctionTag tag : function.getTags()) {
			String name = tag.getName();
			if (!name.startsWith("wiz8:origin:")) {
				continue;
			}
			return switch (name.substring("wiz8:origin:".length())) {
				case "first-party" -> Origin.FIRST_PARTY;
				case "surrender" -> Origin.SURRENDER;
				case "sgp" -> Origin.SGP;
				case "msvc-crt" -> Origin.MSVC_RUNTIME;
				case "zlib" -> Origin.ZLIB;
				case "jpeg" -> Origin.JPEG;
				case "info-zip" -> Origin.INFO_ZIP;
				case "win32", "directx", "mfc", "platform" -> Origin.PLATFORM;
				case "import" -> Origin.IMPORT;
				default -> Origin.UNKNOWN;
			};
		}
		return Origin.UNKNOWN;
	}

	private static Origin libraryOrigin(String library) {
		String name = library == null ? "" : library.toLowerCase();
		if (name.contains("zlib")) return Origin.ZLIB;
		if (name.contains("jpeg")) return Origin.JPEG;
		if (name.contains("zip")) return Origin.INFO_ZIP;
		if (name.contains("sgp")) return Origin.SGP;
		if (name.contains("msv") || name.contains("crt")) return Origin.MSVC_RUNTIME;
		if (name.endsWith(".dll") || !name.isEmpty()) return Origin.PLATFORM;
		return Origin.IMPORT;
	}

	private static Origin identityOrigin(String identity) {
		if (identity.contains("surrender") || identity.contains("::sr")) {
			return Origin.SURRENDER;
		}
		if (identity.contains("sgp")) return Origin.SGP;
		if (identity.contains("zlib")) return Origin.ZLIB;
		if (identity.contains("jpeg")) return Origin.JPEG;
		if (identity.contains("infozip") || identity.contains("info_zip")) {
			return Origin.INFO_ZIP;
		}
		String leaf = identity.substring(identity.lastIndexOf(':') + 1);
		if (leaf.matches("__(?:a?u?ll(?:mul|div|rem|shr)|alloca_probe).*")) {
			return Origin.MSVC_RUNTIME;
		}
		return Origin.FIRST_PARTY;
	}
}
