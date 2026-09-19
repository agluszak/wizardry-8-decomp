package wiz8.recovery;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import ghidra.app.decompiler.ClangFieldToken;
import ghidra.app.decompiler.ClangToken;
import ghidra.app.util.demangler.DemangledFunction;
import ghidra.app.util.demangler.DemangledObject;
import ghidra.app.util.demangler.microsoft.MicrosoftDemangler;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.data.FunctionDefinition;
import ghidra.program.model.data.Pointer;
import ghidra.program.model.data.Structure;
import ghidra.program.model.data.TypeDef;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Program;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolType;

/**
 * The one owner of vftable resolution and base-subobject discovery, shared
 * by the class printer (virtual sections) and the pattern recognizers
 * (virtual-call rewrites, base initializers, receiver folding).
 *
 * A class's tables are its namespace's {@code vftable} symbols: the
 * for-clause-free symbol is the complete-object table, and a
 * {@code vftable{for_'Base'}} symbol belongs to the base subobject whose
 * original base identity the for-clause names. Derived-specific subobject
 * views store that identity in their datatype description rather than in
 * the view's display name. Typed vftable Structures supply table extent and
 * slot contracts; the implementation address remains separate evidence.
 *
 * A leading structure component is a base subobject on either of two
 * grounds: the repository's {@code base}/{@code base_*} field convention
 * (the reviewed naming), or the positive evidence of a for-clause vftable
 * naming the component's type — a table Ghidra only has because the class
 * really inherits that subobject. Composition at offset 0 without either
 * ground stays a member; absence of evidence never promotes it.
 */
final class VtableResolver {
	private static final String VIRTUAL_SLOT_PROPERTY = "wiz8.virtual-slot.decorated";

	/** A source virtual identity, with an optional concrete binary emission. */
	static final class Slot {
		final String name;
		final DataType returnType;
		final Function function;
		final FunctionDefinition contract;

		Slot(String name, DataType returnType, Function function) {
			this(name, returnType, function, null);
		}

		Slot(String name, DataType returnType, Function function, FunctionDefinition contract) {
			this.name = name;
			this.returnType = returnType;
			this.function = function;
			this.contract = contract;
		}
	}

	private final Program program;
	private final Map<String, List<Symbol>> tablesByNamespace = new HashMap<>();
	private final Map<String, List<DataTypeComponent>> basesByStructure = new HashMap<>();
	private final Map<Address, Address> tableBounds = new HashMap<>();
	private final Map<Address, Boolean> unboundedTables = new HashMap<>();

	VtableResolver(Program program) {
		this.program = program;
	}

	/** Whether the symbol name is a vftable spelling (any for-clause). */
	static boolean isVftableName(String symbolName) {
		return SpecialNames.normalize(symbolName).startsWith("vftable");
	}

	/** The repository's base-subobject field naming convention. */
	static boolean isBaseFieldName(String fieldName) {
		return fieldName.equals("base") || fieldName.startsWith("base_");
	}

	/**
	 * Whether a field token denotes a proved base subobject. Structure/offset
	 * evidence is preferred; the reviewed {@code base_*} codec is the one
	 * isolated fallback when the project has not yet materialized richer
	 * inheritance state.
	 */
	boolean isBaseField(ClangFieldToken field) {
		DataType composite = resolve(field.getDataType());
		if (composite instanceof Structure owner && isBaseOffset(owner, field.getOffset())) {
			return true;
		}
		String name = field.getText();
		return name != null && isBaseFieldName(name);
	}

	/** The isolated displayed-name fallback for Ghidra's vftable field token. */
	boolean isVftableField(ClangToken token) {
		return token instanceof ClangFieldToken && token.getText() != null &&
			isVftableName(token.getText());
	}

