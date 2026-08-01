package wiz8.exporter;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import ghidra.app.decompiler.ClangBreak;
import ghidra.app.decompiler.ClangLine;
import ghidra.app.decompiler.ClangNode;
import ghidra.app.decompiler.ClangToken;
import ghidra.app.decompiler.ClangTokenGroup;
import ghidra.app.decompiler.ClangTypeToken;
import ghidra.app.decompiler.PrettyPrinter;
import ghidra.app.decompiler.component.DecompilerUtils;
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
		String owner = TypeNames.map(function.getParentNamespace().getName(true));
		return marker() + "\n// " + owner + "::" +
			FunctionKind.decorateSpecialName(function.getName()) + "\n";
	}

	/**
	 * The function text with the analysis applied. The marker sits
	 * immediately above the declaration — reccmp's pairing requires that
	 * adjacency — so any leading decompiler comments print above the marker.
	 */
	public String print(ClangTokenGroup markup, Msvc6Patterns.Analysis analysis) {
		Set<Integer> touched = new HashSet<>();
		String rendered = render(markup, analysis, touched);
		if (!structurallySound(rendered, touched)) {
			// Every pass validated its own claims, so reaching this final
			// check means the renderer or validator itself is wrong; that is
			// a defect to surface, not a decline.
			analysis.defects.add(
				"print: final structural validation failed after per-pass checks");
			rendered = renderVerbatim(markup);
		}
		return insertMarker(rendered);
	}

	private String insertMarker(String rendered) {
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

	/** Ghidra's own identity-token rendering: the permanent safe fallback. */
	private static String renderVerbatim(ClangTokenGroup markup) {
		StringBuilder out = new StringBuilder();
		for (ClangLine line : DecompilerUtils.toLines(markup)) {
			out.append(line.getIndentString()).append(PrettyPrinter.getText(line)).append('\n');
		}
		return out.toString();
	}

	/**
	 * Cheap syntax invariants for transformed output; not a C++ parser.
	 * Balance checks cover the whole text, while the dangling-artifact
	 * heuristics apply only to lines the analysis touched: Ghidra's own
	 * wrapped rendering legitimately splits an assignment after a cast, and
	 * a line no claim altered cannot carry a claim-induced defect.
	 */
	static boolean structurallySound(String rendered, Set<Integer> touchedLines) {
		int parens = 0;
		int braces = 0;
		int brackets = 0;
		boolean blockComment = false;
		String[] lines = rendered.split("\n", -1);
		for (int lineIndex = 0; lineIndex < lines.length; lineIndex++) {
			String line = lines[lineIndex];
			String trimmed = line.trim();
			boolean touched = touchedLines == null || touchedLines.contains(lineIndex + 1);
			if (touched &&
				trimmed.matches("[-+]?(?:0x[0-9a-fA-F]+|[0-9]+(?:\\.[0-9]*)?[fFlL]?)")) {
				return false;
			}
			if (touched && trimmed.contains("=") && trimmed.endsWith(")") &&
				!nextLineContinuesExpression(lines, lineIndex + 1)) {
				return false;
			}
			boolean string = false;
			boolean character = false;
			boolean escaped = false;
			for (int i = 0; i < line.length(); i++) {
				char c = line.charAt(i);
				char next = i + 1 < line.length() ? line.charAt(i + 1) : '\0';
				if (blockComment) {
					if (c == '*' && next == '/') {
						blockComment = false;
						i++;
					}
					continue;
				}
				if (!string && !character && c == '/' && next == '/') {
					break;
				}
				if (!string && !character && c == '/' && next == '*') {
					blockComment = true;
					i++;
					continue;
				}
				if (escaped) {
					escaped = false;
					continue;
				}
				if ((string || character) && c == '\\') {
					escaped = true;
					continue;
				}
				if (!character && c == '"') {
					string = !string;
					continue;
				}
				if (!string && c == '\'') {
					character = !character;
					continue;
				}
				if (string || character) {
					continue;
				}
				switch (c) {
					case '(' -> parens++;
					case ')' -> parens--;
					case '{' -> braces++;
					case '}' -> braces--;
					case '[' -> brackets++;
					case ']' -> brackets--;
					default -> { }
				}
				if (parens < 0 || braces < 0 || brackets < 0) {
					return false;
				}
			}
			if (string || character) {
				return false;
			}
		}
		return !blockComment && parens == 0 && braces == 0 && brackets == 0;
	}

	private static boolean nextLineContinuesExpression(String[] lines, int index) {
		for (int i = index; i < lines.length; i++) {
			String next = lines[i].trim();
			if (next.isEmpty()) {
				continue;
			}
			return next.startsWith(";") || next.startsWith(",") || next.startsWith(")") ||
				next.startsWith("&&") || next.startsWith("||") || next.startsWith("?") ||
				next.startsWith(":") || next.startsWith("{");
		}
		return false;
	}

	static String render(ClangTokenGroup markup, Msvc6Patterns.Analysis analysis) {
		return render(markup, analysis, null);
	}

	/**
	 * Render the markup with the analysis applied. When {@code touchedLines}
	 * is given, it receives the 1-based numbers of every emitted line whose
	 * content a claim altered — the only lines the dangling-artifact
	 * heuristics may judge.
	 */
	static String render(ClangTokenGroup markup, Msvc6Patterns.Analysis analysis,
			Set<Integer> touchedLines) {
		List<ClangNode> nodes = new ArrayList<>();
		markup.flatten(nodes);
		Set<ClangNode> emittedReplacements = new HashSet<>();

		StringBuilder text = new StringBuilder();
		StringBuilder line = new StringBuilder();
		boolean lineTouched = false;
		int[] emittedCount = {0};
		int start = 0;
		if (!nodes.isEmpty() && nodes.get(0) instanceof ClangBreak brk) {
			appendIndent(line, brk.getIndent());
			start = 1;
		}
		for (int i = start; i < nodes.size(); i++) {
			ClangNode node = nodes.get(i);
			ClangNode recognized = recognizedAncestor(node, analysis);
			if (recognized != null) {
				// A later, wider semantic drop always wins over an earlier
				// token replacement inside the same compiler-owned construct.
				String replacement = analysis.dropped.contains(recognized) ? null
						: analysis.replaced.get(recognized);
				if (replacement != null && emittedReplacements.add(recognized)) {
					line.append(replacement);
				}
				lineTouched = true;
				continue;
			}
			if (node instanceof ClangBreak brk) {
				flushLine(text, line, lineTouched, emittedCount, touchedLines);
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
		flushLine(text, line, lineTouched, emittedCount, touchedLines);
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
			boolean lineTouched, int[] emittedCount, Set<Integer> touchedLines) {
		String rendered = line.toString();
		line.setLength(0);
		String trimmed = rendered.trim();
		if (lineTouched && (trimmed.isEmpty() || trimmed.equals(";"))) {
			return;
		}
		emittedCount[0]++;
		if (lineTouched && touchedLines != null) {
			touchedLines.add(emittedCount[0]);
		}
		text.append(rendered).append('\n');
	}

	private static void appendIndent(StringBuilder line, int indent) {
		for (int i = 0; i < indent; i++) {
			line.append(INDENT_UNIT);
		}
	}
}
