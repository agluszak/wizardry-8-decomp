package wiz8.recovery;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

import ghidra.program.model.data.DataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Parameter;

/**
 * Identity of one source-level callable, independent of how many binary
 * emissions implement it. Known declarations use their compiler semantic ID;
 * unresolved functions use program/entry identity. The owner, kind, and formal
 * signature are presentation and classification facts, never equality inputs.
 */
public final class SourceEntityKey {
	private final Object identity;
	private final GhidraClass owner;
	private final SourceKind kind;
	private final String formalSignature;
	private final boolean sourceOwnedSignature;

	private SourceEntityKey(Object identity, GhidraClass owner, SourceKind kind,
			String formalSignature, boolean sourceOwnedSignature) {
		this.identity = identity;
		this.owner = owner;
		this.kind = kind;
		this.formalSignature = formalSignature;
		this.sourceOwnedSignature = sourceOwnedSignature;
	}

	static SourceEntityKey of(Function emission, Function canonical, SourceKind kind) {
		Function identity = canonical != null ? canonical : emission;
		GhidraClass owner = identity.getParentNamespace() instanceof GhidraClass clazz
			? clazz : emission.getParentNamespace() instanceof GhidraClass clazz ? clazz : null;
		String ownerName = owner == null ? "" : owner.getName(true);
		String name = switch (kind) {
			case CONSTRUCTOR -> owner == null ? identity.getName() : owner.getName();
			case DESTRUCTOR -> "~" + (owner == null ? identity.getName() : owner.getName());
			default -> identity.getName();
		};
		List<String> parameters = new ArrayList<>();
		if (kind != SourceKind.DESTRUCTOR) {
			for (Parameter parameter : identity.getParameters()) {
				if (!parameter.isAutoParameter()) {
					parameters.add(typeIdentity(parameter.getFormalDataType()));
				}
			}
		}
		String qualified = ownerName.isEmpty() ? name : ownerName + "::" + name;
		return new SourceEntityKey(new BinaryIdentity(identity.getProgram(),
			identity.getEntryPoint().getOffset()), owner, kind,
			qualified + "(" + String.join(",", parameters) + ")", false);
	}

	static SourceEntityKey fromHint(Function function, SourceKind kind, SourceHints hints) {
		GhidraClass owner = function.getParentNamespace() instanceof GhidraClass clazz
			? clazz : null;
		boolean sourceOwned = hints.sourceSignature() != null && !hints.sourceSignature().isBlank();
		Object identity = hints.semanticId().isBlank()
			? new BinaryIdentity(function.getProgram(), function.getEntryPoint().getOffset())
			: new SourceIdentity(hints.semanticId());
		return new SourceEntityKey(identity, owner, kind, sourceOwned
			? hints.sourceSignature() : of(function, function, kind).formalSignature(), sourceOwned);
	}

	public GhidraClass owner() { return owner; }
	public SourceKind kind() { return kind; }
	public String formalSignature() { return formalSignature; }

	/** Whether the complete formal signature came from the transient source index. */
	boolean hasSourceSignature() {
		return sourceOwnedSignature;
	}

	@Override
	public boolean equals(Object other) {
		return other instanceof SourceEntityKey key && identity.equals(key.identity);
	}

	@Override
	public int hashCode() {
		return identity.hashCode();
	}

	@Override
	public String toString() {
		return formalSignature;
	}

	private record SourceIdentity(String semanticId) {
		SourceIdentity { Objects.requireNonNull(semanticId); }
	}

	private record BinaryIdentity(ProgramIdentity program, long entry) {
		BinaryIdentity(ghidra.program.model.listing.Program program, long entry) {
			this(new ProgramIdentity(program), entry);
		}
	}

	private static final class ProgramIdentity {
		private final ghidra.program.model.listing.Program program;
		ProgramIdentity(ghidra.program.model.listing.Program program) { this.program = program; }
		@Override public boolean equals(Object other) {
			return other instanceof ProgramIdentity value && program == value.program;
		}
		@Override public int hashCode() { return System.identityHashCode(program); }
	}

	private static String typeIdentity(DataType type) {
		if (type == null) {
			return "<unknown>";
		}
		String path = type.getPathName();
		return path == null || path.isBlank() ? type.getName() : path;
	}
}
