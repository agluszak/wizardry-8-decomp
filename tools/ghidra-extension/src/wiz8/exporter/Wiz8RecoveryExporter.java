/* ###
 * IP: GHIDRA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * The decompiler configuration and per-function decompile/fallback skeleton
 * in this file is reduced from Ghidra's
 * ghidra.app.util.exporter.CppExporter (Ghidra 12.1.2); the header, type,
 * equate, global, and parallel-decompilation machinery was removed.
 */
package wiz8.exporter;

import java.util.ArrayList;
import java.util.List;

import ghidra.app.decompiler.ClangTokenGroup;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSpace;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Program;
import ghidra.util.exception.CancelledException;
import ghidra.util.task.TaskMonitor;

/**
 * Selected-definition C++ exporter for the Wizardry 8 recovery project.
 *
 * Input is the live analyzed {@link Program} and a list of function entry
 * points; output is recovered-style C++ text with reccmp entity markers.
 * Only the selected definitions are emitted: no headers, no data-type
 * definitions, no referenced-global declarations. A function that cannot be
 * decompiled is reported inline as a comment; it never aborts the batch.
 */
public final class Wiz8RecoveryExporter {
	static {
		CxxTypePrinter.verifyRegressionCases();
		DeletingDestructorSemantics.verifyRegressionCases();
	}

	private Wiz8RecoveryExporter() {
	}

	/**
	 * Export the functions whose entry points are given, in the given order,
	 * separated by blank lines.
	 */
	public static String export(Program program, long[] entryPoints, TaskMonitor monitor)
			throws CancelledException {
		return String.join("\n", exportFunctions(program, entryPoints, monitor));
	}

	/**
	 * Export one independently bounded string per requested entry, preserving
	 * request order.  This is the Java/Python protocol; callers never need to
	 * split concatenated C++ by marker regex.
	 */
	public static String[] exportFunctions(Program program, long[] entryPoints,
			TaskMonitor monitor) throws CancelledException {
		DecompileOptions options = new DecompileOptions();
		DecompInterface decompiler = openDecompiler(program, options);
		try {
			String[] output = new String[entryPoints.length];
			AddressSpace space = program.getAddressFactory().getDefaultAddressSpace();
			FunctionManager functionManager = program.getFunctionManager();
			for (int i = 0; i < entryPoints.length; i++) {
				monitor.checkCancelled();
				long entryPoint = entryPoints[i];
				Address entry = space.getAddress(entryPoint);
				Function function = functionManager.getFunctionAt(entry);
				if (function == null) {
					output[i] = String.format(
						"// error: no function at 0x%08x%n", entryPoint);
					continue;
				}
				output[i] = exportFunction(function, decompiler, options, monitor);
			}
			return output;
		}
		finally {
			decompiler.dispose();
		}
	}

	/**
	 * Export the generated initializer suffix plus compound statement for each
	 * requested function.  Prototype/body separation is performed on Clang
	 * markup in Java, so regression tooling never parses C++ braces.
	 */
	public static String[] exportBodies(Program program, long[] entryPoints,
			TaskMonitor monitor) throws CancelledException {
		return exportBodies(program, entryPoints, new long[entryPoints.length], monitor);
	}

	public static String[] exportBodies(Program program, long[] entryPoints,
			long[] referenceMasks, TaskMonitor monitor) throws CancelledException {
		if (referenceMasks.length != entryPoints.length) {
			throw new IllegalArgumentException("reference mask count must match entries");
		}
		DecompileOptions options = new DecompileOptions();
		DecompInterface decompiler = openDecompiler(program, options);
		try {
			String[] output = new String[entryPoints.length];
			AddressSpace space = program.getAddressFactory().getDefaultAddressSpace();
			FunctionManager functions = program.getFunctionManager();
			for (int i = 0; i < entryPoints.length; i++) {
				monitor.checkCancelled();
				Function function = functions.getFunctionAt(space.getAddress(entryPoints[i]));
				output[i] = function == null ? ""
					: exportFunctionBody(function, referenceMasks[i], decompiler, options, monitor);
			}
			return output;
		}
		finally {
			decompiler.dispose();
		}
	}

