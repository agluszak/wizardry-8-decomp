package wiz8.exporter;

import java.util.IdentityHashMap;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;

import ghidra.app.util.demangler.DemangledObject;
import ghidra.app.util.demangler.microsoft.MicrosoftDemangler;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionTag;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Program;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.Symbol;

/**
 * Sole Java authority for mapping a Ghidra function onto a source entity and
 * a compiler emission. Reviewed tags win; Microsoft decorated symbols and
 * Ghidra thunk metadata are structural fallbacks. Displayed special names are
 * accepted only at this one compatibility boundary for projects not yet
 * retagged by source synchronization.
 */
public final class FunctionRoleResolver {
	private static final String ROLE_PREFIX = "wiz8:role:";
	private static final String SOURCE_PREFIX = "wiz8:source:";
	private static final String ORIGIN_PREFIX = "wiz8:origin:";

	private static final Map<String, EmissionKind> ROLE_TAGS = new LinkedHashMap<>();
	private static final Map<String, SourceKind> SOURCE_TAGS = new LinkedHashMap<>();
	private static final Set<String> ORIGIN_TAGS = Set.of("first-party", "library",
		"surrender", "sgp", "msvc-crt", "zlib", "jpeg", "info-zip", "win32",
		"directx", "mfc", "platform", "import", "unknown");
	private final Map<Function, FunctionRole> cache = new IdentityHashMap<>();
	private final Set<Function> resolving = java.util.Collections.newSetFromMap(
		new IdentityHashMap<>());
	static {
		for (EmissionKind kind : EmissionKind.values()) {
			ROLE_TAGS.put(kebab(kind.name()), kind);
		}
		for (SourceKind kind : SourceKind.values()) {
			SOURCE_TAGS.put(kebab(kind.name()), kind);
		}
	}

	FunctionRoleResolver() {
	}

	FunctionRole resolve(Function function) {
		FunctionRole cached = cache.get(function);
		if (cached != null) {
			return cached;
		}
		if (!resolving.add(function)) {
			return ordinaryRole(function, "role recursion guard");
		}
		try {
			FunctionRole role = resolveUncached(function);
			cache.put(function, role);
			return role;
		}
		finally {
			resolving.remove(function);
		}
	}

	private FunctionRole resolveUncached(Function function) {
		Tagged tagged = tags(function);
		if (tagged.emission != null || tagged.source != null) {
			EmissionKind emission = tagged.emission != null ? tagged.emission
				: defaultEmission(function, tagged.source);
			SourceKind source = tagged.source != null ? tagged.source
				: sourceFor(function, emission);
			return role(function, source, emission, tagged.origin, "reviewed Ghidra tag");
		}

		for (Symbol symbol : function.getProgram().getSymbolTable()
				.getSymbols(function.getEntryPoint())) {
			String name = symbol.getName();
			if (!name.startsWith("?")) {
				continue;
			}
			FunctionRole demangled = fromDecorated(function, name);
			if (demangled != null) {
				return demangled;
			}
		}

		FunctionRole named = fromSpecialIdentity(function, function.getName(),
			"centralized Ghidra special-name fallback");
		if (named != null) {
			return named;
		}
		if (function.isThunk()) {
			Function canonical = function.getThunkedFunction(true);
			FunctionRole target = canonical == null || canonical.equals(function) ? null
				: resolve(canonical);
			SourceKind source = target == null ? SourceKind.NONE : target.sourceKind();
			String origin = target == null ? "unknown" : target.origin();
			return role(function, source, EmissionKind.ADJUSTOR_THUNK, canonical,
				true, origin, "Ghidra thunk metadata");
		}

		if (function.isExternal()) {
			return role(function, SourceKind.LIBRARY_ENTITY, EmissionKind.IMPORT_THUNK,
				"import", "Ghidra external-function metadata");
		}

		Namespace namespace = function.getParentNamespace();
		if (namespace instanceof GhidraClass) {
			String owner = CallableIdentity.classLeaf(namespace.getName());
			String name = CallableIdentity.classLeaf(function.getName());
			if (name.equals(owner)) {
				return role(function, SourceKind.CONSTRUCTOR, EmissionKind.AUTHORED_BODY,
					"first-party", "class namespace identity");
			}
			if (function.getName().equals("~" + owner) ||
				function.getName().equals("~" + namespace.getName())) {
				return role(function, SourceKind.DESTRUCTOR, EmissionKind.AUTHORED_BODY,
					"first-party", "class namespace identity");
			}
			return role(function, SourceKind.MEMBER_FUNCTION, EmissionKind.AUTHORED_BODY,
				"first-party", "class namespace membership");
		}
		return role(function, SourceKind.FREE_FUNCTION, EmissionKind.AUTHORED_BODY,
			"first-party", "ordinary Ghidra function");
	}

