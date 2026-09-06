package wiz8.recovery;

import java.util.List;
import java.util.Map;

import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Parameter;

/** Immutable, target-scoped compiler source facts for one recovery session. */
public final class RecoverySourceIndex {
	private final String target;
	private final Map<Long, SourceHints> functions;
	private final List<SourceCandidate> declarations;

	public RecoverySourceIndex(String target, Map<Long, SourceHints> functions,
			List<SourceCandidate> declarations) {
		if (target == null || target.isBlank()) {
			throw new IllegalArgumentException("source-index target is required");
		}
		this.target = target;
		this.functions = Map.copyOf(functions);
		this.declarations = List.copyOf(declarations);
	}

	public String target() {
		return target;
	}

	SourceHints facts(long entry) {
		return functions.getOrDefault(entry, SourceHints.NONE);
	}

	/** Exact-name/arity candidates remain separate from established address facts. */
	public List<SourceCandidate> candidates(Function function, SourceKind kind) {
		if (functions.containsKey(function.getEntryPoint().getOffset())) return List.of();
		String qualifiedName = CallableIdentity.qualifiedName(function, kind);
		int parameterCount = 0;
		for (Parameter parameter : function.getParameters()) {
			if (!parameter.isAutoParameter()) parameterCount++;
		}
		if (kind == SourceKind.DESTRUCTOR) parameterCount = 0;
		final int arity = parameterCount;
		return declarations.stream()
			.filter(candidate -> candidate.sourceKind() == kind)
			.filter(candidate -> candidate.qualifiedName().equals(qualifiedName))
			.filter(candidate -> candidate.parameterCount() == arity)
			.toList();
	}
}
