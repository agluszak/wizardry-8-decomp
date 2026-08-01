package wiz8.exporter;

import java.util.ArrayList;
import java.util.List;

import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.data.Structure;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Program;
import ghidra.program.model.symbol.Symbol;

/**
 * Prints one class declaration from live Ghidra state: base classes from the
 * shared {@link VtableResolver} discovery (the {@code base}/{@code base_*}
 * convention plus for-clause vftable evidence), virtual methods in vtable
 * order read from the primary vftable in program memory, and data fields at
 * their exact offsets with explicit gap padding.
 *
 * The output is a generated projection for review and porting; it never
 * becomes evidence on its own. Anything the project state cannot prove is
 * printed as a comment rather than invented: a purecall slot keeps its slot
 * offset but no name, an inherited slot names the base method it still
 * points at, and a class with no structure or no vftable simply omits that
 * section.
 */
final class Wiz8ClassPrinter {

	private final Program program;
	private final GhidraClass ghidraClass;
	private final Structure structure;
	private final VtableResolver vtables;

	Wiz8ClassPrinter(Program program, GhidraClass ghidraClass) {
		this.program = program;
		this.ghidraClass = ghidraClass;
		this.structure = findStructure();
		this.vtables = new VtableResolver(program);
	}

	String print() {
		String name = TypeNames.map(ghidraClass.getName());
		StringBuilder out = new StringBuilder();
		out.append("/* Generated from the reviewed Ghidra project; a projection for\n");
		out.append("   review and porting, not recovered source or evidence. */\n");
		out.append("class ").append(name);
		List<DataTypeComponent> bases = baseComponents();
		if (!bases.isEmpty()) {
			out.append(" :");
			for (int i = 0; i < bases.size(); i++) {
				out.append(i == 0 ? "\n    public " : ",\n    public ")
						.append(fieldTypeName(bases.get(i).getDataType()));
			}
		}
		out.append(" {\npublic:\n");
		appendVirtuals(out, name);
		appendFields(out, bases);
		out.append("};");
		if (structure != null) {
			out.append(String.format(" /* size 0x%x */", structure.getLength()));
		}
		out.append('\n');
		return out.toString();
	}

	private Structure findStructure() {
		List<Structure> matches = new ArrayList<>();
		var iterator = program.getDataTypeManager().getAllStructures();
		while (iterator.hasNext()) {
			Structure candidate = iterator.next();
			if (candidate.getName().equals(ghidraClass.getName())) {
				matches.add(candidate);
			}
		}
		return matches.size() == 1 ? matches.get(0) : null;
	}

	private List<DataTypeComponent> baseComponents() {
		if (structure == null) {
			return new ArrayList<>();
		}
		return vtables.baseComponents(structure);
	}

	// ------------------------------------------------------------------
	// Virtual methods
	// ------------------------------------------------------------------

	private void appendVirtuals(StringBuilder out, String className) {
		Symbol table = vtables.primaryVftable(ghidraClass);
		if (table == null) {
			return;
		}
		out.append(String.format("    // vftable 0x%08x\n", table.getAddress().getOffset()));
		List<Function> targets = vtables.slots(table);
		for (int i = 0; i < targets.size(); i++) {
			appendSlot(out, className, i * 4L, targets.get(i));
		}
		out.append('\n');
	}

	private void appendSlot(StringBuilder out, String className, long slot, Function target) {
		String comment = String.format(" /* slot 0x%02x, 0x%08x */", slot,
			target.getEntryPoint().getOffset());
		Function resolved = target.isThunk() ? target.getThunkedFunction(true) : target;
		if ("purecall".equals(FunctionKind.normalizeSpecialName(resolved.getName()))) {
			out.append(String.format(
				"    // slot 0x%02x: pure virtual; the vftable stores purecall and\n" +
				"    // cannot name the original declaration.\n", slot));
			return;
		}
		if (!ghidraClass.equals(target.getParentNamespace())) {
			String relation = target.getParentNamespace() instanceof GhidraClass
					? "inherited" : "unnamed slot function";
			out.append("    //").append(comment).append(' ')
					.append(relation).append(' ').append(target.getName(true)).append('\n');
			return;
		}
		FunctionKind kind = FunctionKind.classify(target);
		if (kind == FunctionKind.SYNTHETIC_DELETING_DESTRUCTOR) {
			out.append("    virtual ~").append(bareName(className)).append("();")
					.append(String.format(" /* slot 0x%02x: %s 0x%08x */%n", slot,
						FunctionKind.normalizeSpecialName(target.getName()),
						target.getEntryPoint().getOffset()));
			return;
		}
		if (kind == FunctionKind.DESTRUCTOR) {
			out.append("    virtual ~").append(bareName(className)).append("();")
					.append(comment).append('\n');
			return;
		}
		out.append("    virtual ").append(methodSignature(target)).append(';')
				.append(comment).append('\n');
	}

	private String methodSignature(Function target) {
		StringBuilder signature = new StringBuilder();
		signature.append(fieldTypeName(target.getReturn().getFormalDataType())).append(' ')
				.append(target.getName()).append('(');
		boolean first = true;
		for (int i = 0; i < target.getParameterCount(); i++) {
			var parameter = target.getParameter(i);
			if (parameter.isAutoParameter()) {
				continue; // hidden this / return storage: ABI, not source
			}
			if (!first) {
				signature.append(", ");
			}
			first = false;
			signature.append(CxxTypePrinter.printParameter(parameter));
		}
		return signature.append(')').toString();
	}

	private static String bareName(String className) {
		int bracket = className.indexOf('<');
		return bracket > 0 ? className.substring(0, bracket) : className;
	}

	// ------------------------------------------------------------------
	// Fields
	// ------------------------------------------------------------------

	private void appendFields(StringBuilder out, List<DataTypeComponent> bases) {
		if (structure == null) {
			out.append("    // no structure named ").append(ghidraClass.getName())
					.append(" in the program's data type manager\n");
			return;
		}
		int cursor = bases.isEmpty() ? 0
				: bases.get(bases.size() - 1).getEndOffset() + 1;
		for (DataTypeComponent component : structure.getDefinedComponents()) {
			if (component.getOffset() < cursor || bases.contains(component)) {
				continue;
			}
			if (component.getOffset() > cursor) {
				appendGap(out, cursor, component.getOffset() - cursor);
			}
			String field = component.getFieldName();
			if (field == null) {
				field = String.format("field_%x", component.getOffset());
			}
			out.append("    ").append(fieldDeclaration(component.getDataType(), field))
					.append(';').append(String.format(" /* 0x%02x */%n", component.getOffset()));
			cursor = component.getEndOffset() + 1;
		}
		if (cursor < structure.getLength()) {
			appendGap(out, cursor, structure.getLength() - cursor);
		}
	}

	private static void appendGap(StringBuilder out, int offset, int length) {
		out.append(String.format("    unsigned char unknown_%x[%d]; /* 0x%02x */%n",
			offset, length, offset));
	}

	/** A component's C declarator, through the shared declarator printer. */
	private String fieldDeclaration(DataType type, String name) {
		return CxxTypePrinter.printDeclaration(type, name);
	}

	private String fieldTypeName(DataType type) {
		return CxxTypePrinter.printType(type);
	}
}
