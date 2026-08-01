package wiz8.exporter;

import ghidra.program.model.address.Address;
import ghidra.program.model.data.Array;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.Pointer;
import ghidra.program.model.data.Structure;
import ghidra.program.model.data.TypeDef;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Program;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.Symbol;

/**
 * Prints one global datum as a recovered-style C definition with its
 * {@code // GLOBAL:} marker.
 *
 * The value comes from program memory through the applied data type. Only
 * shapes the image proves are lifted: integer and floating scalars, narrow
 * strings, pointers to named symbols or null, arrays of scalars, and flat
 * all-scalar structures. A datum in an uninitialized block prints without
 * an initializer, matching the recovered sources' spelling of {@code .bss}
 * state. Anything richer declines to a marker plus a comment naming what
 * stopped it.
 */
final class Wiz8DataPrinter {

	private static final int MAX_ARRAY_VALUES = 256;

	private final Program program;

	Wiz8DataPrinter(Program program) {
		this.program = program;
	}

	String print(Address address) {
		String marker = String.format("// GLOBAL: WIZ8 0x%08x%n", address.getOffset());
		Data data = program.getListing().getDataAt(address);
		if (data == null) {
			return marker + "// no defined data at the address; apply a type first\n";
		}
		if (data.getValue() instanceof String text &&
			!(resolve(data.getDataType()) instanceof Array)) {
			// A defined string datum is an anonymous literal in source; its
			// synthetic Ghidra name is not an identifier and must not print
			// as one.
			return marker + "// string literal: " + Msvc6Patterns.cStringLiteral(text) +
				"\n";
		}
		Symbol symbol = program.getSymbolTable().getPrimarySymbol(address);
		String name = symbol != null ? symbol.getName()
				: String.format("DAT_%08x", address.getOffset());
		String declaration = declarator(data.getDataType(), name);
		MemoryBlock block = program.getMemory().getBlock(address);
		if (block == null || !block.isInitialized()) {
			return marker + declaration + ";\n";
		}
		String value = renderValue(data);
		if (value == null) {
			return marker + "// " + declaration +
				"; /* initializer not lifted: unsupported shape */\n";
		}
		if (value.isEmpty()) {
			return marker + declaration + ";\n";
		}
		return marker + declaration + " = " + value + ";\n";
	}

	/** The C declarator with array bounds unwrapped onto the name. */
	private String declarator(DataType type, String name) {
		StringBuilder suffix = new StringBuilder();
		DataType current = resolve(type);
		while (current instanceof Array array) {
			suffix.append('[').append(array.getNumElements()).append(']');
			current = resolve(array.getDataType());
		}
		String display = current.getDisplayName().trim();
		StringBuilder stars = new StringBuilder();
		while (display.endsWith("*")) {
			stars.append('*');
			display = display.substring(0, display.length() - 1).trim();
		}
		return TypeNames.map(display) + stars + " " + name + suffix;
	}

	private static DataType resolve(DataType type) {
		DataType current = type;
		while (current instanceof TypeDef typedef) {
			current = typedef.getBaseDataType();
		}
		return current;
	}

	/**
	 * The initializer text for a datum: empty when the stored value is all
	 * zero (the recovered sources leave zero statics uninitialized), null
	 * when the shape is not provably liftable.
	 */
	private String renderValue(Data data) {
		DataType type = resolve(data.getDataType());
		if (type instanceof Array || type instanceof Structure) {
			return renderComposite(data);
		}
		Object value = data.getValue();
		if (value instanceof String text) {
			return Msvc6Patterns.cStringLiteral(text);
		}
		if (value instanceof Scalar scalar) {
			long signed = scalar.getSignedValue();
			if (signed == 0) {
				return "";
			}
			return signed > -0x10000 && signed < 0x10000 ? Long.toString(signed)
					: String.format("0x%x", scalar.getUnsignedValue());
		}
		if (value instanceof Float || value instanceof Double) {
			double number = ((Number) value).doubleValue();
			if (number == 0.0) {
				return "";
			}
			String text = String.valueOf(value);
			return value instanceof Float ? text + "f" : text;
		}
		if (type instanceof Pointer || value instanceof Address) {
			return renderPointer(value);
		}
		return null;
	}

	private String renderPointer(Object value) {
		if (!(value instanceof Address target)) {
			return null;
		}
		if (target.getOffset() == 0) {
			return "";
		}
		Symbol symbol = program.getSymbolTable().getPrimarySymbol(target);
		if (symbol == null) {
			return null;
		}
		Data pointed = program.getListing().getDataAt(target);
		if (pointed != null && pointed.getValue() instanceof String text) {
			return Msvc6Patterns.cStringLiteral(text);
		}
		return "&" + symbol.getName();
	}

	/** A brace initializer for an array or flat structure of lifted values. */
	private String renderComposite(Data data) {
		int count = data.getNumComponents();
		if (count == 0 || count > MAX_ARRAY_VALUES) {
			return null;
		}
		// A char array holding a defined string lifts as the literal itself.
		if (data.getValue() instanceof String text) {
			return Msvc6Patterns.cStringLiteral(text);
		}
		StringBuilder values = new StringBuilder("{ ");
		boolean allZero = true;
		for (int i = 0; i < count; i++) {
			Data component = data.getComponent(i);
			if (component == null) {
				return null;
			}
			DataType componentType = resolve(component.getDataType());
			if (componentType instanceof Array || componentType instanceof Structure) {
				return null; // nested composites stay unproven in v1
			}
			String value = renderValue(component);
			if (value == null) {
				return null;
			}
			if (value.isEmpty()) {
				value = "0";
			}
			else {
				allZero = false;
			}
			if (i > 0) {
				values.append(", ");
			}
			values.append(value);
		}
		if (allZero) {
			return "";
		}
		return values.append(" }").toString();
	}
}
