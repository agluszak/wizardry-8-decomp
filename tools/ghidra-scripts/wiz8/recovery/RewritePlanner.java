package wiz8.recovery;

import java.util.Map;
import java.util.Set;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;

import ghidra.app.decompiler.ClangNode;
import ghidra.app.decompiler.ClangStatement;

/** Resolves proposal overlap and applies an accepted plan atomically. */
final class RewritePlanner {
	private final Set<ClangNode> dropped;
	private final Map<ClangNode, String> replaced;
	private final Map<ClangNode, String> owners;
	private final Map<String, Integer> priorities;

	RewritePlanner(Set<ClangNode> dropped, Map<ClangNode, String> replaced,
			Map<ClangNode, String> owners, Map<String, Integer> priorities) {
		this.dropped = dropped;
		this.replaced = replaced;
		this.owners = owners;
		this.priorities = priorities;
	}

	PlanResult accept(ProposedRewrite proposal) {
		List<List<NodeEdit>> groups = groups(proposal);
		List<String> rejected = new ArrayList<>();
		boolean accepted = false;
		for (List<NodeEdit> group : groups) {
			String conflict = null;
			for (NodeEdit edit : group) {
				conflict = conflict(proposal, edit.node());
				if (conflict != null) break;
			}
			if (conflict != null) {
				rejected.add(conflict);
				continue;
			}
			apply(proposal, group);
			accepted = true;
		}
		priorities.put(proposal.rule(), proposal.priority());
		return new PlanResult(accepted, List.copyOf(rejected));
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
			owners.put(edit.node(), proposal.rule());
		}
	}

	private static List<List<NodeEdit>> groups(ProposedRewrite proposal) {
		if (proposal.priority() > 1) return List.of(proposal.edits());
		Map<ClangNode, List<NodeEdit>> grouped = new LinkedHashMap<>();
		for (NodeEdit edit : proposal.edits()) {
			ClangNode region = statement(edit.node());
			grouped.computeIfAbsent(region, ignored -> new ArrayList<>()).add(edit);
		}
		return new ArrayList<>(grouped.values());
	}

	private static ClangNode statement(ClangNode node) {
		for (ClangNode current = node; current != null; current = current.Parent()) {
			if (current instanceof ClangStatement) return current;
		}
		return node;
	}

	private String conflict(ProposedRewrite proposal, ClangNode node) {
		String owner = owners.get(node);
		if (owner != null && !owner.equals(proposal.rule()) &&
			proposal.priority() <= priorities.getOrDefault(owner, 1)) {
			return "node already claimed by " + owner;
		}
		for (ClangNode ancestor = node.Parent(); ancestor != null;
				ancestor = ancestor.Parent()) {
			String ancestorOwner = owners.get(ancestor);
			if (ancestorOwner != null && !ancestorOwner.equals(proposal.rule()) &&
				replaced.containsKey(ancestor) &&
				proposal.priority() <= priorities.getOrDefault(ancestorOwner, 1)) {
				return "inside a node replaced by " + ancestorOwner;
			}
		}
		return null;
	}

	record PlanResult(boolean accepted, List<String> rejected) { }
}
