package wiz8.recovery;

/** The binary region carrying one authored source entity's body. */
sealed interface BodyCarrier {
	record Direct(Emission emission) implements BodyCarrier { }
	record Extracted(Emission carrier, ExtractionKind kind) implements BodyCarrier { }
	record None() implements BodyCarrier { }

	enum ExtractionKind {
		REMOVE_DELETING_EPILOGUE
	}
}
