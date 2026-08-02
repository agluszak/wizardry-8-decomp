package wiz8.recovery;

/** Direct, virtual, library, and structure-return ABI recovery. */
final class CallRecovery {
	private CallRecovery() { }

	static void recoverEarly(Msvc6Patterns pipeline) {
		pipeline.recover("call.virtual", pipeline::rewriteVirtualCalls);
		pipeline.recover("call.library", pipeline::rewriteCanonicalLibraryCalls);
		pipeline.recover("call.direct-member", pipeline::rewriteMethodCalls);
	}

	static void recoverStructureReturns(Msvc6Patterns pipeline) {
		pipeline.recover("call.struct-return", pipeline::rewriteStructReturns);
	}
}
