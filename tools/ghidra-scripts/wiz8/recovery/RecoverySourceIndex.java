package wiz8.recovery;

import java.util.Map;

/** Immutable, target-scoped compiler source facts for one recovery session. */
public final class RecoverySourceIndex {
	private final String target;
	private final Map<Long, SourceHints> functions;

	public RecoverySourceIndex(String target, Map<Long, SourceHints> functions) {
		if (target == null || target.isBlank()) {
			throw new IllegalArgumentException("source-index target is required");
		}
		this.target = target;
		this.functions = Map.copyOf(functions);
	}

	public String target() {
		return target;
	}

	SourceHints facts(long entry) {
		return functions.getOrDefault(entry, SourceHints.NONE);
	}
}
