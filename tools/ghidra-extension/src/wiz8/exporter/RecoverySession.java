package wiz8.exporter;

import java.util.IdentityHashMap;
import java.util.Map;

import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Program;
import ghidra.util.exception.CancelledException;
import ghidra.util.task.TaskMonitor;

/**
 * Per-export cache and shared analysis services over one live program.
 *
 * The session is ephemeral: it neither serializes analysis nor competes with
 * Ghidra as authority.  It prevents class-family resolution, role lookup, and
 * call/vtable services from being recreated recursively while one export is in
 * progress.
 */
final class RecoverySession {
	final Program program;
	final CallTargetResolver calls = new CallTargetResolver();
	final VtableResolver vtables;
	private final Map<Function, FunctionRole> roles = new IdentityHashMap<>();
	private final Map<GhidraClass, LifecycleFamilyResolver.LifecycleFamily> families =
		new IdentityHashMap<>();

	RecoverySession(Program program) {
		this.program = program;
		this.vtables = new VtableResolver(program);
	}

	FunctionRole role(Function function) {
		return roles.computeIfAbsent(function, FunctionRoleResolver::resolve);
	}

	/** The family-normalized role used when one binary address is exported alone. */
	FunctionRole exportRole(Function function, TaskMonitor monitor) throws CancelledException {
		FunctionRole raw = role(function);
		if (!(function.getParentNamespace() instanceof GhidraClass owner) ||
			(!raw.isConstructor() && !raw.isDestructor())) {
			return raw;
		}
		for (LifecycleFamilyResolver.Member member : family(owner, monitor).members()) {
			if (member.function().equals(function)) {
				FunctionRole normalized = member.role();
				// An address-selected deleting wrapper remains marker-only.  A class
				// family may use its instructions as the only carrier from which to
				// recover the source destructor, but that does not turn the wrapper
				// emission itself into an authored callable.
				return normalized.isDeletingDestructor() ? raw : normalized;
			}
		}
		return raw;
	}

	LifecycleFamilyResolver.LifecycleFamily family(GhidraClass owner, TaskMonitor monitor)
			throws CancelledException {
		LifecycleFamilyResolver.LifecycleFamily cached = families.get(owner);
		if (cached != null) {
			return cached;
		}
		LifecycleFamilyResolver.LifecycleFamily resolved =
			new LifecycleFamilyResolver(this).resolve(owner, monitor);
		families.put(owner, resolved);
		return resolved;
	}
}