	private static FunctionRole fromDecorated(Function function, String decorated) {
		try {
			MicrosoftDemangler demangler = new MicrosoftDemangler();
			var options = demangler.createDefaultOptions();
			var context = demangler.createMangledContext(decorated, options,
				function.getProgram(), function.getEntryPoint());
			DemangledObject object = demangler.demangle(context);
			if (object == null) {
				return null;
			}
			FunctionRole special = fromSpecialIdentity(function, object.getName(),
				"Microsoft demangler: " + decorated);
			if (special != null) {
				return special;
			}
			if (object.isThunk()) {
				return role(function, sourceForNamespace(function),
					EmissionKind.ADJUSTOR_THUNK, function.getThunkedFunction(true), true,
					"first-party", "Microsoft demangler thunk: " + decorated);
			}
		}
		catch (Exception ignored) {
			// The symbol remains usable as an identity even when this demangler
			// version declines it; fall through to reviewed Ghidra metadata.
		}
		return null;
	}

	private static FunctionRole fromSpecialIdentity(Function function, String spelling,
			String evidence) {
		String name = SpecialNames.normalize(spelling);
		boolean adjustor = name.contains("adjustor{");
		if (name.startsWith("scalar deleting destructor")) {
			return role(function, SourceKind.DESTRUCTOR,
				EmissionKind.SCALAR_DELETING_DESTRUCTOR, function, adjustor,
				"first-party", evidence);
		}
		if (name.startsWith("vector deleting destructor")) {
			return role(function, SourceKind.DESTRUCTOR,
				EmissionKind.VECTOR_DELETING_DESTRUCTOR, function, adjustor,
				"first-party", evidence);
		}
		if (name.startsWith("vbase destructor")) {
			return role(function, SourceKind.DESTRUCTOR, EmissionKind.VBASE_DESTRUCTOR,
				"first-party", evidence);
		}
		if (name.startsWith("default constructor closure")) {
			return role(function, SourceKind.CONSTRUCTOR,
				EmissionKind.DEFAULT_CONSTRUCTOR_CLOSURE, "first-party", evidence);
		}
		if (name.startsWith("vector constructor iterator")) {
			return role(function, SourceKind.LIBRARY_ENTITY,
				EmissionKind.VECTOR_CONSTRUCTOR_ITERATOR, "msvc-crt", evidence);
		}
		if (name.startsWith("vector destructor iterator")) {
			return role(function, SourceKind.LIBRARY_ENTITY,
				EmissionKind.VECTOR_DESTRUCTOR_ITERATOR, "msvc-crt", evidence);
		}
		if (name.startsWith("array unwind")) {
			return role(function, SourceKind.LIBRARY_ENTITY, EmissionKind.ARRAY_UNWIND,
				"msvc-crt", evidence);
		}
		if (name.startsWith("local static guard")) {
			return role(function, SourceKind.NONE, EmissionKind.LOCAL_STATIC_GUARD,
				"compiler", evidence);
		}
		return null;
	}

	private static FunctionRole role(Function function, SourceKind source,
			EmissionKind emission, String origin, String evidence) {
		return role(function, source, emission, function, function.isThunk(), origin, evidence);
	}

	private static FunctionRole role(Function function, SourceKind source,
			EmissionKind emission, Function canonical, boolean adjustor, String origin,
			String evidence) {
		Function bodyOwner = emission == EmissionKind.AUTHORED_BODY ||
			emission == EmissionKind.CONSTRUCTOR_BODY ? function : null;
		return new FunctionRole(SourceEntityKey.of(function, canonical, source), emission,
			bodyOwner, canonical, adjustor, origin, evidence);
	}

