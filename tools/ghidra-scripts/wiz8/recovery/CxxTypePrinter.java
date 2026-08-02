package wiz8.recovery;

import ghidra.program.model.data.Array;
import ghidra.program.model.data.ArrayDataType;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.data.FunctionDefinition;
import ghidra.program.model.data.FunctionDefinitionDataType;
import ghidra.program.model.data.BitFieldDataType;
import ghidra.program.model.data.IntegerDataType;
import ghidra.program.model.data.InvalidDataTypeException;
import ghidra.program.model.data.ParameterDefinition;
import ghidra.program.model.data.ParameterDefinitionImpl;
import ghidra.program.model.data.Pointer;
import ghidra.program.model.data.PointerDataType;
import ghidra.program.model.data.StructureDataType;
import ghidra.program.model.data.TypeDef;
import ghidra.program.model.data.TypedefDataType;
import ghidra.program.model.data.Undefined;
import ghidra.program.model.data.Undefined4DataType;
import ghidra.program.model.data.VoidDataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Parameter;

/**
 * One recursive C++ declarator printer over Ghidra {@link DataType} objects,
 * shared by every place the exporter spells a type: class fields, virtual
 * signatures, data definitions, and parameters.
 *
 * The declarator grammar is honored rather than string-edited: array bounds
 * bind tighter than pointers, so a pointer-to-array parenthesizes
 * ({@code int (*name)[4]}), and a function pointer prints its full
 * parameter list ({@code int (__stdcall *name)(char*)}). A named typedef is
 * preserved as the base spelling rather than unwrapped: the alias may be the
 * original source spelling, and the underlying type is always recoverable
 * while a discarded alias is not. Template bracket encodings and primitive
 * aliases go through {@link TypeNames}. Ghidra's type system carries no
 * const/volatile or reference types, so those spellings can only come from
 * source-owned declarations, not from here.
 */
final class CxxTypePrinter {
	static final class UnresolvedTypeException extends IllegalArgumentException {
		UnresolvedTypeException(String context, DataType type) {
			super(context + " has unresolved ABI type " +
				(type == null ? "<null>" : type.getPathName()));
		}
	}

	private CxxTypePrinter() {
	}

	/**
	 * Executable regression cases for declarator binding. They run when the
	 * exporter class loads, so every supported live export lane verifies the
	 * pointer/array/function/typedef cases that rendered-star parsing cannot
	 * represent safely.
	 */
	static void verifyRegressionCases() {
		DataType integer = IntegerDataType.dataType;
		DataType object = new StructureDataType("W8Object", 1);
		DataType objectPointer = new PointerDataType(object, 4);
		FunctionDefinitionDataType callback = new FunctionDefinitionDataType("callback");
		callback.setReturnType(VoidDataType.dataType);
		callback.setArguments(new ParameterDefinitionImpl("object", objectPointer, null));
		StructureDataType bitFieldHolder = new StructureDataType("BitFieldHolder", 0);
		DataType bitField;
		try {
			bitField = bitFieldHolder.addBitField(integer, 3, "flags", null).getDataType();
		}
		catch (InvalidDataTypeException error) {
			throw new IllegalStateException("cannot construct declarator bit-field fixture", error);
		}
		DataType[] cases = {
			new PointerDataType(new PointerDataType(integer, 4), 4),
			new ArrayDataType(new PointerDataType(integer, 4), 4, 4),
			new PointerDataType(new ArrayDataType(integer, 4, 4), 4),
			new ArrayDataType(new PointerDataType(callback, 4), 8, 4),
			new TypedefDataType("SomeTypedef", integer),
			bitField,
		};
		String[] names = {"value", "values", "values", "callbacks", "value", "flags"};
		String[] expected = {
			"int** value",
			"int* values[4]",
			"int (*values)[4]",
			"void (*callbacks[8])(W8Object*)",
			"SomeTypedef value",
			"int flags",
		};
		for (int i = 0; i < cases.length; i++) {
			String actual = printDeclaration(cases[i], names[i]);
			if (!expected[i].equals(actual)) {
				throw new IllegalStateException("declarator regression: expected " +
					expected[i] + ", got " + actual);
			}
		}
		String storage = printLocal(Undefined4DataType.dataType, "value");
		if (!"/* unresolved undefined4 */ unsigned char value[4]".equals(storage)) {
			throw new IllegalStateException("undefined storage regression: " + storage);
		}
		try {
			requireResolved(Undefined4DataType.dataType, "parameter value");
			throw new IllegalStateException("undefined parameter did not decline");
		}
		catch (UnresolvedTypeException expectedFailure) {
			// An array spelling would decay and silently change the parameter ABI.
		}
	}

	/** The abstract type spelling, without a declared name. */
	static String printType(DataType type) {
		return printDeclaration(type, "").trim();
	}

	/** The full declaration of {@code name} with the given type. */
	static String printDeclaration(DataType type, String name) {
		return declare(type, name);
	}

