package wiz8.recovery;

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
	 * {@code open}, or -1. Missing pair identity is ambiguous and therefore
	 * declines; semantic transformations never restore textual depth parsing.
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
		return -1;
	}

	/** The index of the opener paired with {@code close}, or -1. */
	static int matchingOpen(List<ClangToken> tokens, int close) {
		if (tokens.get(close) instanceof ClangSyntaxToken closer && closer.getClose() >= 0) {
			int id = closer.getClose();
			for (int i = close - 1; i >= 0; i--) {
				if (tokens.get(i) instanceof ClangSyntaxToken candidate &&
					candidate.getOpen() == id) {
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
