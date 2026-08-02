package wiz8.recovery;

import java.util.List;

/** Immutable, pass-local rewrite proposal; never persisted as evidence. */
record ProposedRewrite(String rule, List<NodeEdit> edits, int priority, String evidence) { }
