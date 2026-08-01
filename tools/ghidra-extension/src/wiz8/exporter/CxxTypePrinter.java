package wiz8.exporter;

import ghidra.program.model.data.Array;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.FunctionDefinition;
import ghidra.program.model.data.ParameterDefinition;
import ghidra.program.model.data.Pointer;
import ghidra.program.model.data.TypeDef;

/**
 * One recursive C++ declarator printer over Ghidra {@link DataType} objects,
 * shared by every place the exporter spells a type: class fields, virtual
 * signatures, data definitions, and parameters.
 *
 * The declarator grammar is honored rather than string-edited: array bounds
 * bind tighter than pointers, so a pointer-to-array parenthesizes
 * ({@code int (*name)[4]}), and a function pointer prints its full
 * parameter list ({@code int (__stdcall *name)(char*)}). Template bracket
 * encodings and primitive aliases go through {@link TypeNames}. Ghidra's
 * type system carries no const/volatile or reference types, so those
 * spellings can only come from source-owned declarations, not from here.
 */
final class CxxTypePrinter {

	private CxxTypePrinter() {
	}

	/** The abstract type spelling, without a declared name. */
	static String printType(DataType type) {
		return printDeclaration(type, "").trim();
	}

	/** The full declaration of {@code name} with the given type. */
	static String printDeclaration(DataType type, String name) {
		return declare(resolve(type), name);
	}

	private static String declare(DataType type, String inner) {
		if (type instanceof Pointer pointer) {
			DataType pointed = resolve(pointer.getDataType());
			String star = "*" + inner;
			if (pointed instanceof Array || pointed instanceof FunctionDefinition) {
				// Arrays and function types bind tighter than the pointer.
				return declare(pointed, "(" + star + ")");
			}
			return declare(pointed, star);
		}
		if (type instanceof Array array) {
			return declare(resolve(array.getDataType()),
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
			return declare(resolve(function.getReturnType()),
				head + "(" + parameters + ")");
		}
		String base = TypeNames.map(type == null ? "void" : type.getDisplayName());
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

	/**
	 * Look through typedefs to the base type. Ghidra typedefs in the
	 * reviewed project are import artifacts, not source spellings; the
	 * source-owned declaration is the authority for a meaningful alias.
	 */
	private static DataType resolve(DataType type) {
		DataType current = type;
		while (current instanceof TypeDef typedef) {
			current = typedef.getBaseDataType();
		}
		return current;
	}
}
