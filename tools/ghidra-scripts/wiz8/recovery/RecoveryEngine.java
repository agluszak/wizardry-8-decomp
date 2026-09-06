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
package wiz8.recovery;

import java.util.ArrayList;
import java.util.Iterator;
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
import ghidra.program.model.pcode.PcodeOp;
import ghidra.program.model.pcode.Varnode;
import ghidra.util.exception.CancelledException;
import ghidra.util.task.TaskMonitor;

/**
 * Selected-function C++ recovery engine for the Wizardry 8 project.
 *
 * Input is the live analyzed {@link Program} and a list of function entry
 * points; output is recovered-style C++ text with reccmp entity markers.
 * Only the selected definitions are emitted: no headers, no data-type
 * definitions, no referenced-global declarations. A function that cannot be
 * decompiled is reported inline as a comment; it never aborts the batch.
 */
public final class RecoveryEngine {
	private RecoveryEngine() {
	}

	/** One independently bounded, single-decompile recovery result. */
	public static final class FunctionExport {
		private final String text;
		private final String body;
		private final SourceEntity entity;
		private final Emission emission;
		private final String[] defects;
		private final PassFact[] passes;
		private final CallFact[] calls;

		FunctionExport(String text, String body, SourceEntity entity, Emission emission,
				String[] defects) {
			this(text, body, entity, emission, defects, new PassFact[0],
				new CallFact[0]);
		}

		FunctionExport(String text, String body, SourceEntity entity, Emission emission,
				String[] defects, PassFact[] passes, CallFact[] calls) {
			this.text = text;
			this.body = body;
			this.entity = entity;
			this.emission = emission;
			this.defects = defects;
			this.passes = passes;
			this.calls = calls;
		}

		public String getText() { return text; }
		public String getBody() { return body; }
		public String getEmissionKind() {
			return emission == null ? "missing" : emission.kind().name().toLowerCase();
		}
		public String getSourceKind() {
			return entity == null ? "none" : entity.kind().name().toLowerCase();
		}
		public String getSourceEntity() {
			return entity == null ? "" : entity.key().formalSignature();
		}
		public long getBodyOwner() {
			if (entity == null) return -1;
			return switch (entity.bodyCarrier()) {
				case BodyCarrier.Direct direct -> direct.emission().function()
					.getEntryPoint().getOffset();
				case BodyCarrier.Extracted extracted -> extracted.carrier().function()
					.getEntryPoint().getOffset();
				case BodyCarrier.None ignored -> -1;
			};
		}
		public long getCanonicalTarget() {
			return emission == null || emission.canonicalTarget() == null ? -1
				: emission.canonicalTarget().getEntryPoint().getOffset();
		}
		public String getOrigin() {
			return emission == null ? "unknown" : emission.origin().name().toLowerCase();
		}
		public String getEvidence() {
			return emission == null ? "missing function" : emission.evidence();
		}
		public String[] getDefects() { return defects.clone(); }
		public PassFact[] getPasses() { return passes.clone(); }
		public CallFact[] getCalls() { return calls.clone(); }
	}

	/** Structured recognizer event; no parsing of explain text is required. */
	public static final class PassFact {
		private final String status;
		private final String pass;
		private final String detail;

		PassFact(Msvc6Patterns.TraceEvent event) {
			this.status = event.status;
			this.pass = event.pass;
			this.detail = event.detail;
		}
		public String getStatus() { return status; }
		public String getPass() { return pass; }
		public String getDetail() { return detail; }
	}

	/** One direct P-code call resolved through thunk/origin analysis. */
	public static final class CallFact {
		private final long site;
		private final long referenced;
		private final long canonical;
		private final String referencedName;
		private final String canonicalName;
		private final String origin;
		private final String thunkKind;
		private final long thisAdjustment;
		private final long returnAdjustment;
		private final String evidence;

		CallFact(long site, CallTargetResolver.Target target) {
			this.site = site;
			this.referenced = target.referenced().getEntryPoint().getOffset();
			this.canonical = target.canonical().getEntryPoint().getOffset();
			this.referencedName = target.referenced().getName(true);
			this.canonicalName = target.canonical().getName(true);
			this.origin = target.origin().name().toLowerCase();
			this.thunkKind = target.thunkKind().name().toLowerCase();
			this.thisAdjustment = target.thisAdjustment();
			this.returnAdjustment = target.returnAdjustment();
			this.evidence = target.evidence();
		}
		public long getSite() { return site; }
		public long getReferenced() { return referenced; }
		public long getCanonical() { return canonical; }
		public String getReferencedName() { return referencedName; }
		public String getCanonicalName() { return canonicalName; }
		public String getOrigin() { return origin; }
		public String getThunkKind() { return thunkKind; }
		public long getThisAdjustment() { return thisAdjustment; }
		public long getReturnAdjustment() { return returnAdjustment; }
		public String getEvidence() { return evidence; }
	}

	/**
	 * Export text, body, role, and diagnostics from one decompile per entry.
	 * Source-index reference forms feed the one analysis used by both renderings.
	 */
	public static FunctionExport[] exportFunctionPackets(Program program, long[] entryPoints,
			RecoverySourceIndex source, boolean explain,
			TaskMonitor monitor) throws CancelledException {
		RecoverySession session = new RecoverySession(program, source);
		DecompileOptions options = new DecompileOptions();
		DecompInterface decompiler = openDecompiler(program, options);
		try {
			FunctionExport[] output = new FunctionExport[entryPoints.length];
			AddressSpace space = program.getAddressFactory().getDefaultAddressSpace();
			FunctionManager functions = program.getFunctionManager();
			for (int i = 0; i < entryPoints.length; i++) {
				monitor.checkCancelled();
				Function function = functions.getFunctionAt(space.getAddress(entryPoints[i]));
				if (function == null) {
					output[i] = new FunctionExport(String.format(
						"// error: no function at 0x%08x%n", entryPoints[i]), "", null, null,
						new String[] {"missing function"});
					continue;
				}
				output[i] = renderFunction(session, function,
					session.entity(function, monitor), session.emission(function), decompiler,
					options, explain, monitor);
			}
			return output;
		}
		finally {
			decompiler.dispose();
		}
	}

