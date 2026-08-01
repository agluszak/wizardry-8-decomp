package wiz8.exporter;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import ghidra.program.model.address.Address;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.data.Structure;
import ghidra.program.model.data.TypeDef;
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
 * component type the for-clause names. Vftables carry no length record, so
 * a table is bounded by the next curated symbol (user-defined or imported)
 * or the next vftable symbol; dynamic analysis labels inside a table do not
 * end it. Slot contents are read from program memory.
 *
 * A leading structure component is a base subobject on either of two
 * grounds: the repository's {@code base}/{@code base_*} field convention
 * (the reviewed naming), or the positive evidence of a for-clause vftable
 * naming the component's type — a table Ghidra only has because the class
 * really inherits that subobject. Composition at offset 0 without either
 * ground stays a member; absence of evidence never promotes it.
 */
final class VtableResolver {

	private final Program program;
	private final Map<String, List<Symbol>> tablesByNamespace = new HashMap<>();
	private final Map<String, List<DataTypeComponent>> basesByStructure = new HashMap<>();

	VtableResolver(Program program) {
		this.program = program;
	}

	/** Whether the symbol name is a vftable spelling (any for-clause). */
	static boolean isVftableName(String symbolName) {
		return FunctionKind.normalizeSpecialName(symbolName).startsWith("vftable");
	}

	/** The repository's base-subobject field naming convention. */
	static boolean isBaseFieldName(String fieldName) {
		return fieldName.equals("base") || fieldName.startsWith("base_");
	}

	/** The class namespace with this name, else null. */
	GhidraClass classNamespace(String className) {
		for (Symbol symbol : program.getSymbolTable().getSymbols(className)) {
			if (symbol.getSymbolType() == SymbolType.CLASS &&
				symbol.getObject() instanceof GhidraClass ghidraClass) {
				return ghidraClass;
			}
		}
		return null;
	}

	/** Every vftable symbol the class namespace owns. */
	List<Symbol> vftables(Namespace namespace) {
		return tablesByNamespace.computeIfAbsent(namespace.getName(true), key -> {
			List<Symbol> tables = new ArrayList<>();
			for (Symbol symbol : program.getSymbolTable().getSymbols(namespace)) {
				if (isVftableName(symbol.getName())) {
					tables.add(symbol);
				}
			}
			return tables;
		});
	}

	/** The unique for-clause-free (complete-object) table, else null. */
	Symbol primaryVftable(Namespace namespace) {
		Symbol primary = null;
		for (Symbol symbol : vftables(namespace)) {
			if (FunctionKind.normalizeSpecialName(symbol.getName()).equals("vftable")) {
				if (primary != null) {
					return null; // ambiguous
				}
				primary = symbol;
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
		Namespace namespace = classNamespace(receiverClass.getName());
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
		return forClauseTable(namespace, componentType.getName());
	}

	/** The unique {@code vftable{for_'typeName'}} table, else null. */
	private Symbol forClauseTable(Namespace namespace, String typeName) {
		String wanted = squeeze("vftable{for" + TypeNames.map(typeName) + "}");
		Symbol match = null;
		for (Symbol symbol : vftables(namespace)) {
			if (squeeze(FunctionKind.normalizeSpecialName(symbol.getName())).equals(wanted)) {
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
	 * Vftables have no length record; the table ends at the next curated
	 * symbol (user-defined or imported) or the next vftable symbol. A
	 * dynamic analysis label inside the table is an artifact of an inbound
	 * reference, not a boundary. A slot read past the bound would silently
	 * name a neighbour's entry.
	 */
	private boolean slotInsideTable(Address tableStart, long slotOffset) {
		Address slotEnd = tableStart.add(slotOffset + 4);
		for (Symbol symbol : (Iterable<Symbol>) () -> program.getSymbolTable()
				.getSymbolIterator(tableStart.add(1), true)) {
			if (symbol.getAddress().compareTo(slotEnd) >= 0) {
				return true;
			}
			if (boundsTable(symbol)) {
				return false;
			}
		}
		return true;
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
		if (!(componentType instanceof Structure)) {
			return false;
		}
		Namespace namespace = classNamespace(structure.getName());
		return namespace != null &&
			forClauseTable(namespace, componentType.getName()) != null;
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
