package wiz8.recovery;

import java.util.ArrayList;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;

import ghidra.app.decompiler.ClangNode;
import ghidra.app.decompiler.ClangStatement;
import ghidra.app.decompiler.ClangTokenGroup;
import ghidra.app.decompiler.ClangVariableDecl;

/**
 * Ephemeral lexical-block view over Ghidra's marked-up decompiler tree.
 *
 * Raw {@link ClangTokenGroup} instances are the nodes produced for
 * {@code ELEM_BLOCK}; the specialized subclasses represent functions,
 * statements, declarations, and prototypes.  Consequently block ownership
 * comes from the markup hierarchy and never from counting printed braces.
 */
final class BlockView {

	final ClangTokenGroup markup;
	final BlockView parent;
	final List<ClangStatement> statements = new ArrayList<>();
	final List<ClangVariableDecl> declarations = new ArrayList<>();
	final List<BlockView> children = new ArrayList<>();

	private BlockView(ClangTokenGroup markup, BlockView parent) {
		this.markup = markup;
		this.parent = parent;
	}

	static final class Tree {
		final List<BlockView> roots;
		private final Map<ClangNode, BlockView> owner;

		private Tree(List<BlockView> roots, Map<ClangNode, BlockView> owner) {
			this.roots = Collections.unmodifiableList(roots);
			this.owner = owner;
		}

		BlockView ownerOf(ClangNode node) {
			for (ClangNode current = node; current != null; current = current.Parent()) {
				BlockView block = owner.get(current);
				if (block != null) {
					return block;
				}
			}
			return null;
		}
	}

	static Tree build(ClangTokenGroup root) {
		List<BlockView> roots = new ArrayList<>();
		Map<ClangNode, BlockView> owner = new IdentityHashMap<>();
		collect(root, null, roots, owner);
		return new Tree(roots, owner);
	}

	private static void collect(ClangNode node, BlockView current, List<BlockView> roots,
			Map<ClangNode, BlockView> owner) {
		BlockView active = current;
		if (node.getClass() == ClangTokenGroup.class) {
			active = new BlockView((ClangTokenGroup) node, current);
			if (current == null) {
				roots.add(active);
			}
			else {
				current.children.add(active);
			}
			owner.put(node, active);
		}
		if (active != null) {
			owner.put(node, active);
			if (node instanceof ClangStatement statement) {
				active.statements.add(statement);
			}
			else if (node instanceof ClangVariableDecl declaration) {
				active.declarations.add(declaration);
			}
		}
		if (node instanceof ClangTokenGroup group) {
			for (int i = 0; i < group.numChildren(); i++) {
				collect(group.Child(i), active, roots, owner);
			}
		}
	}
}