	/**
	 * The marker-only block recording that a template member was emitted at
	 * this address: SYNTHETIC for the compiler-generated deleting destructor,
	 * TEMPLATE for every other member, each followed by the specialization
	 * symbol it records.
	 */
	private static String templateEmissionBlock(String target, SourceEntity entity,
			Emission emission) {
		Function function = emission.function();
		if (emission.isDeletingWrapper()) {
			return new CxxRenderer(target, function, entity, emission).printSynthetic();
		}
		String owner = TypeNames.map(function.getParentNamespace().getName(true));
		String name = TypeNames.map(function.getName());
		return String.format("// TEMPLATE: %s 0x%08x%n// %s::%s%n", target,
			function.getEntryPoint().getOffset(), owner, name);
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

	private static FunctionExport renderFunction(RecoverySession session, Function function,
			SourceEntity entity, Emission emission,
			DecompInterface decompiler,
			DecompileOptions options, boolean explain, TaskMonitor monitor) {
		CxxRenderer printer = new CxxRenderer(session.source.target(), function, entity, emission);

		if (function.getParentNamespace() instanceof GhidraClass owner &&
			owner.getName().indexOf('[') >= 0 &&
			!emission.isDeletingWrapper()) {
			return new FunctionExport(templateEmissionBlock(session.source.target(), entity, emission), "", entity,
				emission, new String[0]);
		}
		boolean direct = entity.bodyCarrier() instanceof BodyCarrier.Direct carrier &&
			carrier.emission().equals(emission);
		boolean extracted = entity.bodyCarrier() instanceof BodyCarrier.Extracted carrier &&
			carrier.carrier().equals(emission);
		if (!direct && !extracted) {
			return new FunctionExport(printer.printSynthetic(), "", entity, emission,
				new String[0]);
		}

		Address entry = function.getEntryPoint();
		CodeUnit codeUnit = function.getProgram().getListing().getCodeUnitAt(entry);
		if (!(codeUnit instanceof Instruction)) {
			String defect = "no instruction at entry point";
			return new FunctionExport(printer.marker() +
				"\n/* No instruction at the entry point; cannot decompile " +
				function.getName() + ". */\n", "", entity, emission,
				new String[] {defect});
		}

		monitor.setMessage("Decompiling " + function.getName());
		DecompileResults results =
			decompiler.decompileFunction(function, options.getDefaultTimeout(), monitor);
		ClangTokenGroup markup = results.getCCodeMarkup();
		if (!results.decompileCompleted() || markup == null) {
			String error = results.getErrorMessage();
			String defect = "decompile: " + (error == null ? "no result" : error.trim());
			return new FunctionExport(printer.marker() + "\n/* Unable to decompile '" +
				function.getName() + "': " +
				(error == null ? "no result" : error.trim()) + " */\n", "", entity, emission,
				new String[] {defect});
		}

		try {
			Msvc6Patterns.Analysis analysis =
				Msvc6Patterns.analyze(session, function, entity, emission, results,
					session.source.facts(function.getEntryPoint().getOffset()));
			if (extracted && !analysis.extractedBody) {
				String defect = "body extraction: deleting-wrapper epilogue was not fully proved";
				return new FunctionExport(printer.printSynthetic(), "", entity, emission,
					new String[] {defect}, analysis.trace.stream().map(PassFact::new)
						.toArray(PassFact[]::new), new CallFact[0]);
			}
			String text = printer.print(markup, analysis);
			String body = printer.printBody(markup, analysis);
			String[] defects = analysis.defects.toArray(String[]::new);
			PassFact[] passes = analysis.trace.stream().map(PassFact::new)
				.toArray(PassFact[]::new);
			return new FunctionExport(text, body, entity, emission, defects, passes,
				explain ? callFacts(session, results) : new CallFact[0]);
		}
		catch (Exception e) {
			String decompiled = results.getDecompiledFunction().getC();
			String defect = "print: " + e;
			String text = printer.marker() + "\n" + decompiled +
				(decompiled.endsWith("\n") ? "" : "\n");
			return new FunctionExport(text, "",
				entity, emission, new String[] {defect});
		}
	}

	private static CallFact[] callFacts(RecoverySession session, DecompileResults results) {
		if (results.getHighFunction() == null) {
			return new CallFact[0];
		}
		List<CallFact> facts = new ArrayList<>();
		Iterator<? extends PcodeOp> operations = results.getHighFunction().getPcodeOps();
		while (operations.hasNext()) {
			PcodeOp operation = operations.next();
			if (operation.getOpcode() != PcodeOp.CALL || operation.getNumInputs() < 1) {
				continue;
			}
			Varnode operand = operation.getInput(0);
			if (!operand.isAddress()) {
				continue;
			}
			Function referenced = session.calls.referencedAt(session.program, operand.getAddress());
			CallTargetResolver.Target target = session.calls.resolve(referenced);
			if (target != null) {
				long site = operation.getSeqnum() == null ? -1
					: operation.getSeqnum().getTarget().getOffset();
				facts.add(new CallFact(site, target));
			}
		}
		return facts.toArray(CallFact[]::new);
	}

}
