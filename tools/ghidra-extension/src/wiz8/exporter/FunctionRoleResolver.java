package wiz8.exporter;

import java.util.LinkedHashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import ghidra.app.util.demangler.DemangledObject;
import ghidra.app.util.demangler.microsoft.MicrosoftDemangler;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionTag;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.Symbol;
import ghidra.util.task.TaskMonitor;

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
	static {
		for (EmissionKind kind : EmissionKind.values()) {
			ROLE_TAGS.put(kebab(kind.name()), kind);
		}
		for (SourceKind kind : SourceKind.values()) {
			SOURCE_TAGS.put(kebab(kind.name()), kind);
		}
	}

	private FunctionRoleResolver() {
	}

	public static FunctionRole resolve(Function function) {
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
		FunctionRole wrapper = structuralDeletingWrapper(function);
		if (wrapper != null) {
			return wrapper;
		}
		FunctionRole adjustor = structuralDestructorAdjustor(function);
		if (adjustor != null) {
			return adjustor;
		}

		if (function.isThunk()) {
			Function canonical = function.getThunkedFunction(true);
			FunctionRole target = canonical == null || canonical.equals(function) ? null
				: resolve(canonical);
			SourceKind source = target == null ? SourceKind.NONE : target.sourceKind();
			String origin = target == null ? "unknown" : target.origin();
			return new FunctionRole(source, EmissionKind.ADJUSTOR_THUNK, canonical,
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
				return role(function, SourceKind.CONSTRUCTOR, EmissionKind.CONSTRUCTOR_BODY,
					"first-party", "class namespace identity");
			}
			if (function.getName().equals("~" + owner) ||
				function.getName().equals("~" + namespace.getName())) {
				return role(function, SourceKind.DESTRUCTOR, EmissionKind.COMPLETE_DESTRUCTOR,
					"first-party", "class namespace identity");
			}
			return role(function, SourceKind.MEMBER_FUNCTION, EmissionKind.AUTHORED_BODY,
				"first-party", "class namespace membership");
		}
		return role(function, SourceKind.FREE_FUNCTION, EmissionKind.AUTHORED_BODY,
			"first-party", "ordinary Ghidra function");
	}

	private static FunctionRole structuralDeletingWrapper(Function function) {
		Function destructor = null;
		boolean deallocator = false;
		boolean vectorIterator = false;
		for (Function called : function.getCalledFunctions(TaskMonitor.DUMMY)) {
			String name = SpecialNames.normalize(called.getName());
			if (name.equals("operator delete") || name.startsWith("operator delete ")) {
				deallocator = true;
				continue;
			}
			FunctionRole role = directLifecycleIdentity(called);
			if (role == null) {
				continue;
			}
			if (role.emissionKind() == EmissionKind.VECTOR_DESTRUCTOR_ITERATOR) {
				vectorIterator = true;
			}
			else if (role.isDestructor() && !role.isDeletingDestructor()) {
				if (destructor != null && !destructor.equals(called)) {
					return null;
				}
				destructor = called;
			}
		}
		if (!deallocator || (destructor == null && !vectorIterator)) {
			return null;
		}
		EmissionKind emission = vectorIterator
			? EmissionKind.VECTOR_DELETING_DESTRUCTOR
			: EmissionKind.SCALAR_DELETING_DESTRUCTOR;
		return new FunctionRole(SourceKind.DESTRUCTOR, emission, destructor,
			function.isThunk(), "first-party",
			"direct destructor/iterator plus deallocator call graph");
	}

	private static FunctionRole directLifecycleIdentity(Function function) {
		Tagged tagged = tags(function);
		if (tagged.emission != null || tagged.source != null) {
			EmissionKind emission = tagged.emission != null ? tagged.emission
				: defaultEmission(function, tagged.source);
			return role(function, tagged.source != null ? tagged.source
				: sourceFor(function, emission), emission, tagged.origin,
				"reviewed Ghidra tag");
		}
		FunctionRole special = fromSpecialIdentity(function, function.getName(),
			"Ghidra special-name identity");
		if (special != null) {
			return special;
		}
		Namespace namespace = function.getParentNamespace();
		if (namespace instanceof GhidraClass) {
			String owner = CallableIdentity.classLeaf(namespace.getName());
			if (function.getName().equals("~" + owner) ||
				function.getName().equals("~" + namespace.getName())) {
				return role(function, SourceKind.DESTRUCTOR,
					EmissionKind.COMPLETE_DESTRUCTOR, "first-party",
					"class namespace identity");
			}
		}
		return null;
	}

	private static FunctionRole structuralDestructorAdjustor(Function function) {
		if (function.getParentNamespace() instanceof GhidraClass) {
			return null; // the class-owned source body wins
		}
		Function baseDestructor = soleDirectDestructor(function);
		Set<Long> tables = referencedVftables(function);
		if (baseDestructor == null || tables.size() != 1) {
			return null;
		}
		Function match = null;
		for (Function candidate : function.getProgram().getFunctionManager()
				.getFunctions(true)) {
			if (candidate.equals(function) ||
				!(candidate.getParentNamespace() instanceof GhidraClass)) {
				continue;
			}
			FunctionRole identity = directLifecycleIdentity(candidate);
			if (identity == null || !identity.isDestructor() ||
				!baseDestructor.equals(soleDirectDestructor(candidate)) ||
				!tables.equals(referencedVftables(candidate))) {
				continue;
			}
			if (match != null) {
				return null;
			}
			match = candidate;
		}
		return match == null ? null : new FunctionRole(SourceKind.DESTRUCTOR,
			EmissionKind.ADJUSTOR_THUNK, match, true, "first-party",
			"same vftable and base-destructor calls as class-owned destructor");
	}

	private static Function soleDirectDestructor(Function function) {
		Function found = null;
		for (Function called : function.getCalledFunctions(TaskMonitor.DUMMY)) {
			FunctionRole identity = directLifecycleIdentity(called);
			if (identity == null || !identity.isDestructor() ||
				identity.isDeletingDestructor()) {
				continue;
			}
			if (found != null && !found.equals(called)) {
				return null;
			}
			found = called;
		}
		return found;
	}

	private static Set<Long> referencedVftables(Function function) {
		Set<Long> tables = new HashSet<>();
		var instructions = function.getProgram().getListing()
			.getInstructions(function.getBody(), true);
		while (instructions.hasNext()) {
			Instruction instruction = instructions.next();
			for (var reference : instruction.getReferencesFrom()) {
				Symbol symbol = function.getProgram().getSymbolTable()
					.getPrimarySymbol(reference.getToAddress());
				if (symbol != null && VtableResolver.isVftableName(symbol.getName())) {
					tables.add(symbol.getAddress().getOffset());
				}
			}
		}
		return tables;
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
				return new FunctionRole(sourceForNamespace(function),
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
			return new FunctionRole(SourceKind.DESTRUCTOR,
				EmissionKind.SCALAR_DELETING_DESTRUCTOR, function, adjustor,
				"first-party", evidence);
		}
		if (name.startsWith("vector deleting destructor")) {
			return new FunctionRole(SourceKind.DESTRUCTOR,
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
		return new FunctionRole(source, emission, function, function.isThunk(),
			origin, evidence);
	}

	private static SourceKind sourceForNamespace(Function function) {
		return function.getParentNamespace() instanceof GhidraClass
			? SourceKind.MEMBER_FUNCTION : SourceKind.FREE_FUNCTION;
	}

	private static SourceKind sourceFor(Function function, EmissionKind emission) {
		return switch (emission) {
			case CONSTRUCTOR_BODY, DEFAULT_CONSTRUCTOR_CLOSURE -> SourceKind.CONSTRUCTOR;
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
			case CONSTRUCTOR -> EmissionKind.CONSTRUCTOR_BODY;
			case DESTRUCTOR -> EmissionKind.COMPLETE_DESTRUCTOR;
			case TEMPLATE_MEMBER -> EmissionKind.TEMPLATE_EMISSION;
			case LIBRARY_ENTITY -> EmissionKind.LIBRARY_BODY;
			default -> function.isThunk() ? EmissionKind.ADJUSTOR_THUNK
				: EmissionKind.AUTHORED_BODY;
		};
	}

	private static Tagged tags(Function function) {
		SourceKind source = null;
		EmissionKind emission = null;
		String origin = "first-party";
		for (FunctionTag tag : function.getTags()) {
			String name = tag.getName();
			if (name.startsWith(ROLE_PREFIX)) {
				emission = ROLE_TAGS.get(name.substring(ROLE_PREFIX.length()));
			}
			else if (name.startsWith(SOURCE_PREFIX)) {
				source = SOURCE_TAGS.get(name.substring(SOURCE_PREFIX.length()));
			}
			else if (name.startsWith(ORIGIN_PREFIX)) {
				origin = name.substring(ORIGIN_PREFIX.length());
			}
		}
		return new Tagged(source, emission, origin);
	}

	private static String kebab(String name) {
		return name.toLowerCase().replace('_', '-');
	}

	private record Tagged(SourceKind source, EmissionKind emission, String origin) {
	}
}