	/**
	 * Export one class: its generated declaration, then its ABI family in
	 * address order. A bracket-encoded template class emits emission markers
	 * only — the generic definition lives once in its owning header, so a
	 * body under a TEMPLATE marker would mistake an emission for authored
	 * code.
	 */
	public static String exportClass(Program program, String className, TaskMonitor monitor)
			throws CancelledException {
		GhidraClass ghidraClass = findClass(program, className);
		if (ghidraClass == null) {
			return String.format("// error: no class named %s%n", className);
		}
		LifecycleFamilyResolver.LifecycleFamily family =
			new LifecycleFamilyResolver(program).resolve(ghidraClass, monitor);

		StringBuilder output = new StringBuilder();
		output.append(new Wiz8ClassPrinter(program, ghidraClass).print());
		boolean template = className.indexOf('[') >= 0;
		DecompileOptions options = new DecompileOptions();
		DecompInterface decompiler = template ? null : openDecompiler(program, options);
		try {
			for (LifecycleFamilyResolver.Member member : family.members()) {
				monitor.checkCancelled();
				Function function = member.function();
				output.append('\n');
				if (template) {
					output.append(templateEmissionBlock(function));
				}
				else {
					output.append(exportFunction(function, decompiler, options, monitor));
				}
			}
			return output.toString();
		}
		finally {
			if (decompiler != null) {
				decompiler.dispose();
			}
		}
	}

	/** Explain the complete constructor/destructor/vtable family for one class. */
	public static String explainClass(Program program, String className, TaskMonitor monitor)
			throws CancelledException {
		GhidraClass ghidraClass = findClass(program, className);
		if (ghidraClass == null) {
			return String.format("error: no class named %s%n", className);
		}
		LifecycleFamilyResolver.LifecycleFamily family =
			new LifecycleFamilyResolver(program).resolve(ghidraClass, monitor);
		StringBuilder out = new StringBuilder("class ").append(className)
			.append(" lifecycle family\n");
		for (var table : family.vftables()) {
			out.append(String.format("vftable     0x%08x %s%n",
				table.getAddress().getOffset(), table.getName()));
		}
		for (LifecycleFamilyResolver.Member member : family.members()) {
			Function function = member.function();
			FunctionRole role = member.role();
			out.append(String.format("0x%08x %-30s %-22s %s%n",
				function.getEntryPoint().getOffset(),
				role.emissionKind().name().toLowerCase(),
				role.sourceKind().name().toLowerCase(), function.getName(true)));
			out.append("             role: ").append(role.evidence())
				.append("; family: ").append(member.familyEvidence()).append('\n');
		}
		return out.toString();
	}

	/** Stable machine-facing emission name used by the Python interface. */
	public static String emissionKind(Program program, long entryPoint) {
		Address entry = program.getAddressFactory().getDefaultAddressSpace()
			.getAddress(entryPoint);
		Function function = program.getFunctionManager().getFunctionAt(entry);
		return function == null ? "missing"
			: FunctionRoleResolver.resolve(function).emissionKind().name().toLowerCase();
	}

	/**
	 * The marker-only block recording that a template member was emitted at
	 * this address: SYNTHETIC for the compiler-generated deleting destructor,
	 * TEMPLATE for every other member, each followed by the specialization
	 * symbol it records.
	 */
	private static String templateEmissionBlock(Function function) {
		FunctionKind kind = FunctionKind.classify(function);
		if (kind.isDeletingDestructor()) {
			return new Wiz8CxxPrinter(function, kind).printSynthetic();
		}
		String owner = TypeNames.map(function.getParentNamespace().getName(true));
		String name = TypeNames.map(function.getName());
		return String.format("// TEMPLATE: WIZ8 0x%08x%n// %s::%s%n",
			function.getEntryPoint().getOffset(), owner, name);
	}

