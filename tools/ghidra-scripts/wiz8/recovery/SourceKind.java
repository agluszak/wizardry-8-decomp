package wiz8.recovery;

/** The source-level entity to which a binary emission belongs. */
public enum SourceKind {
	FREE_FUNCTION,
	MEMBER_FUNCTION,
	CONSTRUCTOR,
	DESTRUCTOR,
	TEMPLATE_MEMBER,
	LIBRARY_ENTITY,
	NONE
}
