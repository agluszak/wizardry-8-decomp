package wiz8.recovery;

import java.util.ArrayList;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import ghidra.app.decompiler.ClangFuncNameToken;
import ghidra.app.decompiler.ClangFuncProto;
import ghidra.app.decompiler.ClangFunction;
import ghidra.app.decompiler.ClangNode;
import ghidra.app.decompiler.ClangBreak;
import ghidra.app.decompiler.ClangCommentToken;
import ghidra.app.decompiler.ClangStatement;
import ghidra.app.decompiler.ClangSyntaxToken;
import ghidra.app.decompiler.ClangToken;
import ghidra.app.decompiler.ClangTokenGroup;
import ghidra.app.decompiler.ClangVariableDecl;
import ghidra.app.decompiler.ClangVariableToken;
import ghidra.program.model.pcode.HighFunction;
import ghidra.program.model.pcode.HighSymbol;
import ghidra.program.model.pcode.HighVariable;
import ghidra.program.model.pcode.PcodeOp;

/**
 * Per-decompilation index binding semantic Ghidra objects to rendered markup.
 * It is disposable and contains no recovered meaning of its own.
 */
final class MarkupIndex {
	static final class TokenSpan {
		private final List<ClangToken> raw = new ArrayList<>();
		final List<ClangToken> sig = new ArrayList<>();
		private final List<Integer> rawIndex = new ArrayList<>();

		List<ClangToken> region(int from, int to) {
			if (from < 0 || to < from || to >= sig.size()) return List.of();
			return List.copyOf(raw.subList(rawIndex.get(from), rawIndex.get(to) + 1));
		}
	}

	final ClangTokenGroup root;
	final HighFunction highFunction;
	final BlockView.Tree blocks;
	final List<ClangToken> tokens;
	final List<ClangNode> bodyEntries;
	private final Set<ClangNode> topLevelEntries =
		Collections.newSetFromMap(new IdentityHashMap<>());

	private final Map<PcodeOp, List<ClangToken>> tokensByOp = new IdentityHashMap<>();
	private final Map<PcodeOp, ClangStatement> statementByOp = new IdentityHashMap<>();
	private final Map<HighSymbol, ClangVariableDecl> declarationBySymbol =
		new IdentityHashMap<>();
	private final Map<HighVariable, ClangVariableDecl> declarationByVariable =
		new IdentityHashMap<>();
	private final Map<HighVariable, List<ClangVariableToken>> usesByVariable =
		new IdentityHashMap<>();
	private final Map<Integer, ClangSyntaxToken> openerByPair = new LinkedHashMap<>();
	private final Map<Integer, ClangSyntaxToken> closerByPair = new LinkedHashMap<>();

	MarkupIndex(ClangTokenGroup root, HighFunction highFunction) {
		this.root = root;
		this.highFunction = highFunction;
		this.blocks = BlockView.build(root);
		List<ClangToken> all = new ArrayList<>();
		index(root, null, null, all);
		this.tokens = Collections.unmodifiableList(all);
		this.bodyEntries = Collections.unmodifiableList(indexBody((ClangFunction) root));
	}

	List<ClangNode> bodyNodes() {
		List<ClangNode> nodes = new ArrayList<>();
		boolean seenPrototype = false;
		for (int i = 0; i < root.numChildren(); i++) {
			ClangNode child = root.Child(i);
			if (child instanceof ClangFuncProto) {
				seenPrototype = true;
				continue;
			}
			if (seenPrototype) nodes.add(child);
		}
		return nodes;
	}

	private List<ClangNode> indexBody(ClangFunction root) {
		List<ClangNode> entries = new ArrayList<>();
		boolean seenPrototype = false;
		for (int i = 0; i < root.numChildren(); i++) {
			ClangNode child = root.Child(i);
			if (child instanceof ClangFuncProto) {
				seenPrototype = true;
				continue;
			}
			if (seenPrototype) collectBody(child, entries);
		}
		int bodyDepth = entries.stream()
			.mapToInt(node -> lexicalDepth(blocks.ownerOf(node))).min().orElse(0);
		for (ClangNode entry : entries) {
			if (lexicalDepth(blocks.ownerOf(entry)) == bodyDepth) {
				topLevelEntries.add(entry);
			}
		}
		return entries;
	}

	private static void collectBody(ClangNode node, List<ClangNode> entries) {
		if (node instanceof ClangStatement || node instanceof ClangVariableDecl) {
			entries.add(node);
			return;
		}
		if (node instanceof ClangTokenGroup group) {
			for (int i = 0; i < group.numChildren(); i++) collectBody(group.Child(i), entries);
			return;
		}
		if (node instanceof ClangBreak || node instanceof ClangCommentToken) return;
		if (node instanceof ClangToken token) {
			String text = token.getText();
			if (text != null && !text.isBlank() && !text.equals(";")) {
				entries.add(node);
			}
		}
	}

