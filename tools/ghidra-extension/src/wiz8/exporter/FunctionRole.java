package wiz8.exporter;

import ghidra.program.model.listing.Function;

/** Pass-local classification of one source entity and one binary emission. */
public record FunctionRole(SourceKind sourceKind, EmissionKind emissionKind,
		Function canonicalFunction, boolean adjustorThunk, String origin, String evidence) {

	public boolean isConstructor() {
		return sourceKind == SourceKind.CONSTRUCTOR;
	}

	public boolean isDestructor() {
		return sourceKind == SourceKind.DESTRUCTOR;
	}

	public boolean isDeletingDestructor() {
		return emissionKind == EmissionKind.SCALAR_DELETING_DESTRUCTOR ||
			emissionKind == EmissionKind.VECTOR_DELETING_DESTRUCTOR;
	}

	public boolean hasAuthoredBody() {
		return emissionKind == EmissionKind.AUTHORED_BODY ||
			emissionKind == EmissionKind.CONSTRUCTOR_BODY ||
			emissionKind == EmissionKind.BASE_DESTRUCTOR ||
			emissionKind == EmissionKind.COMPLETE_DESTRUCTOR;
	}
}