	/** The class namespace structurally associated with this data type, else null. */
	GhidraClass classNamespace(Structure structure) {
		String category = structure.getCategoryPath().getPath();
		String qualified = (category == null || category.equals("/"))
			? structure.getName()
			: category.substring(1).replace("/", "::") + "::" + structure.getName();
		GhidraClass match = null;
		for (Symbol symbol : program.getSymbolTable().getSymbols(structure.getName())) {
			if (symbol.getSymbolType() == SymbolType.CLASS &&
				symbol.getObject() instanceof GhidraClass ghidraClass &&
				ghidraClass.getName(true).equals(qualified)) {
				if (match != null) {
					return null;
				}
				match = ghidraClass;
			}
		}
		if (match != null) {
			return match;
		}
		// Fallback: unique GhidraClass with this structure's leaf name. Category
		// paths are organization, not provenance — a Structure at /W8Monster or
		// a legacy /wiz8/classes/W8Monster copy must still resolve to class
		// W8Monster when that namespace is unambiguous.
		for (Symbol symbol : program.getSymbolTable().getSymbols(structure.getName())) {
			if (symbol.getSymbolType() == SymbolType.CLASS &&
				symbol.getObject() instanceof GhidraClass ghidraClass &&
				ghidraClass.getName().equals(structure.getName())) {
				if (match != null) {
					return null;
				}
				match = ghidraClass;
			}
		}
		return match;
	}

	/** Every vftable symbol the class namespace owns. */
	List<Symbol> vftables(Namespace namespace) {
		return tablesByNamespace.computeIfAbsent(namespace.getName(true), key -> {
			Map<Address, Symbol> byAddress = new LinkedHashMap<>();
			for (Symbol symbol : program.getSymbolTable().getSymbols(namespace)) {
				if (isVftableName(symbol.getName())) {
					Symbol previous = byAddress.get(symbol.getAddress());
					if (previous == null || symbol.getSource() == SourceType.USER_DEFINED) {
						byAddress.put(symbol.getAddress(), symbol);
					}
				}
			}
			return new ArrayList<>(byAddress.values());
		});
	}

	/** The unique for-clause-free (complete-object) table, else null. */
	Symbol primaryVftable(Namespace namespace) {
		Symbol primary = null;
		for (Symbol symbol : vftables(namespace)) {
			if (SpecialNames.normalize(symbol.getName()).equals("vftable")) {
				if (primary != null && !primary.getAddress().equals(symbol.getAddress())) {
					return null; // ambiguous
				}
				if (primary == null || symbol.getSource() == SourceType.USER_DEFINED) {
					primary = symbol;
				}
			}
		}
		return primary;
	}

	/**
	 * The table serving the subobject at {@code subobjectOffset} of the
	 * receiver class. Offset 0 is the complete-object table only; a
	 * for-clause table never stands in for it. A non-zero offset needs a
	 * component at exactly that offset whose resolved structure type the
	 * for-clause names — the type is the evidence, so the component's field
	 * name does not gate the match. Null declines.
	 */
	Symbol tableFor(Structure receiverClass, long subobjectOffset) {
		Namespace namespace = classNamespace(receiverClass);
		if (namespace == null) {
			return null;
		}
		if (subobjectOffset == 0) {
			return primaryVftable(namespace);
		}
		if (subobjectOffset < 0 || subobjectOffset > Integer.MAX_VALUE) {
			return null;
		}
		DataTypeComponent component =
			receiverClass.getComponentContaining((int) subobjectOffset);
		if (component == null || component.getOffset() != subobjectOffset) {
			return null;
		}
		DataType componentType = resolve(component.getDataType());
		if (!(componentType instanceof Structure)) {
			return null;
		}
		Structure componentStructure = (Structure) componentType;
		Address vtableAddress = parseSubobjectVtable(componentStructure.getDescription());
		if (vtableAddress != null) {
			Symbol primary = program.getSymbolTable().getPrimarySymbol(vtableAddress);
			if (primary != null) {
				return primary;
			}
		}
		return forClauseTable(namespace, subobjectBaseName(componentStructure));
	}

	/**
	 * Original base identity for a subobject component. Derived-specific views
	 * keep {@code Base_at_0xN} as a unique datatype name; the description names
	 * the canonical base that {@code vftable{for Base}} refers to.
	 */
	static String subobjectBaseName(Structure structure) {
		String parsed = parseSubobjectBase(structure.getDescription());
		return parsed != null ? parsed : structure.getName();
	}

	/** Parse {@code wiz8.subobject-view derived=... base=... offset=... [vtable=...]}; else null. */
	static String parseSubobjectField(String description, String key) {
		if (description == null) {
			return null;
		}
		String text = description.trim();
		final String prefix = "wiz8.subobject-view ";
		if (!text.startsWith(prefix)) {
			return null;
		}
		for (String part : text.substring(prefix.length()).split("\\s+")) {
			if (part.startsWith(key)) {
				return part.substring(key.length());
			}
		}
		return null;
	}

