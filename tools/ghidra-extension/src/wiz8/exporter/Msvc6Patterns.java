package wiz8.exporter;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.OptionalLong;
import java.util.Set;

import ghidra.app.decompiler.ClangBreak;
import ghidra.app.decompiler.ClangCommentToken;
import ghidra.app.decompiler.ClangFieldToken;
import ghidra.app.decompiler.ClangFuncNameToken;
import ghidra.app.decompiler.ClangFuncProto;
import ghidra.app.decompiler.ClangFunction;
import ghidra.app.decompiler.ClangNode;
import ghidra.app.decompiler.ClangStatement;
import ghidra.app.decompiler.ClangToken;
import ghidra.app.decompiler.ClangTokenGroup;
import ghidra.app.decompiler.ClangTypeToken;
import ghidra.app.decompiler.ClangVariableDecl;
import ghidra.app.decompiler.ClangVariableToken;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.data.Pointer;
import ghidra.program.model.data.Structure;
import ghidra.program.model.data.TypeDef;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.pcode.HighFunction;
import ghidra.program.model.pcode.HighSymbol;
import ghidra.program.model.pcode.HighVariable;
import ghidra.program.model.pcode.PcodeOp;
import ghidra.program.model.pcode.Varnode;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolType;

/**
 * Recognition of MSVC/VC6 compiler-owned C++ lowering in the decompiled
 * token tree.
 *
 * Every recognizer works on positive evidence and declines otherwise. The
 * result only records which nodes to suppress or replace; the token writer
 * reproduces everything else verbatim.
 *
 * The recognizers run as named passes. Each pass is contained: a defect
 * inside one pass rolls back only that pass's claims and is recorded as a
 * defect (never silently), and after every pass the rendered body must
 * still satisfy the structural validator or the pass's claims are rolled
 * back with a trace record. Claims are owned: one pass may supersede
 * another only by claiming an enclosing node (the renderer's declared
 * outermost-wins rule); claiming a node another pass already claimed, or a
 * node inside another pass's replacement text, is a conflict and is
 * rejected with a trace record.
 */
final class Msvc6Patterns {

	/** One trace record: what a recognizer did, or why it did not. */
	static final class TraceEvent {
		final String pass;
		final String status; // applied | declined | rolled-back | failed
		final String detail;

		TraceEvent(String pass, String status, String detail) {
			this.pass = pass;
			this.status = status;
			this.detail = detail;
		}

		@Override
		public String toString() {
			return String.format("%-11s %s%s", status, pass,
				detail == null || detail.isEmpty() ? "" : ": " + detail);
		}
	}

	/** What the printer should do differently from verbatim output. */
	static final class Analysis {
		final Set<ClangNode> dropped = new HashSet<>();
		final Map<ClangNode, String> replaced = new HashMap<>();
		boolean liftSignature = false;
		/** Ephemeral per-function transformation trace, in pass order. */
		final List<TraceEvent> trace = new ArrayList<>();
		/** Unexpected exporter defects: {@code pass: exception}. */
		final List<String> defects = new ArrayList<>();
	}

	/** One meaningful element of the linearized body: a statement or a loose token. */
	private static final class Item {
		final ClangNode node;
		final int depth;

		Item(ClangNode node, int depth) {
			this.node = node;
			this.depth = depth;
		}

		boolean isStatement() {
			return node instanceof ClangStatement;
		}

		String text() {
			return node instanceof ClangToken token ? token.getText() : "";
		}
	}

	private final Function function;
	private final FunctionKind kind;
	private final DecompileResults results;
	private final Analysis analysis = new Analysis();

	private ClangFunction root;
	private List<Item> items;
	private Structure structure;
	/** Stack offset of the VC6 EH registration link slot, when the frame is proven. */
	private Long ehLinkOffset;
	/** The function's FuncInfo-derived exception model, when it resolved. */
	private EhModel ehModel;
	/** Compiler-computed null-preserving upcasts: local name -> replacement text. */
	private final Map<String, String> upcasts = new HashMap<>();
	/** Rendered names of the three proven VC6 EH registration-frame slots. */
	private final Set<String> ehSlotNames = new HashSet<>();
	/** Which pass owns each claimed node; drives conflict rejection. */
	private final Map<ClangNode, String> claimOwner = new HashMap<>();
	/** The pass currently running; claims and trace records attach to it. */
	private String currentPass;

	private Msvc6Patterns(Function function, FunctionKind kind, DecompileResults results) {
		this.function = function;
		this.kind = kind;
		this.results = results;
	}

	/**
	 * Run every recognizer pass. Preparation failures (no markup, no
	 * linearizable body) are the only whole-function bailouts and yield the
	 * empty analysis with a defect record; pass failures are contained per
	 * pass inside {@link #runPass}.
	 */
	static Analysis analyze(Function function, FunctionKind kind, DecompileResults results) {
		Msvc6Patterns patterns = new Msvc6Patterns(function, kind, results);
		try {
			patterns.run();
		}
		catch (Exception e) {
			patterns.analysis.defects.add("prepare: " + e);
			patterns.analysis.dropped.clear();
			patterns.analysis.replaced.clear();
			patterns.analysis.liftSignature = false;
		}
		return patterns.analysis;
	}

	private void run() {
		if (!(results.getCCodeMarkup() instanceof ClangFunction clangFunction)) {
			return;
		}
		root = clangFunction;
		items = linearize(root);
		structure = classStructure();

		// Statement-local token rewrites run first so the lifecycle and
		// allocation recognizers read their effective (already lifted) text.
		runPass("call.virtual", this::rewriteVirtualCalls);
		runPass("expression.array-index", this::rewriteArrayIndexing);
		runPass("expression.member-access", this::normalizeMemberAccess);
		runPass("expression.null-cast", this::rewriteNullPointerCasts);
		runPass("call.direct-member", this::rewriteMethodCalls);
		runPass("literal.narrow-string", this::rewriteStringLiterals);
		runPass("call.struct-return", this::rewriteStructReturns);

		runPass("eh.registration-frame", this::analyzeExceptionHandling);
		runPass("eh.stack-local", this::liftStackLocalsPass);

		if (kind == FunctionKind.CONSTRUCTOR || kind == FunctionKind.DESTRUCTOR) {
			runPass(kind == FunctionKind.CONSTRUCTOR ? "lifecycle.constructor"
					: "lifecycle.destructor",
				this::lifecyclePass);
		}
		runPass("allocation.pairs", this::rewriteAllocationPairs);
		if (kind == FunctionKind.ORDINARY) {
			runPass("signature.thiscall", this::liftOrdinarySignature);
		}
	}

	private void lifecyclePass() {
		Set<ClangNode> lifecycleDropped = new HashSet<>();
		List<String> initializers = new ArrayList<>();
		if (!analyzeLifecycle(lifecycleDropped, initializers)) {
			return;
		}
		for (ClangNode node : lifecycleDropped) {
			claimDrop(node);
		}
		analysis.liftSignature = true;
		claimReplace(findProto(root), liftedSignature(initializers));
		trace("applied", currentPass, "signature lifted with " + initializers.size() +
			" explicit initializer(s)");
	}

	// ------------------------------------------------------------------
	// Pass containment, claims, and trace
	// ------------------------------------------------------------------

	/**
	 * Run one pass with containment: a thrown defect rolls back only this
	 * pass's claims and records the failure; a pass whose claims break the
	 * structural validator is rolled back with a trace record. Trace events
	 * the pass produced stay visible in both cases.
	 */
	private void runPass(String name, Runnable body) {
		currentPass = name;
		Set<ClangNode> droppedBefore = new HashSet<>(analysis.dropped);
		Map<ClangNode, String> replacedBefore = new HashMap<>(analysis.replaced);
		Map<ClangNode, String> ownersBefore = new HashMap<>(claimOwner);
		boolean liftBefore = analysis.liftSignature;
		try {
			body.run();
			if (claimsChanged(droppedBefore, replacedBefore, liftBefore)) {
				Set<Integer> touched = new HashSet<>();
				String rendered = Wiz8CxxPrinter.render(root, analysis, touched);
				if (!Wiz8CxxPrinter.structurallySound(rendered, touched)) {
					restoreClaims(droppedBefore, replacedBefore, ownersBefore, liftBefore);
					trace("rolled-back", name, "structural validation failed");
				}
			}
		}
		catch (Exception e) {
			restoreClaims(droppedBefore, replacedBefore, ownersBefore, liftBefore);
			analysis.defects.add(name + ": " + e);
			trace("failed", name, String.valueOf(e));
		}
		finally {
			currentPass = null;
		}
	}

	private boolean claimsChanged(Set<ClangNode> dropped, Map<ClangNode, String> replaced,
			boolean liftSignature) {
		return !analysis.dropped.equals(dropped) || !analysis.replaced.equals(replaced) ||
			analysis.liftSignature != liftSignature;
	}

	private void restoreClaims(Set<ClangNode> dropped, Map<ClangNode, String> replaced,
			Map<ClangNode, String> owners, boolean liftSignature) {
		analysis.dropped.clear();
		analysis.dropped.addAll(dropped);
		analysis.replaced.clear();
		analysis.replaced.putAll(replaced);
		claimOwner.clear();
		claimOwner.putAll(owners);
		analysis.liftSignature = liftSignature;
	}

	/**
	 * Claim a node as dropped. A node already claimed by another pass, or
	 * sitting inside another pass's replacement text, is a conflict: the new
	 * claim is rejected and traced. A pass may re-claim and refine its own
	 * claims freely, and may supersede other passes only by claiming an
	 * enclosing node (the renderer's outermost-wins rule).
	 */
	private boolean claimDrop(ClangNode node) {
		if (conflicts(node)) {
			return false;
		}
		analysis.replaced.remove(node);
		analysis.dropped.add(node);
		claimOwner.put(node, currentPass);
		return true;
	}

	/** Claim a node as replaced by text; same conflict rules as drops. */
	private boolean claimReplace(ClangNode node, String replacement) {
		if (conflicts(node)) {
			return false;
		}
		analysis.dropped.remove(node);
		analysis.replaced.put(node, replacement);
		claimOwner.put(node, currentPass);
		return true;
	}

	/**
	 * Declared claim priority: statement-scope recognizers (lifecycle, EH,
	 * allocation) supersede token-scope rewrites (expression, literal, call,
	 * signature) on the same nodes, because they remove or replace whole
	 * compiler-owned constructs the token rewrites were polishing. Equal
	 * priority never overwrites across passes.
	 */
	private static int passPriority(String pass) {
		return pass != null && (pass.startsWith("lifecycle") || pass.startsWith("eh.") ||
			pass.startsWith("allocation")) ? 2 : 1;
	}

	private boolean conflicts(ClangNode node) {
		String owner = claimOwner.get(node);
		if (owner != null && !owner.equals(currentPass)) {
			if (passPriority(currentPass) > passPriority(owner)) {
				return false; // declared supersede: statement scope over token scope
			}
			trace("declined", currentPass,
				"claim conflict: node already claimed by " + owner);
			return true;
		}
		for (ClangNode ancestor = node.Parent(); ancestor != null;
				ancestor = ancestor.Parent()) {
			String ancestorOwner = claimOwner.get(ancestor);
			if (ancestorOwner != null && !ancestorOwner.equals(currentPass) &&
				analysis.replaced.containsKey(ancestor) &&
				passPriority(currentPass) <= passPriority(ancestorOwner)) {
				trace("declined", currentPass,
					"claim conflict: inside a node replaced by " + ancestorOwner);
				return true;
			}
		}
		return false;
	}

	private void trace(String status, String pass, String detail) {
		analysis.trace.add(new TraceEvent(pass == null ? "?" : pass, status, detail));
	}

	/** The listing address a statement's root operation executes at. */
	private static String statementAddress(ClangStatement statement) {
		PcodeOp op = statement.getPcodeOp();
		if (op == null || op.getSeqnum() == null) {
			return "?";
		}
		return "0x" + op.getSeqnum().getTarget().toString();
	}

	// ------------------------------------------------------------------
	// Tree plumbing
	// ------------------------------------------------------------------

	/** The statement region of the function: everything after the prototype. */
	private static List<ClangNode> bodyNodes(ClangFunction root) {
		List<ClangNode> nodes = new ArrayList<>();
		boolean seenProto = false;
		for (int i = 0; i < root.numChildren(); i++) {
			ClangNode child = root.Child(i);
			if (child instanceof ClangFuncProto) {
				seenProto = true;
				continue;
			}
			if (seenProto) {
				nodes.add(child);
			}
		}
		return nodes;
	}

