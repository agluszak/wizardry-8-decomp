package wiz8.recovery;

import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryAccessException;
import ghidra.program.model.symbol.Reference;

/** Conservative recognition of compiler-emitted functions without usable symbols. */
final class CompilerEmissionClassifier {
	private static final int SCALAR_DELETING_SIZE = 30;

	private CompilerEmissionClassifier() {
	}

	static Function scalarDeletingDestructorTarget(Function function) {
		if (function.getBody().getNumAddresses() != SCALAR_DELETING_SIZE) {
			return null;
		}
		byte[] bytes = new byte[SCALAR_DELETING_SIZE];
		try {
			if (function.getProgram().getMemory().getBytes(function.getEntryPoint(), bytes) !=
				SCALAR_DELETING_SIZE || !isScalarDeletingDestructor(bytes)) {
				return null;
			}
		}
		catch (MemoryAccessException error) {
			return null;
		}

		Address call = function.getEntryPoint().add(3);
		for (Reference reference : function.getProgram().getReferenceManager().getReferencesFrom(call)) {
			Function target = function.getProgram().getFunctionManager()
				.getFunctionAt(reference.getToAddress());
			if (target != null) {
				return target;
			}
		}
		return function;
	}

	static boolean isScalarDeletingDestructor(byte[] code) {
		if (code == null || code.length != SCALAR_DELETING_SIZE) {
			return false;
		}
		int[] fixed = {
			0x56, 0x8b, 0xf1, 0xe8, -1, -1, -1, -1,
			0xf6, 0x44, 0x24, 0x08, 0x01, 0x74, 0x09, 0x56,
			0xe8, -1, -1, -1, -1, 0x83, 0xc4, 0x04,
			0x8b, 0xc6, 0x5e, 0xc2, 0x04, 0x00
		};
		for (int i = 0; i < fixed.length; i++) {
			if (fixed[i] >= 0 && (code[i] & 0xff) != fixed[i]) {
				return false;
			}
		}
		return true;
	}
}
