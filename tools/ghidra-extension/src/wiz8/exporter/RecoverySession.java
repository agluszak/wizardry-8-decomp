package wiz8.exporter;

import java.util.IdentityHashMap;
import java.util.List;

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
	final CallTargetResolver calls;
	final FunctionRoleResolver roles;
	final VtableResolver vtables;
	private final IdentityHashMap<GhidraClass, ClassEntityResolver.ClassEntities> classes =
		new IdentityHashMap<>();

	RecoverySession(Program program) {
		this.program = program;
		this.calls = new CallTargetResolver();
		this.roles = new FunctionRoleResolver(calls);
		this.vtables = new VtableResolver(program);
	}

	private FunctionRoleResolver.Result classification(Function function) {
		return roles.resolve(function);
	}

	Emission emission(Function function) {
		return classification(function).emission();
	}

	SourceEntityKey sourceKey(Function function) {
		return classification(function).sourceEntity();
	}

	SourceEntity entity(Function function, TaskMonitor monitor) throws CancelledException {
		SourceEntityKey key = sourceKey(function);
		if (function.getParentNamespace() instanceof GhidraClass owner &&
			(key.kind() == SourceKind.CONSTRUCTOR || key.kind() == SourceKind.DESTRUCTOR)) {
			for (SourceEntity entity : classEntities(owner, monitor).sourceEntities()) {
				if (entity.key().equals(key)) return entity;
			}
		}
		Emission emission = emission(function);
		BodyCarrier carrier = emission.kind() == EmissionKind.AUTHORED_BODY ||
			emission.kind() == EmissionKind.CONSTRUCTOR_BODY
				? new BodyCarrier.Direct(emission) : new BodyCarrier.None();
		return new SourceEntity(key, List.of(emission), carrier);
	}

	ClassEntityResolver.ClassEntities classEntities(GhidraClass owner, TaskMonitor monitor)
			throws CancelledException {
		ClassEntityResolver.ClassEntities cached = classes.get(owner);
		if (cached != null) {
			return cached;
		}
		ClassEntityResolver.ClassEntities resolved =
			new ClassEntityResolver(this).resolve(owner, monitor);
		classes.put(owner, resolved);
		return resolved;
	}
}