	private static GhidraClass findClass(Program program, String className) {
		for (ghidra.program.model.symbol.Symbol symbol : program.getSymbolTable()
				.getSymbols(className)) {
			if (symbol.getObject() instanceof GhidraClass ghidraClass) {
				return ghidraClass;
			}
		}
		return null;
	}

	private static DecompInterface openDecompiler(Program program, DecompileOptions options) {
		// Take the program's saved decompiler options rather than compiled-in
		// defaults, exactly as Ghidra's own CppExporter does, so rendering is
		// deterministic and matches what a reviewer sees in the CodeBrowser.
		options.grabFromProgram(program);
		options.setCommentStyle(DecompileOptions.CommentStyleEnum.CPPStyle);
		DecompInterface decompiler = new DecompInterface();
		decompiler.setOptions(options);
		decompiler.openProgram(program);
		decompiler.toggleSyntaxTree(true);
		return decompiler;
	}

	/**
	 * Export the data at the given addresses as recovered-style global
	 * definitions with {@code // GLOBAL:} markers, in the given order.
	 */
	public static String exportData(Program program, long[] addresses, TaskMonitor monitor)
			throws CancelledException {
		Wiz8DataPrinter printer = new Wiz8DataPrinter(program);
		StringBuilder output = new StringBuilder();
		AddressSpace space = program.getAddressFactory().getDefaultAddressSpace();
		for (long address : addresses) {
			monitor.checkCancelled();
			if (output.length() > 0) {
				output.append('\n');
			}
			output.append(printer.print(space.getAddress(address)));
		}
		return output.toString();
	}

	private static String exportFunction(Function function, DecompInterface decompiler,
			DecompileOptions options, TaskMonitor monitor) {
		FunctionRole role = FunctionRoleResolver.resolve(function);
		FunctionKind kind = FunctionKind.classify(function);
		Wiz8CxxPrinter printer = new Wiz8CxxPrinter(function, kind);

		if (function.getParentNamespace() instanceof GhidraClass owner &&
			owner.getName().indexOf('[') >= 0 &&
			!kind.isDeletingDestructor()) {
			// A bracket-encoded namespace marks a template specialization
			// member: record the emission, never print a body for it.
			return templateEmissionBlock(function);
		}
		if (!role.hasAuthoredBody()) {
			return printer.printSynthetic();
		}

		Address entry = function.getEntryPoint();
		CodeUnit codeUnit = function.getProgram().getListing().getCodeUnitAt(entry);
		if (!(codeUnit instanceof Instruction)) {
			return printer.marker() + "\n/* No instruction at the entry point; " +
				"cannot decompile " + function.getName() + ". */\n";
		}

		monitor.setMessage("Decompiling " + function.getName());
		DecompileResults results =
			decompiler.decompileFunction(function, options.getDefaultTimeout(), monitor);
		ClangTokenGroup markup = results.getCCodeMarkup();
		if (!results.decompileCompleted() || markup == null) {
			String error = results.getErrorMessage();
			return printer.marker() + "\n/* Unable to decompile '" + function.getName() +
				"': " + (error == null ? "no result" : error.trim()) + " */\n";
		}

		try {
			Msvc6Patterns.Analysis analysis = Msvc6Patterns.analyze(function, kind, results);
			String text = printer.print(markup, analysis);
			if (!analysis.defects.isEmpty()) {
				// A defect in one pass is contained (only that pass's claims
				// were rolled back), but it must never disappear: the block
				// names it so measurement tooling can distinguish a bug from
				// a decline and fail loudly. The lines sit at the end of the
				// block so per-block splitters attribute them to this
				// function, never to its predecessor.
				StringBuilder flagged = new StringBuilder(text);
				if (!text.endsWith("\n")) {
					flagged.append('\n');
				}
				for (String defect : analysis.defects) {
					flagged.append("// exporter-defect: ").append(defect).append('\n');
				}
				text = flagged.toString();
			}
			return text;
		}
		catch (Exception e) {
			// A printer defect must never block the batch; fall back to the
			// decompiler's own flat rendering, flagged as the defect it is.
			String decompiled = results.getDecompiledFunction().getC();
			return printer.marker() + "\n" + decompiled +
				(decompiled.endsWith("\n") ? "" : "\n") +
				"// exporter-defect: print: " + e + "\n";
		}
	}

