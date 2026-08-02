package wiz8.exporter;

import ghidra.program.model.listing.Function;
/**
 * Compatibility projection used by the existing transformation passes.
 * {@link FunctionRoleResolver} is the sole classification authority; new code
 * should consume {@link FunctionRole} directly.
 */
public enum FunctionKind {
	ORDINARY,
	CONSTRUCTOR,
	DESTRUCTOR,
	SCALAR_DELETING_DESTRUCTOR,
	VECTOR_DELETING_DESTRUCTOR;

	public static FunctionKind classify(Function function) {
		return classify(FunctionRoleResolver.resolve(function));
	}

	static FunctionKind classify(FunctionRole role) {
		if (role.emissionKind() == EmissionKind.SCALAR_DELETING_DESTRUCTOR) {
			return SCALAR_DELETING_DESTRUCTOR;
		}
		if (role.emissionKind() == EmissionKind.VECTOR_DELETING_DESTRUCTOR) {
			return VECTOR_DELETING_DESTRUCTOR;
		}
		if (role.hasAuthoredBody() && role.isDestructor()) {
			return DESTRUCTOR;
		}
		if (role.hasAuthoredBody() && role.isConstructor()) {
			return CONSTRUCTOR;
		}
		return switch (role.emissionKind()) {
			default -> role.isConstructor() ? CONSTRUCTOR
				: role.isDestructor() ? DESTRUCTOR : ORDINARY;
		};
	}

	public boolean isDeletingDestructor() {
		return this == SCALAR_DELETING_DESTRUCTOR ||
			this == VECTOR_DELETING_DESTRUCTOR;
	}

}
