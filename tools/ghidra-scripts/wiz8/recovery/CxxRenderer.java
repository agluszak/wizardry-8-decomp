package wiz8.recovery;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import ghidra.app.decompiler.ClangBreak;
import ghidra.app.decompiler.ClangCommentToken;
import ghidra.app.decompiler.ClangLine;
import ghidra.app.decompiler.ClangNode;
import ghidra.app.decompiler.ClangToken;
import ghidra.app.decompiler.ClangTokenGroup;
import ghidra.app.decompiler.ClangTypeToken;
import ghidra.app.decompiler.ClangFuncProto;
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
public final class CxxRenderer {

	/** Matches ghidra.app.decompiler.PrettyPrinter.INDENT_STRING. */
	private static final String INDENT_UNIT = " ";

	private final Function function;
	private final SourceEntity entity;
	private final Emission emission;

	CxxRenderer(Function function, SourceEntity entity, Emission emission) {
		this.function = function;
		this.entity = entity;
		this.emission = emission;
	}

	/** The reccmp entity marker line for this function. */
	public String marker() {
		boolean direct = entity.bodyCarrier() instanceof BodyCarrier.Direct carrier &&
			carrier.emission().equals(emission);
		String prefix = entity.kind() == SourceKind.LIBRARY_ENTITY ? "LIBRARY"
			: entity.kind() == SourceKind.TEMPLATE_MEMBER ? "TEMPLATE"
				: direct ? "FUNCTION" : "SYNTHETIC";
		return String.format("// %s: WIZ8 0x%08x", prefix,
			function.getEntryPoint().getOffset());
	}

	/**
	 * The synthetic block: marker plus the decorated name, no authored body.
	 * The deleting destructor is compiler output; only its address is recorded.
	 */
	public String printSynthetic() {
		if (entity.kind() == SourceKind.LIBRARY_ENTITY) {
			return marker() + "\n";
		}
		Function canonical = emission.canonicalTarget();
		String owner = TypeNames.map((canonical != null
			? canonical.getParentNamespace() : function.getParentNamespace()).getName(true));
		if (entity.kind() == SourceKind.TEMPLATE_MEMBER) {
			return marker() + "\n// " + owner + "::" +
				TypeNames.map(function.getName()) + "\n";
		}
		if ((emission.kind() == EmissionKind.ADJUSTOR_THUNK ||
			emission.kind() == EmissionKind.COVARIANT_RETURN_THUNK) &&
			canonical != null) {
			String source = CallableIdentity.sourceName(canonical, entity.kind());
			return marker() + "\n// " + owner + "::" + source + " (" +
				emission.kind().name().toLowerCase().replace('_', ' ') +
				" emission)\n";
		}
		String normalized = SpecialNames.normalize(function.getName());
		String special = emission.kind() == EmissionKind.SCALAR_DELETING_DESTRUCTOR
			? "`scalar deleting destructor'"
			: emission.kind() == EmissionKind.VECTOR_DELETING_DESTRUCTOR
				? normalized.contains("adjustor{") ? SpecialNames.decorate(function.getName())
					: "`vector deleting destructor'"
				: SpecialNames.decorate(emission.kind().name().toLowerCase());
		return marker() + "\n// " + owner + "::" + special + "\n";
	}

	/**
	 * The function text with the analysis applied. The marker sits
	 * immediately above the declaration — reccmp's pairing requires that
	 * adjacency — so any leading decompiler comments print above the marker.
	 */
	public String print(ClangTokenGroup markup, Msvc6Patterns.Analysis analysis) {
		return insertMarkerStructurally(markup, analysis);
	}

	/**
	 * Generated initializer suffix and compound statement, with the prototype
	 * excluded by markup ancestry rather than by parsing rendered C++.
	 */
	public String printBody(ClangTokenGroup markup, Msvc6Patterns.Analysis analysis) {
		List<ClangNode> nodes = new ArrayList<>();
		markup.flatten(nodes);
		int start = 0;
		for (int i = 0; i < nodes.size(); i++) {
			if (hasAncestor(nodes.get(i), ClangFuncProto.class)) {
				start = i + 1;
			}
		}
		String body = renderNodes(nodes, start, analysis, null);
		return analysis.initializerSuffix.isEmpty() ? body
			: analysis.initializerSuffix + "\n" + body;
	}

