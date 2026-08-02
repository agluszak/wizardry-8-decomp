package wiz8.exporter;

/** Constructor, destructor, vptr, allocation, and source-prototype recovery. */
final class LifecycleRecovery {
	private LifecycleRecovery() { }

	static void recoverScaffolding(Msvc6Patterns pipeline) {
		pipeline.recover("lifecycle.vptr-store", pipeline::suppressCompilerVptrStores);
	}

	static void recoverBodies(Msvc6Patterns pipeline, SourceEntity entity) {
		if (entity.isConstructor() || entity.isDestructor()) {
			pipeline.recover(entity.isConstructor() ? "lifecycle.constructor"
				: "lifecycle.destructor", pipeline::lifecyclePass);
		}
		pipeline.recover("allocation.pairs", pipeline::rewriteAllocationPairs);
		pipeline.recover("signature.prototype", pipeline::renderCompletePrototype);
	}
}
