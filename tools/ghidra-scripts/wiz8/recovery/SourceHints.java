package wiz8.recovery;

/** Compiler-owned source facts supplied for one invocation, never persisted in Ghidra. */
public record SourceHints(MarkerKind markerKind, SourceKind sourceKind, String sourceSignature,
		String semanticId, SourceReferenceForm[] referenceForms, String sourceFile) {
	public static final SourceHints NONE = new SourceHints(MarkerKind.UNKNOWN, null, "", "",
		new SourceReferenceForm[0], "");

	public SourceHints {
		markerKind = markerKind == null ? MarkerKind.UNKNOWN : markerKind;
		sourceSignature = sourceSignature == null ? "" : sourceSignature;
		semanticId = semanticId == null ? "" : semanticId;
		referenceForms = referenceForms == null
			? new SourceReferenceForm[0] : referenceForms.clone();
		sourceFile = sourceFile == null ? "" : sourceFile;
	}

	public static SourceHints of(String markerKind, String sourceKind, String sourceSignature,
			String semanticId, String[] referenceForms, String sourceFile) {
		SourceReferenceForm[] references = new SourceReferenceForm[
			referenceForms == null ? 0 : referenceForms.length];
		for (int i = 0; i < references.length; i++) {
			references[i] = SourceReferenceForm.parse(referenceForms[i]);
		}
		return new SourceHints(MarkerKind.valueOf(markerKind),
			sourceKind == null || sourceKind.isBlank() ? null : SourceKind.valueOf(sourceKind),
			sourceSignature, semanticId, references, sourceFile);
	}

	@Override
	public SourceReferenceForm[] referenceForms() {
		return referenceForms.clone();
	}
}
