package wiz8.recovery;

import java.util.ArrayList;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Predicate;

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
	static final class Item {
		final ClangNode node;
		final int depth;

		Item(ClangNode node, int depth) {
			this.node = node;
			this.depth = depth;
		}

		boolean isStatement() {
			return node instanceof ClangStatement;
		}
	}

	static final class TokenSpan {
		final List<ClangToken> raw = new ArrayList<>();
		final List<ClangToken> sig = new ArrayList<>();
		final List<Integer> rawIndex = new ArrayList<>();
	}

	final ClangTokenGroup root;
	final HighFunction highFunction;
	final BlockView.Tree blocks;
	final List<ClangToken> tokens;
	final List<Item> items;

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
		this.items = Collections.unmodifiableList(linearize((ClangFunction) root, blocks));
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

	private static List<Item> linearize(ClangFunction root, BlockView.Tree blocks) {
		List<Item> raw = new ArrayList<>();
		boolean seenPrototype = false;
		for (int i = 0; i < root.numChildren(); i++) {
			ClangNode child = root.Child(i);
			if (child instanceof ClangFuncProto) {
				seenPrototype = true;
				continue;
			}
			if (seenPrototype) collect(child, raw, blocks);
		}
		int bodyDepth = raw.stream()
			.filter(item -> item.node instanceof ClangStatement ||
				item.node instanceof ClangVariableDecl)
			.mapToInt(item -> item.depth).min().orElse(0);
		List<Item> flat = new ArrayList<>();
		for (Item item : raw) {
			flat.add(new Item(item.node, Math.max(0, item.depth - bodyDepth)));
		}
		return flat;
	}

	private static void collect(ClangNode node, List<Item> flat, BlockView.Tree blocks) {
		int depth = lexicalDepth(blocks.ownerOf(node));
		if (node instanceof ClangStatement || node instanceof ClangVariableDecl) {
			flat.add(new Item(node, depth));
			return;
		}
		if (node instanceof ClangTokenGroup group) {
			for (int i = 0; i < group.numChildren(); i++) collect(group.Child(i), flat, blocks);
			return;
		}
		if (node instanceof ClangBreak || node instanceof ClangCommentToken) return;
		if (node instanceof ClangToken token) {
			String text = token.getText();
			if (text != null && !text.isBlank() && !text.equals(";")) {
				flat.add(new Item(node, depth));
			}
		}
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
		leafTokens(node, span.raw);
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
			if (seenPrototype) leafTokens(child, span.raw);
		}
		index(span);
		return span;
	}

	TokenSpan liveView(TokenSpan source, Predicate<ClangNode> claimed) {
		TokenSpan live = new TokenSpan();
		live.raw.addAll(source.raw);
		for (int i = 0; i < source.sig.size(); i++) {
			if (!claimed.test(source.sig.get(i))) {
				live.sig.add(source.sig.get(i));
				live.rawIndex.add(source.rawIndex.get(i));
			}
		}
		return live;
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

	static List<ClangToken> leafTokens(ClangNode node) {
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(node, tokens);
		return tokens;
	}

	private static void leafTokens(ClangNode node, List<ClangToken> out) {
		if (node instanceof ClangTokenGroup group) {
			for (int i = 0; i < group.numChildren(); i++) {
				leafTokens(group.Child(i), out);
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