	private static String exportFunctionBody(Function function, long referenceMask,
			DecompInterface decompiler,
			DecompileOptions options, TaskMonitor monitor) {
		FunctionKind kind = FunctionKind.classify(function);
		if (!FunctionRoleResolver.resolve(function).hasAuthoredBody() ||
			!(function.getProgram().getListing().getCodeUnitAt(function.getEntryPoint())
				instanceof Instruction)) {
			return "";
		}
		DecompileResults results = decompiler.decompileFunction(function,
			options.getDefaultTimeout(), monitor);
		ClangTokenGroup markup = results.getCCodeMarkup();
		if (!results.decompileCompleted() || markup == null) {
			return "";
		}
		try {
			Msvc6Patterns.Analysis analysis =
				Msvc6Patterns.analyze(function, kind, results, referenceMask);
			String body = new Wiz8CxxPrinter(function, kind).printBody(markup, analysis);
			if (analysis.defects.isEmpty()) {
				return body;
			}
			StringBuilder flagged = new StringBuilder(body);
			if (!body.endsWith("\n")) {
				flagged.append('\n');
			}
			for (String defect : analysis.defects) {
				flagged.append("// exporter-defect: ").append(defect).append('\n');
			}
			return flagged.toString();
		}
		catch (Exception error) {
			return "// exporter-defect: body: " + error + "\n";
		}
	}

	/**
	 * The per-pass transformation trace for one function: what each
	 * recognizer applied, declined (and why), rolled back, or failed on.
	 * Ephemeral diagnostic text; never part of exported source.
	 */
	public static String explain(Program program, long entryPoint, TaskMonitor monitor) {
		Address entry = program.getAddressFactory().getDefaultAddressSpace()
				.getAddress(entryPoint);
		Function function = program.getFunctionManager().getFunctionAt(entry);
		if (function == null) {
			return String.format("error: no function at 0x%08x%n", entryPoint);
		}
		FunctionKind kind = FunctionKind.classify(function);
		FunctionRole role = FunctionRoleResolver.resolve(function);
		if (!role.hasAuthoredBody()) {
			return role.emissionKind().name().toLowerCase() +
				": marker-only block, no analysis (" + role.evidence() + ")\n";
		}
		DecompileOptions options = new DecompileOptions();
		DecompInterface decompiler = openDecompiler(program, options);
		try {
			DecompileResults results =
				decompiler.decompileFunction(function, options.getDefaultTimeout(), monitor);
			if (!results.decompileCompleted() || results.getCCodeMarkup() == null) {
				String error = results.getErrorMessage();
				return "decompiler failure: " +
					(error == null ? "no result" : error.trim()) + "\n";
			}
			Msvc6Patterns.Analysis analysis =
				Msvc6Patterns.analyze(function, kind, results);
			StringBuilder out = new StringBuilder();
			out.append(String.format("%s %s (%s)%n", function.getName(true),
				"0x" + entry, kind.name().toLowerCase()));
			for (Msvc6Patterns.TraceEvent event : analysis.trace) {
				out.append(event).append('\n');
			}
			if (analysis.trace.isEmpty()) {
				out.append("no recognizer applied or declined; verbatim rendering\n");
			}
			for (String defect : analysis.defects) {
				out.append("defect      ").append(defect).append('\n');
			}
			out.append(String.format("claims: %d dropped node(s), %d replacement(s)%n",
				analysis.dropped.size(), analysis.replaced.size()));
			return out.toString();
		}
		finally {
			decompiler.dispose();
		}
	}
}
