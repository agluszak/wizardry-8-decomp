package wiz8.exporter;

import java.util.ArrayList;
import java.util.List;

import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Parameter;

/** Source-level callable identity and complete prototype rendering. */
final class CallableIdentity {

	private CallableIdentity() {
	}

	static String classLeaf(String className) {
		int bracket = className.indexOf('[');
		return bracket < 0 ? className : className.substring(0, bracket);
	}

	static String sourceName(Function function, FunctionKind kind) {
		if (function.getParentNamespace() instanceof GhidraClass owner) {
			String leaf = classLeaf(owner.getName());
			if (kind == FunctionKind.CONSTRUCTOR) {
				return leaf;
			}
			if (kind == FunctionKind.DESTRUCTOR) {
				return "~" + leaf;
			}
		}
		return TypeNames.map(function.getName());
	}

	static String qualifiedName(Function function, FunctionKind kind) {
		String name = sourceName(function, kind);
		if (function.getParentNamespace() instanceof GhidraClass owner) {
			return TypeNames.map(owner.getName(true)) + "::" + name;
		}
		return TypeNames.map(function.getName(true));
	}

	static String prototype(Function function, FunctionKind kind) {
		List<String> parameters = new ArrayList<>();
		for (Parameter parameter : function.getParameters()) {
			if (!parameter.isAutoParameter()) {
				parameters.add(CxxTypePrinter.printParameter(parameter));
			}
		}
		if (function.hasVarArgs()) {
			parameters.add("...");
		}
		String head = qualifiedName(function, kind) + "(" + String.join(", ", parameters) + ")";
		if (kind == FunctionKind.CONSTRUCTOR || kind == FunctionKind.DESTRUCTOR) {
			return head;
		}
		String convention = function.getCallingConventionName();
		String renderedConvention = convention != null && convention.startsWith("__") &&
			!"__cdecl".equals(convention) && !"__thiscall".equals(convention)
				? convention + " " : "";
		return CxxTypePrinter.printType(function.getReturnType()) + " " +
			renderedConvention + head;
	}
}