	static String parseSubobjectBase(String description) {
		String base = parseSubobjectField(description, "base=");
		if (base == null) {
			return null;
		}
		int slash = base.lastIndexOf("::");
		return slash >= 0 ? base.substring(slash + 2) : base;
	}

	private Address parseSubobjectVtable(String description) {
		String value = parseSubobjectField(description, "vtable=");
		if (value == null || value.isEmpty()) {
			return null;
		}
		try {
			if (value.startsWith("0x") || value.startsWith("0X")) {
				return program.getAddressFactory().getDefaultAddressSpace()
					.getAddress(Long.parseLong(value.substring(2), 16));
			}
			return program.getAddressFactory().getAddress(value);
		}
		catch (Exception ignored) {
			return null;
		}
	}

	/** The unique {@code vftable{for_'typeName'}} table, else null. */
	private Symbol forClauseTable(Namespace namespace, String typeName) {
		String wanted = squeeze("vftable{for" + TypeNames.map(typeName) + "}");
		Symbol match = null;
		for (Symbol symbol : vftables(namespace)) {
			if (squeeze(SpecialNames.normalize(symbol.getName())).equals(wanted)) {
				if (match != null) {
					return null; // ambiguous
				}
				match = symbol;
			}
		}
		return match;
	}

	/**
	 * The function stored at byte offset {@code slotOffset} of the table,
	 * read from program memory; null when the slot lies past the table's
	 * bound or holds no function entry.
	 */
	Function slotFunction(Symbol table, long slotOffset) {
		if (slotOffset < 0 || !slotInsideTable(table.getAddress(), slotOffset)) {
			return null;
		}
		try {
			long stored = Integer.toUnsignedLong(
				program.getMemory().getInt(table.getAddress().add(slotOffset)));
			return program.getFunctionManager().getFunctionAt(
				program.getAddressFactory().getDefaultAddressSpace().getAddress(stored));
		}
		catch (Exception e) {
			return null;
		}
	}

	/**
	 * Resolve one virtual slot. A real function body wins. When the table stores
	 * the shared {@code _purecall} helper, the compiler-backed decorated
	 * declaration projected into the reviewed Ghidra property map supplies the
	 * source method identity and return type instead. The Microsoft demangler is
	 * the type parser; displayed token text is never consulted.
	 */
	Slot slot(Symbol table, long slotOffset) {
		Function concrete = slotFunction(table, slotOffset);
		FunctionDefinition contract = typedSlotContract(table, slotOffset);
		if (contract != null) {
			boolean pure = concrete != null &&
				SpecialNames.normalize(concrete.getName()).equals("purecall");
			Function implementation = (concrete != null && !pure) ? concrete : null;
			String name = contract.getName();
			if (name == null || name.isEmpty()) {
				name = implementation != null ? implementation.getName() : null;
			}
			DataType returnType = contract.getReturnType();
			if (name != null) {
				return new Slot(name, returnType, implementation, contract);
			}
		}
		if (concrete != null && !SpecialNames.normalize(concrete.getName()).equals("purecall")) {
			return new Slot(concrete.getName(), concrete.getReturnType(), concrete);
		}
		var properties = program.getUsrPropertyManager()
			.getStringPropertyMap(VIRTUAL_SLOT_PROPERTY);
		if (properties == null) {
			return concrete == null ? null
				: new Slot(concrete.getName(), concrete.getReturnType(), concrete);
		}
		Address slotAddress = table.getAddress().add(slotOffset);
		String decorated = properties.getString(slotAddress);
		if (decorated == null) {
			return concrete == null ? null
				: new Slot(concrete.getName(), concrete.getReturnType(), concrete);
		}
		try {
			MicrosoftDemangler demangler = new MicrosoftDemangler();
			var options = demangler.createDefaultOptions();
			var context = demangler.createMangledContext(decorated, options, program,
				slotAddress);
			DemangledObject object = demangler.demangle(context);
			if (!(object instanceof DemangledFunction method) ||
				method.getReturnType() == null) {
				return null;
			}
			DataType result = method.getReturnType().getDataType(program.getDataTypeManager());
			return result == null ? null : new Slot(method.getName(), result, null);
		}
		catch (Exception ignored) {
			return null;
		}
	}

	/**
	 * The table's slot targets in order, ending at the table's bound or the
	 * first slot that stores no function entry.
	 */
	List<Function> slots(Symbol table) {
		List<Function> functions = new ArrayList<>();
		for (long offset = 0;; offset += 4) {
			Function target = slotFunction(table, offset);
			if (target == null) {
				return functions;
			}
			functions.add(target);
		}
	}

