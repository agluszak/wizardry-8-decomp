package wiz8.recovery;

import java.util.List;

/** Immutable, pass-local rewrite proposal; never persisted as evidence. */
record ProposedRewrite(RewriteOwner owner, List<NodeEdit> edits, String evidence) { }
