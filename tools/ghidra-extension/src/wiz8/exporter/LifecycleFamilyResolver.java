package wiz8.exporter;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Program;
import ghidra.program.model.symbol.Symbol;
import ghidra.util.exception.CancelledException;
import ghidra.util.task.TaskMonitor;

/** Resolves one class's complete compiler ABI family from live Ghidra state. */
final class LifecycleFamilyResolver {
	private final Program program;
	private final VtableResolver vtables;

	LifecycleFamilyResolver(Program program) {
		this.program = program;
		this.vtables = new VtableResolver(program);
	}

	LifecycleFamily resolve(GhidraClass owner, TaskMonitor monitor)
			throws CancelledException {
		Map<Long, Member> members = new LinkedHashMap<>();
		for (Function function : program.getFunctionManager().getFunctions(true)) {
			monitor.checkCancelled();
			FunctionRole role = FunctionRoleResolver.resolve(function);
			Function canonical = role.canonicalFunction();
			if (owner.equals(function.getParentNamespace()) ||
				(canonical != null && owner.equals(canonical.getParentNamespace()))) {
				add(members, function, role, "class namespace or canonical thunk target");
			}
		}

		for (Symbol table : vtables.vftables(owner)) {
			for (Function slot : vtables.slots(table)) {
				monitor.checkCancelled();
				FunctionRole role = FunctionRoleResolver.resolve(slot);
				Function canonical = slot.isThunk() ? slot.getThunkedFunction(true) : null;
				if (!owner.equals(slot.getParentNamespace()) &&
					(canonical == null || !owner.equals(canonical.getParentNamespace()))) {
					continue; // inherited/purecall slot, not this class's emission
				}
				add(members, slot, role,
					"vftable 0x" + table.getAddress() + " slot target");
				if (canonical != null && !canonical.equals(slot)) {
					add(members, canonical, FunctionRoleResolver.resolve(canonical),
						"canonical target of vftable thunk");
				}
			}
		}

		List<Member> ordered = new ArrayList<>(members.values());
		ordered.sort(Comparator
			.comparingInt((Member member) -> sourceOrder(member.role.sourceKind()))
			.thenComparingInt(member -> emissionOrder(member.role.emissionKind()))
			.thenComparing(member -> member.function.getEntryPoint()));
		return new LifecycleFamily(owner, List.copyOf(ordered),
			List.copyOf(vtables.vftables(owner)));
	}

	private static void add(Map<Long, Member> members, Function function,
			FunctionRole role, String evidence) {
		members.putIfAbsent(function.getEntryPoint().getOffset(),
			new Member(function, role, evidence));
	}

	private static int sourceOrder(SourceKind kind) {
		return switch (kind) {
			case CONSTRUCTOR -> 0;
			case MEMBER_FUNCTION -> 1;
			case DESTRUCTOR -> 2;
			case TEMPLATE_MEMBER -> 3;
			case FREE_FUNCTION -> 4;
			case LIBRARY_ENTITY -> 5;
			case NONE -> 6;
		};
	}

	private static int emissionOrder(EmissionKind kind) {
		return switch (kind) {
			case CONSTRUCTOR_BODY -> 0;
			case AUTHORED_BODY -> 1;
			case BASE_DESTRUCTOR -> 2;
			case COMPLETE_DESTRUCTOR -> 3;
			case VBASE_DESTRUCTOR -> 4;
			case SCALAR_DELETING_DESTRUCTOR -> 5;
			case VECTOR_DELETING_DESTRUCTOR -> 6;
			case ADJUSTOR_THUNK, COVARIANT_RETURN_THUNK -> 7;
			default -> 8;
		};
	}

	record Member(Function function, FunctionRole role, String familyEvidence) {
	}

	record LifecycleFamily(GhidraClass owner, List<Member> members, List<Symbol> vftables) {
	}
}