	/**
	 * Prefer the typed vftable Structure's established extent. Neighboring
	 * symbols remain a fallback when the table is still untyped.
	 */
	private boolean slotInsideTable(Address tableStart, long slotOffset) {
		Address slotEnd = tableStart.add(slotOffset + 4);
		int typedLength = typedTableLength(tableStart);
		if (typedLength > 0) {
			return slotOffset >= 0 && slotOffset + 4 <= typedLength;
		}
		Address bound = tableBounds.get(tableStart);
		if (bound != null) {
			return slotEnd.compareTo(bound) <= 0;
		}
		if (unboundedTables.containsKey(tableStart)) {
			return true;
		}
		for (Symbol symbol : (Iterable<Symbol>) () -> program.getSymbolTable()
				.getSymbolIterator(tableStart.add(1), true)) {
			if (boundsTable(symbol)) {
				tableBounds.put(tableStart, symbol.getAddress());
				return slotEnd.compareTo(symbol.getAddress()) <= 0;
			}
		}
		unboundedTables.put(tableStart, Boolean.TRUE);
		return true;
	}

	private int typedTableLength(Address tableStart) {
		Data data = program.getListing().getDataAt(tableStart);
		if (data == null) {
			return -1;
		}
		DataType type = resolve(data.getDataType());
		if (type instanceof Structure structure && structure.getLength() > 0) {
			return structure.getLength();
		}
		return -1;
	}

	private FunctionDefinition typedSlotContract(Symbol table, long slotOffset) {
		if (slotOffset < 0 || slotOffset > Integer.MAX_VALUE) {
			return null;
		}
		Data data = program.getListing().getDataAt(table.getAddress());
		if (data == null) {
			return null;
		}
		DataType type = resolve(data.getDataType());
		if (!(type instanceof Structure structure)) {
			return null;
		}
		DataTypeComponent component = structure.getComponentAt((int) slotOffset);
		if (component == null || component.getOffset() != slotOffset) {
			return null;
		}
		DataType slotType = resolve(component.getDataType());
		if (slotType instanceof Pointer pointer) {
			slotType = resolve(pointer.getDataType());
		}
		return slotType instanceof FunctionDefinition definition ? definition : null;
	}

	private static boolean boundsTable(Symbol symbol) {
		SourceType source = symbol.getSource();
		return source != SourceType.DEFAULT && source != SourceType.ANALYSIS ||
			isVftableName(symbol.getName());
	}

	// ------------------------------------------------------------------
	// Base-subobject discovery
	// ------------------------------------------------------------------

	/**
	 * The structure's leading base-subobject components, in offset order.
	 * The run ends at the first component that is neither base-named by the
	 * repository convention nor evidenced by a for-clause vftable naming
	 * its type.
	 */
	List<DataTypeComponent> baseComponents(Structure structure) {
		return basesByStructure.computeIfAbsent(structure.getPathName(), key -> {
			List<DataTypeComponent> bases = new ArrayList<>();
			for (DataTypeComponent component : structure.getDefinedComponents()) {
				String field = component.getFieldName();
				boolean named = field != null && isBaseFieldName(field);
				if (named || forClauseEvidence(structure, component)) {
					bases.add(component);
				}
				else {
					break; // bases are the leading components only
				}
			}
			return bases;
		});
	}

	/** Whether a for-clause vftable of the owning class names this component's type. */
	private boolean forClauseEvidence(Structure structure, DataTypeComponent component) {
		DataType componentType = resolve(component.getDataType());
		if (!(componentType instanceof Structure componentStructure)) {
			return false;
		}
		Namespace namespace = classNamespace(structure);
		return namespace != null &&
			forClauseTable(namespace, subobjectBaseName(componentStructure)) != null;
	}

	/** Whether a component starting at exactly {@code offset} is a base subobject. */
	boolean isBaseOffset(Structure structure, long offset) {
		for (DataTypeComponent component : baseComponents(structure)) {
			if (component.getOffset() == offset) {
				return true;
			}
		}
		return false;
	}

	private static DataType resolve(DataType type) {
		DataType current = type;
		while (current instanceof TypeDef typedef) {
			current = typedef.getBaseDataType();
		}
		return current;
	}

	private static String squeeze(String text) {
		return text.replace(" ", "");
	}
}
