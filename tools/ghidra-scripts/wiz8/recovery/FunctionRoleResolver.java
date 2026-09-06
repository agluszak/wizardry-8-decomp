package wiz8.recovery;

import java.util.IdentityHashMap;
import java.util.Map;
import java.util.Set;

import ghidra.app.util.demangler.DemangledObject;
import ghidra.app.util.demangler.microsoft.MicrosoftDemangler;
import ghidra.program.model.listing.Function;
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
final class FunctionRoleResolver {
	private final CallTargetResolver calls;
	private final RecoverySourceIndex source;
	private final Map<Function, Result> cache = new IdentityHashMap<>();
	private final Set<Function> resolving = java.util.Collections.newSetFromMap(
		new IdentityHashMap<>());

	FunctionRoleResolver(CallTargetResolver calls, RecoverySourceIndex source) {
		this.calls = calls;
		this.source = source;
	}

	Result resolve(Function function) {
		return resolve(function, source.facts(function.getEntryPoint().getOffset()));
	}

	private Result resolve(Function function, SourceHints hints) {
		Function deletingTarget = CompilerEmissionClassifier
			.scalarDeletingDestructorTarget(function);
		if (deletingTarget != null) {
			Result result = role(function, SourceKind.DESTRUCTOR,
				EmissionKind.SCALAR_DELETING_DESTRUCTOR, deletingTarget, false,
				Origin.FIRST_PARTY, "VC6 scalar deleting-wrapper instruction shape");
			cache.put(function, result);
			return result;
		}
		if (hints != null && hints.sourceKind() != null) {
			EmissionKind emission = switch (hints.markerKind()) {
				case TEMPLATE -> EmissionKind.TEMPLATE_EMISSION;
				case LIBRARY -> EmissionKind.LIBRARY_BODY;
				case FUNCTION -> EmissionKind.AUTHORED_BODY;
				default -> null;
			};
			if (emission != null) {
				Origin origin = hints.sourceFile().startsWith("src/surrender/")
					? Origin.SURRENDER : Origin.FIRST_PARTY;
				Emission binary = emission(function, emission, origin,
					"transient compiler source hint");
				Result result = new Result(SourceEntityKey.fromHint(function,
					hints.sourceKind(), hints), binary);
				cache.put(function, result);
				return result;
			}
		}
		Result cached = cache.get(function);
		if (cached != null) {
			return cached;
		}
		if (!resolving.add(function)) {
			return ordinaryRole(function, "role recursion guard");
		}
		try {
			Result role = resolveUncached(function);
			cache.put(function, role);
			return role;
		}
		finally {
			resolving.remove(function);
		}
	}

	private Emission emission(Function function, EmissionKind kind, Origin origin,
			String evidence) {
		CallTargetResolver.Target target = calls.resolve(function);
		ThunkInfo thunk = function.isThunk() && target != null ? target.thunk() : ThunkInfo.NONE;
		return new Emission(function, kind, function, thunk, origin, evidence);
	}

	private Result resolveUncached(Function function) {
		ReviewedFunctionMetadata.Metadata tagged = ReviewedFunctionMetadata.read(function);
		if (tagged.emission() != null || tagged.source() != null) {
			EmissionKind emission = tagged.emission() != null ? tagged.emission()
				: defaultEmission(function, tagged.source());
			SourceKind source = tagged.source() != null ? tagged.source()
				: sourceFor(function, emission);
			Origin origin = tagged.origin() == Origin.UNKNOWN
				? Origin.FIRST_PARTY : tagged.origin();
			return role(function, source, emission, origin, "reviewed Ghidra tag");
		}

		for (Symbol symbol : function.getProgram().getSymbolTable()
				.getSymbols(function.getEntryPoint())) {
			String name = symbol.getName();
			if (!name.startsWith("?")) {
				continue;
			}
			Result demangled = fromDecorated(function, name);
			if (demangled != null) {
				return demangled;
			}
		}

		Result named = fromSpecialIdentity(function, function.getName(),
			"centralized Ghidra special-name fallback");
		if (named != null) {
			return named;
		}
		if (function.isThunk()) {
			Function canonical = function.getThunkedFunction(true);
			Result target = canonical == null || canonical.equals(function) ? null
				: resolve(canonical);
			SourceKind source = target == null ? SourceKind.NONE : target.sourceEntity.kind();
			Origin origin = target == null ? Origin.UNKNOWN : target.emission.origin();
			return role(function, source, EmissionKind.ADJUSTOR_THUNK, canonical,
				true, origin, "Ghidra thunk metadata");
		}

		if (function.isExternal()) {
			return role(function, SourceKind.LIBRARY_ENTITY, EmissionKind.IMPORT_THUNK,
				Origin.IMPORT, "Ghidra external-function metadata");
		}

		Namespace namespace = function.getParentNamespace();
		if (namespace instanceof GhidraClass) {
			String owner = CallableIdentity.classLeaf(namespace.getName());
			String name = CallableIdentity.classLeaf(function.getName());
			if (name.equals(owner)) {
				return role(function, SourceKind.CONSTRUCTOR, EmissionKind.AUTHORED_BODY,
					Origin.FIRST_PARTY, "class namespace identity");
			}
			if (function.getName().equals("~" + owner) ||
				function.getName().equals("~" + namespace.getName())) {
				return role(function, SourceKind.DESTRUCTOR, EmissionKind.AUTHORED_BODY,
					Origin.FIRST_PARTY, "class namespace identity");
			}
			return role(function, SourceKind.MEMBER_FUNCTION, EmissionKind.AUTHORED_BODY,
				Origin.FIRST_PARTY, "class namespace membership");
		}
		return role(function, SourceKind.FREE_FUNCTION, EmissionKind.AUTHORED_BODY,
			Origin.FIRST_PARTY, "ordinary Ghidra function");
	}

