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
		FunctionRole role = FunctionRoleResolver.resolve(function);
		return switch (role.emissionKind()) {
			case SCALAR_DELETING_DESTRUCTOR -> SCALAR_DELETING_DESTRUCTOR;
			case VECTOR_DELETING_DESTRUCTOR -> VECTOR_DELETING_DESTRUCTOR;
			default -> role.isConstructor() ? CONSTRUCTOR
				: role.isDestructor() ? DESTRUCTOR : ORDINARY;
		};
	}

	public boolean isDeletingDestructor() {
		return this == SCALAR_DELETING_DESTRUCTOR ||
			this == VECTOR_DELETING_DESTRUCTOR;
	}

}
