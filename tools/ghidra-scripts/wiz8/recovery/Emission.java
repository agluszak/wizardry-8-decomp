package wiz8.recovery;

import ghidra.program.model.listing.Function;

/** Facts about one binary function emission, independent of source ownership. */
record Emission(Function function, EmissionKind kind, Function canonicalTarget,
		ThunkInfo thunk, Origin origin, String evidence) {
	boolean isDeletingWrapper() {
		return kind == EmissionKind.SCALAR_DELETING_DESTRUCTOR ||
			kind == EmissionKind.VECTOR_DELETING_DESTRUCTOR;
	}
}
