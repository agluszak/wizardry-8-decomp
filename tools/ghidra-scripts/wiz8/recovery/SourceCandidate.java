package wiz8.recovery;

/** An unbound compiler declaration that structurally fits one binary function. */
public record SourceCandidate(String semanticId, String qualifiedName, String sourceSignature,
		SourceKind sourceKind, int parameterCount, String sourceFile, int line) {
	public SourceCandidate {
		semanticId = semanticId == null ? "" : semanticId;
		qualifiedName = qualifiedName == null ? "" : qualifiedName;
		sourceSignature = sourceSignature == null ? "" : sourceSignature;
		sourceFile = sourceFile == null ? "" : sourceFile;
	}
}