	private static FunctionRole ordinaryRole(Function function, String evidence) {
		SourceKind source = sourceForNamespace(function);
		return role(function, source, EmissionKind.AUTHORED_BODY, "unknown", evidence);
	}

	private static SourceKind sourceForNamespace(Function function) {
		return function.getParentNamespace() instanceof GhidraClass
			? SourceKind.MEMBER_FUNCTION : SourceKind.FREE_FUNCTION;
	}

	private static SourceKind sourceFor(Function function, EmissionKind emission) {
		return switch (emission) {
			case CONSTRUCTOR_BODY, BASE_CONSTRUCTOR, COMPLETE_CONSTRUCTOR,
				VBASE_CONSTRUCTOR, DEFAULT_CONSTRUCTOR_CLOSURE -> SourceKind.CONSTRUCTOR;
			case BASE_DESTRUCTOR, COMPLETE_DESTRUCTOR, SCALAR_DELETING_DESTRUCTOR,
				VECTOR_DELETING_DESTRUCTOR, VBASE_DESTRUCTOR -> SourceKind.DESTRUCTOR;
			case TEMPLATE_EMISSION -> SourceKind.TEMPLATE_MEMBER;
			case IMPORT_THUNK, LIBRARY_BODY, VECTOR_CONSTRUCTOR_ITERATOR,
				VECTOR_DESTRUCTOR_ITERATOR, ARRAY_UNWIND -> SourceKind.LIBRARY_ENTITY;
			default -> sourceForNamespace(function);
		};
	}

	private static EmissionKind defaultEmission(Function function, SourceKind source) {
		return switch (source) {
			case CONSTRUCTOR, DESTRUCTOR -> EmissionKind.AUTHORED_BODY;
			case TEMPLATE_MEMBER -> EmissionKind.TEMPLATE_EMISSION;
			case LIBRARY_ENTITY -> EmissionKind.LIBRARY_BODY;
			default -> function.isThunk() ? EmissionKind.ADJUSTOR_THUNK
				: EmissionKind.AUTHORED_BODY;
		};
	}

	private static Tagged tags(Function function) {
		SourceKind source = null;
		EmissionKind emission = null;
		String origin = null;
		for (FunctionTag tag : function.getTags()) {
			String name = tag.getName();
			if (name.startsWith(ROLE_PREFIX)) {
				String value = name.substring(ROLE_PREFIX.length());
				EmissionKind parsed = ROLE_TAGS.get(value);
				if (parsed == null) {
					throw new IllegalStateException("unknown function role tag " + name +
						" on " + function.getName(true));
				}
				if (emission != null && emission != parsed) {
					throw new IllegalStateException("conflicting function role tags on " +
						function.getName(true));
				}
				emission = parsed;
			}
			else if (name.startsWith(SOURCE_PREFIX)) {
				String value = name.substring(SOURCE_PREFIX.length());
				SourceKind parsed = SOURCE_TAGS.get(value);
				if (parsed == null) {
					throw new IllegalStateException("unknown source role tag " + name +
						" on " + function.getName(true));
				}
				if (source != null && source != parsed) {
					throw new IllegalStateException("conflicting source role tags on " +
						function.getName(true));
				}
				source = parsed;
			}
			else if (name.startsWith(ORIGIN_PREFIX)) {
				String parsed = name.substring(ORIGIN_PREFIX.length());
				if (!ORIGIN_TAGS.contains(parsed)) {
					throw new IllegalStateException("unknown function origin tag " + name +
						" on " + function.getName(true));
				}
				if (origin != null && !origin.equals(parsed)) {
					throw new IllegalStateException("conflicting function origin tags on " +
						function.getName(true));
				}
				origin = parsed;
			}
		}
		return new Tagged(source, emission, origin == null ? "first-party" : origin);
	}

	private static String kebab(String name) {
		return name.toLowerCase().replace('_', '-');
	}

	private record Tagged(SourceKind source, EmissionKind emission, String origin) {
	}
}
