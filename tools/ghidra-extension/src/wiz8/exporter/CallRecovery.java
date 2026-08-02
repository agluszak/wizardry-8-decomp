package wiz8.exporter;

/** Direct, virtual, library, result-local, and structure-return recovery. */
final class CallRecovery {
	private CallRecovery() { }

	static void recoverEarly(Msvc6Patterns pipeline) {
		pipeline.recover("call.result-local", pipeline::materializeCallResultLocals);
		pipeline.recover("call.virtual", pipeline::rewriteVirtualCalls);
		pipeline.recover("call.library", pipeline::rewriteCanonicalLibraryCalls);
		pipeline.recover("call.direct-member", pipeline::rewriteMethodCalls);
	}

	static void recoverStructureReturns(Msvc6Patterns pipeline) {
		pipeline.recover("call.struct-return", pipeline::rewriteStructReturns);
	}
}
