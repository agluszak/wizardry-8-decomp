package wiz8.exporter;

import java.util.Arrays;

/** Compiler-derived source hints passed only for one export request. */
record SourceHints(SourceReferenceForm[] parameterReferences) {
	SourceHints {
		parameterReferences = parameterReferences.clone();
	}

	static SourceHints parse(String[] forms) {
		SourceReferenceForm[] parsed = Arrays.stream(forms)
			.map(SourceReferenceForm::parse).toArray(SourceReferenceForm[]::new);
		return new SourceHints(parsed);
	}

	@Override
	public SourceReferenceForm[] parameterReferences() {
		return parameterReferences.clone();
	}
}
