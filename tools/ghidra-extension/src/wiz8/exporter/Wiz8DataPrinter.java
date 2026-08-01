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
 * strings, pointers to named symbols or null, and bounded aggregate tables
 * whose leaves all have those supported scalar shapes. A datum in an
 * uninitialized block prints without
 * an initializer, matching the recovered sources' spelling of {@code .bss}
 * state. Anything richer declines to a marker plus a comment naming what
 * stopped it.
 */
final class Wiz8DataPrinter {

	private static final int MAX_ARRAY_VALUES = 256;
	private static final int MAX_COMPOSITE_DEPTH = 8;
	private static final String CXX_QUALIFIED_IDENTIFIER =
		"[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*";

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
		String name = symbol != null ? TypeNames.map(symbol.getName(true))
				: String.format("DAT_%08x", address.getOffset());
		if (!name.matches(CXX_QUALIFIED_IDENTIFIER)) {
			return marker + "// data name is not a C++ identifier: " + name + "\n";
		}
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
			return renderComposite(data, 0, new int[] { MAX_ARRAY_VALUES });
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
			return renderFloating((Number) value);
		}
		if (type instanceof Pointer || value instanceof Address) {
			return renderPointer(value);
		}
		return null;
	}

	/** A VC6-accepted, bit-preserving finite floating initializer. */
	static String renderFloating(Number value) {
		if (value instanceof Float number) {
			if (!Float.isFinite(number)) {
				return null;
			}
			int bits = Float.floatToRawIntBits(number);
			if (bits == 0) {
				return "";
			}
			if (bits == 0x80000000) {
				return "-0.0f";
			}
			return Float.toString(number) + "f";
		}
		if (value instanceof Double number) {
			if (!Double.isFinite(number)) {
				return null;
			}
			long bits = Double.doubleToRawLongBits(number);
			if (bits == 0) {
				return "";
			}
			if (bits == 0x8000000000000000L) {
				return "-0.0";
			}
			return Double.toString(number);
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
		String name = TypeNames.map(symbol.getName(true));
		return name.matches(CXX_QUALIFIED_IDENTIFIER) ? "&" + name : null;
	}

	/** A bounded brace initializer whose leaves are all proven scalar values. */
	private String renderComposite(Data data, int depth, int[] remaining) {
		if (depth >= MAX_COMPOSITE_DEPTH) {
			return null;
		}
		int count = data.getNumComponents();
		if (count == 0 || count > remaining[0]) {
			return null;
		}
		remaining[0] -= count;
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
			boolean composite = componentType instanceof Array || componentType instanceof Structure;
			String value = composite
					? renderComposite(component, depth + 1, remaining)
					: renderValue(component);
			if (value == null) {
				return null;
			}
			if (value.isEmpty()) {
				value = composite ? "{ 0 }" : "0";
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
