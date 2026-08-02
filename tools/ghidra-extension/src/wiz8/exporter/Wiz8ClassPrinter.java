package wiz8.exporter;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

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
	 * order read from every primary/secondary vftable in program memory, and data fields at
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
	private final RecoverySession session;

	Wiz8ClassPrinter(RecoverySession session, GhidraClass ghidraClass) {
		this.session = session;
		this.program = session.program;
		this.ghidraClass = ghidraClass;
		this.structure = findStructure();
		this.vtables = session.vtables;
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
		Set<SourceEntityKey> emitted = new HashSet<>();
		for (Symbol table : vtables.vftables(ghidraClass)) {
			out.append(String.format("    // vftable 0x%08x %s\n",
				table.getAddress().getOffset(), table.getName()));
			List<Function> targets = vtables.slots(table);
			for (int i = 0; i < targets.size(); i++) {
				appendSlot(out, className, i * 4L, vtables.slot(table, i * 4L), emitted);
			}
			out.append('\n');
		}
	}

	private void appendSlot(StringBuilder out, String className, long slot,
			VtableResolver.Slot sourceSlot, Set<SourceEntityKey> emitted) {
		if (sourceSlot == null) {
			return;
		}
		Function target = sourceSlot.function;
		if (target == null) {
			out.append(String.format(
				"    // slot 0x%02x: pure virtual %s; parameter spelling unresolved.\n",
				slot, sourceSlot.name));
			return;
		}
		String comment = String.format(" /* slot 0x%02x, 0x%08x */", slot,
			target.getEntryPoint().getOffset());
		Emission emission = session.emission(target);
		SourceEntityKey sourceEntity = session.sourceKey(target);
		Function resolved = emission.canonicalTarget() != null
			? emission.canonicalTarget() : target;
		if ("purecall".equals(SpecialNames.normalize(resolved.getName()))) {
			out.append(String.format(
				"    // slot 0x%02x: pure virtual; the vftable stores purecall and\n" +
				"    // cannot name the original declaration.\n", slot));
			return;
		}
		if (!ghidraClass.equals(resolved.getParentNamespace())) {
			String relation = resolved.getParentNamespace() instanceof GhidraClass
					? "inherited" : "unnamed slot function";
			out.append("    //").append(comment).append(' ')
					.append(relation).append(' ').append(resolved.getName(true)).append('\n');
			return;
		}
		if (!emitted.add(sourceEntity)) {
			out.append("    //").append(comment).append(" duplicate/adjustor emission of ")
				.append(sourceEntity.formalSignature()).append('\n');
			return;
		}
		if (emission.isDeletingWrapper()) {
			out.append("    virtual ~").append(bareName(className)).append("();")
					.append(String.format(" /* slot 0x%02x: %s 0x%08x */%n", slot,
						SpecialNames.normalize(target.getName()),
						target.getEntryPoint().getOffset()));
			return;
		}
		if (sourceEntity.kind() == SourceKind.DESTRUCTOR) {
			out.append("    virtual ~").append(bareName(className)).append("();")
					.append(comment).append('\n');
			return;
		}
		try {
			out.append("    virtual ").append(methodSignature(resolved)).append(';')
					.append(comment).append('\n');
		}
		catch (CxxTypePrinter.UnresolvedTypeException unresolved) {
			out.append("    //").append(comment).append(" signature declined: ")
				.append(unresolved.getMessage()).append('\n');
		}
	}

	private String methodSignature(Function target) {
		StringBuilder signature = new StringBuilder();
		signature.append(CxxTypePrinter.printReturn(target)).append(' ')
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
			out.append("    ").append(CxxTypePrinter.printField(component, field))
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

	private String fieldTypeName(DataType type) {
		return CxxTypePrinter.printType(type);
	}
}
