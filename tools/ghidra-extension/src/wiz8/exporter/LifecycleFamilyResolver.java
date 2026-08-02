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
	private final RecoverySession session;

	LifecycleFamilyResolver(RecoverySession session) {
		this.session = session;
		this.program = session.program;
		this.vtables = session.vtables;
	}

	LifecycleFamily resolve(GhidraClass owner, TaskMonitor monitor)
			throws CancelledException {
		Map<Long, Member> members = new LinkedHashMap<>();
		for (Symbol symbol : program.getSymbolTable().getSymbols(owner)) {
			monitor.checkCancelled();
			if (!(symbol.getObject() instanceof Function function)) {
				continue;
			}
			FunctionRole role = session.role(function);
			Function canonical = role.canonicalFunction();
			add(members, function, role, "class namespace");
			if (canonical != null && !canonical.equals(function) &&
				owner.equals(canonical.getParentNamespace())) {
				add(members, canonical, session.role(canonical), "canonical namespace target");
			}
		}

		for (Symbol table : vtables.vftables(owner)) {
			for (Function slot : vtables.slots(table)) {
				monitor.checkCancelled();
				FunctionRole role = session.role(slot);
				Function canonical = slot.isThunk() ? slot.getThunkedFunction(true) : null;
				if (!owner.equals(slot.getParentNamespace()) &&
					(canonical == null || !owner.equals(canonical.getParentNamespace()))) {
					continue; // inherited/purecall slot, not this class's emission
				}
				add(members, slot, role,
					"vftable 0x" + table.getAddress() + " slot target");
				if (canonical != null && !canonical.equals(slot)) {
					add(members, canonical, session.role(canonical),
						"canonical target of vftable thunk");
				}
			}
		}

		List<Member> ordered = new ArrayList<>(members.values());
		ordered = assignBodyOwners(ordered);
		List<SourceBody> sourceBodies = sourceBodies(ordered);
		ordered.sort(Comparator
			.comparingInt((Member member) -> sourceOrder(member.role.sourceKind()))
			.thenComparingInt(member -> emissionOrder(member.role.emissionKind()))
			.thenComparing(member -> member.function.getEntryPoint()));
		return new LifecycleFamily(owner, List.copyOf(ordered), List.copyOf(sourceBodies),
			List.copyOf(vtables.vftables(owner)));
	}

	/**
	 * When VC6 emitted no standalone destructor, its only authored cleanup may
	 * live inside a deleting wrapper. Keep that wrapper marker-only, but expose
	 * one separate unmarked source body whose lifecycle pass must prove and
	 * remove the flag-controlled deallocation epilogue.
	 */
	private static List<SourceBody> sourceBodies(List<Member> members) {
		Map<SourceEntityKey, List<Member>> byEntity = new LinkedHashMap<>();
		for (Member member : members) {
			byEntity.computeIfAbsent(member.role.sourceEntity(), ignored -> new ArrayList<>())
				.add(member);
		}
		List<SourceBody> bodies = new ArrayList<>();
		for (List<Member> emissions : byEntity.values()) {
			if (emissions.isEmpty() || emissions.get(0).role.sourceKind() != SourceKind.DESTRUCTOR ||
				emissions.stream().anyMatch(member -> member.role.hasAuthoredBody())) {
				continue;
			}
			Member carrier = emissions.stream()
				.filter(member -> member.role.isDeletingDestructor() && !member.function.isThunk())
				.min(Comparator
					.comparingLong((Member member) -> -member.function.getBody().getNumAddresses())
					.thenComparing(member -> member.function.getEntryPoint()))
				.orElse(null);
			if (carrier == null) {
				continue;
			}
			FunctionRole sourceRole = new FunctionRole(carrier.role.sourceEntity(),
				EmissionKind.COMPLETE_DESTRUCTOR, carrier.function, carrier.function, false,
				carrier.role.origin(), carrier.role.evidence() +
					"; family-selected deleting-wrapper body carrier");
			bodies.add(new SourceBody(carrier.function, sourceRole,
				"no standalone destructor emission; wrapper epilogue requires P-code proof"));
		}
		return bodies;
	}

	/**
	 * Select one and only one authored emission for each source entity.  A
	 * compiler-backed authored-body marker wins.  If a family has only explicit
	 * constructor/destructor variants, the least synthetic concrete emission is
	 * selected; wrappers and thunks are never eligible.
	 */
	private static List<Member> assignBodyOwners(List<Member> members) {
		Map<SourceEntityKey, List<Member>> byEntity = new LinkedHashMap<>();
		for (Member member : members) {
			byEntity.computeIfAbsent(member.role.sourceEntity(), ignored -> new ArrayList<>())
				.add(member);
		}
		List<Member> normalized = new ArrayList<>();
		for (List<Member> emissions : byEntity.values()) {
			Member owner = emissions.stream()
				.filter(member -> member.role.hasAuthoredBody())
				.min(bodyPreference())
				.orElseGet(() -> emissions.stream()
					.filter(member -> variantCanOwnBody(member.role.emissionKind()))
					.min(bodyPreference()).orElse(null));
			for (Member member : emissions) {
				Function selected = owner != null && owner.function.equals(member.function)
					? member.function : null;
				normalized.add(new Member(member.function,
					member.role.withBodyOwner(selected), member.familyEvidence));
			}
		}
		return normalized;
	}

	private static Comparator<Member> bodyPreference() {
		return Comparator
			.comparingInt((Member member) -> bodyOrder(member.role.emissionKind()))
			.thenComparingLong(member -> -member.function.getBody().getNumAddresses())
			.thenComparing(member -> member.function.getEntryPoint());
	}

	private static boolean variantCanOwnBody(EmissionKind kind) {
		return switch (kind) {
			case CONSTRUCTOR_BODY, BASE_CONSTRUCTOR, COMPLETE_CONSTRUCTOR,
				VBASE_CONSTRUCTOR, BASE_DESTRUCTOR, COMPLETE_DESTRUCTOR,
				VBASE_DESTRUCTOR -> true;
			default -> false;
		};
	}

	private static int bodyOrder(EmissionKind kind) {
		return switch (kind) {
			case AUTHORED_BODY -> 0;
			case COMPLETE_CONSTRUCTOR, COMPLETE_DESTRUCTOR -> 1;
			case CONSTRUCTOR_BODY, BASE_CONSTRUCTOR, BASE_DESTRUCTOR -> 2;
			case VBASE_CONSTRUCTOR, VBASE_DESTRUCTOR -> 3;
			default -> 10;
		};
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
			case CONSTRUCTOR_BODY, BASE_CONSTRUCTOR, COMPLETE_CONSTRUCTOR,
				VBASE_CONSTRUCTOR -> 0;
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

	record SourceBody(Function carrier, FunctionRole role, String evidence) {
	}

	record LifecycleFamily(GhidraClass owner, List<Member> members,
		List<SourceBody> sourceBodies, List<Symbol> vftables) {
	}
}
