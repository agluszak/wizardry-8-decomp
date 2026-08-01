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
import ghidra.program.model.pcode.HighFunction;
import ghidra.program.model.pcode.HighSymbol;
import ghidra.program.model.pcode.HighVariable;
import ghidra.program.model.pcode.PcodeOp;
import ghidra.program.model.pcode.Varnode;
import ghidra.program.model.symbol.Symbol;

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

	private static String tokenText(ClangNode node) {
		List<ClangToken> tokens = new ArrayList<>();
		leafTokens(node, tokens);
		StringBuilder text = new StringBuilder();
		for (ClangToken token : tokens) {
			if (token.getText() != null) {
				text.append(token.getText());
			}
		}
		return text.toString();
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
				? target.getParentNamespace().getName()
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
			String text = tokens.get(i).getText();
			if (text == null) {
				continue;
			}
			switch (text) {
				case "(":
					depth++;
					current.append(text);
					break;
				case ")":
					depth--;
					if (depth > 0) {
						current.append(text);
					}
					break;
				case ",":
					if (depth == 1) {
						arguments.add(finishArgument(current));
						current.setLength(0);
					}
					else {
						current.append(text);
					}
					break;
				default:
					current.append(text);
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
		String owner = function.getParentNamespace().getName(true);
		String name = function.getParentNamespace().getName();
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
		String type = constructor.getParentNamespace().getName(true);
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
		return argument.get(0).getText();
	}
}