	private static boolean hasAncestor(ClangNode node, Class<?> type) {
		for (ClangNode current = node; current != null; current = current.Parent()) {
			if (type.isInstance(current)) {
				return true;
			}
		}
		return false;
	}

	private String insertMarkerStructurally(ClangTokenGroup markup,
			Msvc6Patterns.Analysis analysis) {
		List<ClangNode> nodes = new ArrayList<>();
		markup.flatten(nodes);
		int declaration = -1;
		for (int i = 0; i < nodes.size(); i++) {
			if (hasAncestor(nodes.get(i), ClangFuncProto.class)) {
				declaration = i;
				break;
			}
		}
		if (declaration < 0) {
			return marker() + "\n" + renderNodes(nodes, 0, analysis, null);
		}
		String prefix = renderNodes(new ArrayList<>(nodes.subList(0, declaration)), 0,
			analysis, null).stripTrailing();
		String definition = renderNodes(
			new ArrayList<>(nodes.subList(declaration, nodes.size())), 0, analysis, null);
		return (prefix.isEmpty() ? "" : prefix + "\n") + marker() + "\n" + definition;
	}

	/** Ghidra's own identity-token rendering: the permanent safe fallback. */
	private static String renderVerbatim(ClangTokenGroup markup) {
		StringBuilder out = new StringBuilder();
		for (ClangLine line : DecompilerUtils.toLines(markup)) {
			out.append(line.getIndentString()).append(PrettyPrinter.getText(line)).append('\n');
		}
		return out.toString();
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
		return renderNodes(nodes, 0, analysis, touchedLines);
	}

	private static String renderNodes(List<ClangNode> nodes, int requestedStart,
			Msvc6Patterns.Analysis analysis, Set<Integer> touchedLines) {
		Set<ClangNode> emittedReplacements = new HashSet<>();

		StringBuilder text = new StringBuilder();
		StringBuilder line = new StringBuilder();
		boolean lineTouched = false;
		int[] emittedCount = {0};
		int start = requestedStart;
		if (start < nodes.size() && nodes.get(start) instanceof ClangBreak brk) {
			appendIndent(line, brk.getIndent());
			start++;
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
				if (token instanceof ClangTypeToken type) {
					tokenText = TypeNames.mapToken(type);
				}
				else {
					// Namespace qualifiers of template classes print as plain
					// syntax tokens but still carry the bracket encoding.
					tokenText = TypeNames.mapTemplateSpelling(tokenText);
				}
				tokenText = Msvc6Patterns.sanitizeReservedName(node, tokenText);
				tokenText = lexSafeSpelling(token, tokenText);
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
	 * Recovered output may be semantically invalid where recovery declined —
	 * visible junk marks the declined construct — but it must always lex. A
	 * symbol spelling that carries quote or backtick characters (a decorated
	 * special name such as {@code `vftable'}) would read as a malformed
	 * character constant and corrupt the parse of every function after it,
	 * so its measurable damage would land on innocent neighbours. Genuine
	 * string and character literals pass through untouched; comments may
	 * contain anything.
	 */
	private static String lexSafeSpelling(ClangToken token, String text) {
		if (text == null || token instanceof ClangCommentToken ||
			(text.indexOf('\'') < 0 && text.indexOf('`') < 0)) {
			return text;
		}
		if (text.startsWith("\"")) {
			return text; // a string literal; embedded apostrophes are legal
		}
		if (text.matches("'(?:[^'\\\\]|\\\\.|\\\\x[0-9a-fA-F]{1,2}|\\\\[0-7]{1,3})'")) {
			return text; // a real single-character literal
		}
		return SpecialNames.normalize(text).replace(' ', '_');
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