	boolean isTopLevel(ClangNode node) {
		return topLevelEntries.contains(node);
	}

	private static int lexicalDepth(BlockView block) {
		int depth = 0;
		for (BlockView current = block; current != null; current = current.parent) depth++;
		return depth;
	}

	List<ClangToken> tokensFor(PcodeOp operation) {
		return tokensByOp.getOrDefault(operation, List.of());
	}

	ClangStatement statementFor(PcodeOp operation) {
		return statementByOp.get(operation);
	}

	ClangVariableDecl declarationFor(HighSymbol symbol) {
		return declarationBySymbol.get(symbol);
	}

	ClangVariableDecl declarationFor(HighVariable variable) {
		return declarationByVariable.get(variable);
	}

	List<ClangVariableToken> usesOf(HighVariable variable) {
		return usesByVariable.getOrDefault(variable, List.of());
	}

	TokenSpan tokenSpan(ClangNode node) {
		TokenSpan span = new TokenSpan();
		collectTokens(node, span.raw);
		index(span);
		return span;
	}

	TokenSpan bodySpan() {
		TokenSpan span = new TokenSpan();
		boolean seenPrototype = false;
		for (int i = 0; i < root.numChildren(); i++) {
			ClangNode child = root.Child(i);
			if (child instanceof ghidra.app.decompiler.ClangFuncProto) {
				seenPrototype = true;
				continue;
			}
			if (seenPrototype) collectTokens(child, span.raw);
		}
		index(span);
		return span;
	}

	private static void index(TokenSpan span) {
		for (int i = 0; i < span.raw.size(); i++) {
			String text = span.raw.get(i).getText();
			if (text != null && !text.isBlank()) {
				span.sig.add(span.raw.get(i));
				span.rawIndex.add(i);
			}
		}
	}

	private static void collectTokens(ClangNode node, List<ClangToken> out) {
		if (node instanceof ClangTokenGroup group) {
			for (int i = 0; i < group.numChildren(); i++) {
				collectTokens(group.Child(i), out);
			}
		}
		else if (node instanceof ClangToken token &&
			!(token instanceof ghidra.app.decompiler.ClangBreak)) {
			out.add(token);
		}
	}

	ClangSyntaxToken matchingClose(ClangSyntaxToken opener) {
		return opener.getOpen() < 0 ? null : closerByPair.get(opener.getOpen());
	}

	ClangFuncNameToken exactFunctionToken(PcodeOp operation) {
		for (ClangToken token : tokensFor(operation)) {
			if (token instanceof ClangFuncNameToken name && name.getPcodeOp() == operation) {
				return name;
			}
		}
		return null;
	}

	private void index(ClangNode node, ClangStatement containing,
			ClangVariableDecl containingDeclaration,
			List<ClangToken> all) {
		ClangStatement statement = node instanceof ClangStatement current ? current : containing;
		ClangVariableDecl activeDeclaration = node instanceof ClangVariableDecl current
			? current : containingDeclaration;
		if (node instanceof ClangStatement current && current.getPcodeOp() != null) {
			statementByOp.putIfAbsent(current.getPcodeOp(), current);
		}
		if (node instanceof ClangVariableDecl declaration) {
			HighSymbol symbol = declaration.getHighSymbol();
			if (symbol != null) {
				declarationBySymbol.putIfAbsent(symbol, declaration);
			}
		}
		if (node instanceof ClangToken token) {
			all.add(token);
			PcodeOp operation = token.getPcodeOp();
			if (operation != null) {
				tokensByOp.computeIfAbsent(operation, ignored -> new ArrayList<>()).add(token);
				if (statement != null) {
					statementByOp.putIfAbsent(operation, statement);
				}
			}
			if (token instanceof ClangVariableToken variable) {
				HighVariable high = variable.getHighVariable();
				if (high != null) {
					usesByVariable.computeIfAbsent(high, ignored -> new ArrayList<>()).add(variable);
					if (activeDeclaration != null) {
						declarationByVariable.putIfAbsent(high, activeDeclaration);
					}
				}
			}
			if (token instanceof ClangSyntaxToken syntax) {
				if (syntax.getOpen() >= 0) {
					openerByPair.putIfAbsent(syntax.getOpen(), syntax);
				}
				if (syntax.getClose() >= 0) {
					closerByPair.putIfAbsent(syntax.getClose(), syntax);
				}
			}
		}
		if (node instanceof ClangTokenGroup group) {
			for (int i = 0; i < group.numChildren(); i++) {
				index(group.Child(i), statement, activeDeclaration, all);
			}
		}
	}
}
