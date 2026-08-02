package wiz8.recovery;

import java.util.ArrayList;
import java.util.List;

/** Compiler-owned source facts supplied for one invocation, never persisted in Ghidra. */
record SourceHints(MarkerKind markerKind, SourceKind sourceKind, String sourceSignature,
		String semanticId, SourceReferenceForm[] referenceForms, String sourceFile) {
	static final SourceHints NONE = new SourceHints(MarkerKind.UNKNOWN, null, "", "",
		new SourceReferenceForm[0], "");

	SourceHints {
		markerKind = markerKind == null ? MarkerKind.UNKNOWN : markerKind;
		sourceSignature = sourceSignature == null ? "" : sourceSignature;
		semanticId = semanticId == null ? "" : semanticId;
		referenceForms = referenceForms == null
			? new SourceReferenceForm[0] : referenceForms.clone();
		sourceFile = sourceFile == null ? "" : sourceFile;
	}

	static SourceHints parse(String[] values) {
		MarkerKind marker = MarkerKind.UNKNOWN;
		SourceKind source = null;
		String signature = "";
		String semanticId = "";
		String sourceFile = "";
		List<SourceReferenceForm> references = new ArrayList<>();
		for (String value : values == null ? new String[0] : values) {
			if (value.startsWith("marker=")) marker = MarkerKind.valueOf(value.substring(7));
			else if (value.startsWith("source=")) source = SourceKind.valueOf(value.substring(7));
			else if (value.startsWith("signature=")) signature = value.substring(10);
			else if (value.startsWith("semantic=")) semanticId = value.substring(9);
			else if (value.startsWith("file=")) sourceFile = value.substring(5);
			else if (value.startsWith("reference=")) {
				references.add(SourceReferenceForm.parse(value.substring(10)));
			}
			else references.add(SourceReferenceForm.parse(value));
		}
		return new SourceHints(marker, source, signature, semanticId,
			references.toArray(SourceReferenceForm[]::new), sourceFile);
	}

	@Override
	public SourceReferenceForm[] referenceForms() {
		return referenceForms.clone();
	}
}
