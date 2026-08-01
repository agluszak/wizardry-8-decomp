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

	private Wiz8RecoveryExporter() {
	}

	/**
	 * Export the functions whose entry points are given, in the given order,
	 * separated by blank lines.
	 */
	public static String export(Program program, long[] entryPoints, TaskMonitor monitor) {
		DecompileOptions options = new DecompileOptions();
		DecompInterface decompiler = openDecompiler(program, options);
		try {
			StringBuilder output = new StringBuilder();
			AddressSpace space = program.getAddressFactory().getDefaultAddressSpace();
			FunctionManager functionManager = program.getFunctionManager();
			for (long entryPoint : entryPoints) {
				if (output.length() > 0) {
					output.append('\n');
				}
				Address entry = space.getAddress(entryPoint);
				Function function = functionManager.getFunctionAt(entry);
				if (function == null) {
					output.append(String.format(
						"// error: no function at 0x%08x%n", entryPoint));
					continue;
				}
				output.append(exportFunction(function, decompiler, options, monitor));
			}
			return output.toString();
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
	public static String exportClass(Program program, String className, TaskMonitor monitor) {
		GhidraClass ghidraClass = findClass(program, className);
		if (ghidraClass == null) {
			return String.format("// error: no class named %s%n", className);
		}
		List<Function> family = new ArrayList<>();
		for (Function function : program.getFunctionManager().getFunctions(true)) {
			if (ghidraClass.equals(function.getParentNamespace())) {
				family.add(function);
			}
		}
		family.sort((a, b) -> a.getEntryPoint().compareTo(b.getEntryPoint()));

		StringBuilder output = new StringBuilder();
		output.append(new Wiz8ClassPrinter(program, ghidraClass).print());
		boolean template = className.indexOf('[') >= 0;
		DecompileOptions options = new DecompileOptions();
		DecompInterface decompiler = template ? null : openDecompiler(program, options);
		try {
			for (Function function : family) {
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

	/**
	 * The marker-only block recording that a template member was emitted at
	 * this address: SYNTHETIC for the compiler-generated deleting destructor,
	 * TEMPLATE for every other member, each followed by the specialization
	 * symbol it records.
	 */
	private static String templateEmissionBlock(Function function) {
		FunctionKind kind = FunctionKind.classify(function);
		if (kind == FunctionKind.SYNTHETIC_DELETING_DESTRUCTOR) {
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
		options.setCommentStyle(DecompileOptions.CommentStyleEnum.CPPStyle);
		DecompInterface decompiler = new DecompInterface();
		decompiler.setOptions(options);
		decompiler.openProgram(program);
		decompiler.toggleSyntaxTree(true);
		return decompiler;
	}

	private static String exportFunction(Function function, DecompInterface decompiler,
			DecompileOptions options, TaskMonitor monitor) {
		FunctionKind kind = FunctionKind.classify(function);
		Wiz8CxxPrinter printer = new Wiz8CxxPrinter(function, kind);

		if (kind == FunctionKind.SYNTHETIC_DELETING_DESTRUCTOR) {
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
			return printer.print(markup, analysis);
		}
		catch (Exception e) {
			// A printer defect must never block the batch; fall back to the
			// decompiler's own flat rendering.
			return printer.marker() + "\n/* printer error: " + e + " */\n" +
				results.getDecompiledFunction().getC();
		}
	}
}