	/**
	 * Flatten the post-prototype region into meaningful items with brace
	 * depth. Statements and variable declarations are leaf items; plain
	 * groups are transparent nesting artifacts and are descended into. The
	 * function's own outermost braces are excluded so the top level of the
	 * body is depth zero from its first item to its last.
	 */
	private static List<Item> linearize(ClangFunction root) {
		List<Item> flat = new ArrayList<>();
		int[] depth = {-1}; // the function's own '{' brings the body to depth 0
		for (ClangNode child : bodyNodes(root)) {
			collect(child, flat, depth);
		}
		return flat;
	}

	private static void collect(ClangNode node, List<Item> flat, int[] depth) {
		if (node instanceof ClangStatement || node instanceof ClangVariableDecl) {
			flat.add(new Item(node, Math.max(depth[0], 0)));
			return;
		}
		if (node instanceof ClangTokenGroup group) {
			for (int i = 0; i < group.numChildren(); i++) {
				collect(group.Child(i), flat, depth);
			}
			return;
		}
		if (node instanceof ClangBreak || node instanceof ClangCommentToken) {
			return;
		}
		if (node instanceof ClangToken token) {
			String text = token.getText();
			if (text == null || text.isBlank() || text.equals(";")) {
				return;
			}
			if (text.equals("{")) {
				depth[0]++;
				if (depth[0] > 0) {
					flat.add(new Item(node, depth[0]));
				}
				return;
			}
			if (text.equals("}")) {
				if (depth[0] > 0) {
					flat.add(new Item(node, depth[0]));
				}
				depth[0]--;
				return;
			}
			flat.add(new Item(node, Math.max(depth[0], 0)));
		}
	}

	private static ClangFuncProto findProto(ClangFunction root) {
		for (int i = 0; i < root.numChildren(); i++) {
			if (root.Child(i) instanceof ClangFuncProto proto) {
				return proto;
			}
		}
		throw new IllegalStateException("no function prototype in markup");
	}

	private static void leafTokens(ClangNode node, List<ClangToken> out) {
		if (node instanceof ClangTokenGroup group) {
			for (int i = 0; i < group.numChildren(); i++) {
				leafTokens(group.Child(i), out);
			}
		}
		else if (node instanceof ClangToken token && !(token instanceof ClangBreak)) {
			out.add(token);
		}
	}

	private String tokenText(ClangNode node) {
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(node, tokens);
		StringBuilder text = new StringBuilder();
		for (ClangToken token : tokens) {
			text.append(effectiveTokenText(token));
		}
		return text.toString();
	}

	/** The text a token contributes after claims and type mapping. */
	private String effectiveTokenText(ClangToken token) {
		String replacement = analysis.replaced.get(token);
		if (replacement != null) {
			return replacement;
		}
		if (analysis.dropped.contains(token)) {
			return "";
		}
		String text = token.getText();
		if (token instanceof ClangTypeToken) {
			text = TypeNames.map(text);
		}
		else {
			text = TypeNames.mapTemplateSpelling(text);
		}
		text = sanitizeReservedName(token, text);
		return text == null ? "" : text;
	}

	/**
	 * Ghidra names the receiver local of inlined method code {@code this}
	 * even inside a plain function, where the reserved word can never
	 * compile. The real this parameter is untouched.
	 */
	static String sanitizeReservedName(ClangNode node, String text) {
		if ("this".equals(text) && node instanceof ClangVariableToken variable &&
			!isThisSymbol(variable.getHighVariable())) {
			return "this_";
		}
		return text;
	}

	private boolean isClaimed(ClangNode node) {
		return analysis.dropped.contains(node) || analysis.replaced.containsKey(node);
	}

	/**
	 * One statement's leaf tokens, plus the non-blank subsequence the pattern
	 * matchers work on and its mapping back into the full list.
	 */
	private static final class TokenLine {
		final List<ClangToken> raw = new ArrayList<>();
		final List<ClangToken> sig = new ArrayList<>();
		final List<Integer> rawIndex = new ArrayList<>();
	}

	private static TokenLine tokenLine(ClangNode statement) {
		TokenLine line = new TokenLine();
		leafTokens(statement, line.raw);
		index(line);
		return line;
	}

	/**
	 * The whole function body as one token line. Conditions and loop heads
	 * are not statements in the markup, so expression-level rewrites must
	 * see every body token, not just statement tokens.
	 */
	private TokenLine bodyTokenLine() {
		TokenLine line = new TokenLine();
		for (ClangNode node : bodyNodes(root)) {
			leafTokens(node, line.raw);
		}
		index(line);
		return line;
	}

	private static void index(TokenLine line) {
		for (int i = 0; i < line.raw.size(); i++) {
			String text = line.raw.get(i).getText();
			if (text != null && !text.isBlank()) {
				line.sig.add(line.raw.get(i));
				line.rawIndex.add(i);
			}
		}
	}

	/** The line's significant tokens that no recognizer has claimed yet. */
	private TokenLine liveView(TokenLine line) {
		TokenLine live = new TokenLine();
		live.raw.addAll(line.raw);
		for (int i = 0; i < line.sig.size(); i++) {
			if (!isClaimed(line.sig.get(i))) {
				live.sig.add(line.sig.get(i));
				live.rawIndex.add(line.rawIndex.get(i));
			}
		}
		return live;
	}

	/**
	 * Claim the raw token span between two significant tokens: the whole span
	 * disappears and the replacement text prints once in its place.
	 */
	/**
	 * Claim a whole token span atomically: every token disappears and the
	 * replacement prints once in the first token's place. The claim is
	 * all-or-nothing — if the anchor token or any other pass's replacement
	 * sits inside the span, the whole range declines, because a partial
	 * claim would print duplicated or orphaned text.
	 */
	private boolean claimRange(TokenLine line, int sigFrom, int sigTo, String replacement) {
		int rawFrom = line.rawIndex.get(sigFrom);
		int rawTo = line.rawIndex.get(sigTo);
		ClangToken first = line.raw.get(rawFrom);
		String firstOwner = claimOwner.get(first);
		if (firstOwner != null && !firstOwner.equals(currentPass) &&
			passPriority(currentPass) <= passPriority(firstOwner)) {
			trace("declined", currentPass,
				"claim conflict: range anchor already claimed by " + firstOwner);
			return false;
		}
		for (int i = rawFrom; i <= rawTo; i++) {
			ClangToken token = line.raw.get(i);
			String owner = claimOwner.get(token);
			if (owner != null && !owner.equals(currentPass) &&
				analysis.replaced.containsKey(token) &&
				passPriority(currentPass) <= passPriority(owner)) {
				trace("declined", currentPass,
					"claim conflict: range contains a replacement owned by " + owner);
				return false;
			}
		}
		for (int i = rawFrom; i <= rawTo; i++) {
			ClangToken token = line.raw.get(i);
			String owner = claimOwner.get(token);
			if (owner == null || owner.equals(currentPass) ||
				passPriority(currentPass) > passPriority(owner)) {
				claimDrop(token);
			}
		}
		if (replacement.isEmpty()) {
			return true;
		}
		return claimReplace(first, replacement);
	}

	private void dropToken(ClangToken token) {
		claimDrop(token);
	}

