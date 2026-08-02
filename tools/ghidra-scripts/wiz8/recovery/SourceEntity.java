package wiz8.recovery;

import java.util.List;

/** One source callable and all compiler emissions that implement it. */
record SourceEntity(SourceEntityKey key, List<Emission> emissions, BodyCarrier bodyCarrier) {
	SourceKind kind() {
		return key.kind();
	}

	boolean isConstructor() {
		return kind() == SourceKind.CONSTRUCTOR;
	}

	boolean isDestructor() {
		return kind() == SourceKind.DESTRUCTOR;
	}

	boolean hasSourceBody() {
		return !(bodyCarrier instanceof BodyCarrier.None);
	}
}
