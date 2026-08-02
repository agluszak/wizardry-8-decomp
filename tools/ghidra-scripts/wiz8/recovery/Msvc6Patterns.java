package wiz8.recovery;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.IdentityHashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
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
import ghidra.program.model.data.Undefined;
import ghidra.program.model.listing.AutoParameterType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Parameter;
import ghidra.program.model.listing.VariableStorage;
import ghidra.program.model.pcode.HighFunction;
import ghidra.program.model.pcode.HighSymbol;
import ghidra.program.model.pcode.HighVariable;
import ghidra.program.model.pcode.PcodeOp;
import ghidra.program.model.pcode.Varnode;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.Symbol;

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
		/** Structurally generated constructor initializer suffix, if any. */
		String initializerSuffix = "";
		/** Ephemeral per-function transformation trace, in pass order. */
		final List<TraceEvent> trace = new ArrayList<>();
		/** Unexpected exporter defects: {@code pass: exception}. */
		final List<String> defects = new ArrayList<>();
	}

	private final Function function;
	private final RecoverySession session;
	private final SourceEntity entity;
	private final Emission emission;
	private final DecompileResults results;
	private final SourceHints sourceHints;
	private final SourceReferenceForm[] sourceReferenceForms;
	private final Analysis analysis = new Analysis();

	private ClangFunction root;
	private HighFunction highFunction;
	private MarkupIndex markup;
	private RecoveryContext context;
	private BlockView.Tree blocks;
	private List<ClangNode> bodyEntries;
	private Structure structure;
	/** Stack offset of the VC6 EH registration link slot, when the frame is proven. */
	private Long ehLinkOffset;
	/** The function's FuncInfo-derived exception model, when it resolved. */
	private EhModel ehModel;
	/** Program symbol address of the VC6 thread exception-list head. */
	private Address exceptionListAddress;
	/** Which pass owns each claimed node; drives conflict rejection. */
	private final Map<ClangNode, String> claimOwner = new HashMap<>();
	private final Map<String, Integer> claimPriority = new HashMap<>();
	private final RewritePlanner planner = new RewritePlanner(analysis.dropped,
		analysis.replaced, claimOwner, claimPriority);
	/** The pass currently running; claims and trace records attach to it. */
	private String currentPass;
	/** Staged all-or-nothing claims for the semantic pass currently running. */
	private ClaimTransaction currentClaims;

	/** Shared vftable and base-subobject resolution over the program. */
	private final VtableResolver vtables;
	private final CallTargetResolver callTargets;

	private Msvc6Patterns(RecoverySession session, Function function, SourceEntity entity,
			Emission emission,
			DecompileResults results,
			String[] sourceReferenceForms) {
		this.session = session;
		this.function = function;
		this.entity = entity;
		this.emission = emission;
		this.results = results;
		this.sourceHints = SourceHints.parse(sourceReferenceForms);
		this.sourceReferenceForms = sourceHints.referenceForms();
		this.vtables = session.vtables;
		this.callTargets = session.calls;
	}

	/**
	 * Run every recognizer pass. Preparation failures (no markup, no
	 * linearizable body) are the only whole-function bailouts and yield the
	 * empty analysis with a defect record; pass failures are contained per
	 * pass inside {@link #recover}.
	 */
	static Analysis analyze(RecoverySession session, Function function, SourceEntity entity,
			Emission emission,
			DecompileResults results) {
		return analyze(session, function, entity, emission, results, new String[0]);
	}

	static Analysis analyze(RecoverySession session, Function function, SourceEntity entity,
			Emission emission,
			DecompileResults results, String[] sourceReferenceForms) {
		Msvc6Patterns patterns =
			new Msvc6Patterns(session, function, entity, emission, results,
				sourceReferenceForms);
		try {
			patterns.run();
		}
		catch (Exception e) {
			patterns.analysis.defects.add("prepare: " + e);
			patterns.analysis.dropped.clear();
			patterns.analysis.replaced.clear();
			patterns.analysis.liftSignature = false;
			patterns.analysis.initializerSuffix = "";
		}
		return patterns.analysis;
	}

	private void run() {
		if (!(results.getCCodeMarkup() instanceof ClangFunction clangFunction)) {
			return;
		}
		root = clangFunction;
		highFunction = results.getHighFunction();
		if (highFunction == null) {
			throw new IllegalStateException("decompilation has no HighFunction");
		}
		markup = new MarkupIndex(root, highFunction);
		context = new RecoveryContext(session, function, entity, emission,
			sourceHints, results, highFunction, markup);
		blocks = markup.blocks;
		bodyEntries = markup.bodyEntries;
		structure = classStructure();

		// Domain order is retained for stable diagnostics. Each recognizer sees
		// the immutable decompiler/source context; only the planner observes and
		// resolves claims accepted from another domain.
		MatchShaping.recover(this);
		CallRecovery.recoverEarly(this);
		LifecycleRecovery.recoverScaffolding(this);
		ExpressionRecovery.recover(this);
		CallRecovery.recoverStructureReturns(this);
		EhRecovery.recover(this);
		LifecycleRecovery.recoverBodies(this, entity);
	}

	void lifecyclePass() {
		String prototype = CallableIdentity.prototypeOrNull(function, entity.key());
		if (prototype == null) {
			trace("declined", currentPass, "formal signature contains an unresolved ABI type");
			return;
		}
		Set<ClangNode> lifecycleDropped = new HashSet<>();
		List<String> initializers = new ArrayList<>();
		if (!analyzeLifecycle(lifecycleDropped, initializers)) {
			return;
		}
		for (ClangNode node : lifecycleDropped) {
			claimDrop(node);
		}
		analysis.liftSignature = true;
		analysis.initializerSuffix = initializerSuffix(initializers);
		claimReplace(findProto(root), prototype + analysis.initializerSuffix);
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
	void recover(String name, Runnable body) {
		currentPass = name;
		currentClaims = new ClaimTransaction(name);
		Set<ClangNode> droppedBefore = new HashSet<>(analysis.dropped);
		Map<ClangNode, String> replacedBefore = new HashMap<>(analysis.replaced);
		Map<ClangNode, String> ownersBefore = new HashMap<>(claimOwner);
		boolean liftBefore = analysis.liftSignature;
		String initializerBefore = analysis.initializerSuffix;
		int traceBefore = analysis.trace.size();
		try {
			body.run();
			RewritePlanner.PlanResult planned = planner.accept(currentClaims.proposal());
			if (!planned.accepted() && !currentClaims.proposal().edits().isEmpty()) {
				restoreClaims(droppedBefore, replacedBefore, ownersBefore, liftBefore,
					initializerBefore);
				analysis.trace.subList(traceBefore, analysis.trace.size())
					.removeIf(event -> "applied".equals(event.status));
				trace("declined", name, "atomic claim set rejected: " +
					String.join("; ", planned.rejected()));
				return;
			}
			if (!planned.rejected().isEmpty()) {
				trace("declined", name, "overlapping region rejected: " +
					String.join("; ", planned.rejected()));
			}
		}
		catch (Exception e) {
			restoreClaims(droppedBefore, replacedBefore, ownersBefore, liftBefore,
				initializerBefore);
			analysis.defects.add(name + ": " + e);
			trace("failed", name, String.valueOf(e));
		}
		finally {
			currentClaims = null;
			currentPass = null;
		}
	}

	/**
	 * Pass-local claim transaction.  Recognizers may stage any number of node
	 * drops/replacements, but the shared analysis is untouched until every claim
	 * has proved conflict-free.  A single ignored claim failure therefore
	 * declines the complete semantic rewrite instead of leaking a valid-looking
	 * subset into the renderer.
	 */
	private final class ClaimTransaction {
		final String owner;
		final Set<ClangNode> dropped = new HashSet<>();
		final Map<ClangNode, String> replaced = new HashMap<>();
		String failure;

		ClaimTransaction(String owner) {
			this.owner = owner;
		}

		boolean drop(ClangNode node) {
			replaced.remove(node);
			dropped.add(node);
			return true;
		}

		boolean replace(ClangNode node, String text) {
			dropped.remove(node);
			replaced.put(node, text);
			return true;
		}

		ProposedRewrite proposal() {
			List<NodeEdit> edits = new ArrayList<>();
			for (ClangNode node : dropped) edits.add(new NodeEdit.Drop(node));
			for (var entry : replaced.entrySet()) {
				edits.add(new NodeEdit.Replace(entry.getKey(), entry.getValue()));
			}
			return new ProposedRewrite(owner, List.copyOf(edits), passPriority(owner), "");
		}
	}

	private void restoreClaims(Set<ClangNode> dropped, Map<ClangNode, String> replaced,
			Map<ClangNode, String> owners, boolean liftSignature, String initializerSuffix) {
		analysis.dropped.clear();
		analysis.dropped.addAll(dropped);
		analysis.replaced.clear();
		analysis.replaced.putAll(replaced);
		claimOwner.clear();
		claimOwner.putAll(owners);
		analysis.liftSignature = liftSignature;
		analysis.initializerSuffix = initializerSuffix;
	}

	/**
	 * Claim a node as dropped. A node already claimed by another pass, or
	 * sitting inside another pass's replacement text, is a conflict: the new
	 * claim is rejected and traced. A pass may re-claim and refine its own
	 * claims freely, and may supersede other passes only by claiming an
	 * enclosing node (the renderer's outermost-wins rule).
	 */
	private boolean claimDrop(ClangNode node) {
		if (currentClaims == null) {
			throw new IllegalStateException("claim outside pass transaction");
		}
		return currentClaims.drop(node);
	}

	/** Claim a node as replaced by text; same conflict rules as drops. */
	private boolean claimReplace(ClangNode node, String replacement) {
		if (currentClaims == null) {
			throw new IllegalStateException("claim outside pass transaction");
		}
		return currentClaims.replace(node, replacement);
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
			pass.startsWith("allocation") || pass.equals("shape.call-result-local")) ? 2 : 1;
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

	private static ClangFuncProto findProto(ClangFunction root) {
		for (int i = 0; i < root.numChildren(); i++) {
			if (root.Child(i) instanceof ClangFuncProto proto) {
				return proto;
			}
		}
		throw new IllegalStateException("no function prototype in markup");
	}

	/** The token's source spelling, independent of every other recovery pass. */
	private String sourceTokenText(ClangToken token) {
		String text = token.getText();
		if (token instanceof ClangTypeToken type) {
			text = TypeNames.mapToken(type);
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
	private boolean replaceSpan(MarkupIndex.TokenSpan line, int sigFrom, int sigTo,
			String replacement) {
		List<ClangToken> region = line.region(sigFrom, sigTo);
		if (region.isEmpty()) return false;
		ClangToken first = region.get(0);
		String firstOwner = claimOwner.get(first);
		if (firstOwner != null && !firstOwner.equals(currentPass) &&
			passPriority(currentPass) <= passPriority(firstOwner)) {
			trace("declined", currentPass,
				"claim conflict: range anchor already claimed by " + firstOwner);
			return false;
		}
		for (ClangToken token : region) {
			String owner = claimOwner.get(token);
			if (owner != null && !owner.equals(currentPass) &&
				analysis.replaced.containsKey(token) &&
				passPriority(currentPass) <= passPriority(owner)) {
				trace("declined", currentPass,
					"claim conflict: range contains a replacement owned by " + owner);
				return false;
			}
		}
		for (ClangToken token : region) {
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
		return SyntaxPairs.matchingClose(sig, open);
	}

	// ------------------------------------------------------------------
	// P-code plumbing
	// ------------------------------------------------------------------

	private Function callee(PcodeOp op) {
		CallTargetResolver.Target resolved = callTarget(op);
		if (resolved == null) {
			return null;
		}
		return resolved.safelyErasable() ? resolved.canonical() : resolved.referenced();
	}

	/** Render the source spelling for a proved raw-storage allocation call. */
	void rewriteCanonicalLibraryCalls() {
		for (ClangToken token : markup.tokens) {
			if (!(token instanceof ClangFuncNameToken name)) {
				continue;
			}
			CallTargetResolver.Target target = callTarget(name.getPcodeOp());
			if (target == null) {
				continue;
			}
			String normalized = SpecialNames.normalize(target.canonical().getName());
			String source = normalized.startsWith("operator new") ? "::operator new"
				: normalized.startsWith("operator delete") && rawStorageDelete(name.getPcodeOp())
					? "::operator delete" : null;
			DataType destination = source == null ? null : typedPointerDestination(name.getPcodeOp());
			if (destination != null) {
				source = "(" + CxxTypePrinter.printType(destination) + ")" + source;
			}
			if (source != null && claimReplace(name, source)) {
				trace("applied", currentPass, source + " @ " +
					name.getPcodeOp().getSeqnum().getTarget());
			}
		}
	}

	/** True when raw deallocation receives storage rather than a T object. */
	private static boolean rawStorageDelete(PcodeOp call) {
		if (call == null || call.getNumInputs() != 2) {
			return false;
		}
		HighVariable high = call.getInput(1).getHigh();
		DataType type = resolvedType(high == null ? null : high.getDataType());
		if (!(type instanceof Pointer pointer)) {
			return false;
		}
		return !(resolvedType(pointer.getDataType()) instanceof Structure);
	}

	/**
	 * Follow only representation-preserving copies/casts from a call result to
	 * the typed pointer receiving raw {@code void*} storage. This is the C++
	 * cast the source must carry; the type comes from HighVariable, never text.
	 */
	private static DataType typedPointerDestination(PcodeOp call) {
		if (call == null || call.getOutput() == null) {
			return null;
		}
		ArrayDeque<Varnode> pending = new ArrayDeque<>();
		Set<Varnode> seen = Collections.newSetFromMap(new IdentityHashMap<>());
		pending.add(call.getOutput());
		while (!pending.isEmpty()) {
			Varnode value = pending.removeFirst();
			if (!seen.add(value)) {
				continue;
			}
			HighVariable high = value.getHigh();
			DataType type = high == null ? null : high.getDataType();
			if (isTypedPointer(type)) {
				return type;
			}
			Iterator<PcodeOp> uses = value.getDescendants();
			while (uses.hasNext()) {
				PcodeOp use = uses.next();
				if ((use.getOpcode() == PcodeOp.COPY || use.getOpcode() == PcodeOp.CAST) &&
					use.getOutput() != null) {
					pending.add(use.getOutput());
				}
			}
		}
		return null;
	}

	/** Recover a nested demangler namespace for a declaration's base type. */
	void qualifyDemangledTypeTokens() {
		for (ClangToken token : markup.tokens) {
			if (!(token instanceof ClangTypeToken typeToken)) {
				continue;
			}
			DataType type = typeToken.getDataType();
			if (type == null) {
				for (ClangNode owner = token.Parent(); owner != null; owner = owner.Parent()) {
					if (owner instanceof ClangVariableDecl declaration) {
						type = declaration.getDataType();
						break;
					}
				}
			}
			while (type instanceof TypeDef typedef) {
				type = typedef.getDataType();
			}
			while (type instanceof Pointer pointer) {
				type = pointer.getDataType();
			}
			String qualified = TypeNames.qualifiedBase(type, TypeNames.map(typeToken.getText()));
			if (qualified == null) {
				qualified = programQualifiedBase(type, TypeNames.map(typeToken.getText()));
			}
			if (qualified != null && claimReplace(token, qualified)) {
				trace("applied", currentPass, qualified);
			}
		}
	}

	/** Qualify a category path only when every segment is a live Ghidra namespace. */
	private String programQualifiedBase(DataType type, String rendered) {
		if (type == null || rendered == null) {
			return null;
		}
		String path = type.getCategoryPath().getPath();
		if (path == null || path.equals("/") || path.startsWith("/Demangler")) {
			return null;
		}
		Namespace parent = function.getProgram().getGlobalNamespace();
		List<String> names = new ArrayList<>();
		for (String segment : path.substring(1).split("/")) {
			Namespace found = null;
			for (Symbol symbol : function.getProgram().getSymbolTable().getSymbols(segment, parent)) {
				if (symbol.getObject() instanceof Namespace namespace) {
					found = namespace;
					break;
				}
			}
			if (found == null) {
				return null;
			}
			names.add(segment);
			parent = found;
		}
		return String.join("::", names) + "::" + rendered;
	}

	/**
	 * Ghidra occasionally prints an internal P-code primitive as though it were
	 * a C function. Render the proved operation itself so exported C++ never
	 * depends on undeclared decompiler helpers. Delimiter text only binds the
	 * exact operation's presentation span.
	 */
	void rewritePcodeIntrinsics() {
		MarkupIndex.TokenSpan line = markup.bodySpan();
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp operation = operations.next();
			if (operation.getOpcode() != PcodeOp.INT_SBORROW ||
				operation.getNumInputs() != 2 || operation.getInput(0).getSize() != 4 ||
				operation.getInput(1).getSize() != 4) {
				continue;
			}
			String left = sourceValue(operation.getInput(0));
			String right = sourceValue(operation.getInput(1));
			int[] span = operationCallSpan(line, operation);
			if (left == null || right == null || span == null) {
				trace("declined", currentPass, operation.getSeqnum() +
					": exact operands or markup span unavailable");
				continue;
			}
			String a = "(unsigned int)(" + left + ")";
			String b = "(unsigned int)(" + right + ")";
			String rendered = "(((" + a + " ^ " + b + ") & (" + a + " ^ (" + a +
				" - " + b + "))) >> 31)";
			if (replaceSpan(line, span[0], span[1], rendered)) {
				trace("applied", currentPass, "INT_SBORROW @ " + operation.getSeqnum());
			}
		}
	}

	private int[] operationCallSpan(MarkupIndex.TokenSpan line, PcodeOp operation) {
		Set<ClangToken> anchors = Collections.newSetFromMap(new IdentityHashMap<>());
		anchors.addAll(markup.tokensFor(operation));
		for (int i = 0; i < line.sig.size(); i++) {
			if (!anchors.contains(line.sig.get(i))) {
				continue;
			}
			int open = i + 1;
			if (open < line.sig.size() && "(".equals(text(line.sig, open))) {
				int close = SyntaxPairs.matchingClose(line.sig, open);
				if (close >= 0) {
					return new int[] { i, close };
				}
			}
		}
		return null;
	}

	private static String sourceValue(Varnode value) {
		if (value.isConstant()) {
			long raw = value.getOffset();
			if (value.getSize() == 4) {
				return Integer.toString((int) raw);
			}
			return Long.toString(raw);
		}
		HighVariable high = value.getHigh();
		return sanitizeHighName(high);
	}

	private CallTargetResolver.Target callTarget(PcodeOp op) {
		if (op == null || op.getOpcode() != PcodeOp.CALL || op.getNumInputs() < 1) {
			return null;
		}
		Varnode target = op.getInput(0);
		if (!target.isAddress()) {
			return null;
		}
		Function referenced = callTargets.referencedAt(function.getProgram(), target.getAddress());
		return callTargets.resolve(referenced);
	}

	private static boolean isThisSymbol(HighVariable high) {
		if (high == null) {
			return false;
		}
		HighSymbol symbol = high.getSymbol();
		if (symbol == null) {
			return false;
		}
		return symbol.isThisPointer();
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
	void analyzeExceptionHandling() {
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
		if (ehModel == null && !entity.isConstructor() && !entity.isDestructor()) {
			trace("declined", currentPass,
				"EH frame detected but FuncInfo unresolved; only lifecycle bodies " +
					"use the frame-shape fallback");
			ehLinkOffset = null;
			return;
		}
		Set<ClangNode> ehDropped = new HashSet<>();
		for (ClangNode node : bodyEntries) {
			if (node instanceof ClangStatement statement) {
				if (isEhScaffolding(statement)) {
					ehDropped.add(statement);
				}
			}
			else if (node instanceof ClangVariableDecl declaration &&
				isEhSlotDeclaration(declaration)) {
				ehDropped.add(declaration);
			}
		}
		// Subpiece stores can have a unique-space output even though their high
		// symbol storage overlaps the proven state slot.
		for (ClangNode node : bodyEntries) {
			if (node instanceof ClangStatement && mentionsEhSlot(node)) {
				ehDropped.add(node);
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
	void liftStackLocalsPass() {
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
		for (ClangNode node : bodyEntries) {
			if (!(node instanceof ClangStatement statement)) {
				continue;
			}
			PcodeOp op = statement.getPcodeOp();
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
		return EhModel.resolve(session, thunk);
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
		for (ClangNode node : markup.bodyNodes()) {
			tokens.addAll(markup.tokenSpan(node).sig);
		}
		for (ClangToken token : tokens) {
			if (!isEhSlotReference(token)) {
				continue;
			}
			boolean claimed = false;
			for (ClangNode current = token; current != null; current = current.Parent()) {
				if (ehDropped.contains(current)) {
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

	private boolean mentionsEhSlot(ClangNode node) {
		List<ClangToken> tokens = new ArrayList<>();
		tokens.addAll(markup.tokenSpan(node).sig);
		for (ClangToken token : tokens) {
			if (isEhSlotReference(token)) {
				return true;
			}
		}
		return false;
	}

	private boolean isEhSlotReference(ClangToken token) {
		if (!(token instanceof ClangVariableToken variable)) {
			return false;
		}
		HighVariable high = variable.getHighVariable();
		HighSymbol symbol = variable.getHighSymbol(highFunction);
		if (symbol != null && symbol.getStorage() != null && symbol.getStorage().isStackStorage()) {
			for (Varnode storage : symbol.getStorage().getVarnodes()) {
				long start = storage.getAddress().getOffset();
				long end = start + storage.getSize();
				if (start < ehLinkOffset + 12 && end > ehLinkOffset) {
					return true;
				}
			}
		}
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
		for (ClangNode node : bodyEntries) {
			if (!(node instanceof ClangStatement statement)) {
				continue;
			}
			PcodeOp op = statement.getPcodeOp();
			Function target = callee(op);
			if (target == null || op.getNumInputs() < 2 ||
				!owner.equals(target.getParentNamespace())) {
				continue;
			}
			SourceKind targetKind = session.sourceKey(target).kind();
			ClangVariableToken receiver = addressOfLocalReceiver(statement, op, stackOffset);
			if (receiver == null) {
				continue;
			}
			if (targetKind == SourceKind.CONSTRUCTOR) {
				if (constructorStatement != null) {
					trace("declined", currentPass, "slot " + stackOffset +
						": two constructions of one slot are not one lifetime");
					return;
				}
				constructorStatement = statement;
				constructorOp = op;
				local = receiver;
			}
			else if (targetKind == SourceKind.DESTRUCTOR) {
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
		HighVariable localHigh = local.getHighVariable();
		String name = localHigh != null && localHigh.getName() != null
			? localHigh.getName() : local.getText();
		ClangVariableDecl declaration = localDeclaration(local, stackOffset);
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
		Long traced = stackAddressOffset(op.getInput(1));
		if (traced == null || traced != stackOffset) {
			return null;
		}
		for (ClangToken token : arguments.get(0)) {
			if (token instanceof ClangVariableToken variable &&
				variableStackOffset(variable) == stackOffset) {
				return variable;
			}
		}
		return null;
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
	private ClangVariableDecl localDeclaration(ClangVariableToken local, long stackOffset) {
		HighSymbol localSymbol = local.getHighSymbol(highFunction);
		if (localSymbol != null) {
			ClangVariableDecl indexed = markup.declarationFor(localSymbol);
			if (indexed != null) {
				return indexed;
			}
		}
		for (ClangNode node : bodyEntries) {
			if (!(node instanceof ClangVariableDecl declaration)) {
				continue;
			}
			List<ClangToken> tokens = new ArrayList<>();
			tokens.addAll(markup.tokenSpan(declaration).sig);
			for (ClangToken token : tokens) {
				if (!(token instanceof ClangVariableToken variable)) {
					continue;
				}
				HighVariable high = variable.getHighVariable();
				Varnode representative = high == null ? null : high.getRepresentative();
				if (variableStackOffset(variable) == stackOffset) {
					return declaration;
				}
			}
		}
		return null;
	}

	private long variableStackOffset(ClangVariableToken variable) {
		HighSymbol symbol = variable.getHighSymbol(highFunction);
		if (symbol != null && symbol.getStorage() != null && symbol.getStorage().isStackStorage()) {
			Varnode[] storage = symbol.getStorage().getVarnodes();
			if (storage.length == 1) {
				return storage[0].getAddress().getOffset();
			}
		}
		HighVariable high = variable.getHighVariable();
		Varnode representative = high == null ? null : high.getRepresentative();
		return representative != null && representative.getAddress() != null &&
			representative.getAddress().isStackAddress()
				? representative.getAddress().getOffset() : Long.MIN_VALUE;
	}

	private boolean mentionsExceptionList(ClangNode statement) {
		if (exceptionListAddress == null || !(statement instanceof ClangStatement clang)) {
			return false;
		}
		return operationReferencesAddress(clang.getPcodeOp(), exceptionListAddress.getOffset(), 0);
	}

	/**
	 * Prove the VC6 EH registration frame and remember the link slot. The
	 * handler and state slots sit at fixed offsets above it, so their stores
	 * are identifiable wherever the compiler scheduled them.
	 */
	private void detectEhFrame() {
		exceptionListAddress = null;
		for (Symbol symbol : function.getProgram().getSymbolTable().getSymbols("ExceptionList")) {
			if (symbol.getAddress() != null && symbol.getAddress().isMemoryAddress()) {
				exceptionListAddress = symbol.getAddress();
				break;
			}
		}
		if (exceptionListAddress == null) {
			return;
		}
		boolean registered = false;
		for (ClangNode node : bodyEntries) {
			if (!(node instanceof ClangStatement statement) || !mentionsExceptionList(node)) {
				continue;
			}
			PcodeOp op = statement.getPcodeOp();
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

	private static boolean operationReferencesAddress(PcodeOp operation, long address, int depth) {
		if (operation == null || depth > 12) {
			return false;
		}
		Varnode output = operation.getOutput();
		if (varnodeReferencesAddress(output, address)) {
			return true;
		}
		for (int i = 0; i < operation.getNumInputs(); i++) {
			Varnode input = operation.getInput(i);
			if (varnodeReferencesAddress(input, address) ||
				operationReferencesAddress(input.getDef(), address, depth + 1)) {
				return true;
			}
		}
		return false;
	}

	private static boolean varnodeReferencesAddress(Varnode value, long address) {
		if (value == null) {
			return false;
		}
		if ((value.isAddress() || value.isConstant()) && value.getOffset() == address) {
			return true;
		}
		Long constant = tracedConstant(value, 0);
		return constant != null && constant == address;
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
		tokens.addAll(markup.tokenSpan(declaration).sig);
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
		PcodeOp rootOp = statement.getPcodeOp();
		if (rootOp != null && vftableStore(rootOp)) {
			return true;
		}
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp operation = operations.next();
			if (markup.statementFor(operation) == statement && vftableStore(operation)) {
				return true;
			}
		}
		return false;
	}

	/**
	 * A store of a reviewed vftable address is compiler-owned object-lifetime
	 * machinery even when the object is a local/inlined subobject rather than
	 * this function's {@code this}. Suppress the complete statement only when
	 * its exact STORE operation carries that symbol.
	 */
	void suppressCompilerVptrStores() {
		Set<ClangStatement> proved = Collections.newSetFromMap(new IdentityHashMap<>());
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp operation = operations.next();
			if (operation.getOpcode() == PcodeOp.STORE && operation.getNumInputs() >= 3 &&
				valueIsVftableSymbol(operation.getInput(2))) {
				ClangStatement statement = markup.statementFor(operation);
				if (statement != null) {
					proved.add(statement);
				}
			}
		}
		for (ClangStatement statement : proved) {
			if (claimDrop(statement)) {
				trace("applied", currentPass, statementAddress(statement));
			}
		}
	}

	private boolean vftableStore(PcodeOp op) {
		if (op.getOpcode() != PcodeOp.STORE || op.getNumInputs() < 3) {
			return false;
		}
		OptionalLong destination = thisOffset(op.getInput(1));
		if (destination.isEmpty()) {
			return false;
		}
		Varnode value = op.getInput(2);
		return valueIsVftableSymbol(value);
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
				return symbol != null && VtableResolver.isVftableName(symbol.getName());
			}
			PcodeOp def = current.getDef();
			if (def == null) {
				return false;
			}
			if (def.getOpcode() == PcodeOp.COPY || def.getOpcode() == PcodeOp.CAST) {
				current = def.getInput(0);
			}
			else if (def.getOpcode() == PcodeOp.PTRSUB &&
				def.getInput(0).isConstant() && def.getInput(0).getOffset() == 0 &&
				def.getInput(1).isConstant()) {
				current = def.getInput(1); // constant-space address materialization
			}
			else {
				return false;
			}
		}
		return false;
	}

	// ------------------------------------------------------------------
	// Constructor / destructor lifting
	// ------------------------------------------------------------------

	private boolean analyzeLifecycle(Set<ClangNode> dropped, List<String> initializers) {
		if (!hasThisParameter(function)) {
			trace("declined", currentPass, "prototype does not carry the this parameter");
			return false;
		}
		if (entity.bodyCarrier() instanceof BodyCarrier.Extracted extracted &&
			extracted.carrier().equals(emission) &&
			!suppressSelectedDeletingWrapperEpilogue(dropped)) {
			trace("declined", currentPass,
				"selected deleting-wrapper carrier has no fully proved compiler epilogue");
			return false;
		}
		int vptrStores = 0;
		int parameterPadding = 0;
		for (ClangNode node : bodyEntries) {
			if (node instanceof ClangVariableDecl declaration &&
				isParameterSlotPadding(declaration)) {
				dropped.add(declaration);
				parameterPadding++;
				continue;
			}
			if (node instanceof ClangStatement statement &&
				isVftableStore(statement)) {
				dropped.add(node);
				vptrStores++;
			}
		}
		if (vptrStores > 0) {
			trace("applied", "lifecycle.vptr-store", "x" + vptrStores);
		}
		if (parameterPadding > 0) {
			trace("applied", "lifecycle.parameter-padding", "x" + parameterPadding);
		}
		dropTerminalLifecycleReturn(dropped);
		if (entity.isConstructor()) {
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

	/**
	 * VC6 may inline the only observable source destructor body into a deleting
	 * wrapper.  Family resolution may select that emission as the body's
	 * carrier, but the wrapper's flag-controlled deallocation is still compiler
	 * output.  Remove it only after proving the exact hidden flag, the same
	 * complete-object receiver, the controlling bit-one branch, and an otherwise
	 * empty lexical guard.  Decorated/tagged role identity authorizes considering
	 * the emission; this P-code proof authorizes the source-span claim.
	 */
	private boolean suppressSelectedDeletingWrapperEpilogue(Set<ClangNode> dropped) {
		HighVariable thisHigh = null;
		HighVariable flagHigh = null;
		int explicitParameters = 0;
		for (Parameter parameter : function.getParameters()) {
			HighSymbol symbol = highFunction.getLocalSymbolMap()
					.getParamSymbol(parameter.getOrdinal());
			if (parameter.isAutoParameter() &&
				parameter.getAutoParameterType() == AutoParameterType.THIS) {
				thisHigh = symbol == null ? null : symbol.getHighVariable();
			}
			else if (!parameter.isAutoParameter()) {
				explicitParameters++;
				flagHigh = symbol == null ? null : symbol.getHighVariable();
			}
		}
		if (thisHigh == null || flagHigh == null || explicitParameters != 1) {
			return false;
		}

		PcodeOp deallocation = null;
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp operation = operations.next();
			if (operation.getOpcode() != PcodeOp.CALL || operation.getNumInputs() != 2) {
				continue;
			}
			Function target = callee(operation);
			OptionalLong receiverOffset = thisOffset(operation.getInput(1));
			if (target == null || !target.getName().startsWith("operator_delete") ||
				receiverOffset.isEmpty()) {
				continue;
			}
			if (deallocation != null) {
				return false;
			}
			deallocation = operation;
		}
		if (deallocation == null || deallocation.getParent() == null) {
			return false;
		}

		PcodeOp controllingBranch = null;
		operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp candidate = operations.next();
			if (candidate.getOpcode() != PcodeOp.CBRANCH || candidate.getParent() == null ||
				candidate.getNumInputs() < 2 ||
				!testsFlagBitOne(candidate.getInput(1), flagHigh) ||
				!isSuccessor(candidate.getParent(), deallocation.getParent())) {
				continue;
			}
			if (controllingBranch != null) {
				return false;
			}
			controllingBranch = candidate;
		}
		ClangStatement statement = markup.statementFor(deallocation);
		BlockView guard = statement == null ? null : blocks.ownerOf(statement);
		if (controllingBranch == null || guard == null || guard.parent == null) {
			return false;
		}
		for (ClangStatement contained : guard.statements) {
			if (contained != statement) {
				return false;
			}
		}
		List<ClangToken> guardSpan = boundGuardSpan(guard, controllingBranch);
		if (guardSpan == null) {
			return false;
		}
		dropped.addAll(guardSpan);
		dropped.add(statement);
		trace("applied", "lifecycle.deleting-wrapper-epilogue",
			"proved flag-bit-one deallocation of this @ " + statementAddress(statement));
		return true;
	}

	/** True only for a boolean expression derived from {@code flag & 1}. */
	private static boolean testsFlagBitOne(Varnode value, HighVariable flag) {
		Varnode current = value;
		for (int depth = 0; current != null && depth < 8; depth++) {
			PcodeOp definition = current.getDef();
			if (definition == null) {
				return false;
			}
			switch (definition.getOpcode()) {
				case PcodeOp.COPY, PcodeOp.CAST, PcodeOp.INT_ZEXT, PcodeOp.INT_SEXT,
					PcodeOp.BOOL_NEGATE:
					current = definition.getInput(0);
					break;
				case PcodeOp.INT_EQUAL, PcodeOp.INT_NOTEQUAL:
					if (definition.getInput(0).isConstant() &&
						definition.getInput(0).getOffset() == 0) {
						current = definition.getInput(1);
					}
					else if (definition.getInput(1).isConstant() &&
						definition.getInput(1).getOffset() == 0) {
						current = definition.getInput(0);
					}
					else {
						return false;
					}
					break;
				case PcodeOp.INT_AND:
					for (int i = 0; i < 2; i++) {
						Varnode mask = definition.getInput(i);
						Varnode operand = definition.getInput(1 - i);
						if (mask.isConstant() && mask.getOffset() == 1 &&
							traceableToHigh(operand, flag)) {
							return true;
						}
					}
					return false;
				default:
					return false;
			}
		}
		return false;
	}

	private boolean isParameterSlotPadding(ClangVariableDecl declaration) {
		DataType declaredType = declaration.getDataType();
		while (declaredType instanceof ghidra.program.model.data.Array array) {
			declaredType = array.getDataType();
		}
		if (declaredType == null || !Undefined.isUndefined(declaredType)) {
			return false;
		}
		HighSymbol symbol = declaration.getHighSymbol();
		VariableStorage storage = symbol == null ? null : symbol.getStorage();
		if (storage == null || !storage.isStackStorage()) {
			return false;
		}
		long start = storage.getStackOffset();
		long end = start + storage.size();
		HighVariable padding = declaration.getHighVariable();
		if (padding == null) {
			return false;
		}
		for (ClangVariableToken use : markup.usesOf(padding)) {
			if (!isInside(use, declaration)) {
				return false;
			}
		}
		for (ClangToken token : markup.tokens) {
			if (!(token instanceof ClangVariableToken variable)) {
				continue;
			}
			HighSymbol other = variable.getHighSymbol(highFunction);
			VariableStorage otherStorage = other == null ? null : other.getStorage();
			if (other == symbol || otherStorage == null || !otherStorage.isStackStorage()) {
				continue;
			}
			long otherStart = otherStorage.getStackOffset();
			long otherEnd = otherStart + otherStorage.size();
			if (otherStart < end && start < otherEnd) {
				return false; // the rounded slot is reused by a meaningful symbol
			}
		}
		for (Parameter parameter : function.getParameters()) {
			VariableStorage formal = parameter.getVariableStorage();
			if (parameter.isAutoParameter() || !formal.isStackStorage()) {
				continue;
			}
			long formalStart = formal.getStackOffset();
			long valueEnd = formalStart + formal.size();
			long slotEnd = (valueEnd + 3) & ~3L;
			if (start >= valueEnd && end <= slotEnd) {
				return true;
			}
		}
		return false;
	}

	/** Drop proved unused bytes in a rounded stack parameter slot in any function. */
	void suppressParameterPadding() {
		int count = 0;
		for (ClangNode node : bodyEntries) {
			if (node instanceof ClangVariableDecl declaration &&
				isParameterSlotPadding(declaration) && claimDrop(declaration)) {
				count++;
			}
		}
		if (count > 0) {
			trace("applied", currentPass, "x" + count);
		}
	}

	private void dropTerminalLifecycleReturn(Set<ClangNode> dropped) {
		ClangStatement onlyReturn = null;
		int returnCount = 0;
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp operation = operations.next();
			if (operation.getOpcode() != PcodeOp.RETURN) {
				continue;
			}
			returnCount++;
			onlyReturn = markup.statementFor(operation);
		}
		if (returnCount == 1 && onlyReturn != null) {
			dropped.add(onlyReturn);
			return;
		}
		for (int i = bodyEntries.size() - 1; i >= 0; i--) {
			ClangNode node = bodyEntries.get(i);
			if (!markup.isTopLevel(node) || !(node instanceof ClangStatement statement)) {
				continue;
			}
			PcodeOp op = statement.getPcodeOp();
			if (op != null && op.getOpcode() == PcodeOp.RETURN) {
				dropped.add(node);
			}
			return;
		}
	}

	private void consumeConstructorPrefix(Set<ClangNode> dropped, List<String> initializers) {
		for (ClangNode node : bodyEntries) {
			if (!(node instanceof ClangStatement statement)) {
				continue;
			}
			if (dropped.contains(statement)) {
				continue;
			}
			String initializer = subobjectConstructorInitializer(statement);
			if (initializer != null) {
				if (!initializer.isEmpty()) {
					initializers.add(initializer);
				}
				dropped.add(statement);
				continue;
			}
			PcodeOp operation = statement.getPcodeOp();
			if (operation != null && (operation.getOpcode() == PcodeOp.CALL ||
				operation.getOpcode() == PcodeOp.CALLIND)) {
				break; // the first authored call ends the construction prefix
			}
		}
	}

	/**
	 * A subobject construction becomes an initializer: base classes by the
	 * shared discovery (the base/base_* convention plus for-clause vftable
	 * evidence), members by field name. Empty argument lists are implicit
	 * C++ and produce no initializer text.
	 */
	private String subobjectConstructorInitializer(ClangStatement statement) {
		PcodeOp op = statement.getPcodeOp();
		Function target = callee(op);
		if (target == null || op == null || structure == null) {
			return null;
		}
		if (session.sourceKey(target).kind() != SourceKind.CONSTRUCTOR) {
			trace("declined", "lifecycle.constructor-candidate",
				target.getName(true) + " has no reviewed constructor identity");
			return null;
		}
		OptionalLong offset = op.getNumInputs() >= 2
			? thisOffset(op.getInput(1)) : OptionalLong.empty();
		if (offset.isEmpty() || offset.getAsLong() < 0 ||
			offset.getAsLong() > Integer.MAX_VALUE) {
			trace("declined", "lifecycle.constructor-candidate",
				target.getName(true) + " receiver does not trace to this");
			return null;
		}
		DataTypeComponent component =
			structure.getComponentContaining((int) offset.getAsLong());
		if (component == null || component.getOffset() != offset.getAsLong()) {
			trace("declined", "lifecycle.constructor-candidate",
				target.getName(true) + " offset 0x" + Long.toHexString(offset.getAsLong()) +
					" is not an exact component");
			return null;
		}
		boolean base = vtables.isBaseOffset(structure, offset.getAsLong());
		if (!base && component.getFieldName() == null) {
			return null; // a member initializer needs the field's name
		}
		List<String> arguments = callArgumentsAfterReceiver(statement, op);
		if (arguments == null) {
			trace("declined", "lifecycle.constructor-candidate",
				target.getName(true) + " rendered arguments do not bind to p-code");
			return null;
		}
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

	/**
	 * The call's rendered arguments, minus the receiver, with compiler
	 * upcast locals substituted. Returns null when the token shape and the
	 * p-code disagree.
	 */
	private List<String> callArgumentsAfterReceiver(ClangStatement statement, PcodeOp op) {
		return callArguments(statement, op, 1);
	}

	private List<String> callArguments(ClangStatement statement, PcodeOp op, int skip) {
		List<List<ClangToken>> arguments = callArgumentTokens(statement, op);
		if (arguments == null || arguments.size() < skip) {
			return null;
		}
		List<String> rendered = new ArrayList<>();
		for (int i = skip; i < arguments.size(); i++) {
			String structural = i + 1 < op.getNumInputs()
				? nullPreservingSource(op.getInput(i + 1)) : null;
			rendered.add(structural != null ? structural : argumentText(arguments.get(i)));
		}
		return rendered;
	}

	/** The source root behind a proved MULTIEQUAL(0, root+baseOffset). */
	private static String nullPreservingSource(Varnode value) {
		Varnode current = value;
		for (int step = 0; current != null && step < 16; step++) {
			PcodeOp definition = current.getDef();
			if (definition == null) {
				return null;
			}
			if (definition.getOpcode() == PcodeOp.COPY ||
				definition.getOpcode() == PcodeOp.CAST ||
				definition.getOpcode() == PcodeOp.INDIRECT) {
				current = definition.getInput(0);
				continue;
			}
			if (definition.getOpcode() != PcodeOp.MULTIEQUAL ||
				definition.getNumInputs() != 2) {
				return null;
			}
			int zero = tracedConstant(definition.getInput(0), 0) != null &&
				tracedConstant(definition.getInput(0), 0) == 0 ? 0
					: tracedConstant(definition.getInput(1), 0) != null &&
						tracedConstant(definition.getInput(1), 0) == 0 ? 1 : -1;
			if (zero < 0) {
				return null;
			}
			PointerValue adjusted = pointerValue(definition.getInput(1 - zero));
			return adjusted == null ? null : sanitizeHighName(adjusted.root);
		}
		return null;
	}

	private void consumeDestructorTail(Set<ClangNode> dropped) {
		List<ClangNode> topLevelStatements = bodyEntries.stream()
			.filter(node -> node instanceof ClangStatement && markup.isTopLevel(node)).toList();
		for (int i = topLevelStatements.size() - 1; i >= 0; i--) {
			ClangStatement statement = (ClangStatement) topLevelStatements.get(i);
			if (dropped.contains(statement)) {
				continue;
			}
			PcodeOp op = statement.getPcodeOp();
			if (op != null && op.getOpcode() == PcodeOp.RETURN) {
				dropped.add(statement);
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
		if (target == null || op == null || op.getNumInputs() < 2) {
			return false;
		}
		if (session.sourceKey(target).kind() != SourceKind.DESTRUCTOR) {
			trace("declined", "lifecycle.subobject-destructor",
				target.getName(true) + " is not classified as a destructor");
			return false;
		}
		OptionalLong offset = thisOffset(op.getInput(1));
		if (offset.isEmpty() || offset.getAsLong() < 0) {
			trace("declined", "lifecycle.subobject-destructor",
				target.getName(true) + " receiver does not trace to this");
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
		boolean constructor = entity.isConstructor();
		for (ClangNode node : bodyEntries) {
			if (!(node instanceof ClangStatement statement) || dropped.contains(node)) {
				continue;
			}
			PcodeOp op = statement.getPcodeOp();
			Function target = callee(op);
			SourceKind targetKind = target == null ? null : session.sourceKey(target).kind();
			if (targetKind == null ||
				(constructor ? targetKind != SourceKind.CONSTRUCTOR
					: targetKind != SourceKind.DESTRUCTOR) ||
				op.getNumInputs() < 2) {
				continue;
			}
			if (thisOffset(op.getInput(1)).isPresent()) {
				return false;
			}
		}
		return true;
	}

	private boolean isLifecycle(Function target) {
		SourceKind targetKind = session.sourceKey(target).kind();
		return targetKind == SourceKind.CONSTRUCTOR || targetKind == SourceKind.DESTRUCTOR;
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
	void rewriteVirtualCalls() {
		MarkupIndex.TokenSpan line = markup.bodySpan();
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp op = operations.next();
			if (op.getOpcode() != PcodeOp.CALLIND) {
				continue;
			}
			int[] span = virtualTargetSpan(line, op);
			if (span == null) {
				trace("declined", currentPass, "CALLIND @ " + op.getSeqnum() +
					": exact rendered target span unavailable");
				continue;
			}
			int start = span[0];
			int close = span[1];
			int argsOpen = span[2];
			if (rangeConflicts(line.sig, start, matchingParen(line.sig, argsOpen))) {
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
			Emission slotEmission = call.slot.function == null ? null
				: session.emission(call.slot.function);
			SourceKind slotSource = call.slot.function == null ? null
				: session.sourceKey(call.slot.function).kind();
			if (slotEmission == null || (slotSource != SourceKind.CONSTRUCTOR &&
				slotSource != SourceKind.DESTRUCTOR)) {
				String prefix = call.receiver.isEmpty() ? "" : call.receiver + "->";
				int argsClose = matchingParen(line.sig, argsOpen);
				List<int[]> argumentSpans = argsClose < 0 ? null
					: renderedArgumentSpans(line.sig, argsOpen, argsClose);
				if (argumentSpans == null || argumentSpans.size() != op.getNumInputs() - 1) {
					trace("declined", currentPass, site + ": rendered argument span mismatch");
					continue;
				}
				for (int i = 0; i < argumentSpans.size(); i++) {
					String parameter = sourceParameterExpression(op.getInput(i + 1));
					if (parameter != null) {
						int[] argument = argumentSpans.get(i);
						replaceSpan(line, argument[0], argument[1], parameter);
					}
				}
				if (replaceSpan(line, start, close, prefix + call.slot.name)) {
					trace("applied", currentPass,
						prefix + call.slot.name + " " + site);
				}
				continue;
			}
			if (slotEmission.isDeletingWrapper()) {
				// Virtual dispatch of the deleting destructor is the compiled
				// form of a source-level polymorphic delete.
				int argsClose = matchingParen(line.sig, argsOpen);
				if (argsClose < 0 || op.getNumInputs() < 2) {
					continue;
				}
				Varnode flag = op.getInput(op.getNumInputs() - 1);
				String object = call.receiver.isEmpty() ? "this" : call.receiver;
				String form = flag.isConstant()
					? deletingForm(slotEmission.kind(), flag.getOffset()) : null;
				if (form != null && replaceSpan(line, start, argsClose, form + object)) {
					trace("applied", currentPass, form + object + " " + site);
				}
				else if (form == null) {
					trace("declined", currentPass, site + ": " + slotEmission.kind() +
						" flag is not a source-level delete form");
				}
			}
			else {
				trace("declined", currentPass, site + ": slot holds " +
					call.slot.name + " (" + slotEmission.kind() + "), not a callable rewrite");
			}
		}
	}

	private boolean rangeConflicts(List<ClangToken> tokens, int start, int end) {
		if (start < 0 || end < start || end >= tokens.size()) {
			return true;
		}
		return false;
	}

	/**
	 * Preserve a virtual call result as a typed local when SSA proves that the
	 * result is subsequently consumed only as a member-call receiver. The slot's
	 * compiler-owned declaration supplies the type and source method identity;
	 * HighVariable identity binds every rendered use. Token spellings contribute
	 * neither meaning nor matching evidence.
	 */
	void materializeCallResultLocals() {
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp operation = operations.next();
			if (operation.getOpcode() != PcodeOp.CALLIND) {
				continue;
			}
			if (operation.getNumInputs() < 1 || operation.getNumInputs() > 2 ||
				operation.getOutput() == null) {
				trace("declined", currentPass, "CALLIND @ " + operation.getSeqnum() +
					": inputs=" + operation.getNumInputs() + ", output=" +
					(operation.getOutput() != null));
				continue;
			}
			VirtualCall call = resolveVirtualCall(operation);
			HighVariable result = declaredCallResult(operation.getOutput());
			HighSymbol symbol = result == null ? null : result.getSymbol();
			ClangVariableDecl declaration = symbol == null ? null : markup.declarationFor(symbol);
			if (declaration == null && result != null) {
				declaration = markup.declarationFor(result);
			}
			ClangStatement definition = markup.statementFor(operation);
			String localName = call == null ? null : callResultLocalName(call.slot.name);
			if (call == null || call.slot.returnType == null ||
				Undefined.isUndefined(call.slot.returnType) || result == null ||
				declaration == null || definition == null || localName == null) {
				if (call != null) {
					trace("declined", currentPass, "CALLIND @ " + operation.getSeqnum() +
						": return=" + call.slot.returnType + ", high=" + (result != null) +
						", symbol=" + (symbol != null) + ", declaration=" +
						(declaration != null) + ", statement=" + (definition != null));
				}
				continue;
			}

			Map<ClangStatement, String> receiverUses = new LinkedHashMap<>();
			boolean unsupportedUse = false;
			for (ClangVariableToken use : markup.usesOf(result)) {
				if (isInside(use, declaration) || isInside(use, definition)) {
					continue;
				}
				ClangStatement statement = containingStatement(use);
				PcodeOp useOp = statement == null ? null : statement.getPcodeOp();
				if (useOp == null) {
					unsupportedUse = true;
					break;
				}
				String renderedUse = directResultReceiverUse(useOp, statement, result, localName);
				if (renderedUse == null) {
					renderedUse = virtualResultFieldUse(useOp, result,
						call.slot.returnType, localName);
				}
				if (renderedUse == null) {
					trace("declined", currentPass, "CALLIND @ " + operation.getSeqnum() +
						": unsupported result use op=" +
						(useOp == null ? "null" : useOp.getMnemonic()) + " @ " +
						(statement == null ? "null" : statementAddress(statement)));
					unsupportedUse = true;
					break;
				}
				String previous = receiverUses.putIfAbsent(statement, renderedUse);
				if (previous != null && !previous.equals(renderedUse)) {
					unsupportedUse = true;
					break;
				}
			}
			if (unsupportedUse || receiverUses.isEmpty() || localNameInUse(localName, result)) {
				trace("declined", currentPass, "CALLIND @ " + operation.getSeqnum() +
					": result is ambiguous or has a non-receiver use");
				continue;
			}

			String prefix = call.receiver.isEmpty() ? "" : call.receiver + "->";
			String initializer = CxxTypePrinter.printDeclaration(call.slot.returnType,
				localName) + " = " + prefix + call.slot.name + "()";
			if (conflicts(declaration) || conflicts(definition) ||
				receiverUses.keySet().stream().anyMatch(this::conflicts)) {
				continue;
			}
			claimReplace(declaration, initializer);
			claimDrop(definition);
			for (Map.Entry<ClangStatement, String> use : receiverUses.entrySet()) {
				claimReplace(use.getKey(), use.getValue());
			}
			trace("applied", currentPass, initializer + " @ " + operation.getSeqnum() +
				", " + receiverUses.size() + " receiver use(s)");
		}
	}

	private String directResultReceiverUse(PcodeOp useOp, ClangStatement statement,
			HighVariable result, String localName) {
		Function target = callee(useOp);
		if (target == null || !(target.getParentNamespace() instanceof GhidraClass) ||
			useOp.getNumInputs() < 2 || !traceableToHigh(useOp.getInput(1), result)) {
			return null;
		}
		List<String> arguments = callArgumentsAfterReceiver(statement, useOp);
		return arguments == null ? null : localName + "->" + target.getName() + "(" +
			String.join(", ", arguments) + ")";
	}

	/**
	 * Render a virtual call argument that SSA proves is the address of a field
	 * in a typed virtual-call result. Token punctuation only binds the remaining
	 * arguments; the returned structure and field offset determine meaning.
	 */
	private String virtualResultFieldUse(PcodeOp useOp, HighVariable result,
			DataType resultType, String localName) {
		if (useOp.getOpcode() != PcodeOp.CALLIND) {
			return null;
		}
		VirtualCall call = resolveVirtualCall(useOp);
		MarkupIndex.TokenSpan line = markup.bodySpan();
		int[] target = virtualTargetSpan(line, useOp);
		if (call == null || target == null) {
			trace("declined", currentPass, "result field use: virtual=" + (call != null) +
				", span=" + (target != null));
			return null;
		}
		int argsClose = matchingParen(line.sig, target[2]);
		List<int[]> spans = argsClose < 0 ? null
			: renderedArgumentSpans(line.sig, target[2], argsClose);
		if (spans == null || spans.size() != useOp.getNumInputs() - 1) {
			trace("declined", currentPass, "result field use: arguments=" +
				(spans == null ? "null" : spans.size()) + ", inputs=" + useOp.getNumInputs());
			return null;
		}
		List<String> arguments = new ArrayList<>();
		boolean usedResult = false;
		for (int i = 0; i < spans.size(); i++) {
			String field = resultFieldAddress(useOp.getInput(i + 1), result,
				resultType, localName);
			if (field != null) {
				if (usedResult) {
					return null;
				}
				usedResult = true;
				arguments.add(field);
			}
			else {
				Long offset = offsetFromHigh(useOp.getInput(i + 1), result);
				if (offset != null) {
					trace("declined", currentPass, "result field use: unresolved offset 0x" +
						Long.toHexString(offset) + " in " + resultType);
					return null;
				}
				int[] span = spans.get(i);
				arguments.add(argumentText(line.sig.subList(span[0], span[1] + 1)));
			}
		}
		if (!usedResult) {
			trace("declined", currentPass, "result field use: no operand rooted at " +
				result.getName());
			return null;
		}
		String prefix = call.receiver.isEmpty() ? "" : call.receiver + "->";
		return prefix + call.slot.name + "(" + String.join(", ", arguments) + ")";
	}

	private static String resultFieldAddress(Varnode argument, HighVariable result,
			DataType resultType, String localName) {
		Long offset = offsetFromHigh(argument, result);
		Structure pointed = pointedStructure(resultType);
		if (offset == null || pointed == null || offset < 0 || offset > Integer.MAX_VALUE) {
			return null;
		}
		DataTypeComponent component = pointed.getComponentContaining(offset.intValue());
		if (component == null || component.getOffset() != offset ||
			component.getFieldName() == null) {
			return null;
		}
		return "&" + localName + "->" + component.getFieldName();
	}

	/** Constant byte offset from one SSA high through copies and pointer adds. */
	private static Long offsetFromHigh(Varnode value, HighVariable root) {
		Varnode current = value;
		long offset = 0;
		for (int i = 0; current != null && i < 32; i++) {
			PcodeOp definition = current.getDef();
			if (definition != null) {
				switch (definition.getOpcode()) {
					case PcodeOp.PTRSUB:
					case PcodeOp.INT_ADD:
						if (!definition.getInput(1).isConstant()) {
							return null;
						}
						offset += definition.getInput(1).getOffset();
						current = definition.getInput(0);
						continue;
					case PcodeOp.COPY:
					case PcodeOp.CAST:
					case PcodeOp.INDIRECT:
						current = definition.getInput(0);
						continue;
					default:
						break;
				}
			}
			return current.getHigh() == root ? offset : null;
		}
		return null;
	}

	private boolean localNameInUse(String name, HighVariable except) {
		for (ClangToken token : markup.tokens) {
			if (token instanceof ClangVariableToken variable) {
				HighVariable high = variable.getHighVariable();
				if (high != null && high != except && name.equals(high.getName())) {
					return true;
				}
			}
		}
		return false;
	}

	/** Follow the call-induced copy/cast chain to one rendered local variable. */
	private HighVariable declaredCallResult(Varnode output) {
		ArrayDeque<Varnode> pending = new ArrayDeque<>();
		Set<Varnode> visited = new HashSet<>();
		Set<HighVariable> candidates = new HashSet<>();
		pending.add(output);
		while (!pending.isEmpty()) {
			Varnode current = pending.removeFirst();
			if (!visited.add(current)) {
				continue;
			}
			HighVariable high = current.getHigh();
			if (high != null && markup.declarationFor(high) != null) {
				candidates.add(high);
			}
			Iterator<PcodeOp> descendants = current.getDescendants();
			while (descendants.hasNext()) {
				PcodeOp descendant = descendants.next();
				if (descendant.getOutput() == null ||
					(descendant.getOpcode() != PcodeOp.COPY &&
						descendant.getOpcode() != PcodeOp.CAST &&
						descendant.getOpcode() != PcodeOp.INDIRECT)) {
					continue;
				}
				pending.add(descendant.getOutput());
			}
		}
		return candidates.size() == 1 ? candidates.iterator().next() : null;
	}

	private static String callResultLocalName(String method) {
		if (method == null || !method.startsWith("Get") || method.length() <= 3) {
			return null;
		}
		String stem = method.substring(3);
		String name = Character.toLowerCase(stem.charAt(0)) + stem.substring(1);
		if (!Character.isJavaIdentifierStart(name.charAt(0))) {
			return null;
		}
		for (int i = 1; i < name.length(); i++) {
			if (!Character.isJavaIdentifierPart(name.charAt(i))) {
				return null;
			}
		}
		return name;
	}

	private static boolean isInside(ClangNode node, ClangNode ancestor) {
		for (ClangNode current = node; current != null; current = current.Parent()) {
			if (current == ancestor) {
				return true;
			}
		}
		return false;
	}

	private static ClangStatement containingStatement(ClangNode node) {
		for (ClangNode current = node; current != null; current = current.Parent()) {
			if (current instanceof ClangStatement statement) {
				return statement;
			}
		}
		return null;
	}

	/**
	 * Locate the paired target and argument spans of one exact CALLIND. The
	 * output variable and assignment operator may also carry the CALLIND, so an
	 * arbitrary token with the operation is not a valid anchor. Instead, bind
	 * the adjacent syntax pairs whose target span contains the operation and
	 * whose argument count agrees with the P-code inputs.
	 */
	private static int[] virtualTargetSpan(MarkupIndex.TokenSpan line, PcodeOp operation) {
		for (int argsOpen = 0; argsOpen < line.sig.size(); argsOpen++) {
			ClangToken candidate = line.sig.get(argsOpen);
			if (!SyntaxPairs.opensPair(candidate) || !"(".equals(candidate.getText())) {
				continue;
			}
			int argsClose = SyntaxPairs.matchingClose(line.sig, argsOpen);
			int targetClose = argsOpen - 1;
			if (argsClose < 0 || targetClose < 0 ||
				!SyntaxPairs.closesPair(line.sig.get(targetClose))) {
				continue;
			}
			int targetOpen = SyntaxPairs.matchingOpen(line.sig, targetClose);
			if (targetOpen < 0 || !containsOperation(line.sig, targetOpen, targetClose,
				operation) || renderedArgumentCount(line.sig, argsOpen, argsClose) !=
				operation.getNumInputs() - 1) {
				continue;
			}
			return new int[] {targetOpen, targetClose, argsOpen};
		}
		return null;
	}

	private static boolean containsOperation(List<ClangToken> tokens, int start, int end,
			PcodeOp operation) {
		for (int i = start; i <= end; i++) {
			if (tokens.get(i).getPcodeOp() == operation) {
				return true;
			}
		}
		return false;
	}

	/** Argument punctuation is markup binding; P-code supplies the expected count. */
	private static int renderedArgumentCount(List<ClangToken> tokens, int open, int close) {
		if (close == open + 1) {
			return 0;
		}
		int count = 1;
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
				count++;
			}
		}
		return count;
	}

	private static List<int[]> renderedArgumentSpans(List<ClangToken> tokens, int open,
			int close) {
		List<int[]> spans = new ArrayList<>();
		if (close == open + 1) {
			return spans;
		}
		int start = open + 1;
		int nested = 0;
		for (int i = start; i < close; i++) {
			ClangToken token = tokens.get(i);
			if (SyntaxPairs.opensPair(token)) {
				nested++;
			}
			else if (SyntaxPairs.closesPair(token)) {
				nested--;
			}
			else if (nested == 0 && ",".equals(token.getText())) {
				spans.add(new int[] {start, i - 1});
				start = i + 1;
			}
		}
		spans.add(new int[] {start, close - 1});
		return spans;
	}

	/**
	 * Recover one source parameter from a P-code value assembled with ABI slot
	 * padding. Every non-parameter leaf must be undefined storage in the rounded
	 * bytes belonging to that same formal parameter.
	 */
	private String sourceParameterExpression(Varnode value) {
		Set<HighSymbol> parameters = new HashSet<>();
		Set<HighVariable> otherLeaves = new HashSet<>();
		collectParameterLeaves(value, new HashSet<>(), parameters, otherLeaves, 0);
		if (parameters.size() != 1) {
			return null;
		}
		HighSymbol parameter = parameters.iterator().next();
		for (HighVariable other : otherLeaves) {
			if (!isRoundedParameterPadding(other, parameter)) {
				return null;
			}
		}
		return parameter.getName();
	}

	private static void collectParameterLeaves(Varnode value, Set<Varnode> visited,
			Set<HighSymbol> parameters, Set<HighVariable> otherLeaves, int depth) {
		if (value == null || depth > 16 || !visited.add(value) || value.isConstant()) {
			return;
		}
		HighVariable high = value.getHigh();
		HighSymbol symbol = high == null ? null : high.getSymbol();
		if (symbol != null && symbol.isParameter()) {
			parameters.add(symbol);
			return;
		}
		PcodeOp definition = value.getDef();
		if (definition != null) {
			switch (definition.getOpcode()) {
				case PcodeOp.COPY, PcodeOp.CAST, PcodeOp.INT_ZEXT, PcodeOp.INT_SEXT,
					PcodeOp.SUBPIECE, PcodeOp.PIECE:
					for (int i = 0; i < definition.getNumInputs(); i++) {
						collectParameterLeaves(definition.getInput(i), visited, parameters,
							otherLeaves, depth + 1);
					}
					return;
				default:
					break;
			}
		}
		if (high != null) {
			otherLeaves.add(high);
		}
	}

	private boolean isRoundedParameterPadding(HighVariable high, HighSymbol parameter) {
		DataType type = high.getDataType();
		if (type == null || !Undefined.isUndefined(type)) {
			return false;
		}
		HighSymbol symbol = high.getSymbol();
		VariableStorage storage = symbol == null ? null : symbol.getStorage();
		VariableStorage formal = parameter.getStorage();
		if (storage == null || formal == null || !storage.isStackStorage() ||
			!formal.isStackStorage()) {
			return false;
		}
		long valueEnd = formal.getStackOffset() + formal.size();
		long slotEnd = (valueEnd + 3) & ~3L;
		long start = storage.getStackOffset();
		return start >= valueEnd && start + storage.size() <= slotEnd;
	}

	/** A resolved virtual call site: the receiver's spelling and the slot. */
	private static final class VirtualCall {
		final String receiver; // empty = implicit this
		final VtableResolver.Slot slot;

		VirtualCall(String receiver, VtableResolver.Slot slot) {
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
							vtables.isBaseOffset(structure, fieldOff.getAsLong())) {
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

		VtableResolver.Slot slot = vtableSlot(receiverClass, subobjectOffset, slotOffset);
		if (slot == null || (slot.function != null &&
			!(slot.function.getParentNamespace() instanceof GhidraClass) &&
			!session.emission(slot.function).isDeletingWrapper())) {
			return null;
		}
		return new VirtualCall(receiver, slot);
	}

	/**
	 * The function installed at byte offset {@code slotOffset} of the class's
	 * vftable for the given subobject, through the shared resolver.
	 */
	private VtableResolver.Slot vtableSlot(Structure receiverClass, long subobjectOffset,
			long slotOffset) {
		if (slotOffset < 0 || slotOffset > 0x1000) {
			return null;
		}
		Symbol table = vtables.tableFor(receiverClass, subobjectOffset);
		if (table == null) {
			return null;
		}
		return vtables.slot(table, slotOffset);
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

	// ------------------------------------------------------------------
	// Typed array indexing
	// ------------------------------------------------------------------

	/**
	 * Recover the exact pointer-arithmetic lowering
	 * {@code *(T *)(base + index * sizeof(T))} as
	 * {@code ((T *)base)[index]}. The cast type and stride are both required;
	 * a byte offset that does not equal the applied element width declines.
	 */
	void rewriteArrayIndexing() {
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp load = operations.next();
			ArrayAccess access = arrayAccess(load);
			if (access == null) {
				continue;
			}
			ArraySpan span = bindArraySpan(access);
			if (span == null) {
				trace("declined", currentPass, "LOAD @ " + load.getSeqnum() +
					": operands do not bind one exact rendered expression span");
				continue;
			}
			String base = sourceTokenText(span.base);
			String index = sourceTokenText(span.index);
			if (replaceSpan(span.line, span.start, span.end,
				"((" + CxxTypePrinter.printType(access.element) + "*)" + base + ")[" +
					index + "]")) {
				trace("applied", currentPass, "LOAD(PTRADD) stride " + access.stride);
			}
		}
	}

	/**
	 * C accepts an implicit conversion from {@code void*}; C++ does not. When a
	 * LOAD from a reviewed void-pointer field feeds a typed pointer local, retain
	 * the decompiler's data flow with the explicit cast required by the source
	 * language. Field offset and both types come from Ghidra objects.
	 */
	void rewriteVoidPointerConversions() {
		if (structure == null) {
			return;
		}
		for (ClangNode node : bodyEntries) {
			if (!(node instanceof ClangStatement statement)) {
				continue;
			}
			PcodeOp operation = statement.getPcodeOp();
			if (operation == null || operation.getOpcode() != PcodeOp.LOAD ||
				operation.getOutput() == null || operation.getNumInputs() < 2) {
				continue;
			}
			HighVariable destination = operation.getOutput().getHigh();
			DataType destinationType = destination == null ? null : destination.getDataType();
			OptionalLong offset = thisOffset(operation.getInput(1));
			DataTypeComponent field = offset.isEmpty() ? null
				: sourceFieldAtOffset(structure, offset.getAsLong());
			if (destination == null || sanitizeHighName(destination) == null ||
				!isTypedPointer(destinationType) || field == null ||
				!isVoidPointer(field.getDataType()) || field.getFieldName() == null) {
				continue;
			}
			String rendered = sanitizeHighName(destination) + " = (" +
				CxxTypePrinter.printType(destinationType) + ")" + field.getFieldName();
			if (claimReplace(statement, rendered)) {
				trace("applied", currentPass, rendered + " @ " + statementAddress(statement));
			}
		}
	}

	private DataTypeComponent sourceFieldAtOffset(Structure owner, long absoluteOffset) {
		if (absoluteOffset < 0 || absoluteOffset > Integer.MAX_VALUE) {
			return null;
		}
		Structure current = owner;
		int relative = (int) absoluteOffset;
		for (int depth = 0; depth < 16; depth++) {
			DataTypeComponent component = current.getComponentContaining(relative);
			if (component == null) {
				return null;
			}
			DataType nested = resolvedType(component.getDataType());
			if (nested instanceof Structure base &&
				vtables.isBaseOffset(current, component.getOffset())) {
				relative -= component.getOffset();
				current = base;
				continue;
			}
			return component.getOffset() == relative ? component : null;
		}
		return null;
	}

	private static boolean isTypedPointer(DataType type) {
		DataType resolved = resolvedType(type);
		return resolved instanceof Pointer pointer &&
			!"void".equals(resolvedType(pointer.getDataType()).getName());
	}

	private static boolean isVoidPointer(DataType type) {
		DataType resolved = resolvedType(type);
		return resolved instanceof Pointer pointer &&
			"void".equals(resolvedType(pointer.getDataType()).getName());
	}

	private static DataType resolvedType(DataType type) {
		DataType current = type;
		while (current instanceof TypeDef typedef) {
			current = typedef.getBaseDataType();
		}
		return current;
	}

	private static final class ArrayAccess {
		final PcodeOp load;
		final Varnode base;
		final Varnode index;
		final DataType element;
		final long stride;
		final Set<PcodeOp> operations;

		ArrayAccess(PcodeOp load, Varnode base, Varnode index, DataType element,
				long stride, Set<PcodeOp> operations) {
			this.load = load;
			this.base = base;
			this.index = index;
			this.element = element;
			this.stride = stride;
			this.operations = operations;
		}
	}

	private static final class ArraySpan {
		final MarkupIndex.TokenSpan line;
		final int start;
		final int end;
		final ClangVariableToken base;
		final ClangVariableToken index;

		ArraySpan(MarkupIndex.TokenSpan line, int start, int end, ClangVariableToken base,
				ClangVariableToken index) {
			this.line = line;
			this.start = start;
			this.end = end;
			this.base = base;
			this.index = index;
		}
	}

	/** Prove LOAD(PTRADD) or LOAD(INT_ADD(base, INT_MULT(index, stride))). */
	private static ArrayAccess arrayAccess(PcodeOp load) {
		if (load.getOpcode() != PcodeOp.LOAD || load.getNumInputs() < 2 ||
			load.getOutput() == null || load.getOutput().getHigh() == null) {
			return null;
		}
		Varnode address = stripCopies(load.getInput(1));
		PcodeOp addressOp = address == null ? null : address.getDef();
		Long stride = null;
		Varnode base = null;
		Varnode index = null;
		Set<PcodeOp> semanticOps = Collections.newSetFromMap(new IdentityHashMap<>());
		semanticOps.add(load);
		if (addressOp != null && addressOp.getOpcode() == PcodeOp.PTRADD &&
			addressOp.getNumInputs() >= 3 && addressOp.getInput(2).isConstant()) {
			stride = addressOp.getInput(2).getOffset();
			base = stripCopies(addressOp.getInput(0));
			index = stripCopies(addressOp.getInput(1));
			semanticOps.add(addressOp);
		}
		else if (addressOp != null && addressOp.getOpcode() == PcodeOp.INT_ADD) {
			for (int i = 0; i < 2; i++) {
				Varnode term = stripCopies(addressOp.getInput(i));
				PcodeOp multiply = term == null ? null : term.getDef();
				if (multiply == null || multiply.getOpcode() != PcodeOp.INT_MULT) {
					continue;
				}
				for (int j = 0; j < 2; j++) {
					if (multiply.getInput(j).isConstant()) {
						stride = multiply.getInput(j).getOffset();
						index = stripCopies(multiply.getInput(1 - j));
					}
				}
				base = stripCopies(addressOp.getInput(1 - i));
				semanticOps.add(addressOp);
				semanticOps.add(multiply);
			}
		}
		DataType element = load.getOutput().getHigh().getDataType();
		if (stride == null || base == null || index == null || base.isConstant() ||
			index.isConstant() || element == null || element.getLength() <= 0 ||
			stride != element.getLength() || Undefined.isUndefined(element)) {
			return null;
		}
		return new ArrayAccess(load, base, index, element, stride, semanticOps);
	}

	/** Bind the proved LOAD and its exact base/index varnodes to one paired span. */
	private ArraySpan bindArraySpan(ArrayAccess access) {
		ClangStatement statement = markup.statementFor(access.load);
		if (statement == null) {
			return null;
		}
		MarkupIndex.TokenSpan line = markup.tokenSpan(statement);
		int anchor = -1;
		ClangVariableToken base = null;
		ClangVariableToken index = null;
		int baseCount = 0;
		int indexCount = 0;
		for (int i = 0; i < line.sig.size(); i++) {
			ClangToken token = line.sig.get(i);
			if (token.getPcodeOp() == access.load) {
				if (anchor >= 0) return null;
				anchor = i;
			}
			if (token instanceof ClangVariableToken variable) {
				if (sameValue(variable.getVarnode(), access.base)) {
					base = variable;
					baseCount++;
				}
				if (sameValue(variable.getVarnode(), access.index)) {
					index = variable;
					indexCount++;
				}
			}
		}
		if (anchor < 0 || baseCount != 1 || indexCount != 1 || base == null || index == null) {
			return null;
		}
		int castOpen = anchor + 1;
		if (castOpen >= line.sig.size() || !SyntaxPairs.opensPair(line.sig.get(castOpen))) {
			return null;
		}
		int castClose = SyntaxPairs.matchingClose(line.sig, castOpen);
		int expressionOpen = castClose + 1;
		if (castClose <= castOpen || expressionOpen >= line.sig.size() ||
			!SyntaxPairs.opensPair(line.sig.get(expressionOpen))) {
			return null;
		}
		int expressionClose = SyntaxPairs.matchingClose(line.sig, expressionOpen);
		int baseIndex = line.sig.indexOf(base);
		int indexIndex = line.sig.indexOf(index);
		if (expressionClose <= expressionOpen || baseIndex <= expressionOpen ||
			baseIndex >= expressionClose || indexIndex <= expressionOpen ||
			indexIndex >= expressionClose) {
			return null;
		}
		boolean typedElement = false;
		boolean pointerDeclarator = false;
		for (int i = castOpen + 1; i < castClose; i++) {
			if (line.sig.get(i) instanceof ClangTypeToken type &&
				type.getDataType() != null && type.getDataType().isEquivalent(access.element)) {
				typedElement = true;
			}
			if ("*".equals(line.sig.get(i).getText())) {
				pointerDeclarator = true;
			}
		}
		if (!typedElement || !pointerDeclarator) {
			return null;
		}
		for (int i = expressionOpen + 1; i < expressionClose; i++) {
			ClangToken token = line.sig.get(i);
			PcodeOp operation = token.getPcodeOp();
			if (operation != null && !access.operations.contains(operation)) {
				return null;
			}
			if (token instanceof ClangVariableToken variable && variable != base &&
				variable != index) {
				var scalar = variable.getScalar();
				if (scalar == null || scalar.getUnsignedValue() != access.stride) {
					return null;
				}
			}
		}
		return new ArraySpan(line, anchor, expressionClose, base, index);
	}

	private static Varnode stripCopies(Varnode value) {
		Varnode current = value;
		for (int i = 0; current != null && i < 32; i++) {
			PcodeOp definition = current.getDef();
			if (definition == null || (definition.getOpcode() != PcodeOp.COPY &&
				definition.getOpcode() != PcodeOp.CAST &&
				definition.getOpcode() != PcodeOp.INDIRECT)) {
				return current;
			}
			current = definition.getInput(0);
		}
		return current;
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
	void normalizeMemberAccess() {
		applyBaseAccessPatterns(markup.bodySpan());
	}

	/**
	 * Ghidra materializes C++ references as pointers. Regression export carries
	 * compiler-indexed reference slots explicitly, so only those proven formal
	 * parameters have the rendered pointer member operator changed to object
	 * member syntax. Text identifies the operator span; the source AST and
	 * HighSymbol identity decide the meaning.
	 */
	void normalizeSourceReferences() {
		if (sourceReferenceForms.length == 0) {
			return;
		}
		Set<HighSymbol> references = new HashSet<>();
		int sourceIndex = 0;
		for (Parameter parameter : function.getParameters()) {
			if (parameter.isAutoParameter()) {
				continue;
			}
			if (sourceIndex < sourceReferenceForms.length &&
				sourceReferenceForms[sourceIndex].rendersObjectMemberAccess()) {
				HighSymbol symbol = highFunction.getLocalSymbolMap()
						.getParamSymbol(parameter.getOrdinal());
				if (symbol != null) {
					references.add(symbol);
				}
			}
			sourceIndex++;
		}
		if (references.isEmpty()) {
			return;
		}
		MarkupIndex.TokenSpan line = markup.bodySpan();
		for (int i = 0; i + 1 < line.sig.size(); i++) {
			if (!(line.sig.get(i) instanceof ClangVariableToken variable) ||
				!references.contains(variable.getHighSymbol(highFunction)) ||
				!"->".equals(line.sig.get(i + 1).getText())) {
				continue;
			}
			if (claimReplace(line.sig.get(i + 1), ".")) {
				trace("applied", currentPass,
					"reference parameter slot " + variable.getHighSymbol(highFunction).getCategoryIndex());
			}
		}
	}

	/** One sweep applying every non-overlapping match; returns the count. */
	private int applyBaseAccessPatterns(MarkupIndex.TokenSpan live) {
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
				replaceSpan(live, i, i + 3,
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

	/**
	 * A field token accessing a base subobject: by the naming convention,
	 * or by the token's own composite type and offset through the shared
	 * discovery (the field token carries which structure it reads and
	 * where — the evidence, independent of the field's name).
	 */
	private boolean isBaseFieldToken(ClangToken token) {
		return token instanceof ClangFieldToken field && vtables.isBaseField(field);
	}

	private boolean nextFieldIsVftable(List<ClangToken> sig, int index) {
		return index < sig.size() && vtables.isVftableField(sig.get(index));
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
	void rewriteStringLiterals() {
		MarkupIndex.TokenSpan line = markup.bodySpan();
		int literals = 0;
		for (ClangToken token : line.sig) {
			if (!(token instanceof ClangVariableToken variable)) {
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
	void rewriteNullPointerCasts() {
		MarkupIndex.TokenSpan live = markup.bodySpan();
		List<ClangToken> sig = live.sig;
		for (int zero = 0; zero < sig.size(); zero++) {
			if (!(sig.get(zero) instanceof ClangVariableToken token) ||
				!isPointerZero(token)) {
				continue;
			}
			int from = zero;
			if (zero > 0 && sig.get(zero - 1) instanceof ghidra.app.decompiler.ClangSyntaxToken close &&
				close.getClose() >= 0) {
				for (int candidate = zero - 2; candidate >= 0; candidate--) {
					if (!(sig.get(candidate) instanceof ghidra.app.decompiler.ClangSyntaxToken open) ||
						open.getOpen() != close.getClose()) {
						continue;
					}
					for (int type = candidate + 1; type < zero - 1; type++) {
						if (sig.get(type) instanceof ClangTypeToken) {
							from = candidate;
							break;
						}
					}
					break;
				}
			}
			if (replaceSpan(live, from, zero, "0")) {
				trace("applied", currentPass,
					"typed zero pointer" + (from == zero ? "" : " with rendered cast"));
			}
		}
	}

	private static boolean isPointerZero(ClangVariableToken token) {
		if (token.getScalar() == null || token.getScalar().getValue() != 0) {
			return false;
		}
		HighVariable high = token.getHighVariable();
		if (high != null && isPointerType(high.getDataType())) {
			return true;
		}
		Varnode varnode = token.getVarnode();
		if (varnode != null && varnode.getHigh() != null &&
			isPointerType(varnode.getHigh().getDataType())) {
			return true;
		}
		PcodeOp operation = token.getPcodeOp();
		return operation != null && operation.getOpcode() == PcodeOp.CAST &&
			operation.getOutput() != null && operation.getOutput().getHigh() != null &&
			isPointerType(operation.getOutput().getHigh().getDataType());
	}

	private static boolean isPointerType(DataType type) {
		DataType current = type;
		while (current instanceof TypeDef typedef) {
			current = typedef.getBaseDataType();
		}
		return current instanceof Pointer;
	}

	/** SSA proof for the compiler's null-preserving derived-to-base temporary. */
	void rewriteNullPreservingUpcasts() {
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp merge = operations.next();
			if (merge.getOpcode() != PcodeOp.MULTIEQUAL || merge.getNumInputs() != 2 ||
				merge.getOutput() == null || merge.getOutput().getHigh() == null) {
				continue;
			}
			int zeroIndex = tracedConstant(merge.getInput(0), 0) != null &&
				tracedConstant(merge.getInput(0), 0) == 0 ? 0
					: tracedConstant(merge.getInput(1), 0) != null &&
						tracedConstant(merge.getInput(1), 0) == 0 ? 1 : -1;
			if (zeroIndex < 0 || !isPointerType(merge.getOutput().getHigh().getDataType())) {
				continue;
			}
			PointerValue adjusted = pointerValue(merge.getInput(1 - zeroIndex));
			if (adjusted == null || adjusted.root == null || adjusted.offset == 0) {
				continue;
			}
			ClangStatement zero = markup.statementFor(merge.getInput(zeroIndex).getDef());
			ClangStatement offset = markup.statementFor(merge.getInput(1 - zeroIndex).getDef());
			BlockView zeroBlock = zero == null ? null : blocks.ownerOf(zero);
			BlockView offsetBlock = offset == null ? null : blocks.ownerOf(offset);
			if (zeroBlock == null || offsetBlock == null || zeroBlock == offsetBlock ||
				zeroBlock.parent == null || zeroBlock.parent != offsetBlock.parent) {
				continue;
			}
			if (zero.getPcodeOp() == null || offset.getPcodeOp() == null) {
				continue;
			}
			HighVariable destination = merge.getOutput().getHigh();
			String source = sanitizeHighName(adjusted.root);
			int outsideUses = 0;
			for (ClangVariableToken use : markup.usesOf(destination)) {
				BlockView owner = blocks.ownerOf(use);
				if (owner != zeroBlock && owner != offsetBlock) {
					outsideUses++;
				}
			}
			if (source == null || outsideUses == 0) {
				continue;
			}
			PcodeOp branch = nullBranchFor(adjusted.root, zero.getPcodeOp(), offset.getPcodeOp());
			if (branch == null || !claimConditionalRegion(branch, zeroBlock, offsetBlock)) {
				continue;
			}
			String cast = "(" + CxxTypePrinter.printType(destination.getDataType()) + ")" + source;
			int replacedUses = 0;
			for (ClangVariableToken use : markup.usesOf(destination)) {
				boolean declarationUse = false;
				for (ClangNode ancestor = use; ancestor != null; ancestor = ancestor.Parent()) {
					if (ancestor instanceof ClangVariableDecl) {
						declarationUse = true;
						break;
					}
				}
				if (!declarationUse && claimReplace(use, cast)) {
					replacedUses++;
				}
			}
			if (replacedUses > 0) {
				ClangVariableDecl declaration = markup.declarationFor(destination);
				if (declaration != null) {
					claimDrop(declaration);
				}
				trace("applied", currentPass, "MULTIEQUAL null/base pointer -> " + cast);
			}
		}
	}

	private static final class PointerValue {
		final HighVariable root;
		final long offset;

		PointerValue(HighVariable root, long offset) {
			this.root = root;
			this.offset = offset;
		}
	}

	private static PointerValue pointerValue(Varnode value) {
		Varnode current = value;
		long offset = 0;
		for (int i = 0; current != null && i < 32; i++) {
			if (current.getHigh() != null && current.getDef() == null) {
				return new PointerValue(current.getHigh(), offset);
			}
			PcodeOp definition = current.getDef();
			if (definition == null) {
				return current.getHigh() == null ? null : new PointerValue(current.getHigh(), offset);
			}
			switch (definition.getOpcode()) {
				case PcodeOp.COPY:
				case PcodeOp.CAST:
				case PcodeOp.INDIRECT:
					current = definition.getInput(0);
					break;
				case PcodeOp.PTRSUB:
				case PcodeOp.INT_ADD:
					if (!definition.getInput(1).isConstant()) {
						return null;
					}
					offset += definition.getInput(1).getOffset();
					current = definition.getInput(0);
					break;
				default:
					return null;
			}
		}
		return null;
	}

	private PcodeOp nullBranchFor(HighVariable source, PcodeOp zeroDefinition,
			PcodeOp offsetDefinition) {
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp comparison = operations.next();
			if (!isNullTestOp(comparison, source) || comparison.getOutput() == null) {
				continue;
			}
			var uses = comparison.getOutput().getDescendants();
			while (uses.hasNext()) {
				PcodeOp branch = uses.next();
				if (branch.getOpcode() == PcodeOp.CBRANCH && branch.getParent() != null &&
					zeroDefinition.getParent() != null && offsetDefinition.getParent() != null &&
					isSuccessor(branch.getParent(), zeroDefinition.getParent()) &&
					isSuccessor(branch.getParent(), offsetDefinition.getParent())) {
					return branch;
				}
			}
		}
		return null;
	}

	private boolean claimConditionalRegion(PcodeOp branch, BlockView first, BlockView second) {
		Varnode condition = branch.getNumInputs() > 1 ? stripCopies(branch.getInput(1)) : null;
		PcodeOp comparison = condition == null ? null : condition.getDef();
		MarkupIndex.TokenSpan line = markup.bodySpan();
		int conditionIndex = -1;
		for (int i = 0; comparison != null && i < line.sig.size(); i++) {
			if (line.sig.get(i).getPcodeOp() == comparison) {
				conditionIndex = i;
				break;
			}
		}
		int start = -1;
		for (int i = conditionIndex - 1; i >= 0 && i >= conditionIndex - 32; i--) {
			if ("if".equals(line.sig.get(i).getText())) {
				start = i;
				break;
			}
		}
		int end = Math.max(blockCloseIndex(first, line), blockCloseIndex(second, line));
		return start >= 0 && end > conditionIndex && replaceSpan(line, start, end, "");
	}

	private int blockCloseIndex(BlockView block, MarkupIndex.TokenSpan line) {
		if (!(block.markup.Parent() instanceof ClangTokenGroup parent)) {
			return -1;
		}
		boolean after = false;
		for (int child = 0; child < parent.numChildren(); child++) {
			ClangNode node = parent.Child(child);
			if (node == block.markup) {
				after = true;
				continue;
			}
			if (!after) {
				continue;
			}
			List<ClangToken> tokens = new ArrayList<>();
			tokens.addAll(markup.tokenSpan(node).sig);
			for (ClangToken token : tokens) {
				if ("}".equals(token.getText())) {
					return line.sig.indexOf(token);
				}
			}
		}
		return -1;
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
	void rewriteMethodCalls() {
		MarkupIndex.TokenSpan live = markup.bodySpan();
		List<ClangToken> sig = live.sig;
		for (int i = 0; i < sig.size(); i++) {
			if (!(sig.get(i) instanceof ClangFuncNameToken)) {
				continue;
			}
			PcodeOp op = sig.get(i).getPcodeOp();
			ClangStatement containing = markup.statementFor(op);
			Function target = callee(op);
			if (target == null || op.getNumInputs() < 2 ||
				!(target.getParentNamespace() instanceof GhidraClass) ||
				isLifecycle(target) ||
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
			List<List<ClangToken>> arguments = containing == null ? null
				: callArgumentTokens(containing, op);
			if (arguments == null) {
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
			replaceSpan(live, start, statementCall ? close + 1 : close,
				prefix + target.getName() + "(" + String.join(", ", rendered) + ")" +
					(statementCall ? ";" : ""));
		}
	}

	private String argumentText(List<ClangToken> tokens) {
		if (tokens.size() == 1 && tokens.get(0) instanceof ClangVariableToken variable) {
			HighVariable high = variable.getHighVariable();
			HighSymbol symbol = high == null ? null : high.getSymbol();
			if (symbol != null && symbol.getStorage() != null) {
				for (Parameter parameter : function.getParameters()) {
					if (parameter.getVariableStorage().equals(symbol.getStorage())) {
						return parameter.getName();
					}
				}
			}
		}
		StringBuilder text = new StringBuilder();
		for (ClangToken token : tokens) {
			text.append(sourceTokenText(token));
		}
		return text.toString();
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
			if (component == null || component.getOffset() != value) {
				return null;
			}
			if (vtables.isBaseOffset(structure, value)) {
				return "";
			}
			if (component.getFieldName() == null) {
				return null;
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
			if (!sourceTokenText(token).isEmpty()) {
				live.add(token);
			}
		}
		if (live.size() == 1 && live.get(0) instanceof ClangVariableToken) {
			return text + "->";
		}
		// A composite receiver expression, e.g. a cast: parenthesize it.
		return "(" + text + ")->";
	}

	/**
	 * Lift MSVC structure-return lowering back to a value return:
	 * {@code p = Class::Method(this, &storage, args)} becomes
	 * {@code storage = Method(args)}. The callee's auto parameter identifies
	 * the hidden storage input, whose p-code must address the same stack slot
	 * as a variable token in the bound argument span. The returned alias
	 * pointer must be unused — a used
	 * alias would lose real dataflow, so it declines.
	 */
	void rewriteStructReturns() {
		for (ClangNode node : bodyEntries) {
			if (!(node instanceof ClangStatement statement)) {
				continue;
			}
			PcodeOp op = statement.getPcodeOp();
			Function target = callee(op);
			if (target == null) {
				continue;
			}
			int storageIndex = returnStorageParameterIndex(target);
			if (storageIndex < 0 || isLifecycle(target)) {
				continue;
			}
			String site = "@ " + statementAddress(statement);
			List<List<ClangToken>> arguments = callArgumentTokens(statement, op);
			if (arguments == null || storageIndex >= arguments.size()) {
				trace("declined", currentPass, site +
					": argument tokens disagree with the p-code operands");
				continue;
			}
			if (1 + storageIndex >= op.getNumInputs()) {
				continue;
			}
			Long storageOffset = stackAddressOffset(op.getInput(1 + storageIndex));
			ClangVariableToken destination = null;
			if (storageOffset != null) {
				for (ClangToken token : arguments.get(storageIndex)) {
					if (token instanceof ClangVariableToken variable &&
						variableStackOffset(variable) == storageOffset) {
						destination = variable;
						break;
					}
				}
			}
			if (destination == null) {
				trace("declined", currentPass, site +
					": hidden storage input is not a proved stack local");
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
		MarkupIndex.TokenSpan line = markup.bodySpan();
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
			ClangToken arrow = i + 1 < line.sig.size() &&
				"->".equals(text(line.sig, i + 1))
						? line.sig.get(i + 1) : null;
			uses.add(new ClangToken[] {token, arrow});
		}
		return !uses.isEmpty();
	}

	private static int returnStorageParameterIndex(Function target) {
		for (int i = 0; i < target.getParameterCount(); i++) {
			Parameter parameter = target.getParameter(i);
			if (parameter.isAutoParameter() &&
				parameter.getAutoParameterType() == AutoParameterType.RETURN_STORAGE_PTR) {
				return i;
			}
		}
		return -1;
	}

	private static boolean hasReturnStorageParameter(Function target) {
		return returnStorageParameterIndex(target) >= 0;
	}

	/** Whether the function receives a hidden {@code this}, by ABI identity. */
	private static boolean hasThisParameter(Function target) {
		for (int i = 0; i < target.getParameterCount(); i++) {
			Parameter parameter = target.getParameter(i);
			if (parameter.isAutoParameter() &&
				parameter.getAutoParameterType() == AutoParameterType.THIS) {
				return true;
			}
		}
		return false;
	}

	void renderCompletePrototype() {
		ClangFuncProto prototype = findProto(root);
		String rendered = CallableIdentity.prototypeOrNull(function, entity.key());
		if (rendered == null) {
			trace("declined", currentPass, "formal signature contains an unresolved ABI type");
			return;
		}
		if (claimReplace(prototype, rendered)) {
			analysis.liftSignature = true;
			trace("applied", currentPass, entity.key().hasSourceSignature()
				? "complete prototype rendered from transient source hints"
				: "complete prototype rendered from Function");
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

	private static String initializerSuffix(List<String> initializers) {
		StringBuilder signature = new StringBuilder();
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

	void rewriteAllocationPairs() {
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp operation = operations.next();
			Function target = callee(operation);
			if (target == null || !target.getName().startsWith("operator_new")) {
				continue;
			}
			ClangStatement statement = markup.statementFor(operation);
			if (statement != null) {
				rewriteGuardedNew(statement, operation);
			}
		}
		for (BlockView rootBlock : blocks.roots) {
			rewriteAllocationPairs(rootBlock);
		}
	}

	/** Match adjacent allocation/lifecycle statements within exact lexical blocks. */
	private void rewriteAllocationPairs(BlockView block) {
		List<ClangStatement> statements = block.statements;
		for (int i = 0; i < statements.size(); i++) {
			ClangStatement statement = statements.get(i);
			if (rewriteScalarDeletingCall(statement)) {
				continue;
			}
			if (rewriteTypedObjectDelete(statement)) {
				continue;
			}
			ClangStatement next = nextLiveStatement(statements, i);
			if (next == null) {
				continue;
			}
			if (rewriteDestructorDeletePair(statement, next)) {
				continue;
			}
			rewriteNewConstructorPair(statement, next);
		}
		for (BlockView child : block.children) {
			rewriteAllocationPairs(child);
		}
	}

	/**
	 * A global deallocator receiving a typed object pointer is the trivial-
	 * destructor lowering of {@code delete object}. Pointer buffers (T**) and
	 * void/primitive storage remain explicit raw-storage deallocation calls.
	 */
	private boolean rewriteTypedObjectDelete(ClangStatement statement) {
		PcodeOp op = statement.getPcodeOp();
		Function deallocator = callee(op);
		if (deallocator == null || op == null || op.getNumInputs() != 2 ||
			!SpecialNames.normalize(deallocator.getName()).startsWith("operator delete")) {
			return false;
		}
		Varnode argument = op.getInput(1);
		HighVariable high = argument.getHigh();
		DataType type = resolvedType(high == null ? null : high.getDataType());
		if (!(type instanceof Pointer pointer) ||
			!(resolvedType(pointer.getDataType()) instanceof Structure)) {
			return false;
		}
		String object = singleVariableArgument(statement, op, 1);
		if (object == null || !claimReplace(statement, "delete " + object)) {
			return false;
		}
		trace("applied", "allocation.trivial-delete",
			"delete " + object + " @ " + statementAddress(statement));
		return true;
	}

	private ClangStatement nextLiveStatement(List<ClangStatement> statements, int index) {
		for (int j = index + 1; j < statements.size(); j++) {
			ClangStatement candidate = statements.get(j);
			return candidate;
		}
		return null;
	}

	/** A scalar/vector deleting wrapper call with an ABI-proven constant flag. */
	private boolean rewriteScalarDeletingCall(ClangStatement statement) {
		PcodeOp op = statement.getPcodeOp();
		Function target = callee(op);
		Emission targetEmission = target == null ? null : session.emission(target);
		if (targetEmission == null || !targetEmission.isDeletingWrapper() ||
			op.getNumInputs() != 3 || !op.getInput(2).isConstant()) {
			return false;
		}
		String object = singleVariableArgument(statement, op, 1);
		if (object == null) {
			return false;
		}
		long flags = op.getInput(2).getOffset();
		String form = deletingForm(targetEmission.kind(), flags);
		if (form == null) {
			trace("declined", "allocation.delete", targetEmission.kind() + " flag " + flags +
				" is compiler-internal, not a source-level delete");
			return false;
		}
		if (!claimReplace(statement, form + object)) {
			return false;
		}
		trace("applied", "allocation.delete",
			form.trim() + " " + object + " @ " + statementAddress(statement));
		return true;
	}

	private static String deletingForm(EmissionKind emission, long flags) {
		return DeletingDestructorSemantics.sourceOperator(emission, flags);
	}

	/** `T::~T(x); operator_delete(x);` collapses to `delete x`. */
	private boolean rewriteDestructorDeletePair(ClangStatement first, ClangStatement second) {
		PcodeOp destructorOp = first.getPcodeOp();
		Function destructor = callee(destructorOp);
		if (destructor == null ||
			session.sourceKey(destructor).kind() != SourceKind.DESTRUCTOR ||
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
		if (object == null || !sameValue(destructorOp.getInput(1), deleteOp.getInput(1))) {
			trace("declined", "allocation.scalar-delete", "@ " +
				statementAddress(first) +
				": destructor receiver and deallocated pointer differ");
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

	/**
	 * VC6 lowers a source-level {@code r = new T(args)} with the compiler's
	 * own null guard around the construction:
	 *
	 * <pre>
	 *   x = operator_new(N);
	 *   r = 0;                  // optional failure-path value
	 *   if (x != 0) {
	 *     T::T(x, args...);
	 *     r = extraout_EAX;     // optional: the constructor returns this
	 *   }
	 * </pre>
	 *
	 * The whole shape collapses back to the one source expression — ordinary
	 * typed construction, so the compiler re-emits the same lowering. The
	 * guard must test exactly the allocation result against null and contain
	 * nothing but the construction and the result copy; when the emission
	 * rebinds a separate result variable, the raw allocation pointer must
	 * have no uses outside the matched shape.
	 */
	private boolean rewriteGuardedNew(ClangStatement allocation, PcodeOp newOp) {
		Function allocator = callee(newOp);
		if (allocator == null || !allocator.getName().startsWith("operator_new") ||
			newOp.getOutput() == null || newOp.getOutput().getHigh() == null) {
			return false;
		}
		HighVariable allocated = newOp.getOutput().getHigh();
		PcodeOp constructorOp = null;
		Function constructor = null;
		Iterator<? extends PcodeOp> operations = highFunction.getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp candidate = operations.next();
			Function target = callee(candidate);
			if (target == null || session.sourceKey(target).kind() != SourceKind.CONSTRUCTOR ||
				candidate.getNumInputs() < 2 ||
				!traceableToHigh(candidate.getInput(1), allocated)) {
				continue;
			}
			if (constructorOp != null) {
				trace("declined", "allocation.guarded-new", "@ " + statementAddress(allocation) +
					": more than one constructor consumes the allocation");
				return false;
			}
			constructorOp = candidate;
			constructor = target;
		}
		if (constructorOp == null) {
			trace("declined", "allocation.guarded-new", "@ " + statementAddress(allocation) +
				": no constructor receiver traces to the allocation high");
			return false;
		}
		ClangStatement construction = markup.statementFor(constructorOp);
		BlockView allocationBlock = blocks.ownerOf(allocation);
		BlockView guardBlock = construction == null ? null : blocks.ownerOf(construction);
		if (allocationBlock == null || guardBlock == null || guardBlock.parent != allocationBlock) {
			trace("declined", "allocation.guarded-new", "@ " + statementAddress(allocation) +
				": constructor is not in one direct lexical guard block");
			return false;
		}
		PcodeOp branch = guardedNullBranch(newOp.getOutput(), constructorOp);
		if (branch == null || constructorOp.getParent() == null || branch.getParent() == null ||
			!isSuccessor(branch.getParent(), constructorOp.getParent())) {
			trace("declined", "allocation.guarded-new", "@ " + statementAddress(allocation) +
				": CFG does not guard construction with the allocation null test");
			return false;
		}
		ClangStatement copyOut = null;
		HighVariable result = null;
		for (ClangStatement statement : guardBlock.statements) {
			if (statement == construction) {
				continue;
			}
			HighVariable target = constructorResultCopyTarget(statement);
			if (target == null || copyOut != null) {
				trace("declined", "allocation.guarded-new", "@ " + statementAddress(allocation) +
					": guarded block contains unrelated statements");
				return false;
			}
			copyOut = statement;
			result = target;
		}
		ClangStatement zero = null;
		if (result != null) {
			for (ClangStatement statement : allocationBlock.statements) {
				if (zeroAssignmentTarget(statement) == result) {
					zero = statement;
					break;
				}
			}
		}
		if (result == null) {
			result = allocated;
		}
		if (result != allocated && hasUnrelatedAllocationUse(newOp.getOutput(), constructorOp,
			branch)) {
			trace("declined", "allocation.guarded-new", "@ " + statementAddress(allocation) +
				": raw allocation has unrelated uses");
			return false;
		}
		List<String> arguments = callArgumentsAfterReceiver(construction, constructorOp);
		if (arguments == null) {
			return false;
		}
		String resultName = sanitizeHighName(result);
		String type = TypeNames.map(constructor.getParentNamespace().getName(true));
		if (resultName == null) {
			return false;
		}
		if (!claimGuardSpan(guardBlock, branch)) {
			trace("declined", "allocation.guarded-new", "@ " + statementAddress(allocation) +
				": proved guard has no complete rendered span");
			return false;
		}
		if (!claimReplace(allocation,
			resultName + " = new " + type + "(" + String.join(", ", arguments) + ")")) {
			return false;
		}
		if (zero != null) {
			claimDrop(zero);
		}
		trace("applied", "allocation.guarded-new",
			resultName + " = new " + type + "(...) @ " + statementAddress(allocation));
		return true;
	}

	/** Bind the complete if/compound-statement presentation after CFG proof. */
	private boolean claimGuardSpan(BlockView guardBlock, PcodeOp branch) {
		List<ClangToken> span = boundGuardSpan(guardBlock, branch);
		if (span == null) {
			return false;
		}
		for (ClangToken token : span) {
			claimDrop(token);
		}
		return true;
	}

	/** Locate, but do not claim, a complete rendered guard span. */
	private List<ClangToken> boundGuardSpan(BlockView guardBlock, PcodeOp branch) {
		Varnode condition = branch.getNumInputs() > 1 ? stripCopies(branch.getInput(1)) : null;
		PcodeOp comparison = condition == null ? null : condition.getDef();
		if (comparison == null) {
			return null;
		}
		MarkupIndex.TokenSpan line = markup.bodySpan();
		int conditionToken = -1;
		for (int i = 0; i < line.sig.size(); i++) {
			if (line.sig.get(i).getPcodeOp() == comparison) {
				conditionToken = i;
				break;
			}
		}
		if (conditionToken < 0) {
			return null;
		}
		int start = -1;
		for (int i = conditionToken - 1; i >= 0 && i >= conditionToken - 32; i--) {
			if ("if".equals(line.sig.get(i).getText())) {
				start = i;
				break;
			}
		}
		List<ClangToken> blockTokens = new ArrayList<>();
		blockTokens.addAll(markup.tokenSpan(guardBlock.markup).sig);
		if (start < 0 || blockTokens.isEmpty()) {
			return null;
		}
		int firstBlock = -1;
		for (ClangToken token : blockTokens) {
			firstBlock = line.sig.indexOf(token);
			if (firstBlock >= 0) {
				break;
			}
		}
		int end = -1;
		for (int i = firstBlock - 1; i > conditionToken; i--) {
			if (SyntaxPairs.opensPair(line.sig.get(i)) &&
				"{".equals(line.sig.get(i).getText())) {
				end = SyntaxPairs.matchingClose(line.sig, i);
				break;
			}
		}
		if (end < 0 && guardBlock.markup.Parent() instanceof ClangTokenGroup parent) {
			boolean afterBlock = false;
			for (int child = 0; child < parent.numChildren() && end < 0; child++) {
				ClangNode node = parent.Child(child);
				if (node == guardBlock.markup) {
					afterBlock = true;
					continue;
				}
				if (!afterBlock) {
					continue;
				}
				List<ClangToken> sibling = new ArrayList<>();
				sibling.addAll(markup.tokenSpan(node).sig);
				for (ClangToken token : sibling) {
					if ("}".equals(token.getText())) {
						end = line.sig.indexOf(token);
						break;
					}
				}
			}
		}
		if (end < conditionToken) {
			return null;
		}
		return line.region(start, end);
	}

	private static boolean isSuccessor(ghidra.program.model.pcode.PcodeBlock from,
			ghidra.program.model.pcode.PcodeBlock target) {
		for (int i = 0; i < from.getOutSize(); i++) {
			if (from.getOut(i) == target) {
				return true;
			}
		}
		return false;
	}

	private static PcodeOp guardedNullBranch(Varnode allocation, PcodeOp constructor) {
		var uses = allocation.getDescendants();
		while (uses.hasNext()) {
			PcodeOp comparison = uses.next();
			if (!isNullTestOp(comparison, allocation.getHigh()) || comparison.getOutput() == null) {
				continue;
			}
			var comparisonUses = comparison.getOutput().getDescendants();
			while (comparisonUses.hasNext()) {
				PcodeOp branch = comparisonUses.next();
				if (branch.getOpcode() == PcodeOp.CBRANCH && branch.getParent() != null &&
					constructor.getParent() != null) {
					return branch;
				}
			}
		}
		return null;
	}

	private static boolean hasUnrelatedAllocationUse(Varnode allocation, PcodeOp constructor,
			PcodeOp branch) {
		var uses = allocation.getDescendants();
		while (uses.hasNext()) {
			PcodeOp use = uses.next();
			if (use == constructor || use == branch || use.getOpcode() == PcodeOp.INDIRECT ||
				use.getOpcode() == PcodeOp.INT_NOTEQUAL || use.getOpcode() == PcodeOp.INT_EQUAL ||
				use.getOpcode() == PcodeOp.MULTIEQUAL) {
				continue;
			}
			return true;
		}
		return false;
	}

	private static String sanitizeHighName(HighVariable high) {
		if (high == null || high.getName() == null) {
			return null;
		}
		return "this".equals(high.getName()) &&
			(high.getSymbol() == null || !high.getSymbol().isThisPointer()) ? "this_" : high.getName();
	}

	/** The high assigned by `target = 0;`, else null. */
	private static HighVariable zeroAssignmentTarget(ClangStatement statement) {
		PcodeOp op = statement.getPcodeOp();
		if (op == null ||
			(op.getOpcode() != PcodeOp.COPY && op.getOpcode() != PcodeOp.CAST) ||
			!op.getInput(0).isConstant() || op.getInput(0).getOffset() != 0 ||
			op.getOutput() == null) {
			return null;
		}
		return op.getOutput().getHigh();
	}

	/** Whether the op is the rendered `x != 0` comparison testing the high. */
	private static boolean isNullTestOp(PcodeOp op, HighVariable tested) {
		if (op == null || (op.getOpcode() != PcodeOp.INT_NOTEQUAL &&
			op.getOpcode() != PcodeOp.INT_EQUAL)) {
			return false;
		}
		Varnode left = op.getInput(0);
		Varnode right = op.getInput(1);
		if (left.isConstant() && left.getOffset() == 0) {
			return traceableToHigh(right, tested);
		}
		if (right.isConstant() && right.getOffset() == 0) {
			return traceableToHigh(left, tested);
		}
		return false;
	}

	/** The high assigned from the constructor's EAX result storage, else null. */
	private HighVariable constructorResultCopyTarget(ClangStatement statement) {
		PcodeOp op = statement.getPcodeOp();
		if (op == null || op.getOpcode() != PcodeOp.COPY || op.getOutput() == null) {
			return null;
		}
		var register = function.getProgram().getRegister(op.getInput(0));
		if (register == null || !"EAX".equalsIgnoreCase(register.getName())) {
			return null;
		}
		return op.getOutput().getHigh();
	}

	/** Whether the varnode is the high, looking through copies and casts. */
	private static boolean traceableToHigh(Varnode varnode, HighVariable high) {
		Varnode current = varnode;
		for (int step = 0; current != null && step < 8; step++) {
			if (current.getHigh() == high) {
				return true;
			}
			PcodeOp def = current.getDef();
			if (def == null || (def.getOpcode() != PcodeOp.COPY &&
				def.getOpcode() != PcodeOp.CAST && def.getOpcode() != PcodeOp.INDIRECT)) {
				return false;
			}
			current = def.getInput(0);
		}
		return false;
	}

	/** `x = operator_new(N); T::T(x, args...);` collapses to `x = new T(args...)`. */
	private boolean rewriteNewConstructorPair(ClangStatement first, ClangStatement second) {
		PcodeOp newOp = first.getPcodeOp();
		Function allocator = callee(newOp);
		if (allocator == null || !allocator.getName().startsWith("operator_new") ||
			newOp.getOutput() == null) {
			return false;
		}
		HighVariable allocated = newOp.getOutput().getHigh();
		String result = sanitizeHighName(allocated);
		if (allocated == null || result == null) {
			return false;
		}
		PcodeOp constructorOp = second.getPcodeOp();
		Function constructor = callee(constructorOp);
		if (constructor == null ||
			session.sourceKey(constructor).kind() != SourceKind.CONSTRUCTOR ||
			constructorOp.getNumInputs() < 2) {
			return false;
		}
		if (!traceableToHigh(constructorOp.getInput(1), allocated)) {
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

	/** Whether two call operands carry the same value through decompiler copies/casts. */
	private static boolean sameValue(Varnode left, Varnode right) {
		Varnode a = stripCopies(left);
		Varnode b = stripCopies(right);
		return a != null && b != null && (a.equals(b) ||
			(a.getHigh() != null && a.getHigh() == b.getHigh()));
	}

	/**
	 * The call's significant argument tokens, split at top-level commas.
	 * The function-name token must carry the same p-code operation, so the
	 * rendered argument list belongs to exactly the call being analyzed;
	 * the list scope comes from the decompiler's own delimiter pairing.
	 * Returns null when the token shape and the p-code operand count
	 * disagree.
	 */
	private List<List<ClangToken>> callArgumentTokens(ClangStatement statement,
			PcodeOp op) {
		RenderedCall call = RenderedCall.bind(markup, op);
		if (call == null || blocks.ownerOf(call.functionName) != blocks.ownerOf(statement)) {
			return null;
		}
		List<List<ClangToken>> arguments = new ArrayList<>();
		for (RenderedCall.Span argument : call.arguments) {
			arguments.add(argument.tokens());
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
