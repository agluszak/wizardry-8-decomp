package wiz8.exporter;

import java.util.List;

import ghidra.app.decompiler.ClangSyntaxToken;
import ghidra.app.decompiler.ClangToken;

/**
 * Matched-delimiter navigation over the decompiler's own pair identities.
 *
 * {@link ClangSyntaxToken} carries the id of the open/close pair it belongs
 * to, assigned by the decompiler itself. Matching by that id replaces
 * counting {@code "("} and {@code ")"} strings, which can miscount any
 * parenthesis-shaped text the token stream carries for another reason.
 * Token text is still what decides that a token *is* an opening
 * parenthesis at a call site; the pair id decides where it closes.
 */
final class SyntaxPairs {

	private SyntaxPairs() {
	}

	/**
	 * The index in {@code tokens} of the token closing the pair opened at
	 * {@code open}, or -1. Falls back to depth counting when the markup
	 * carries no pair id for the opener, so a caller never loses the match
	 * it would have found before.
	 */
	static int matchingClose(List<ClangToken> tokens, int open) {
		if (tokens.get(open) instanceof ClangSyntaxToken opener && opener.getOpen() >= 0) {
			int id = opener.getOpen();
			for (int i = open + 1; i < tokens.size(); i++) {
				if (tokens.get(i) instanceof ClangSyntaxToken candidate &&
					candidate.getClose() == id) {
					return i;
				}
			}
			return -1;
		}
		int depth = 0;
		for (int i = open; i < tokens.size(); i++) {
			String text = tokens.get(i).getText();
			if ("(".equals(text)) {
				depth++;
			}
			else if (")".equals(text)) {
				depth--;
				if (depth == 0) {
					return i;
				}
			}
		}
		return -1;
	}

	/** Whether the token opens a syntax pair (any delimiter kind). */
	static boolean opensPair(ClangToken token) {
		return token instanceof ClangSyntaxToken syntax && syntax.getOpen() >= 0;
	}

	/** Whether the token closes a syntax pair (any delimiter kind). */
	static boolean closesPair(ClangToken token) {
		return token instanceof ClangSyntaxToken syntax && syntax.getClose() >= 0;
	}
}
