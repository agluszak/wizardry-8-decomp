package wiz8.recovery;

/** Source-equivalent choices retained only when they improve pinned VC6 emission. */
final class MatchShaping {
	private MatchShaping() { }

	static void recover(Msvc6Patterns pipeline) {
		pipeline.recover("shape.call-result-local", pipeline::materializeCallResultLocals);
	}
}