	/**
	 * One parameter's declaration. The formal type is the source-level
	 * spelling; {@code getDataType()} may be the effective ABI type after
	 * an indirect-passing adjustment.
	 */
	static String printParameter(Parameter parameter) {
		String name = parameter.getName();
		DataType type = parameter.getFormalDataType();
		requireResolved(type, "parameter " + (name == null ? "" : name));
		return printDeclaration(type, name == null ? "" : name);
	}

	static String printReturn(Function function) {
		DataType type = function.getReturn().getFormalDataType();
		requireResolved(type, "return of " + function.getName(true));
		return printType(type);
	}

	static String printField(DataTypeComponent component, String name) {
		DataType type = component.getDataType();
		if (type instanceof BitFieldDataType bitField) {
			return printDeclaration(bitField.getBaseDataType(), name) + " : " +
				bitField.getDeclaredBitSize();
		}
		return printDeclaration(type, name);
	}

	static String printLocal(DataType type, String name) {
		return printDeclaration(type, name);
	}

	static String printPointee(DataType type) {
		requireResolved(type, "pointer pointee");
		return printType(type);
	}

	private static void requireResolved(DataType type, String context) {
		if (hasUnresolvedAbiType(type)) {
			throw new UnresolvedTypeException(context, type);
		}
	}

	private static boolean hasUnresolvedAbiType(DataType type) {
		if (type == null || Undefined.isUndefined(type) || "float10".equals(type.getName())) {
			return true;
		}
		if (type instanceof TypeDef) {
			return false; // the named source spelling is itself usable evidence
		}
		if (type instanceof Pointer pointer) {
			return hasUnresolvedAbiType(pointer.getDataType());
		}
		if (type instanceof Array array) {
			return hasUnresolvedAbiType(array.getDataType());
		}
		if (type instanceof FunctionDefinition function) {
			if (hasUnresolvedAbiType(function.getReturnType())) return true;
			for (ParameterDefinition parameter : function.getArguments()) {
				if (hasUnresolvedAbiType(parameter.getDataType())) return true;
			}
		}
		return false;
	}

	private static String declare(DataType type, String inner) {
		if (type != null && (Undefined.isUndefined(type) || "float10".equals(type.getName()))) {
			String unresolved = "/* unresolved " + type.getName() + " */ unsigned char";
			if (!inner.isEmpty() && type.getLength() > 1 && isPlainName(inner)) {
				return unresolved + " " + inner + "[" + type.getLength() + "]";
			}
			return attach(unresolved, inner);
		}
		if (type instanceof BitFieldDataType bitField) {
			return declare(bitField.getBaseDataType(), inner);
		}
		if (type instanceof TypeDef typedef) {
			// The alias is a simple identifier, so no pointer/array binding
			// below it ever needs parentheses.
			return attach(TypeNames.map(typedef.getDisplayName()), inner);
		}
		if (type instanceof Pointer pointer) {
			DataType pointed = pointer.getDataType();
			String star = "*" + inner;
			if (pointed instanceof Array || pointed instanceof FunctionDefinition) {
				// Arrays and function types bind tighter than the pointer.
				return declare(pointed, "(" + star + ")");
			}
			return declare(pointed, star);
		}
		if (type instanceof Array array) {
			return declare(array.getDataType(),
				inner + "[" + array.getNumElements() + "]");
		}
		if (type instanceof FunctionDefinition function) {
			StringBuilder parameters = new StringBuilder();
			for (ParameterDefinition parameter : function.getArguments()) {
				if (parameters.length() > 0) {
					parameters.append(", ");
				}
				parameters.append(printType(parameter.getDataType()));
			}
			if (function.hasVarArgs()) {
				parameters.append(parameters.length() > 0 ? ", ..." : "...");
			}
			String convention = function.getCallingConventionName();
			String head = inner;
			if (convention != null && convention.startsWith("__") && head.startsWith("(")) {
				head = "(" + convention + " " + head.substring(1);
			}
			return declare(function.getReturnType(),
				head + "(" + parameters + ")");
		}
		return attach(TypeNames.map(type == null ? "void" : type.getDisplayName()), inner);
	}

	private static boolean isPlainName(String inner) {
		for (int i = 0; i < inner.length(); i++) {
			char c = inner.charAt(i);
			if (!Character.isJavaIdentifierPart(c)) {
				return false;
			}
		}
		return true;
	}

	private static String attach(String base, String inner) {
		if (inner.isEmpty()) {
			return base;
		}
		// Pointer stars attach to the type in the repository's spelling
		// (Type* name); everything else separates with one space.
		if (inner.startsWith("*")) {
			int stars = 0;
			while (stars < inner.length() && inner.charAt(stars) == '*') {
				stars++;
			}
			String rest = inner.substring(stars);
			return base + "*".repeat(stars) + (rest.isEmpty() ? "" : " " + rest);
		}
		return base + " " + inner;
	}
}
