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
 * reproduces everything else verbatim. Any internal error downgrades the
 * whole analysis to the empty (fully verbatim) result.
 */
final class Msvc6Patterns {

	/** What the printer should do differently from verbatim output. */
	static final class Analysis {
		final Set<ClangNode> dropped = new HashSet<>();
		final Map<ClangNode, String> replaced = new HashMap<>();
		boolean liftSignature = false;
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
	/** Compiler-computed null-preserving upcasts: local name -> replacement text. */
	private final Map<String, String> upcasts = new HashMap<>();

	private Msvc6Patterns(Function function, FunctionKind kind, DecompileResults results) {
		this.function = function;
		this.kind = kind;
		this.results = results;
	}

	static Analysis analyze(Function function, FunctionKind kind, DecompileResults results) {
		Msvc6Patterns patterns = new Msvc6Patterns(function, kind, results);
		try {
			patterns.run();
			return patterns.analysis;
		}
		catch (Exception e) {
			return new Analysis();
		}
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
		rewriteVirtualCalls();
		normalizeMemberAccess();
		rewriteNullPointerCasts();
		rewriteMethodCalls();

		Set<ClangNode> lifecycleDropped = new HashSet<>();
		List<String> initializers = new ArrayList<>();
		boolean lifted = false;
		if (kind == FunctionKind.CONSTRUCTOR || kind == FunctionKind.DESTRUCTOR) {
			lifted = analyzeLifecycle(lifecycleDropped, initializers);
		}
		if (lifted) {
			analysis.dropped.addAll(lifecycleDropped);
			analysis.liftSignature = true;
			analysis.replaced.put(findProto(root), liftedSignature(initializers));
		}
		rewriteAllocationPairs();
		if (kind == FunctionKind.ORDINARY) {
			liftOrdinarySignature();
		}
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
	private void claimRange(TokenLine line, int sigFrom, int sigTo, String replacement) {
		int rawFrom = line.rawIndex.get(sigFrom);
		int rawTo = line.rawIndex.get(sigTo);
		for (int i = rawFrom; i <= rawTo; i++) {
			ClangToken token = line.raw.get(i);
			analysis.replaced.remove(token);
			analysis.dropped.add(token);
		}
		ClangToken first = line.raw.get(rawFrom);
		if (replacement.isEmpty()) {
			return;
		}
		analysis.dropped.remove(first);
		analysis.replaced.put(first, replacement);
	}

	private void dropToken(ClangToken token) {
		analysis.replaced.remove(token);
		analysis.dropped.add(token);
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
			return false;
		}
		detectEhFrame();

		for (Item item : items) {
			if (item.isStatement()) {
				ClangStatement statement = (ClangStatement) item.node;
				if (isEhScaffolding(statement) || isVftableStore(statement)) {
					dropped.add(statement);
				}
			}
			else if (item.node instanceof ClangVariableDecl declaration &&
				isEhSlotDeclaration(declaration)) {
				dropped.add(declaration);
			}
		}
		if (kind == FunctionKind.CONSTRUCTOR) {
			consumeConstructorPrefix(dropped, initializers);
		}
		else {
			consumeDestructorTail(dropped);
		}
		return noSubobjectLifecycleCallsRemain(dropped);
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
				if (dropped.contains(statement)) {
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
		String name = isBaseField(component.getFieldName())
				? TypeNames.map(target.getParentNamespace().getName())
				: component.getFieldName();
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
			if (dropped.contains(statement)) {
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
			if (!item.isStatement() || dropped.contains(item.node)) {
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
			VirtualCall call = resolveVirtualCall(op);
			if (call == null) {
				continue;
			}
			FunctionKind slotKind = FunctionKind.classify(call.slot);
			if (slotKind == FunctionKind.ORDINARY) {
				String prefix = call.receiver.isEmpty() ? "" : call.receiver + "->";
				claimRange(line, start, close, prefix + call.slot.getName());
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
				if ("1".equals(flag)) {
					claimRange(line, start, argsClose, "delete " + object);
				}
				else if ("3".equals(flag)) {
					claimRange(line, start, argsClose, "delete[] " + object);
				}
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
			claimRange(live, start, close,
				prefix + target.getName() + "(" + String.join(", ", rendered) + ")");
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
		analysis.replaced.put(statement, form + object);
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
			return false;
		}
		analysis.replaced.put(first, "delete " + object);
		analysis.dropped.add(second);
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
			return false;
		}
		List<String> arguments = callArgumentsAfterReceiver(second, constructorOp);
		if (arguments == null) {
			return false;
		}
		String type = TypeNames.map(constructor.getParentNamespace().getName(true));
		analysis.replaced.put(first,
			result + " = new " + type + "(" + String.join(", ", arguments) + ")");
		analysis.dropped.add(second);
		return true;
	}

	/**
	 * The rendered argument at the given call operand position, accepted
	 * only when it is one plain variable token, so text identity is safe.
	 */
	private String singleVariableArgument(ClangStatement statement, PcodeOp op, int input) {
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
		if (arguments.size() != op.getNumInputs() - 1 || input - 1 >= arguments.size()) {
			return null;
		}
		List<ClangToken> argument = arguments.get(input - 1);
		if (argument.size() != 1 || !(argument.get(0) instanceof ClangVariableToken)) {
			return null;
		}
		return sanitizeReservedName(argument.get(0), argument.get(0).getText());
	}
}
