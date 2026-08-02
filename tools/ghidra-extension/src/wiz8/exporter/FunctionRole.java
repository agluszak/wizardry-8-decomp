package wiz8.exporter;

import ghidra.program.model.listing.Function;

/** Pass-local classification of one source entity and one binary emission. */
public record FunctionRole(SourceEntityKey sourceEntity, EmissionKind emissionKind,
		Function bodyOwner, Function canonicalTarget, boolean adjustorThunk,
		String origin, String evidence) {

	public SourceKind sourceKind() {
		return sourceEntity.kind();
	}

	/** Compatibility name for callers not yet interested in thunk metadata. */
	public Function canonicalFunction() {
		return canonicalTarget;
	}

	public boolean isConstructor() {
		return sourceKind() == SourceKind.CONSTRUCTOR;
	}

	public boolean isDestructor() {
		return sourceKind() == SourceKind.DESTRUCTOR;
	}

	public boolean isDeletingDestructor() {
		return emissionKind == EmissionKind.SCALAR_DELETING_DESTRUCTOR ||
			emissionKind == EmissionKind.VECTOR_DELETING_DESTRUCTOR;
	}

	public boolean hasAuthoredBody() {
		return bodyOwner != null;
	}

	FunctionRole withBodyOwner(Function owner) {
		if (isDeletingDestructor()) {
			owner = null;
		}
		return new FunctionRole(sourceEntity, emissionKind, owner, canonicalTarget,
			adjustorThunk, origin, evidence);
	}
}
