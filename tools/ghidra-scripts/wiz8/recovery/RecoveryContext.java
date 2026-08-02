package wiz8.recovery;

import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import ghidra.program.model.pcode.HighFunction;

/** Immutable semantic input shared by every recoverer for one function. */
record RecoveryContext(RecoverySession session, Function function,
		SourceEntity sourceEntity, Emission emission, SourceHints sourceHints,
		DecompileResults decompileResults, HighFunction highFunction,
		MarkupIndex markup) { }
