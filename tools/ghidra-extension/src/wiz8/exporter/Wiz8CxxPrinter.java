package wiz8.exporter;

import java.util.ArrayList;
import java.util.List;

import ghidra.app.decompiler.ClangBreak;
import ghidra.app.decompiler.ClangNode;
import ghidra.app.decompiler.ClangToken;
import ghidra.app.decompiler.ClangTokenGroup;
import ghidra.program.model.listing.Function;

/**
 * Renders one decompiled function as recovered-style C++ text.
 *
 * The renderer walks Ghidra's marked-up decompiler tree; it never parses the
 * flat pseudo-C string. The verbatim path below reproduces Ghidra's own line
 * layout byte for byte (flatten the token tree, split at {@link ClangBreak},
 * indent with the decompiler's single-space unit) and is the permanent
 * fallback for every construct no recognizer positively claims.
 */
public final class Wiz8CxxPrinter {

	/** Matches ghidra.app.decompiler.PrettyPrinter.INDENT_STRING. */
	private static final String INDENT_UNIT = " ";

	private final Function function;
	private final FunctionKind kind;

	public Wiz8CxxPrinter(Function function, FunctionKind kind) {
		this.function = function;
		this.kind = kind;
	}

	/** The reccmp entity marker line for this function. */
	public String marker() {
		String prefix =
			kind == FunctionKind.SYNTHETIC_DELETING_DESTRUCTOR ? "SYNTHETIC" : "FUNCTION";
		return String.format("// %s: WIZ8 0x%08x", prefix,
			function.getEntryPoint().getOffset());
	}

	/**
	 * The synthetic block: marker plus the decorated name, no authored body.
	 * The deleting destructor is compiler output; only its address is recorded.
	 */
	public String printSynthetic() {
		String owner = function.getParentNamespace().getName(true);
		return marker() + "\n// " + owner + "::" +
			FunctionKind.decorateSpecialName(function.getName()) + "\n";
	}

	/** Marker plus the verbatim decompiler text for the whole function. */
	public String print(ClangTokenGroup markup) {
		return marker() + "\n" + renderVerbatim(markup);
	}

	/**
	 * Reproduce Ghidra's rendering of a token tree. Mirrors the line-splitting
	 * in DecompilerUtils.toLines without the GUI module dependency: a leading
	 * break sets the first line's indent, every later break ends a line.
	 */
	static String renderVerbatim(ClangTokenGroup markup) {
		List<ClangNode> nodes = new ArrayList<>();
		markup.flatten(nodes);

		StringBuilder text = new StringBuilder();
		StringBuilder line = new StringBuilder();
		int start = 0;
		if (!nodes.isEmpty() && nodes.get(0) instanceof ClangBreak brk) {
			appendIndent(line, brk.getIndent());
			start = 1;
		}
		for (int i = start; i < nodes.size(); i++) {
			ClangNode node = nodes.get(i);
			if (node instanceof ClangBreak brk) {
				text.append(line).append('\n');
				line.setLength(0);
				appendIndent(line, brk.getIndent());
				continue;
			}
			if (node instanceof ClangToken token) {
				String tokenText = token.getText();
				if (tokenText != null) {
					line.append(tokenText);
				}
			}
		}
		text.append(line);
		if (text.length() == 0 || text.charAt(text.length() - 1) != '\n') {
			text.append('\n');
		}
		return text.toString();
	}

	private static void appendIndent(StringBuilder line, int indent) {
		for (int i = 0; i < indent; i++) {
			line.append(INDENT_UNIT);
		}
	}
}
