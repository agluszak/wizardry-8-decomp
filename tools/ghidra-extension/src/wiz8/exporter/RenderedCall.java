package wiz8.exporter;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import ghidra.app.decompiler.ClangFuncNameToken;
import ghidra.app.decompiler.ClangSyntaxToken;
import ghidra.app.decompiler.ClangToken;
import ghidra.program.model.pcode.PcodeOp;

/**
 * Exact markup binding for a call already proved through P-code.
 * Delimiter/comma text is punctuation only; operation identity and argument
 * count come exclusively from the P-code operation.
 */
final class RenderedCall {

	record Span(List<ClangToken> tokens) {
		Span {
			tokens = List.copyOf(tokens);
		}
	}

	final PcodeOp operation;
	final ClangFuncNameToken functionName;
	final ClangSyntaxToken opener;
	final ClangSyntaxToken closer;
	final List<Span> arguments;

	private RenderedCall(PcodeOp operation, ClangFuncNameToken functionName,
			ClangSyntaxToken opener, ClangSyntaxToken closer, List<Span> arguments) {
		this.operation = operation;
		this.functionName = functionName;
		this.opener = opener;
		this.closer = closer;
		this.arguments = Collections.unmodifiableList(arguments);
	}

	static RenderedCall bind(MarkupIndex index, PcodeOp operation) {
		ClangFuncNameToken name = index.exactFunctionToken(operation);
		if (name == null) {
			return null;
		}
		int nameIndex = index.tokens.indexOf(name);
		ClangSyntaxToken opener = null;
		for (int i = nameIndex + 1; i < index.tokens.size(); i++) {
			ClangToken token = index.tokens.get(i);
			if (token instanceof ClangSyntaxToken syntax && syntax.getOpen() >= 0 &&
				"(".equals(token.getText())) {
				opener = syntax;
				break;
			}
			if (token instanceof ClangFuncNameToken) {
				return null;
			}
		}
		if (opener == null) {
			return null;
		}
		ClangSyntaxToken closer = index.matchingClose(opener);
		if (closer == null) {
			return null;
		}
		int openIndex = index.tokens.indexOf(opener);
		int closeIndex = index.tokens.indexOf(closer);
		if (closeIndex <= openIndex) {
			return null;
		}
		List<Span> arguments = split(index.tokens, openIndex, closeIndex);
		if (arguments.size() != operation.getNumInputs() - 1) {
			return null;
		}
		return new RenderedCall(operation, name, opener, closer, arguments);
	}

	private static List<Span> split(List<ClangToken> tokens, int open, int close) {
		List<Span> result = new ArrayList<>();
		List<ClangToken> current = new ArrayList<>();
		int nested = 0;
		for (int i = open + 1; i < close; i++) {
			ClangToken token = tokens.get(i);
			if (SyntaxPairs.opensPair(token)) {
				nested++;
			}
			else if (SyntaxPairs.closesPair(token)) {
				nested--;
			}
			else if (nested == 0 && ",".equals(token.getText())) {
				result.add(new Span(current));
				current = new ArrayList<>();
				continue;
			}
			if (token.getText() != null && !token.getText().isBlank()) {
				current.add(token);
			}
		}
		if (!current.isEmpty() || !result.isEmpty()) {
			result.add(new Span(current));
		}
		return result;
	}
}
