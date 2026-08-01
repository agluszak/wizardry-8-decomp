package wiz8.exporter;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import ghidra.app.decompiler.ClangBreak;
import ghidra.app.decompiler.ClangNode;
import ghidra.app.decompiler.ClangToken;
import ghidra.app.decompiler.ClangTokenGroup;
import ghidra.app.decompiler.ClangTypeToken;
import ghidra.program.model.listing.Function;

/**
 * Renders one decompiled function as recovered-style C++ text.
 *
 * The renderer walks Ghidra's marked-up decompiler tree; it never parses the
 * flat pseudo-C string. Tokens print verbatim (reproducing Ghidra's own line
 * layout byte for byte) unless a recognizer claimed their subtree: dropped
 * subtrees vanish, replaced subtrees print their replacement text once. A
 * line whose recognized content vanished entirely is removed rather than
 * left blank, so suppressed statements take their trailing semicolons and
 * line breaks with them.
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

	/**
	 * The function text with the analysis applied. The marker sits
	 * immediately above the declaration — reccmp's pairing requires that
	 * adjacency — so any leading decompiler comments print above the marker.
	 */
	public String print(ClangTokenGroup markup, Msvc6Patterns.Analysis analysis) {
		String rendered = render(markup, analysis);
		String[] lines = rendered.split("\n", -1);
		int declaration = 0;
		boolean inBlockComment = false;
		while (declaration < lines.length) {
			String trimmed = lines[declaration].trim();
			if (inBlockComment) {
				inBlockComment = !trimmed.endsWith("*/");
			}
			else if (trimmed.startsWith("/*")) {
				inBlockComment = !trimmed.endsWith("*/");
			}
			else if (!trimmed.isEmpty() && !trimmed.startsWith("//")) {
				break;
			}
			declaration++;
		}
		StringBuilder out = new StringBuilder();
		for (int i = 0; i < declaration; i++) {
			String trimmed = lines[i].trim();
			if (!trimmed.isEmpty()) {
				out.append(lines[i]).append('\n');
			}
		}
		out.append(marker()).append('\n');
		for (int i = declaration; i < lines.length; i++) {
			out.append(lines[i]);
			if (i < lines.length - 1) {
				out.append('\n');
			}
		}
		return out.toString();
	}

	static String render(ClangTokenGroup markup, Msvc6Patterns.Analysis analysis) {
		List<ClangNode> nodes = new ArrayList<>();
		markup.flatten(nodes);
		Set<ClangNode> emittedReplacements = new HashSet<>();

		StringBuilder text = new StringBuilder();
		StringBuilder line = new StringBuilder();
		boolean lineTouched = false;
		int start = 0;
		if (!nodes.isEmpty() && nodes.get(0) instanceof ClangBreak brk) {
			appendIndent(line, brk.getIndent());
			start = 1;
		}
		for (int i = start; i < nodes.size(); i++) {
			ClangNode node = nodes.get(i);
			ClangNode recognized = recognizedAncestor(node, analysis);
			if (recognized != null) {
				String replacement = analysis.replaced.get(recognized);
				if (replacement != null && emittedReplacements.add(recognized)) {
					line.append(replacement);
				}
				else {
					lineTouched = true;
				}
				continue;
			}
			if (node instanceof ClangBreak brk) {
				flushLine(text, line, lineTouched);
				lineTouched = false;
				appendIndent(line, brk.getIndent());
				continue;
			}
			if (node instanceof ClangToken token) {
				String tokenText = token.getText();
				if (token instanceof ClangTypeToken) {
					tokenText = TypeNames.map(tokenText);
				}
				else {
					// Namespace qualifiers of template classes print as plain
					// syntax tokens but still carry the bracket encoding.
					tokenText = TypeNames.mapTemplateSpelling(tokenText);
				}
				tokenText = Msvc6Patterns.sanitizeReservedName(node, tokenText);
				if (tokenText != null) {
					line.append(tokenText);
				}
			}
		}
		flushLine(text, line, lineTouched);
		if (text.length() == 0 || text.charAt(text.length() - 1) != '\n') {
			text.append('\n');
		}
		return text.toString();
	}

	/**
	 * The outermost enclosing node a recognizer dropped or replaced, if any.
	 * The outermost claim wins so a statement-level drop silences token-level
	 * rewrites recorded inside it.
	 */
	private static ClangNode recognizedAncestor(ClangNode node,
			Msvc6Patterns.Analysis analysis) {
		ClangNode outermost = null;
		for (ClangNode current = node; current != null; current = current.Parent()) {
			if (analysis.replaced.containsKey(current) || analysis.dropped.contains(current)) {
				outermost = current;
			}
		}
		return outermost;
	}

	/**
	 * Ends the current line. Lines that lost recognized content and kept
	 * nothing but whitespace or a stray semicolon disappear entirely.
	 */
	private static void flushLine(StringBuilder text, StringBuilder line,
			boolean lineTouched) {
		String rendered = line.toString();
		line.setLength(0);
		String trimmed = rendered.trim();
		if (lineTouched && (trimmed.isEmpty() || trimmed.equals(";"))) {
			return;
		}
		text.append(rendered).append('\n');
	}

	private static void appendIndent(StringBuilder line, int indent) {
		for (int i = 0; i < indent; i++) {
			line.append(INDENT_UNIT);
		}
	}
}