	private Result fromDecorated(Function function, String decorated) {
		try {
			MicrosoftDemangler demangler = new MicrosoftDemangler();
			var options = demangler.createDefaultOptions();
			var context = demangler.createMangledContext(decorated, options,
				function.getProgram(), function.getEntryPoint());
			DemangledObject object = demangler.demangle(context);
			if (object == null) {
				return null;
			}
			Result special = fromSpecialIdentity(function, object.getName(),
				"Microsoft demangler: " + decorated);
			if (special != null) {
				return special;
			}
			if (object.isThunk()) {
				return role(function, sourceForNamespace(function),
					EmissionKind.ADJUSTOR_THUNK, function.getThunkedFunction(true), true,
					Origin.FIRST_PARTY, "Microsoft demangler thunk: " + decorated);
			}
		}
		catch (Exception ignored) {
			// The symbol remains usable as an identity even when this demangler
			// version declines it; fall through to reviewed Ghidra metadata.
		}
		return null;
	}

	private Result fromSpecialIdentity(Function function, String spelling,
			String evidence) {
		String name = SpecialNames.normalize(spelling);
		boolean adjustor = name.contains("adjustor{");
		if (name.startsWith("scalar deleting destructor")) {
			return role(function, SourceKind.DESTRUCTOR,
				EmissionKind.SCALAR_DELETING_DESTRUCTOR, function, adjustor,
				Origin.FIRST_PARTY, evidence);
		}
		if (name.startsWith("vector deleting destructor")) {
			return role(function, SourceKind.DESTRUCTOR,
				EmissionKind.VECTOR_DELETING_DESTRUCTOR, function, adjustor,
				Origin.FIRST_PARTY, evidence);
		}
		if (name.startsWith("vbase destructor")) {
			return role(function, SourceKind.DESTRUCTOR, EmissionKind.VBASE_DESTRUCTOR,
				Origin.FIRST_PARTY, evidence);
		}
		if (name.startsWith("default constructor closure")) {
			return role(function, SourceKind.CONSTRUCTOR,
				EmissionKind.DEFAULT_CONSTRUCTOR_CLOSURE, Origin.FIRST_PARTY, evidence);
		}
		if (name.startsWith("vector constructor iterator")) {
			return role(function, SourceKind.LIBRARY_ENTITY,
				EmissionKind.VECTOR_CONSTRUCTOR_ITERATOR, Origin.MSVC_RUNTIME, evidence);
		}
		if (name.startsWith("vector destructor iterator")) {
			return role(function, SourceKind.LIBRARY_ENTITY,
				EmissionKind.VECTOR_DESTRUCTOR_ITERATOR, Origin.MSVC_RUNTIME, evidence);
		}
		if (name.startsWith("array unwind")) {
			return role(function, SourceKind.LIBRARY_ENTITY, EmissionKind.ARRAY_UNWIND,
				Origin.MSVC_RUNTIME, evidence);
		}
		if (name.startsWith("local static guard")) {
			return role(function, SourceKind.NONE, EmissionKind.LOCAL_STATIC_GUARD,
				Origin.MSVC_RUNTIME, evidence);
		}
		return null;
	}

	private Result role(Function function, SourceKind source,
			EmissionKind emission, Origin origin, String evidence) {
		return role(function, source, emission, function, function.isThunk(), origin, evidence);
	}

	private Result role(Function function, SourceKind source,
			EmissionKind emission, Function canonical, boolean adjustor, Origin origin,
			String evidence) {
		CallTargetResolver.Target target = calls.resolve(function);
		ThunkInfo thunk = adjustor && target != null ? target.thunk() : ThunkInfo.NONE;
		return new Result(SourceEntityKey.of(function, canonical, source),
			new Emission(function, emission, canonical, thunk, origin, evidence));
	}

	private Result ordinaryRole(Function function, String evidence) {
		SourceKind source = sourceForNamespace(function);
		return role(function, source, EmissionKind.AUTHORED_BODY, Origin.UNKNOWN, evidence);
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

	record Result(SourceEntityKey sourceEntity, Emission emission) { }
}
