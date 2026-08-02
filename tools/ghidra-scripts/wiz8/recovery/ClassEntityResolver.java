package wiz8.recovery;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.symbol.Symbol;
import ghidra.util.exception.CancelledException;
import ghidra.util.task.TaskMonitor;

/** Resolves a class's source entities and complete compiler ABI emissions. */
final class ClassEntityResolver {
	private final RecoverySession session;

	ClassEntityResolver(RecoverySession session) {
		this.session = session;
	}

	ClassEntities resolve(GhidraClass owner, TaskMonitor monitor) throws CancelledException {
		Map<Long, Member> members = new LinkedHashMap<>();
		for (Symbol symbol : session.program.getSymbolTable().getSymbols(owner)) {
			monitor.checkCancelled();
			if (!(symbol.getObject() instanceof Function function)) continue;
			add(members, function, "class namespace");
			Function canonical = session.emission(function).canonicalTarget();
			if (canonical != null && !canonical.equals(function) &&
				owner.equals(canonical.getParentNamespace())) {
				add(members, canonical, "canonical namespace target");
			}
		}

		for (Symbol table : session.vtables.vftables(owner)) {
			for (Function slot : session.vtables.slots(table)) {
				monitor.checkCancelled();
				Function canonical = slot.isThunk() ? slot.getThunkedFunction(true) : null;
				if (!owner.equals(slot.getParentNamespace()) &&
					(canonical == null || !owner.equals(canonical.getParentNamespace()))) {
					continue;
				}
				add(members, slot, "vftable 0x" + table.getAddress() + " slot target");
				if (canonical != null && !canonical.equals(slot)) {
					add(members, canonical, "canonical target of vftable thunk");
				}
			}
		}

		Map<SourceEntityKey, List<Member>> grouped = new LinkedHashMap<>();
		for (Member member : members.values()) {
			grouped.computeIfAbsent(member.key(), ignored -> new ArrayList<>()).add(member);
		}
		List<SourceEntity> entities = new ArrayList<>();
		for (var entry : grouped.entrySet()) {
			List<Member> group = entry.getValue();
			group.sort(Comparator.comparing(member -> member.emission().function().getEntryPoint()));
			Emission direct = group.stream().map(Member::emission)
				.filter(ClassEntityResolver::canCarryDirectBody)
				.min(bodyPreference()).orElse(null);
			BodyCarrier carrier;
			if (direct != null) {
				carrier = new BodyCarrier.Direct(direct);
			}
			else if (entry.getKey().kind() == SourceKind.DESTRUCTOR) {
				Emission extracted = group.stream().map(Member::emission)
					.filter(emission -> emission.isDeletingWrapper() &&
						!emission.function().isThunk())
					.min(Comparator
						.comparingLong((Emission emission) ->
							-emission.function().getBody().getNumAddresses())
						.thenComparing(emission -> emission.function().getEntryPoint()))
					.orElse(null);
				carrier = extracted == null ? new BodyCarrier.None()
					: new BodyCarrier.Extracted(extracted,
						BodyCarrier.ExtractionKind.REMOVE_DELETING_EPILOGUE);
			}
			else {
				carrier = new BodyCarrier.None();
			}
			entities.add(new SourceEntity(entry.getKey(),
				group.stream().map(Member::emission).toList(), carrier));
		}
		entities.sort(Comparator
			.comparingInt((SourceEntity entity) -> sourceOrder(entity.kind()))
			.thenComparing(entity -> entity.key().formalSignature()));
		return new ClassEntities(owner, List.copyOf(entities),
			List.copyOf(session.vtables.vftables(owner)));
	}

	private void add(Map<Long, Member> members, Function function, String evidence) {
		members.putIfAbsent(function.getEntryPoint().getOffset(),
			new Member(session.sourceKey(function), session.emission(function), evidence));
	}

	private static boolean canCarryDirectBody(Emission emission) {
		return switch (emission.kind()) {
			case AUTHORED_BODY, CONSTRUCTOR_BODY, BASE_CONSTRUCTOR,
				COMPLETE_CONSTRUCTOR, VBASE_CONSTRUCTOR, BASE_DESTRUCTOR,
				COMPLETE_DESTRUCTOR, VBASE_DESTRUCTOR -> !emission.function().isThunk();
			default -> false;
		};
	}

	private static Comparator<Emission> bodyPreference() {
		return Comparator.comparingInt((Emission emission) -> bodyOrder(emission.kind()))
			.thenComparingLong(emission -> -emission.function().getBody().getNumAddresses())
			.thenComparing(emission -> emission.function().getEntryPoint());
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

	record Member(SourceEntityKey key, Emission emission, String familyEvidence) { }
	record ClassEntities(GhidraClass owner, List<SourceEntity> sourceEntities,
		List<Symbol> vftables) { }
}
