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

import ghidra.app.decompiler.ClangTokenGroup;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSpace;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
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
		options.setCommentStyle(DecompileOptions.CommentStyleEnum.CPPStyle);
		DecompInterface decompiler = new DecompInterface();
		decompiler.setOptions(options);
		decompiler.openProgram(program);
		decompiler.toggleSyntaxTree(true);
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
			return printer.print(markup);
		}
		catch (Exception e) {
			// A printer defect must never block the batch; fall back to the
			// decompiler's own flat rendering.
			return printer.marker() + "\n/* printer error: " + e + " */\n" +
				results.getDecompiledFunction().getC();
		}
	}
}
