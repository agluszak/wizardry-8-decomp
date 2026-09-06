package wiz8.recovery;

import java.util.Map;
import java.util.Set;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.decompiler.ClangNode;

/** Resolves proposal overlap and applies an accepted plan atomically. */
final class RewritePlanner {
	private final Set<ClangNode> dropped;
	private final Map<ClangNode, String> replaced;
	private final Map<ClangNode, RewriteOwner> owners;

	RewritePlanner(Set<ClangNode> dropped, Map<ClangNode, String> replaced,
			Map<ClangNode, RewriteOwner> owners) {
		this.dropped = dropped;
		this.replaced = replaced;
		this.owners = owners;
	}

	PlanResult accept(ProposedRewrite proposal) {
		List<String> rejected = new ArrayList<>();
		String internal = internalConflict(proposal.edits());
		if (internal != null) {
			rejected.add(internal);
			return new PlanResult(false, List.copyOf(rejected));
		}
		for (NodeEdit edit : proposal.edits()) {
			String conflict = conflict(proposal, edit.node());
			if (conflict != null) {
				rejected.add(conflict);
				return new PlanResult(false, List.copyOf(rejected));
			}
		}
		apply(proposal, proposal.edits());
		return new PlanResult(true, List.of());
	}

	private void apply(ProposedRewrite proposal, List<NodeEdit> edits) {
		for (NodeEdit edit : edits) {
			if (edit instanceof NodeEdit.Drop drop) {
				replaced.remove(drop.node());
				dropped.add(drop.node());
			}
			else if (edit instanceof NodeEdit.Replace replace) {
				dropped.remove(replace.node());
				replaced.put(replace.node(), replace.text());
			}
			owners.put(edit.node(), proposal.owner());
		}
	}

	private static String internalConflict(List<NodeEdit> edits) {
		for (int i = 0; i < edits.size(); i++) {
			for (int j = i + 1; j < edits.size(); j++) {
				if (isAncestor(edits.get(i).node(), edits.get(j).node()) ||
					isAncestor(edits.get(j).node(), edits.get(i).node())) {
					return "semantic rewrite contains nested edits";
				}
			}
		}
		return null;
	}

	private String conflict(ProposedRewrite proposal, ClangNode node) {
		RewriteOwner owner = owners.get(node);
		if (owner != null && !owner.equals(proposal.owner())) {
			return "node already claimed by " + owner.rule();
		}
		for (ClangNode ancestor = node.Parent(); ancestor != null;
				ancestor = ancestor.Parent()) {
			RewriteOwner ancestorOwner = owners.get(ancestor);
			if (ancestorOwner != null && !ancestorOwner.equals(proposal.owner()) &&
				(replaced.containsKey(ancestor) || dropped.contains(ancestor))) {
				return "inside a node consumed by " + ancestorOwner.rule();
			}
		}
		for (var entry : owners.entrySet()) {
			if (!entry.getValue().equals(proposal.owner()) && isAncestor(node, entry.getKey())) {
				return "contains a node claimed by " + entry.getValue().rule();
			}
		}
		return null;
	}

	private static boolean isAncestor(ClangNode ancestor, ClangNode node) {
		for (ClangNode current = node.Parent(); current != null; current = current.Parent()) {
			if (current == ancestor) return true;
		}
		return false;
	}

	record PlanResult(boolean accepted, List<String> rejected) { }
}
