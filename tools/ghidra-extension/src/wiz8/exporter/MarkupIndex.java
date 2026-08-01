package wiz8.exporter;

import java.util.ArrayList;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import ghidra.app.decompiler.ClangFuncNameToken;
import ghidra.app.decompiler.ClangNode;
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

	final ClangTokenGroup root;
	final HighFunction highFunction;
	final BlockView.Tree blocks;
	final List<ClangToken> tokens;

	private final Map<PcodeOp, List<ClangToken>> tokensByOp = new IdentityHashMap<>();
	private final Map<PcodeOp, ClangStatement> statementByOp = new IdentityHashMap<>();
	private final Map<HighSymbol, ClangVariableDecl> declarationBySymbol =
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
		index(root, null, all);
		this.tokens = Collections.unmodifiableList(all);
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

	List<ClangVariableToken> usesOf(HighVariable variable) {
		return usesByVariable.getOrDefault(variable, List.of());
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
			List<ClangToken> all) {
		ClangStatement statement = node instanceof ClangStatement current ? current : containing;
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
				index(group.Child(i), statement, all);
			}
		}
	}
}
