package wiz8.exporter;

/** Exact Microsoft deleting-wrapper flag semantics used at call sites. */
final class DeletingDestructorSemantics {
	private DeletingDestructorSemantics() {
	}

	static String sourceOperator(EmissionKind kind, long flags) {
		if (kind == EmissionKind.SCALAR_DELETING_DESTRUCTOR) {
			return flags == 1 ? "delete " : null;
		}
		if (kind == EmissionKind.VECTOR_DELETING_DESTRUCTOR) {
			return flags == 1 ? "delete " : flags == 3 ? "delete[] " : null;
		}
		return null;
	}

	static void verifyRegressionCases() {
		if (!"delete ".equals(sourceOperator(
			EmissionKind.SCALAR_DELETING_DESTRUCTOR, 1)) ||
			sourceOperator(EmissionKind.SCALAR_DELETING_DESTRUCTOR, 0) != null ||
			sourceOperator(EmissionKind.SCALAR_DELETING_DESTRUCTOR, 2) != null ||
			sourceOperator(EmissionKind.SCALAR_DELETING_DESTRUCTOR, 3) != null ||
			!"delete ".equals(sourceOperator(
				EmissionKind.VECTOR_DELETING_DESTRUCTOR, 1)) ||
			!"delete[] ".equals(sourceOperator(
				EmissionKind.VECTOR_DELETING_DESTRUCTOR, 3)) ||
			sourceOperator(EmissionKind.VECTOR_DELETING_DESTRUCTOR, 0) != null ||
			sourceOperator(EmissionKind.VECTOR_DELETING_DESTRUCTOR, 2) != null) {
			throw new AssertionError("deleting-destructor flag regression");
		}
	}
}
