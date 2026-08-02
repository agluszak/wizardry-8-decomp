package wiz8.recovery;

/** Compiler-owned source reference shape for one formal parameter. */
enum SourceReferenceForm {
	VALUE,
	LVALUE_REFERENCE_TO_OBJECT,
	LVALUE_REFERENCE_TO_POINTER,
	LVALUE_REFERENCE_TO_ARRAY,
	LVALUE_REFERENCE_TO_FUNCTION,
	RVALUE_REFERENCE_TO_OBJECT,
	RVALUE_REFERENCE_TO_POINTER,
	RVALUE_REFERENCE_TO_ARRAY,
	RVALUE_REFERENCE_TO_FUNCTION;

	static SourceReferenceForm parse(String spelling) {
		if (spelling == null || spelling.isBlank()) {
			return VALUE;
		}
		return valueOf(spelling.toUpperCase().replace('-', '_'));
	}

	boolean rendersObjectMemberAccess() {
		return this == LVALUE_REFERENCE_TO_OBJECT ||
			this == RVALUE_REFERENCE_TO_OBJECT;
	}
}
