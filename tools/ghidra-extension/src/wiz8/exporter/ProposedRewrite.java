package wiz8.exporter;

import java.util.List;

/** Immutable, pass-local rewrite proposal; never persisted as evidence. */
record ProposedRewrite(String rule, List<NodeEdit> edits, int priority, String evidence) { }
