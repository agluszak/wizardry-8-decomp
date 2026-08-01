package wiz8.exporter;

import ghidra.program.model.data.Array;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.FunctionDefinition;
import ghidra.program.model.data.ParameterDefinition;
import ghidra.program.model.data.Pointer;
import ghidra.program.model.data.TypeDef;
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

	private CxxTypePrinter() {
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
		return printDeclaration(parameter.getFormalDataType(), name == null ? "" : name);
	}

	private static String declare(DataType type, String inner) {
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
