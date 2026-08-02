package wiz8.recovery;

/** Exception-registration and stack-lifetime recovery. */
final class EhRecovery {
	private EhRecovery() { }

	static void recover(Msvc6Patterns pipeline) {
		pipeline.recover("eh.registration-frame", pipeline::analyzeExceptionHandling);
		pipeline.recover("eh.stack-local", pipeline::liftStackLocalsPass);
	}
}
