package wiz8.exporter;

/** Type, cast, reference, literal, member, intrinsic, and array recovery. */
final class ExpressionRecovery {
	private ExpressionRecovery() { }

	static void recover(Msvc6Patterns pipeline) {
		pipeline.recover("render.qualified-type", pipeline::qualifyDemangledTypeTokens);
		pipeline.recover("signature.parameter-padding", pipeline::suppressParameterPadding);
		pipeline.recover("expression.pcode-intrinsic", pipeline::rewritePcodeIntrinsics);
		pipeline.recover("expression.null-upcast", pipeline::rewriteNullPreservingUpcasts);
		pipeline.recover("expression.array-index", pipeline::rewriteArrayIndexing);
		pipeline.recover("expression.void-pointer-conversion",
			pipeline::rewriteVoidPointerConversions);
		pipeline.recover("expression.member-access", pipeline::normalizeMemberAccess);
		pipeline.recover("expression.source-reference", pipeline::normalizeSourceReferences);
		pipeline.recover("expression.null-cast", pipeline::rewriteNullPointerCasts);
		pipeline.recover("literal.narrow-string", pipeline::rewriteStringLiterals);
	}
}