	/** The index of the matching close paren in the significant list, or -1. */
	private static int matchingParen(List<ClangToken> sig, int open) {
		int depth = 0;
		for (int i = open; i < sig.size(); i++) {
			String text = sig.get(i).getText();
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

	// ------------------------------------------------------------------
	// P-code plumbing
	// ------------------------------------------------------------------

	private Function callee(PcodeOp op) {
		if (op == null || op.getOpcode() != PcodeOp.CALL || op.getNumInputs() < 1) {
			return null;
		}
		Varnode target = op.getInput(0);
		if (!target.isAddress()) {
			return null;
		}
		return function.getProgram().getFunctionManager().getFunctionAt(target.getAddress());
	}

	private static boolean isThisSymbol(HighVariable high) {
		if (high == null) {
			return false;
		}
		HighSymbol symbol = high.getSymbol();
		if (symbol == null) {
			return false;
		}
		return symbol.isThisPointer() ||
			(symbol.isParameter() && "this".equals(symbol.getName()));
	}

	/** Trace a varnode to a constant offset from the current `this`. */
	private OptionalLong thisOffset(Varnode varnode) {
		Varnode current = varnode;
		long offset = 0;
		for (int step = 0; current != null && step < 64; step++) {
			if (isThisSymbol(current.getHigh())) {
				return OptionalLong.of(offset);
			}
			PcodeOp def = current.getDef();
			if (def == null) {
				return OptionalLong.empty();
			}
			switch (def.getOpcode()) {
				case PcodeOp.COPY:
				case PcodeOp.CAST:
				case PcodeOp.INDIRECT:
					current = def.getInput(0);
					break;
				case PcodeOp.PTRSUB:
				case PcodeOp.INT_ADD:
					if (!def.getInput(1).isConstant()) {
						return OptionalLong.empty();
					}
					offset += def.getInput(1).getOffset();
					current = def.getInput(0);
					break;
				case PcodeOp.PTRADD:
					if (!def.getInput(1).isConstant() || !def.getInput(2).isConstant()) {
						return OptionalLong.empty();
					}
					offset += def.getInput(1).getOffset() * def.getInput(2).getOffset();
					current = def.getInput(0);
					break;
				case PcodeOp.INT_AND: {
					// VC6 null-preserving adjustment: -(uint)(this != 0) & (this + k)
					Varnode left = def.getInput(0);
					Varnode right = def.getInput(1);
					if (isThisNullMask(left)) {
						current = right;
					}
					else if (isThisNullMask(right)) {
						current = left;
					}
					else {
						return OptionalLong.empty();
					}
					break;
				}
				default:
					return OptionalLong.empty();
			}
		}
		return OptionalLong.empty();
	}

	/** Whether the varnode computes the all-ones/all-zeroes mask of `this != 0`. */
	private boolean isThisNullMask(Varnode varnode) {
		Varnode current = varnode;
		for (int step = 0; current != null && step < 16; step++) {
			PcodeOp def = current.getDef();
			if (def == null) {
				return false;
			}
			switch (def.getOpcode()) {
				case PcodeOp.COPY:
				case PcodeOp.CAST:
				case PcodeOp.INT_2COMP:
				case PcodeOp.INT_NEGATE:
				case PcodeOp.INT_ZEXT:
				case PcodeOp.INT_SEXT:
				case PcodeOp.SUBPIECE:
					current = def.getInput(0);
					break;
				case PcodeOp.INT_MULT:
					current = def.getInput(0).isConstant() ? def.getInput(1) : def.getInput(0);
					break;
				case PcodeOp.INT_NOTEQUAL:
				case PcodeOp.INT_EQUAL: {
					Varnode a = def.getInput(0);
					Varnode b = def.getInput(1);
					Varnode compared = a.isConstant() && a.getOffset() == 0 ? b
							: b.isConstant() && b.getOffset() == 0 ? a : null;
					if (compared == null) {
						return false;
					}
					OptionalLong traced = thisOffset(compared);
					return traced.isPresent() && traced.getAsLong() == 0;
				}
				default:
					return false;
			}
		}
		return false;
	}

	private Structure classStructure() {
		HighFunction high = results.getHighFunction();
		if (high == null || high.getFunctionPrototype() == null ||
			high.getFunctionPrototype().getNumParams() == 0) {
			return null;
		}
		DataType type = high.getFunctionPrototype().getParam(0).getDataType();
		if (!(type instanceof Pointer pointer)) {
			return null;
		}
		DataType pointed = pointer.getDataType();
		while (pointed instanceof TypeDef typedef) {
			pointed = typedef.getBaseDataType();
		}
		return pointed instanceof Structure struct ? struct : null;
	}

	// ------------------------------------------------------------------
	// EH frame and vftable stores
	// ------------------------------------------------------------------

	/**
	 * Recover MSVC exception-handling lifetimes from the registration frame
	 * and the FuncInfo record it installs.
	 *
	 * When the record resolves and declares no {@code try} blocks, every EH
	 * statement in the body is compiler scaffolding regardless of the
	 * function's kind, and each unwind state that destroys a directly
	 * addressed frame object marks a source-level local whose constructor
	 * call becomes its definition. A record with {@code try} blocks means
	 * the registration carries source semantics, so everything stays
	 * verbatim. An unresolvable record falls back to the frame-shape-only
	 * suppression that constructors and destructors used before the model
	 * existed.
	 */
	private void analyzeExceptionHandling() {
		detectEhFrame();
		if (ehLinkOffset == null) {
			return;
		}
		ehModel = resolveEhModel();
		if (ehModel != null && ehModel.tryBlockCount > 0) {
			trace("declined", currentPass, "FuncInfo declares " + ehModel.tryBlockCount +
				" try block(s); the registration carries source semantics");
			ehLinkOffset = null;
			ehModel = null;
			return;
		}
		if (ehModel == null && kind != FunctionKind.CONSTRUCTOR &&
			kind != FunctionKind.DESTRUCTOR) {
			trace("declined", currentPass,
				"EH frame detected but FuncInfo unresolved; only lifecycle bodies " +
					"use the frame-shape fallback");
			ehLinkOffset = null;
			return;
		}
		Set<ClangNode> ehDropped = new HashSet<>();
		for (Item item : items) {
			if (item.isStatement()) {
				ClangStatement statement = (ClangStatement) item.node;
				if (isEhScaffolding(statement)) {
					ehDropped.add(statement);
				}
			}
			else if (item.node instanceof ClangVariableDecl declaration &&
				isEhSlotDeclaration(declaration)) {
				rememberEhSlotNames(declaration);
				ehDropped.add(declaration);
			}
		}
		// Subpiece stores such as local_4._0_1_ = 1 can have a p-code
		// output in unique space even though the rendered lvalue is the proven
		// state slot. Drop the complete statement by its remembered declaration
		// name; otherwise token-level rewrites can leave a dangling literal.
		for (Item item : items) {
			if (item.isStatement() && mentionsEhSlotName(item.node)) {
				ehDropped.add(item.node);
			}
		}
		if (ehSlotReferencesSurvive(ehDropped)) {
			// A slot access the statement pass cannot claim (for example a
			// state store folded into a condition) would print as a dangling
			// reference; keep the whole frame verbatim instead.
			trace("declined", currentPass,
				"an EH slot reference survives outside the claimable statements");
			ehLinkOffset = null;
			ehModel = null;
			return;
		}
		for (ClangNode node : ehDropped) {
			claimDrop(node);
		}
		trace("applied", currentPass, ehDropped.size() + " scaffolding statement(s)/" +
			"declaration(s) suppressed" + (ehModel == null ? " (frame-shape fallback)" : ""));
	}

	/** The eh.stack-local pass: runs only when the model survived the frame pass. */
	private void liftStackLocalsPass() {
		if (ehModel == null || ehLinkOffset == null) {
			return;
		}
		liftStackLocals();
	}

	/**
	 * Resolve the FuncInfo model from the handler thunk the prolog stores
	 * into the slot above the registration link.
	 */
	private EhModel resolveEhModel() {
		Long thunk = null;
		for (Item item : items) {
			if (!item.isStatement()) {
				continue;
			}
			PcodeOp op = ((ClangStatement) item.node).getPcodeOp();
			if (op == null) {
				continue;
			}
			Varnode output = op.getOutput();
			if (output == null || output.getAddress() == null ||
				!output.getAddress().isStackAddress() ||
				output.getAddress().getOffset() != ehLinkOffset + 4) {
				continue;
			}
			Long value = storedConstant(op);
			if (value == null) {
				continue;
			}
			if (thunk != null && !thunk.equals(value)) {
				return null;
			}
			thunk = value;
		}
		if (thunk == null) {
			return null;
		}
		return EhModel.resolve(function.getProgram(), thunk);
	}

	/**
	 * The constant value a defining op stores, when the whole expression
	 * folds to one. An address-of prints as {@code PTRSUB(0, offset)}, so
	 * constant-folding the pointer arithmetic recovers label addresses too.
	 */
	private static Long storedConstant(PcodeOp op) {
		return storedConstantAt(op, 0);
	}

	/** The constant a varnode carries, folding copies, casts, and adds. */
	private static Long tracedConstant(Varnode varnode, int depth) {
		if (varnode == null || depth > 8) {
			return null;
		}
		if (varnode.isConstant()) {
			return varnode.getOffset();
		}
		if (varnode.isAddress()) {
			return varnode.getAddress().getOffset();
		}
		PcodeOp def = varnode.getDef();
		return def == null ? null : storedConstantAt(def, depth + 1);
	}

	private static Long storedConstantAt(PcodeOp op, int depth) {
		switch (op.getOpcode()) {
			case PcodeOp.COPY:
			case PcodeOp.CAST:
			case PcodeOp.INDIRECT:
				return tracedConstant(op.getInput(0), depth);
			case PcodeOp.PTRSUB:
			case PcodeOp.INT_ADD: {
				Long left = tracedConstant(op.getInput(0), depth);
				Long right = tracedConstant(op.getInput(1), depth);
				return left == null || right == null ? null : left + right;
			}
			default:
				return null;
		}
	}

	/**
	 * Whether any body token still reaches an EH slot after the planned
	 * drops. The declarations and statement claims must account for every
	 * reference, or the printed body would name a variable that no longer
	 * exists.
	 */
	private boolean ehSlotReferencesSurvive(Set<ClangNode> ehDropped) {
		List<ClangToken> tokens = new ArrayList<>();
		for (ClangNode node : bodyNodes(root)) {
			leafTokens(node, tokens);
		}
		for (ClangToken token : tokens) {
			if (!isEhSlotReference(token) && !ehSlotNames.contains(token.getText())) {
				continue;
			}
			boolean claimed = false;
			for (ClangNode current = token; current != null; current = current.Parent()) {
				if (ehDropped.contains(current) || isClaimed(current)) {
					claimed = true;
					break;
				}
			}
			if (!claimed) {
				return true;
			}
		}
		return false;
	}

	private void rememberEhSlotNames(ClangVariableDecl declaration) {
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(declaration, tokens);
		for (ClangToken token : tokens) {
			if (token instanceof ClangVariableToken && token.getText() != null) {
				ehSlotNames.add(token.getText());
			}
		}
	}

	private boolean mentionsEhSlotName(ClangNode node) {
		if (ehSlotNames.isEmpty()) {
			return false;
		}
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(node, tokens);
		for (ClangToken token : tokens) {
			if (ehSlotNames.contains(token.getText())) {
				return true;
			}
		}
		return false;
	}

	private boolean isEhSlotReference(ClangToken token) {
		if (!(token instanceof ClangVariableToken variable)) {
			return false;
		}
		if ("ExceptionList".equals(token.getText())) {
			return true;
		}
		HighVariable high = variable.getHighVariable();
		Varnode representative = high == null ? null : high.getRepresentative();
		if (representative == null || representative.getAddress() == null ||
			!representative.getAddress().isStackAddress()) {
			return false;
		}
		long offset = representative.getAddress().getOffset();
		return offset >= ehLinkOffset && offset < ehLinkOffset + 12;
	}

	// ------------------------------------------------------------------
	// Stack-local object lifetimes
	// ------------------------------------------------------------------

	/**
	 * Turn each proven frame object back into a local definition. The
	 * unwind map names the slot and the destructor; the body must contain
	 * exactly one constructor call of the same class on that slot and at
	 * least one direct destructor call, which the source never spelled.
	 * The registration link sits at {@code ebp-12} in every VC6 EH frame,
	 * which anchors the ebp-to-decompiler offset conversion.
	 */
	private void liftStackLocals() {
		long delta = ehLinkOffset + 12;
		for (EhModel.UnwindState state : ehModel.states) {
			if (state.destructor == null || state.objectEbpOffset == null) {
				continue;
			}
			liftStackLocal(state.objectEbpOffset + delta, state.destructor);
		}
	}

	private void liftStackLocal(long stackOffset, Function destructor) {
		Namespace owner = destructor.getParentNamespace();
		ClangStatement constructorStatement = null;
		PcodeOp constructorOp = null;
		ClangVariableToken local = null;
		List<ClangStatement> destructorCalls = new ArrayList<>();
		for (Item item : items) {
			if (!item.isStatement() || isClaimed(item.node)) {
				continue;
			}
			ClangStatement statement = (ClangStatement) item.node;
			PcodeOp op = statement.getPcodeOp();
			Function target = callee(op);
			if (target == null || op.getNumInputs() < 2 ||
				!owner.equals(target.getParentNamespace())) {
				continue;
			}
			FunctionKind targetKind = FunctionKind.classify(target);
			ClangVariableToken receiver = addressOfLocalReceiver(statement, op, stackOffset);
			if (receiver == null) {
				continue;
			}
			if (targetKind == FunctionKind.CONSTRUCTOR) {
				if (constructorStatement != null) {
					trace("declined", currentPass, "slot " + stackOffset +
						": two constructions of one slot are not one lifetime");
					return;
				}
				constructorStatement = statement;
				constructorOp = op;
				local = receiver;
			}
			else if (targetKind == FunctionKind.DESTRUCTOR) {
				destructorCalls.add(statement);
			}
		}
		if (constructorStatement == null || destructorCalls.isEmpty()) {
			trace("declined", currentPass, "slot " + stackOffset + " (~" +
				destructor.getName() + "): " +
				(constructorStatement == null
						? "no direct same-class constructor call on the slot"
						: "no direct destructor call on the slot"));
			return;
		}
		List<String> arguments = callArgumentsAfterReceiver(constructorStatement, constructorOp);
		if (arguments == null) {
			trace("declined", currentPass, "slot " + stackOffset +
				": constructor argument tokens disagree with the p-code operands");
			return;
		}
		String name = local.getText();
		ClangVariableDecl declaration = localDeclaration(name, stackOffset);
		if (declaration == null) {
			trace("declined", currentPass, "slot " + stackOffset +
				": no hoisted declaration found for " + name);
			return;
		}
		String type = TypeNames.map(owner.getName());
		String definition = arguments.isEmpty() ? type + " " + name
				: type + " " + name + "(" + String.join(", ", arguments) + ")";
		claimReplace(constructorStatement, definition);
		claimDrop(declaration);
		for (ClangStatement call : destructorCalls) {
			claimDrop(call);
		}
		trace("applied", currentPass, type + " " + name + " @ " +
			statementAddress(constructorStatement) + ", " + destructorCalls.size() +
			" scope-end destructor call(s) dropped");
	}

	/**
	 * The call's receiver argument when it is exactly {@code &local} for the
	 * stack slot at the given decompiler offset; null otherwise.
	 */
	private ClangVariableToken addressOfLocalReceiver(ClangStatement statement, PcodeOp op,
			long stackOffset) {
		List<List<ClangToken>> arguments = callArgumentTokens(statement, op);
		if (arguments == null || arguments.isEmpty()) {
			return null;
		}
		List<ClangToken> receiver = arguments.get(0);
		if (receiver.size() != 2 || !"&".equals(receiver.get(0).getText()) ||
			!(receiver.get(1) instanceof ClangVariableToken variable)) {
			return null;
		}
		Long traced = stackAddressOffset(op.getInput(1));
		if (traced == null || traced != stackOffset) {
			return null;
		}
		return variable;
	}

	/**
	 * The frame offset a pointer varnode addresses, when it is a plain
	 * address-of-stack-slot: {@code PTRSUB(stack pointer, #offset)} behind
	 * any number of copies and casts. The constant is sign-extended from
	 * its storage size, matching the decompiler's rendering.
	 */
	private Long stackAddressOffset(Varnode varnode) {
		Varnode current = varnode;
		for (int step = 0; current != null && step < 8; step++) {
			PcodeOp def = current.getDef();
			if (def == null) {
				return null;
			}
			switch (def.getOpcode()) {
				case PcodeOp.COPY:
				case PcodeOp.CAST:
				case PcodeOp.INDIRECT:
					current = def.getInput(0);
					break;
				case PcodeOp.PTRSUB: {
					Varnode base = def.getInput(0);
					Varnode offset = def.getInput(1);
					if (!base.isRegister() || !offset.isConstant() ||
						!isStackPointer(base)) {
						return null;
					}
					long value = offset.getOffset();
					if (offset.getSize() == 4) {
						value = (int) value;
					}
					return value;
				}
				default:
					return null;
			}
		}
		return null;
	}

	private boolean isStackPointer(Varnode varnode) {
		var register = function.getProgram().getCompilerSpec().getStackPointer();
		return register != null && varnode.getAddress().equals(register.getAddress()) &&
			varnode.getSize() == register.getNumBytes();
	}

	/**
	 * The declaration of the local at the given stack slot. Storage decides
	 * where the decompiler exposed it; an aggregate whose uses are all
	 * address-of carries no high variable on its declaration token, so its
	 * unique rendered name identifies it instead.
	 */
	private ClangVariableDecl localDeclaration(String name, long stackOffset) {
		for (Item item : items) {
			if (!(item.node instanceof ClangVariableDecl declaration)) {
				continue;
			}
			List<ClangToken> tokens = new ArrayList<>();
			leafTokens(declaration, tokens);
			for (ClangToken token : tokens) {
				if (!(token instanceof ClangVariableToken variable)) {
					continue;
				}
				HighVariable high = variable.getHighVariable();
				Varnode representative = high == null ? null : high.getRepresentative();
				if (representative != null && representative.getAddress() != null &&
					representative.getAddress().isStackAddress() &&
					representative.getAddress().getOffset() == stackOffset) {
					return declaration;
				}
				if (representative == null && name.equals(variable.getText())) {
					return declaration;
				}
			}
		}
		return null;
	}

	private static boolean mentionsExceptionList(ClangNode statement) {
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(statement, tokens);
		for (ClangToken token : tokens) {
			if (token instanceof ClangVariableToken && "ExceptionList".equals(token.getText())) {
				return true;
			}
		}
		return false;
	}

	/**
	 * Prove the VC6 EH registration frame and remember the link slot. The
	 * handler and state slots sit at fixed offsets above it, so their stores
	 * are identifiable wherever the compiler scheduled them.
	 */
	private void detectEhFrame() {
		boolean registered = false;
		for (Item item : items) {
			if (!item.isStatement() || !mentionsExceptionList(item.node)) {
				continue;
			}
			PcodeOp op = ((ClangStatement) item.node).getPcodeOp();
			if (op == null) {
				continue;
			}
			Varnode output = op.getOutput();
			if (output != null && output.getAddress() != null &&
				output.getAddress().isStackAddress()) {
				ehLinkOffset = output.getAddress().getOffset();
			}
			else {
				registered = true; // a write into ExceptionList itself
			}
		}
		if (!registered) {
			ehLinkOffset = null;
		}
	}

	private boolean isEhScaffolding(ClangStatement statement) {
		if (mentionsExceptionList(statement)) {
			return ehLinkOffset != null;
		}
		if (ehLinkOffset == null) {
			return false;
		}
		PcodeOp op = statement.getPcodeOp();
		if (op == null) {
			return false;
		}
		Varnode output = op.getOutput();
		if (output == null || output.getAddress() == null ||
			!output.getAddress().isStackAddress()) {
			return false;
		}
		long start = output.getAddress().getOffset();
		long end = start + output.getSize();
		long slots = ehLinkOffset + 4;      // handler slot
		long slotsEnd = ehLinkOffset + 12;  // past the state slot
		return start < slotsEnd && end > slots;
	}

	/** A local declaration for one of the three proven EH frame slots. */
	private boolean isEhSlotDeclaration(ClangVariableDecl declaration) {
		if (ehLinkOffset == null) {
			return false;
		}
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(declaration, tokens);
		for (ClangToken token : tokens) {
			if (!(token instanceof ClangVariableToken variable)) {
				continue;
			}
			HighVariable high = variable.getHighVariable();
			if (high == null || high.getRepresentative() == null) {
				continue;
			}
			Varnode representative = high.getRepresentative();
			if (representative.getAddress() == null ||
				!representative.getAddress().isStackAddress()) {
				continue;
			}
			long offset = representative.getAddress().getOffset();
			return offset >= ehLinkOffset && offset < ehLinkOffset + 12;
		}
		return false;
	}

	private boolean isVftableStore(ClangStatement statement) {
		PcodeOp op = statement.getPcodeOp();
		if (op == null || op.getOpcode() != PcodeOp.STORE) {
			return false;
		}
		OptionalLong destination = thisOffset(op.getInput(1));
		if (destination.isEmpty()) {
			return false;
		}
		Varnode value = op.getInput(2);
		if (valueIsVftableSymbol(value)) {
			return true;
		}
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(statement, tokens);
		boolean afterAssign = false;
		for (ClangToken token : tokens) {
			String text = token.getText();
			if ("=".equals(text)) {
				afterAssign = true;
				continue;
			}
			if (afterAssign && text != null &&
				FunctionKind.normalizeSpecialName(text).startsWith("vftable")) {
				return true;
			}
		}
		return false;
	}

	private boolean valueIsVftableSymbol(Varnode value) {
		Varnode current = value;
		for (int step = 0; current != null && step < 8; step++) {
			Address address = null;
			if (current.isAddress()) {
				address = current.getAddress();
			}
			else if (current.isConstant()) {
				address = function.getProgram().getAddressFactory().getDefaultAddressSpace()
						.getAddress(current.getOffset());
			}
			if (address != null) {
				Symbol symbol =
					function.getProgram().getSymbolTable().getPrimarySymbol(address);
				return symbol != null &&
					FunctionKind.normalizeSpecialName(symbol.getName()).startsWith("vftable");
			}
			PcodeOp def = current.getDef();
			if (def == null || (def.getOpcode() != PcodeOp.COPY &&
				def.getOpcode() != PcodeOp.CAST)) {
				return false;
			}
			current = def.getInput(0);
		}
		return false;
	}

	// ------------------------------------------------------------------
	// Constructor / destructor lifting
	// ------------------------------------------------------------------

	private boolean analyzeLifecycle(Set<ClangNode> dropped, List<String> initializers) {
		List<ClangVariableDecl> params = protoParameters();
		if (params.isEmpty() || !tokenText(params.get(0)).endsWith("this")) {
			trace("declined", currentPass, "prototype does not carry the this parameter");
			return false;
		}
		int vptrStores = 0;
		for (Item item : items) {
			if (item.isStatement() && !isClaimed(item.node) &&
				isVftableStore((ClangStatement) item.node)) {
				dropped.add(item.node);
				vptrStores++;
			}
		}
		if (vptrStores > 0) {
			trace("applied", "lifecycle.vptr-store", "x" + vptrStores);
		}
		if (kind == FunctionKind.CONSTRUCTOR) {
			consumeConstructorPrefix(dropped, initializers);
		}
		else {
			consumeDestructorTail(dropped);
		}
		if (!noSubobjectLifecycleCallsRemain(dropped)) {
			trace("declined", currentPass, "an unproven subobject lifecycle call " +
				"remains in the body; the whole lift is withdrawn");
			return false;
		}
		return true;
	}

	private void consumeConstructorPrefix(Set<ClangNode> dropped, List<String> initializers) {
		int index = 0;
		while (index < items.size()) {
			Item item = items.get(index);
			if (item.depth != 0) {
				break;
			}
			if (item.node instanceof ClangVariableDecl) {
				index++;
				continue;
			}
			if (item.isStatement()) {
				ClangStatement statement = (ClangStatement) item.node;
				if (dropped.contains(statement) || isClaimed(statement)) {
					index++;
					continue;
				}
				String initializer = subobjectConstructorInitializer(statement);
				if (initializer == null) {
					break;
				}
				if (!initializer.isEmpty()) {
					initializers.add(initializer);
				}
				dropped.add(statement);
				index++;
				continue;
			}
			if ("if".equals(item.text())) {
				int consumed = matchNullPreservingUpcast(index, dropped);
				if (consumed > 0) {
					index += consumed;
					continue;
				}
			}
			break;
		}
	}

	/**
	 * A subobject construction becomes an initializer: base classes by the
	 * repository's base/base_* field convention, members by field name.
	 * Empty argument lists are implicit C++ and produce no initializer text.
	 */
	private String subobjectConstructorInitializer(ClangStatement statement) {
		PcodeOp op = statement.getPcodeOp();
		Function target = callee(op);
		if (target == null || FunctionKind.classify(target) != FunctionKind.CONSTRUCTOR ||
			op.getNumInputs() < 2 || structure == null) {
			return null;
		}
		OptionalLong offset = thisOffset(op.getInput(1));
		if (offset.isEmpty() || offset.getAsLong() < 0 ||
			offset.getAsLong() > Integer.MAX_VALUE) {
			return null;
		}
		DataTypeComponent component =
			structure.getComponentContaining((int) offset.getAsLong());
		if (component == null || component.getOffset() != offset.getAsLong() ||
			component.getFieldName() == null) {
			return null;
		}
		List<String> arguments = callArgumentsAfterReceiver(statement, op);
		if (arguments == null) {
			return null;
		}
		boolean base = isBaseField(component.getFieldName());
		String name = base ? TypeNames.map(target.getParentNamespace().getName())
				: component.getFieldName();
		trace("applied", base ? "lifecycle.base-initializer" : "lifecycle.member-initializer",
			name + " @ this+0x" + Long.toHexString(offset.getAsLong()) + ", " +
				statementAddress(statement));
		if (arguments.isEmpty()) {
			return "";
		}
		return name + "(" + String.join(", ", arguments) + ")";
	}

	private static boolean isBaseField(String fieldName) {
		return fieldName.equals("base") || fieldName.startsWith("base_");
	}

	/**
	 * The call's rendered arguments, minus the receiver, with compiler
	 * upcast locals substituted. Returns null when the token shape and the
	 * p-code disagree.
	 */
	private List<String> callArgumentsAfterReceiver(ClangStatement statement, PcodeOp op) {
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(statement, tokens);
		int open = -1;
		for (int i = 0; i < tokens.size(); i++) {
			if (tokens.get(i) instanceof ClangFuncNameToken) {
				for (int j = i + 1; j < tokens.size(); j++) {
					if ("(".equals(tokens.get(j).getText())) {
						open = j;
						break;
					}
				}
				break;
			}
		}
		if (open < 0) {
			return null;
		}
		List<String> arguments = new ArrayList<>();
		StringBuilder current = new StringBuilder();
		int depth = 1;
		for (int i = open + 1; i < tokens.size() && depth > 0; i++) {
			ClangToken token = tokens.get(i);
			String text = token.getText();
			if (text == null) {
				continue;
			}
			switch (text) {
				case "(":
					depth++;
					current.append(effectiveTokenText(token));
					break;
				case ")":
					depth--;
					if (depth > 0) {
						current.append(effectiveTokenText(token));
					}
					break;
				case ",":
					if (depth == 1) {
						arguments.add(finishArgument(current));
						current.setLength(0);
					}
					else {
						current.append(effectiveTokenText(token));
					}
					break;
				default:
					current.append(effectiveTokenText(token));
			}
		}
		if (current.length() > 0 || !arguments.isEmpty()) {
			arguments.add(finishArgument(current));
		}
		if (arguments.size() != op.getNumInputs() - 1) {
			return null;
		}
		return arguments.subList(1, arguments.size());
	}

	private String finishArgument(StringBuilder text) {
		String argument = text.toString().trim();
		return upcasts.getOrDefault(argument, argument);
	}

	/**
	 * The compiler's null-preserving derived-to-base conversion:
	 * {@code if (p == 0) t = 0; else t = &p->base_X;} collapses back into
	 * the source-level cast {@code (X *)p}. Returns the number of consumed
	 * items, or 0 when the shape does not match.
	 */
	private int matchNullPreservingUpcast(int start, Set<ClangNode> dropped) {
		List<ClangNode> consumed = new ArrayList<>();
		int i = start;
		// condition: if ( <var> == <cast tokens> 0x0 )
		String tested = null;
		boolean sawEquals = false;
		boolean sawNull = false;
		while (i < items.size() && !"{".equals(items.get(i).text())) {
			Item item = items.get(i);
			if (item.isStatement()) {
				return 0;
			}
			String text = item.text();
			if (item.node instanceof ClangVariableToken) {
				if ("0x0".equals(text)) {
					sawNull = true;
				}
				else if (tested == null) {
					tested = text;
				}
				else {
					return 0;
				}
			}
			else if ("==".equals(text)) {
				sawEquals = true;
			}
			else if ("!=".equals(text) || "if".equals(text) || "*".equals(text) ||
				item.node instanceof ClangTypeToken) {
				if ("!=".equals(text)) {
					return 0;
				}
			}
			consumed.add(item.node);
			i++;
		}
		if (tested == null || !sawEquals || !sawNull || i >= items.size()) {
			return 0;
		}
		Branch nullBranch = matchBranch(i, consumed);
		if (nullBranch == null) {
			return 0;
		}
		i = nullBranch.next;
		if (i >= items.size() || !"else".equals(items.get(i).text())) {
			return 0;
		}
		consumed.add(items.get(i).node);
		i++;
		Branch fieldBranch = matchBranch(i, consumed);
		if (fieldBranch == null) {
			return 0;
		}
		i = fieldBranch.next;

		String destination = assignedVariable(nullBranch.statement);
		if (destination == null ||
			!destination.equals(assignedVariable(fieldBranch.statement))) {
			return 0;
		}
		String castType = nullAssignmentCastType(nullBranch.statement);
		String source = baseFieldSource(fieldBranch.statement, tested);
		if (castType == null && source != null) {
			// branches may be in either order
			castType = nullAssignmentCastType(fieldBranch.statement);
			source = baseFieldSource(nullBranch.statement, tested);
		}
		if (castType == null || source == null) {
			return 0;
		}
		upcasts.put(destination, "(" + castType + " *)" + source);
		dropped.addAll(consumed);
		dropped.add(nullBranch.statement);
		dropped.add(fieldBranch.statement);
		return i - start;
	}

	private static final class Branch {
		final ClangStatement statement;
		final int next;

		Branch(ClangStatement statement, int next) {
			this.statement = statement;
			this.next = next;
		}
	}

	/** Match `{ <single statement> }` starting at the opening brace item. */
	private Branch matchBranch(int start, List<ClangNode> consumed) {
		int i = start;
		if (i >= items.size() || !"{".equals(items.get(i).text())) {
			return null;
		}
		consumed.add(items.get(i).node);
		i++;
		if (i >= items.size() || !items.get(i).isStatement()) {
			return null;
		}
		ClangStatement statement = (ClangStatement) items.get(i).node;
		i++;
		if (i >= items.size() || !"}".equals(items.get(i).text())) {
			return null;
		}
		consumed.add(items.get(i).node);
		return new Branch(statement, i + 1);
	}

	private static String assignedVariable(ClangStatement statement) {
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(statement, tokens);
		if (tokens.isEmpty() || !(tokens.get(0) instanceof ClangVariableToken)) {
			return null;
		}
		return tokens.get(0).getText();
	}

	/** For `t = (X *)0x0`, the cast type name X; null when it is not that shape. */
	private static String nullAssignmentCastType(ClangStatement statement) {
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(statement, tokens);
		String type = null;
		boolean sawNull = false;
		for (ClangToken token : tokens) {
			if (token instanceof ClangTypeToken) {
				type = token.getText();
			}
			if (token instanceof ClangVariableToken && "0x0".equals(token.getText())) {
				sawNull = true;
			}
			if ("&".equals(token.getText()) || "->".equals(token.getText())) {
				return null;
			}
		}
		return sawNull ? type : null;
	}

	/** For `t = &src->base_X` with the tested variable as src, the source text. */
	private static String baseFieldSource(ClangStatement statement, String tested) {
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(statement, tokens);
		boolean sawAmp = false;
		String source = null;
		String field = null;
		for (int i = 1; i < tokens.size(); i++) {
			ClangToken token = tokens.get(i);
			String text = token.getText();
			if ("&".equals(text)) {
				sawAmp = true;
			}
			else if (token instanceof ClangVariableToken && sawAmp && source == null) {
				source = text;
			}
			else if (token instanceof ClangFieldToken) {
				field = text;
			}
		}
		if (!sawAmp || source == null || field == null || !source.equals(tested) ||
			!isBaseField(field)) {
			return null;
		}
		return source;
	}

	private void consumeDestructorTail(Set<ClangNode> dropped) {
		for (int i = items.size() - 1; i >= 0; i--) {
			Item item = items.get(i);
			if (item.depth != 0) {
				break;
			}
			if (!item.isStatement()) {
				break;
			}
			ClangStatement statement = (ClangStatement) item.node;
			if (dropped.contains(statement) || isClaimed(statement)) {
				continue;
			}
			PcodeOp op = statement.getPcodeOp();
			if (op != null && op.getOpcode() == PcodeOp.RETURN) {
				continue;
			}
			if (isSubobjectDestructorCall(statement)) {
				dropped.add(statement);
				continue;
			}
			break;
		}
	}

	private boolean isSubobjectDestructorCall(ClangStatement statement) {
		PcodeOp op = statement.getPcodeOp();
		Function target = callee(op);
		if (target == null || FunctionKind.classify(target) != FunctionKind.DESTRUCTOR ||
			op.getNumInputs() < 2) {
			return false;
		}
		OptionalLong offset = thisOffset(op.getInput(1));
		if (offset.isEmpty() || offset.getAsLong() < 0) {
			return false;
		}
		if (offset.getAsLong() == 0) {
			return true;
		}
		if (structure == null || offset.getAsLong() > Integer.MAX_VALUE) {
			return false;
		}
		DataTypeComponent component =
			structure.getComponentContaining((int) offset.getAsLong());
		return component != null && component.getOffset() == offset.getAsLong();
	}

	/**
	 * The completeness gate: after lifting, no constructor/destructor call
	 * on a `this` subobject may remain anywhere, or the printed body would
	 * silently miss lifecycle semantics. Failing the gate reverts the whole
	 * function to verbatim output.
	 */
	private boolean noSubobjectLifecycleCallsRemain(Set<ClangNode> dropped) {
		FunctionKind lifecycle =
			kind == FunctionKind.CONSTRUCTOR ? FunctionKind.CONSTRUCTOR
					: FunctionKind.DESTRUCTOR;
		for (Item item : items) {
			if (!item.isStatement() || dropped.contains(item.node) ||
				analysis.dropped.contains(item.node)) {
				continue;
			}
			PcodeOp op = ((ClangStatement) item.node).getPcodeOp();
			Function target = callee(op);
			if (target == null || FunctionKind.classify(target) != lifecycle ||
				op.getNumInputs() < 2) {
				continue;
			}
			if (thisOffset(op.getInput(1)).isPresent()) {
				return false;
			}
		}
		return true;
	}

	// ------------------------------------------------------------------
	// Virtual dispatch
	// ------------------------------------------------------------------

	/**
	 * Replace the decompiler's raw vtable dispatch expression with the named
	 * virtual call the source contained. The slot function is read from the
	 * receiver's static class vftable in program memory; the rendered argument
	 * list stays verbatim. Any unresolved step declines the call site.
	 */
	private void rewriteVirtualCalls() {
		TokenLine line = bodyTokenLine();
		for (int i = 1; i < line.sig.size(); i++) {
			ClangToken star = line.sig.get(i);
			if (!"*".equals(star.getText()) || isClaimed(star)) {
				continue;
			}
			PcodeOp op = star.getPcodeOp();
			if (op == null || op.getOpcode() != PcodeOp.CALLIND) {
				continue;
			}
			if (!"(".equals(line.sig.get(i - 1).getText())) {
				continue;
			}
			int start = i - 1;
			int close = matchingParen(line.sig, start);
			if (close < 0 || close + 1 >= line.sig.size() ||
				!"(".equals(line.sig.get(close + 1).getText())) {
				continue;
			}
			String site = "@ 0x" + (op.getSeqnum() == null ? "?"
					: op.getSeqnum().getTarget().toString());
			VirtualCall call = resolveVirtualCall(op);
			if (call == null) {
				trace("declined", currentPass, site +
					": receiver class, vftable, or slot function unresolved");
				continue;
			}
			FunctionKind slotKind = FunctionKind.classify(call.slot);
			if (slotKind == FunctionKind.ORDINARY) {
				String prefix = call.receiver.isEmpty() ? "" : call.receiver + "->";
				if (claimRange(line, start, close, prefix + call.slot.getName())) {
					trace("applied", currentPass,
						prefix + call.slot.getName() + " " + site);
				}
				continue;
			}
			if (slotKind == FunctionKind.SYNTHETIC_DELETING_DESTRUCTOR) {
				// Virtual dispatch of the deleting destructor is the compiled
				// form of a source-level polymorphic delete.
				int argsClose = matchingParen(line.sig, close + 1);
				if (argsClose != close + 3) {
					continue;
				}
				String flag = text(line.sig, close + 2);
				String object = call.receiver.isEmpty() ? "this" : call.receiver;
				String form = "1".equals(flag) ? "delete " : "3".equals(flag) ? "delete[] " : null;
				if (form != null && claimRange(line, start, argsClose, form + object)) {
					trace("applied", currentPass, form + object + " " + site);
				}
			}
			else {
				trace("declined", currentPass, site + ": slot holds " +
					call.slot.getName(true) + " (" + slotKind + "), not a callable rewrite");
			}
		}
	}

	/** A resolved virtual call site: the receiver's spelling and the slot. */
	private static final class VirtualCall {
		final String receiver; // empty = implicit this
		final Function slot;

		VirtualCall(String receiver, Function slot) {
			this.receiver = receiver;
			this.slot = slot;
		}
	}

	private VirtualCall resolveVirtualCall(PcodeOp callind) {
		PcodeOp slotLoad = definingLoad(callind.getInput(0));
		if (slotLoad == null) {
			return null;
		}
		long slotOffset = 0;
		Varnode current = slotLoad.getInput(1);
		for (int step = 0; current != null && step < 16; step++) {
			PcodeOp def = current.getDef();
			if (def == null) {
				return null;
			}
			int opcode = def.getOpcode();
			if (opcode == PcodeOp.LOAD) {
				break;
			}
			switch (opcode) {
				case PcodeOp.COPY:
				case PcodeOp.CAST:
				case PcodeOp.INDIRECT:
					current = def.getInput(0);
					break;
				case PcodeOp.PTRSUB:
				case PcodeOp.INT_ADD:
					if (!def.getInput(1).isConstant()) {
						return null;
					}
					slotOffset += def.getInput(1).getOffset();
					current = def.getInput(0);
					break;
				case PcodeOp.PTRADD:
					if (!def.getInput(1).isConstant() || !def.getInput(2).isConstant()) {
						return null;
					}
					slotOffset += def.getInput(1).getOffset() * def.getInput(2).getOffset();
					current = def.getInput(0);
					break;
				default:
					return null;
			}
		}
		PcodeOp vptrLoad = current == null ? null : current.getDef();
		if (vptrLoad == null || vptrLoad.getOpcode() != PcodeOp.LOAD) {
			return null;
		}
		Varnode object = vptrLoad.getInput(1);

		Structure receiverClass = null;
		long subobjectOffset = 0;
		String receiver = null;
		OptionalLong thisOff = thisOffset(object);
		if (thisOff.isPresent()) {
			if (structure == null) {
				return null;
			}
			receiverClass = structure;
			subobjectOffset = thisOff.getAsLong();
			receiver = "";
		}
		else {
			// Walk to the receiver's root: a named pointer variable or a
			// pointer member of this, with any constant subobject adjustment.
			long sub = 0;
			Varnode cursor = object;
			for (int step = 0; cursor != null && step < 16; step++) {
				HighVariable high = cursor.getHigh();
				if (high != null && high.getSymbol() != null) {
					Structure pointed = pointedStructure(high.getDataType());
					if (pointed == null) {
						return null;
					}
					receiverClass = pointed;
					subobjectOffset = sub;
					receiver = high.getName();
					break;
				}
				PcodeOp def = cursor.getDef();
				if (def == null) {
					return null;
				}
				switch (def.getOpcode()) {
					case PcodeOp.COPY:
					case PcodeOp.CAST:
					case PcodeOp.INDIRECT:
						cursor = def.getInput(0);
						break;
					case PcodeOp.PTRSUB:
					case PcodeOp.INT_ADD:
						if (!def.getInput(1).isConstant()) {
							return null;
						}
						sub += def.getInput(1).getOffset();
						cursor = def.getInput(0);
						break;
					case PcodeOp.LOAD: {
						// A pointer member of this: value = LOAD(this + off).
						if (structure == null) {
							return null;
						}
						OptionalLong fieldOff = thisOffset(def.getInput(1));
						if (fieldOff.isEmpty() || fieldOff.getAsLong() < 0 ||
							fieldOff.getAsLong() > Integer.MAX_VALUE) {
							return null;
						}
						DataTypeComponent component = structure
								.getComponentContaining((int) fieldOff.getAsLong());
						if (component == null ||
							component.getOffset() != fieldOff.getAsLong() ||
							component.getFieldName() == null ||
							isBaseField(component.getFieldName())) {
							return null;
						}
						Structure pointed = pointedStructure(component.getDataType());
						if (pointed == null) {
							return null;
						}
						receiverClass = pointed;
						subobjectOffset = sub;
						receiver = component.getFieldName();
						cursor = null;
						break;
					}
					default:
						return null;
				}
			}
			if (receiver == null) {
				return null;
			}
		}

		Function slot = vtableSlotFunction(receiverClass, subobjectOffset, slotOffset);
		if (slot == null || !(slot.getParentNamespace() instanceof GhidraClass)) {
			return null;
		}
		return new VirtualCall(receiver, slot);
	}

	/** The structure a pointer type points at, through typedefs; else null. */
	private static Structure pointedStructure(DataType type) {
		DataType current = type;
		while (current instanceof TypeDef typedef) {
			current = typedef.getBaseDataType();
		}
		if (!(current instanceof Pointer pointer)) {
			return null;
		}
		DataType pointed = pointer.getDataType();
		while (pointed instanceof TypeDef typedef) {
			pointed = typedef.getBaseDataType();
		}
		return pointed instanceof Structure structure ? structure : null;
	}

	/** The LOAD op producing the varnode, looking through copies and casts. */
	private static PcodeOp definingLoad(Varnode varnode) {
		Varnode current = varnode;
		for (int step = 0; current != null && step < 8; step++) {
			PcodeOp def = current.getDef();
			if (def == null) {
				return null;
			}
			switch (def.getOpcode()) {
				case PcodeOp.LOAD:
					return def;
				case PcodeOp.COPY:
				case PcodeOp.CAST:
				case PcodeOp.INDIRECT:
					current = def.getInput(0);
					break;
				default:
					return null;
			}
		}
		return null;
	}

	/**
	 * The function installed at byte offset {@code slotOffset} of the class's
	 * vftable for the given subobject, read from program memory.
	 */
	private Function vtableSlotFunction(Structure receiverClass, long subobjectOffset,
			long slotOffset) {
		if (slotOffset < 0 || slotOffset > 0x1000) {
			return null;
		}
		Namespace namespace = classNamespace(receiverClass.getName());
		if (namespace == null) {
			return null;
		}
		List<Symbol> candidates = new ArrayList<>();
		for (Symbol symbol : function.getProgram().getSymbolTable().getSymbols(namespace)) {
			if (FunctionKind.normalizeSpecialName(symbol.getName()).startsWith("vftable")) {
				candidates.add(symbol);
			}
		}
		Symbol chosen = chooseVftable(candidates, receiverClass, subobjectOffset);
		if (chosen == null || !slotInsideTable(chosen.getAddress(), slotOffset)) {
			return null;
		}
		try {
			int stored = function.getProgram().getMemory()
					.getInt(chosen.getAddress().add(slotOffset));
			Address target = function.getProgram().getAddressFactory()
					.getDefaultAddressSpace().getAddress(Integer.toUnsignedLong(stored));
			return function.getProgram().getFunctionManager().getFunctionAt(target);
		}
		catch (Exception e) {
			return null;
		}
	}

	/**
	 * Vftables have no length record; the next vftable symbol in the address
	 * space bounds the table. A slot read past that bound would silently
	 * name a slot of an unrelated class.
	 */
	private boolean slotInsideTable(Address tableStart, long slotOffset) {
		Address slotEnd = tableStart.add(slotOffset + 4);
		for (Symbol symbol : (Iterable<Symbol>) () -> function.getProgram().getSymbolTable()
				.getSymbolIterator(tableStart.add(1), true)) {
			Address address = symbol.getAddress();
			if (address.compareTo(slotEnd) >= 0) {
				return true;
			}
			if (FunctionKind.normalizeSpecialName(symbol.getName()).startsWith("vftable")) {
				return false;
			}
		}
		return true;
	}

	private Namespace classNamespace(String className) {
		for (Symbol symbol : function.getProgram().getSymbolTable()
				.getSymbols(className)) {
			if (symbol.getSymbolType() == SymbolType.CLASS &&
				symbol.getObject() instanceof GhidraClass ghidraClass) {
				return ghidraClass;
			}
		}
		return null;
	}

	private Symbol chooseVftable(List<Symbol> candidates, Structure receiverClass,
			long subobjectOffset) {
		if (candidates.isEmpty()) {
			return null;
		}
		if (subobjectOffset == 0) {
			// The complete-object table only: a {for_'Base'} candidate is a
			// different subobject's table and must never stand in for it.
			Symbol primary = null;
			for (Symbol symbol : candidates) {
				if (FunctionKind.normalizeSpecialName(symbol.getName()).equals("vftable")) {
					if (primary != null) {
						return null;
					}
					primary = symbol;
				}
			}
			return primary;
		}
		if (subobjectOffset > Integer.MAX_VALUE) {
			return null;
		}
		DataTypeComponent component =
			receiverClass.getComponentContaining((int) subobjectOffset);
		if (component == null || component.getOffset() != subobjectOffset ||
			component.getFieldName() == null || !isBaseField(component.getFieldName())) {
			return null;
		}
		DataType componentType = component.getDataType();
		while (componentType instanceof TypeDef typedef) {
			componentType = typedef.getBaseDataType();
		}
		String wanted = squeeze("vftable{for" + TypeNames.map(componentType.getName()) + "}");
		Symbol match = null;
		for (Symbol symbol : candidates) {
			if (squeeze(FunctionKind.normalizeSpecialName(symbol.getName())).equals(wanted)) {
				if (match != null) {
					return null;
				}
				match = symbol;
			}
		}
		return match;
	}

	private static String squeeze(String text) {
		return text.replace(" ", "");
	}

	// ------------------------------------------------------------------
	// Typed array indexing
	// ------------------------------------------------------------------

	/**
	 * Recover the exact pointer-arithmetic lowering
	 * {@code *(T *)(base + index * sizeof(T))} as
	 * {@code ((T *)base)[index]}. The cast type and stride are both required;
	 * a byte offset that does not equal the applied element width declines.
	 */
	private void rewriteArrayIndexing() {
		TokenLine full = bodyTokenLine();
		for (int pass = 0; pass < 8; pass++) {
			TokenLine live = liveView(full);
			List<ClangToken> sig = live.sig;
			boolean changed = false;
			for (int i = 0; i + 11 < sig.size(); i++) {
				if (!"*".equals(text(sig, i)) || !"(".equals(text(sig, i + 1)) ||
					!(sig.get(i + 2) instanceof ClangTypeToken typeToken) ||
					!"*".equals(text(sig, i + 3)) || !")".equals(text(sig, i + 4)) ||
					!"(".equals(text(sig, i + 5)) || !"+".equals(text(sig, i + 7)) ||
					!"*".equals(text(sig, i + 9)) || !")".equals(text(sig, i + 11))) {
					continue;
				}
				Long stride = integerToken(sig.get(i + 10));
				DataType element = typeToken.getDataType();
				if (stride == null || element == null || element.getLength() <= 0 ||
					stride != element.getLength() || !simpleValueToken(sig.get(i + 6)) ||
					!simpleValueToken(sig.get(i + 8))) {
					continue;
				}
				String type = TypeNames.map(typeToken.getText());
				String base = effectiveTokenText(sig.get(i + 6));
				String index = effectiveTokenText(sig.get(i + 8));
				claimRange(live, i, i + 11,
					"((" + type + " *)" + base + ")[" + index + "]");
				changed = true;
				i += 11;
			}
			if (!changed) {
				break;
			}
		}
	}

	private static boolean simpleValueToken(ClangToken token) {
		return token instanceof ClangVariableToken || integerToken(token) != null;
	}

	private static Long integerToken(ClangToken token) {
		String text = token.getText();
		if (text == null) {
			return null;
		}
		String normalized = text.replaceFirst("(?i)[uUlL]+$", "");
		try {
			return Long.decode(normalized);
		}
		catch (NumberFormatException e) {
			return null;
		}
	}

	// ------------------------------------------------------------------
	// Typed member access
	// ------------------------------------------------------------------

	/**
	 * Rewrite Ghidra's base-subobject navigation onto the C++ inheritance
	 * model: {@code (x->base).f} becomes {@code x->f}, {@code this->f}
	 * becomes the bare member, and {@code &this->base_X} becomes {@code this}
	 * (the source-level implicit upcast). {@code vftable} accesses are left
	 * alone; a surviving one marks dispatch the recognizers declined.
	 */
	private void normalizeMemberAccess() {
		TokenLine full = bodyTokenLine();
		for (int pass = 0; pass < 16; pass++) {
			TokenLine live = liveView(full);
			if (applyBaseAccessPatterns(live) == 0) {
				break;
			}
		}
	}

	/** One sweep applying every non-overlapping match; returns the count. */
	private int applyBaseAccessPatterns(TokenLine live) {
		int applied = 0;
		List<ClangToken> sig = live.sig;
		for (int i = 0; i < sig.size(); i++) {
			// ( X -> base ) . f   =>   X -> f       (and X . base variant)
			if (i + 5 < sig.size() && "(".equals(text(sig, i)) &&
				sig.get(i + 1) instanceof ClangVariableToken &&
				isAccessOperator(text(sig, i + 2)) && isBaseFieldToken(sig.get(i + 3)) &&
				")".equals(text(sig, i + 4)) && ".".equals(text(sig, i + 5)) &&
				!nextFieldIsVftable(sig, i + 6)) {
				dropToken(sig.get(i));
				dropToken(sig.get(i + 3));
				dropToken(sig.get(i + 4));
				dropToken(sig.get(i + 5));
				applied++;
				i += 5;
				continue;
			}
			// X . base . f  /  X -> base . f   =>   X . f  /  X -> f
			if (i + 1 < sig.size() && isBaseFieldToken(sig.get(i)) &&
				".".equals(text(sig, i + 1)) && i > 0 && isAccessOperator(text(sig, i - 1)) &&
				!nextFieldIsVftable(sig, i + 2)) {
				dropToken(sig.get(i));
				dropToken(sig.get(i + 1));
				applied++;
				i += 1;
				continue;
			}
			// & this -> base_X   =>   this        (implicit upcast)
			// & v -> base_X      =>   v
			if (i + 3 < sig.size() && "&".equals(text(sig, i)) &&
				sig.get(i + 1) instanceof ClangVariableToken &&
				"->".equals(text(sig, i + 2)) && isBaseFieldToken(sig.get(i + 3)) &&
				!isAccessOperator(text(sig, i + 4)) && !"[".equals(text(sig, i + 4))) {
				claimRange(live, i, i + 3,
					sanitizeReservedName(sig.get(i + 1), sig.get(i + 1).getText()));
				applied++;
				i += 3;
				continue;
			}
			// this -> f   =>   f                  (implicit receiver). Only the
			// real this parameter qualifies; a local Ghidra happened to name
			// "this" (a plain function receiving an object) must keep its
			// explicit access.
			if (i + 2 < sig.size() && sig.get(i) instanceof ClangVariableToken thisToken &&
				"this".equals(text(sig, i)) &&
				isThisSymbol(thisToken.getHighVariable()) &&
				"->".equals(text(sig, i + 1)) &&
				sig.get(i + 2) instanceof ClangFieldToken &&
				!isBaseFieldToken(sig.get(i + 2)) && !nextFieldIsVftable(sig, i + 2)) {
				dropToken(sig.get(i));
				dropToken(sig.get(i + 1));
				applied++;
				i += 1;
				continue;
			}
		}
		return applied;
	}

	private static String text(List<ClangToken> sig, int index) {
		if (index < 0 || index >= sig.size()) {
			return "";
		}
		String text = sig.get(index).getText();
		return text == null ? "" : text;
	}

	private static boolean isAccessOperator(String text) {
		return "->".equals(text) || ".".equals(text);
	}

	private static boolean isBaseFieldToken(ClangToken token) {
		return token instanceof ClangFieldToken && token.getText() != null &&
			isBaseField(token.getText());
	}

	private static boolean nextFieldIsVftable(List<ClangToken> sig, int index) {
		return index < sig.size() &&
			FunctionKind.normalizeSpecialName(text(sig, index)).startsWith("vftable");
	}

	// ------------------------------------------------------------------
	// String literals
	// ------------------------------------------------------------------

	/**
	 * A reference to a defined string datum prints as the quoted literal the
	 * source contained, not as Ghidra's synthetic identifier (whose embedded
	 * path characters cannot even lex). The bytes come from program memory,
	 * so the full text survives Ghidra's name truncation.
	 */
	private void rewriteStringLiterals() {
		TokenLine line = bodyTokenLine();
		int literals = 0;
		for (ClangToken token : line.sig) {
			if (!(token instanceof ClangVariableToken variable) || isClaimed(token)) {
				continue;
			}
			Address address = referencedDataAddress(variable);
			if (address == null) {
				continue;
			}
			String value = stringDatumAt(address);
			if (value != null && claimReplace(token, cStringLiteral(value))) {
				literals++;
			}
		}
		if (literals > 0) {
			trace("applied", currentPass, literals + " string literal(s) recovered");
		}
	}

	/** The ram address a global-reference token resolves to, else null. */
	private Address referencedDataAddress(ClangVariableToken variable) {
		Varnode varnode = variable.getVarnode();
		if (varnode != null && varnode.getAddress() != null &&
			varnode.getAddress().isMemoryAddress() &&
			!varnode.getAddress().isStackAddress()) {
			return varnode.getAddress();
		}
		// A global datum's reference renders with no varnode; the token's own
		// op is the canonical address-of, PTRSUB(0, address). Anything else
		// (an assignment target, a loaded value) must not be treated as one.
		if (varnode != null) {
			return null;
		}
		PcodeOp op = variable.getPcodeOp();
		if (op == null || op.getOpcode() != PcodeOp.PTRSUB ||
			!op.getInput(0).isConstant() || op.getInput(0).getOffset() != 0 ||
			!op.getInput(1).isConstant()) {
			return null;
		}
		long folded = op.getInput(1).getOffset();
		if (folded <= 0) {
			return null;
		}
		Address address = function.getProgram().getAddressFactory()
				.getDefaultAddressSpace().getAddress(folded);
		return function.getProgram().getMemory().contains(address) ? address : null;
	}

	/** The narrow string defined at the address, else null. */
	private String stringDatumAt(Address address) {
		var data = function.getProgram().getListing().getDataAt(address);
		if (data == null) {
			return null;
		}
		Object value = data.getValue();
		if (!(value instanceof String text)) {
			return null;
		}
		// A wide string would need the L prefix and byte-order care; only
		// plain char data is claimed.
		DataType type = data.getBaseDataType();
		String typeName = type == null ? "" : type.getName().toLowerCase();
		if (typeName.contains("unicode") || typeName.contains("wchar")) {
			return null;
		}
		return text;
	}

	/**
	 * The C source spelling of a narrow string. A hex escape eats every
	 * following hex digit, so a literal is split after one whenever the next
	 * character would extend it.
	 */
	static String cStringLiteral(String value) {
		StringBuilder text = new StringBuilder("\"");
		boolean pendingHexEscape = false;
		for (int i = 0; i < value.length(); i++) {
			char c = value.charAt(i);
			if (pendingHexEscape && isHexDigit(c)) {
				text.append("\" \"");
			}
			pendingHexEscape = false;
			switch (c) {
				case '\\' -> text.append("\\\\");
				case '"' -> text.append("\\\"");
				case '\n' -> text.append("\\n");
				case '\r' -> text.append("\\r");
				case '\t' -> text.append("\\t");
				default -> {
					if (c < 0x20 || c > 0x7e) {
						text.append(String.format("\\x%02x", (int) c));
						pendingHexEscape = true;
					}
					else {
						text.append(c);
					}
				}
			}
		}
		return text.append('"').toString();
	}

	private static boolean isHexDigit(char c) {
		return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
	}

	/** {@code (T *)0x0} is the source-level null constant {@code 0}. */
	private void rewriteNullPointerCasts() {
		TokenLine live = liveView(bodyTokenLine());
		List<ClangToken> sig = live.sig;
		for (int i = 0; i + 3 < sig.size(); i++) {
			if (!"(".equals(text(sig, i)) || !(sig.get(i + 1) instanceof ClangTypeToken)) {
				continue;
			}
			int j = i + 2;
			while (j < sig.size() && "*".equals(text(sig, j))) {
				j++;
			}
			if (j == i + 2 || j + 1 >= sig.size() || !")".equals(text(sig, j)) ||
				!"0x0".equals(text(sig, j + 1))) {
				continue;
			}
			claimRange(live, i, j + 1, "0");
			i = j + 1;
		}
	}

	// ------------------------------------------------------------------
	// Direct method calls
	// ------------------------------------------------------------------

	/**
	 * Rewrite {@code Class::Method(receiver, args)} into the member-call
	 * syntax the source used: bare {@code Method(args)} on {@code this} or a
	 * base subobject, {@code f.Method(args)} on a member, {@code v->Method(args)}
	 * or {@code x.f.Method(args)} on an external receiver.
	 */
	private void rewriteMethodCalls() {
		TokenLine live = liveView(bodyTokenLine());
		List<ClangToken> sig = live.sig;
		for (int i = 0; i < sig.size(); i++) {
			if (!(sig.get(i) instanceof ClangFuncNameToken) || isClaimed(sig.get(i))) {
				continue;
			}
			PcodeOp op = sig.get(i).getPcodeOp();
			Function target = callee(op);
			if (target == null || op.getNumInputs() < 2 ||
				!(target.getParentNamespace() instanceof GhidraClass) ||
				FunctionKind.classify(target) != FunctionKind.ORDINARY ||
				!"__thiscall".equals(target.getCallingConventionName()) ||
				hasReturnStorageParameter(target)) {
				continue;
			}
			int start = i;
			while (start >= 2 && "::".equals(text(sig, start - 1))) {
				start -= 2;
			}
			if (i + 1 >= sig.size() || !"(".equals(text(sig, i + 1))) {
				continue;
			}
			int close = matchingParen(sig, i + 1);
			if (close < 0) {
				continue;
			}
			List<List<ClangToken>> arguments = splitArguments(sig, i + 1, close);
			if (arguments.size() != op.getNumInputs() - 1) {
				continue;
			}
			String prefix = receiverPrefix(op.getInput(1), arguments.get(0));
			if (prefix == null) {
				continue;
			}
			List<String> rendered = new ArrayList<>();
			for (int argument = 1; argument < arguments.size(); argument++) {
				rendered.add(argumentText(arguments.get(argument)));
			}
			boolean statementCall = close + 1 < sig.size() && ";".equals(text(sig, close + 1));
			claimRange(live, start, statementCall ? close + 1 : close,
				prefix + target.getName() + "(" + String.join(", ", rendered) + ")" +
					(statementCall ? ";" : ""));
		}
	}

	/** Top-level argument token slices between an open and close paren. */
	private static List<List<ClangToken>> splitArguments(List<ClangToken> sig, int open,
			int close) {
		List<List<ClangToken>> arguments = new ArrayList<>();
		List<ClangToken> current = new ArrayList<>();
		int depth = 1;
		for (int i = open + 1; i < close; i++) {
			String text = text(sig, i);
			if ("(".equals(text)) {
				depth++;
			}
			else if (")".equals(text)) {
				depth--;
			}
			else if (",".equals(text) && depth == 1) {
				arguments.add(current);
				current = new ArrayList<>();
				continue;
			}
			current.add(sig.get(i));
		}
		if (!current.isEmpty() || !arguments.isEmpty()) {
			arguments.add(current);
		}
		return arguments;
	}

	private String argumentText(List<ClangToken> tokens) {
		StringBuilder text = new StringBuilder();
		for (ClangToken token : tokens) {
			text.append(effectiveTokenText(token));
		}
		return upcasts.getOrDefault(text.toString(), text.toString());
	}

	/**
	 * How the receiver spells in member-call syntax; null declines. The
	 * p-code decides for this-relative receivers, the rendered tokens for
	 * external ones.
	 */
	private String receiverPrefix(Varnode receiver, List<ClangToken> rendered) {
		OptionalLong offset = thisOffset(receiver);
		if (offset.isPresent()) {
			long value = offset.getAsLong();
			if (value == 0) {
				return "";
			}
			if (structure == null || value < 0 || value > Integer.MAX_VALUE) {
				return null;
			}
			DataTypeComponent component = structure.getComponentContaining((int) value);
			if (component == null || component.getOffset() != value ||
				component.getFieldName() == null) {
				return null;
			}
			if (isBaseField(component.getFieldName())) {
				return "";
			}
			return component.getFieldName() + ".";
		}
		String text = argumentText(rendered);
		if (text.isEmpty()) {
			return null;
		}
		if (text.startsWith("&") && text.length() > 1) {
			return text.substring(1) + ".";
		}
		List<ClangToken> live = new ArrayList<>();
		for (ClangToken token : rendered) {
			if (!effectiveTokenText(token).isEmpty()) {
				live.add(token);
			}
		}
		if (live.size() == 1 && live.get(0) instanceof ClangVariableToken &&
			!analysis.replaced.containsKey(live.get(0))) {
			return text + "->";
		}
		// A composite receiver expression, e.g. a cast: parenthesize it.
		return "(" + text + ")->";
	}

	/**
	 * Lift MSVC structure-return lowering back to a value return:
	 * {@code p = Class::Method(this, &storage, args)} becomes
	 * {@code storage = Method(args)}. The callee's own prototype names the
	 * hidden storage parameter, the storage argument must render as
	 * {@code &local}, and the returned alias pointer must be unused — a used
	 * alias would lose real dataflow, so it declines.
	 */
	private void rewriteStructReturns() {
		for (Item item : items) {
			if (!item.isStatement() || isClaimed(item.node)) {
				continue;
			}
			ClangStatement statement = (ClangStatement) item.node;
			PcodeOp op = statement.getPcodeOp();
			Function target = callee(op);
			if (target == null) {
				continue;
			}
			int storageIndex = returnStorageParameterIndex(target);
			if (storageIndex < 0 || FunctionKind.classify(target) != FunctionKind.ORDINARY) {
				continue;
			}
			String site = "@ " + statementAddress(statement);
			List<List<ClangToken>> arguments = callArgumentTokens(statement, op);
			if (arguments == null || storageIndex >= arguments.size()) {
				trace("declined", currentPass, site +
					": argument tokens disagree with the p-code operands");
				continue;
			}
			List<ClangToken> storage = arguments.get(storageIndex);
			if (storage.size() != 2 || !"&".equals(storage.get(0).getText()) ||
				!(storage.get(1) instanceof ClangVariableToken destination)) {
				trace("declined", currentPass, site +
					": storage argument does not render as &local");
				continue;
			}
			// The callee returns the storage pointer; Ghidra often keeps that
			// alias for later uses. Every alias use is by construction a use
			// of &storage, so the uses are rewritten; an alias whose uses
			// cannot all be claimed declines.
			Varnode output = op.getOutput();
			List<ClangToken[]> aliasUses = new ArrayList<>();
			if (output != null && output.getDescendants().hasNext()) {
				HighVariable alias = output.getHigh();
				if (alias == null || !collectAliasUses(statement, alias, aliasUses)) {
					trace("declined", currentPass, site +
						": the returned storage alias has unclaimable uses");
					continue;
				}
			}
			boolean member = target.getParentNamespace() instanceof GhidraClass &&
				"__thiscall".equals(target.getCallingConventionName());
			int receiverIndex = -1;
			String prefix = "";
			if (member) {
				receiverIndex = storageIndex == 0 ? 1 : 0;
				if (receiverIndex >= arguments.size()) {
					continue;
				}
				prefix = receiverPrefix(op.getInput(1 + receiverIndex),
					arguments.get(receiverIndex));
				if (prefix == null) {
					trace("declined", currentPass, site + ": receiver unresolved");
					continue;
				}
			}
			List<String> rendered = new ArrayList<>();
			for (int i = 0; i < arguments.size(); i++) {
				if (i == storageIndex || i == receiverIndex) {
					continue;
				}
				rendered.add(argumentText(arguments.get(i)));
			}
			String name = sanitizeReservedName(destination, destination.getText());
			if (claimReplace(statement, name + " = " + prefix + target.getName() +
				"(" + String.join(", ", rendered) + ")")) {
				for (ClangToken[] use : aliasUses) {
					if (use[1] != null) {
						// alias->field is storage.field
						claimReplace(use[0], name);
						claimReplace(use[1], ".");
					}
					else {
						claimReplace(use[0], "&" + name);
					}
				}
				trace("applied", currentPass, name + " = " + prefix + target.getName() +
					"(...) " + site + (aliasUses.isEmpty() ? ""
							: ", " + aliasUses.size() + " alias use(s) rewritten"));
			}
		}
	}

	/**
	 * Collect every rendered use of the alias variable outside the defining
	 * statement and its own declaration, paired with the {@code ->} operator
	 * that follows it when one does ({@code alias->f} rewrites to
	 * {@code storage.f}; a bare alias rewrites to {@code &storage}). True
	 * only when each use is a plain, unclaimed variable token.
	 */
	private boolean collectAliasUses(ClangStatement definition, HighVariable alias,
			List<ClangToken[]> uses) {
		TokenLine line = bodyTokenLine();
		for (int i = 0; i < line.sig.size(); i++) {
			ClangToken token = line.sig.get(i);
			if (!(token instanceof ClangVariableToken variable) ||
				variable.getHighVariable() != alias) {
				continue;
			}
			boolean skip = false;
			for (ClangNode ancestor = token; ancestor != null; ancestor = ancestor.Parent()) {
				if (ancestor == definition || ancestor instanceof ClangVariableDecl) {
					skip = true;
					break;
				}
			}
			if (skip) {
				continue;
			}
			if (isClaimed(token)) {
				return false;
			}
			ClangToken arrow = i + 1 < line.sig.size() &&
				"->".equals(text(line.sig, i + 1)) && !isClaimed(line.sig.get(i + 1))
						? line.sig.get(i + 1) : null;
			uses.add(new ClangToken[] {token, arrow});
		}
		return !uses.isEmpty();
	}

	private static int returnStorageParameterIndex(Function target) {
		for (int i = 0; i < target.getParameterCount(); i++) {
			if ("__return_storage_ptr__".equals(target.getParameter(i).getName())) {
				return i;
			}
		}
		return -1;
	}

	private static boolean hasReturnStorageParameter(Function target) {
		for (int i = 0; i < target.getParameterCount(); i++) {
			if ("__return_storage_ptr__".equals(target.getParameter(i).getName())) {
				return true;
			}
		}
		return false;
	}

	// ------------------------------------------------------------------
	// Ordinary method signatures
	// ------------------------------------------------------------------

	/**
	 * A class method's exported definition must not carry the ABI spelling:
	 * {@code void __thiscall Class::M(Class *this, int x)} becomes
	 * {@code void Class::M(int x)}. Applied only when the prototype has
	 * exactly the expected shape.
	 */
	private void liftOrdinarySignature() {
		if (!(function.getParentNamespace() instanceof GhidraClass) ||
			!"__thiscall".equals(function.getCallingConventionName())) {
			return;
		}
		TokenLine line = tokenLine(findProto(root));
		List<ClangToken> sig = line.sig;
		int convention = -1;
		int open = -1;
		for (int i = 0; i < sig.size(); i++) {
			if ("__thiscall".equals(text(sig, i))) {
				convention = i;
			}
			if ("(".equals(text(sig, i))) {
				open = i;
				break;
			}
		}
		if (convention < 0 || open < 0) {
			return;
		}
		int end = open + 1;
		while (end < sig.size() && !",".equals(text(sig, end)) && !")".equals(text(sig, end))) {
			end++;
		}
		if (end >= sig.size() || !"this".equals(text(sig, end - 1))) {
			return;
		}
		claimRange(line, convention, convention, "");
		int blankAfter = line.rawIndex.get(convention) + 1;
		if (blankAfter < line.raw.size()) {
			String blankText = line.raw.get(blankAfter).getText();
			if (blankText != null && blankText.isBlank()) {
				dropToken(line.raw.get(blankAfter));
			}
		}
		if (",".equals(text(sig, end))) {
			claimRange(line, open + 1, end, "");
		}
		else {
			claimRange(line, open + 1, end - 1, "");
		}
	}

	// ------------------------------------------------------------------
	// Signature construction
	// ------------------------------------------------------------------

	private List<ClangVariableDecl> protoParameters() {
		ClangFuncProto proto = findProto(root);
		List<ClangVariableDecl> parameters = new ArrayList<>();
		for (int i = 0; i < proto.numChildren(); i++) {
			if (proto.Child(i) instanceof ClangVariableDecl declaration) {
				parameters.add(declaration);
			}
		}
		return parameters;
	}

	private String liftedSignature(List<String> initializers) {
		String owner = TypeNames.map(function.getParentNamespace().getName(true));
		String name = function.getParentNamespace().getName();
		// The constructor of a template specialization is spelled with the
		// bare template name: W8GrowableVector<int>::W8GrowableVector(...).
		int bracket = name.indexOf('[');
		if (bracket > 0) {
			name = name.substring(0, bracket);
		}
		if (kind == FunctionKind.DESTRUCTOR) {
			name = "~" + name;
		}
		List<ClangVariableDecl> parameters = protoParameters();
		List<String> rendered = new ArrayList<>();
		for (int i = 1; i < parameters.size(); i++) {
			rendered.add(tokenText(parameters.get(i)));
		}
		StringBuilder signature = new StringBuilder();
		signature.append(owner).append("::").append(name).append('(')
				.append(String.join(", ", rendered)).append(')');
		if (!initializers.isEmpty()) {
			String single = "\n    : " + String.join(", ", initializers);
			if (single.length() <= 100) {
				signature.append(single);
			}
			else {
				signature.append("\n    : ").append(initializers.get(0));
				for (int i = 1; i < initializers.size(); i++) {
					signature.append(",\n      ").append(initializers.get(i));
				}
			}
		}
		return signature.toString();
	}

	// ------------------------------------------------------------------
	// Typed deletion and allocation
	// ------------------------------------------------------------------

	private void rewriteAllocationPairs() {
		List<Item> statements = new ArrayList<>();
		for (Item item : items) {
			if (item.node instanceof ClangVariableDecl) {
				continue;
			}
			if (item.isStatement()) {
				statements.add(item);
			}
			else if (!"{".equals(item.text()) && !"}".equals(item.text())) {
				statements.add(item); // control keyword breaks adjacency
			}
		}
		for (int i = 0; i < statements.size(); i++) {
			Item item = statements.get(i);
			if (!item.isStatement() || analysis.dropped.contains(item.node) ||
				analysis.replaced.containsKey(item.node)) {
				continue;
			}
			ClangStatement statement = (ClangStatement) item.node;
			if (rewriteScalarDeletingCall(statement)) {
				continue;
			}
			Item next = nextLiveStatement(statements, i, item.depth);
			if (next == null) {
				continue;
			}
			if (rewriteDestructorDeletePair(statement, (ClangStatement) next.node)) {
				continue;
			}
			rewriteNewConstructorPair(statement, (ClangStatement) next.node);
		}
	}

	private Item nextLiveStatement(List<Item> statements, int index, int depth) {
		for (int j = index + 1; j < statements.size(); j++) {
			Item candidate = statements.get(j);
			if (!candidate.isStatement()) {
				return null; // an intervening control keyword
			}
			if (candidate.depth != depth) {
				return null;
			}
			if (analysis.dropped.contains(candidate.node)) {
				continue; // EH scaffolding may sit between the pair
			}
			return candidate;
		}
		return null;
	}

	/** `T::'scalar_deleting_destructor'(x, flags)` with a constant flag. */
	private boolean rewriteScalarDeletingCall(ClangStatement statement) {
		PcodeOp op = statement.getPcodeOp();
		Function target = callee(op);
		if (target == null ||
			FunctionKind.classify(target) != FunctionKind.SYNTHETIC_DELETING_DESTRUCTOR ||
			op.getNumInputs() != 3 || !op.getInput(2).isConstant()) {
			return false;
		}
		String object = singleVariableArgument(statement, op, 1);
		if (object == null) {
			return false;
		}
		long flags = op.getInput(2).getOffset();
		if ((flags & 1) == 0) {
			return false; // no deallocation: not a source-level delete
		}
		String form = (flags & 2) != 0 ? "delete[] " : "delete ";
		if (!claimReplace(statement, form + object)) {
			return false;
		}
		trace("applied", "allocation.scalar-delete",
			form.trim() + " " + object + " @ " + statementAddress(statement));
		return true;
	}

	/** `T::~T(x); operator_delete(x);` collapses to `delete x`. */
	private boolean rewriteDestructorDeletePair(ClangStatement first, ClangStatement second) {
		PcodeOp destructorOp = first.getPcodeOp();
		Function destructor = callee(destructorOp);
		if (destructor == null ||
			FunctionKind.classify(destructor) != FunctionKind.DESTRUCTOR ||
			destructorOp.getNumInputs() != 2) {
			return false;
		}
		PcodeOp deleteOp = second.getPcodeOp();
		Function deallocator = callee(deleteOp);
		if (deallocator == null || deleteOp.getNumInputs() != 2 ||
			!deallocator.getName().startsWith("operator_delete")) {
			return false;
		}
		String object = singleVariableArgument(first, destructorOp, 1);
		String deleted = singleVariableArgument(second, deleteOp, 1);
		if (object == null || !object.equals(deleted)) {
			if (object != null || deleted != null) {
				trace("declined", "allocation.scalar-delete", "@ " +
					statementAddress(first) +
					": destructor receiver and deallocated pointer differ");
			}
			return false;
		}
		if (!claimReplace(first, "delete " + object)) {
			return false;
		}
		claimDrop(second);
		trace("applied", "allocation.scalar-delete",
			"delete " + object + " @ " + statementAddress(first));
		return true;
	}

	/** `x = operator_new(N); T::T(x, args...);` collapses to `x = new T(args...)`. */
	private boolean rewriteNewConstructorPair(ClangStatement first, ClangStatement second) {
		PcodeOp newOp = first.getPcodeOp();
		Function allocator = callee(newOp);
		if (allocator == null || !allocator.getName().startsWith("operator_new") ||
			newOp.getOutput() == null) {
			return false;
		}
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(first, tokens);
		if (tokens.isEmpty() || !(tokens.get(0) instanceof ClangVariableToken)) {
			return false;
		}
		String result = tokens.get(0).getText();
		PcodeOp constructorOp = second.getPcodeOp();
		Function constructor = callee(constructorOp);
		if (constructor == null ||
			FunctionKind.classify(constructor) != FunctionKind.CONSTRUCTOR ||
			constructorOp.getNumInputs() < 2) {
			return false;
		}
		String receiver = singleVariableArgument(second, constructorOp, 1);
		if (receiver == null || !receiver.equals(result)) {
			trace("declined", "allocation.scalar-new", "@ " + statementAddress(first) +
				": construction receiver is not the allocation result");
			return false;
		}
		List<String> arguments = callArgumentsAfterReceiver(second, constructorOp);
		if (arguments == null) {
			trace("declined", "allocation.scalar-new", "@ " + statementAddress(first) +
				": constructor argument tokens disagree with the p-code operands");
			return false;
		}
		String type = TypeNames.map(constructor.getParentNamespace().getName(true));
		if (!claimReplace(first,
			result + " = new " + type + "(" + String.join(", ", arguments) + ")")) {
			return false;
		}
		claimDrop(second);
		trace("applied", "allocation.scalar-new",
			result + " = new " + type + "(...) @ " + statementAddress(first));
		return true;
	}

	/**
	 * The call's significant argument tokens, split at top-level commas.
	 * Returns null when the token shape and the p-code operand count
	 * disagree.
	 */
	private static List<List<ClangToken>> callArgumentTokens(ClangStatement statement,
			PcodeOp op) {
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(statement, tokens);
		int open = -1;
		for (int i = 0; i < tokens.size(); i++) {
			if (tokens.get(i) instanceof ClangFuncNameToken) {
				for (int j = i + 1; j < tokens.size(); j++) {
					if ("(".equals(tokens.get(j).getText())) {
						open = j;
						break;
					}
				}
				break;
			}
		}
		if (open < 0) {
			return null;
		}
		List<List<ClangToken>> arguments = new ArrayList<>();
		List<ClangToken> current = new ArrayList<>();
		int depth = 1;
		for (int i = open + 1; i < tokens.size() && depth > 0; i++) {
			ClangToken token = tokens.get(i);
			String text = token.getText();
			if ("(".equals(text)) {
				depth++;
			}
			else if (")".equals(text)) {
				depth--;
				if (depth == 0) {
					break;
				}
			}
			else if (",".equals(text) && depth == 1) {
				arguments.add(current);
				current = new ArrayList<>();
				continue;
			}
			if (text != null && !text.isBlank()) {
				current.add(token);
			}
		}
		if (!current.isEmpty() || !arguments.isEmpty()) {
			arguments.add(current);
		}
		if (arguments.size() != op.getNumInputs() - 1) {
			return null;
		}
		return arguments;
	}

	/**
	 * The rendered argument at the given call operand position, accepted
	 * only when it is one plain variable token, so text identity is safe.
	 */
	private String singleVariableArgument(ClangStatement statement, PcodeOp op, int input) {
		List<List<ClangToken>> arguments = callArgumentTokens(statement, op);
		if (arguments == null || input - 1 >= arguments.size()) {
			return null;
		}
		List<ClangToken> argument = arguments.get(input - 1);
		if (argument.size() != 1 || !(argument.get(0) instanceof ClangVariableToken)) {
			return null;
		}
		return sanitizeReservedName(argument.get(0), argument.get(0).getText());
	}
}
