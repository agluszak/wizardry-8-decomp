package wiz8.exporter;

import java.util.ArrayList;
import java.util.List;

import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Program;
import ghidra.program.model.mem.Memory;

/**
 * The MSVC C++ exception model of one function, re-derived from program
 * memory.
 *
 * VC6 registers a per-function handler thunk ({@code mov eax, &FuncInfo;
 * jmp __CxxFrameHandler}); the FuncInfo record owns the unwind map, whose
 * states name the cleanup funclets that destroy live locals during an
 * unwind. Reading that record proves two things the decompiled body cannot:
 * that the function has no {@code try} blocks (so every EH statement is
 * compiler scaffolding, not source semantics), and that a frame slot holds
 * an object whose destructor runs on unwind (so a constructor call on that
 * slot is a local-object definition, not an arbitrary call).
 *
 * The record layout is the published MSVC one ({@code ehdata.h}); this
 * mirrors the proven parser in {@code tools/wiz8decomp/eh_metadata.py},
 * reading the live Program instead of the (protected) PE file.
 */
final class EhModel {

	/** One unwind-map state and its decoded cleanup funclet. */
	static final class UnwindState {
		final int state;
		final int toState;
		final Address funclet;
		/** ebp-relative offset of a directly addressed object, else null. */
		final Long objectEbpOffset;
		/** The destructor the object funclet branches to, else null. */
		final Function destructor;

		UnwindState(int state, int toState, Address funclet, Long objectEbpOffset,
				Function destructor) {
			this.state = state;
			this.toState = toState;
			this.funclet = funclet;
			this.objectEbpOffset = objectEbpOffset;
			this.destructor = destructor;
		}
	}

	private static final int[] FUNC_INFO_MAGICS = {0x19930520, 0x19930521, 0x19930522};

	final Address funcInfo;
	final int maxState;
	final int tryBlockCount;
	final List<UnwindState> states;

	private EhModel(Address funcInfo, int maxState, int tryBlockCount,
			List<UnwindState> states) {
		this.funcInfo = funcInfo;
		this.maxState = maxState;
		this.tryBlockCount = tryBlockCount;
		this.states = states;
	}

	/**
	 * Resolve the model from the handler thunk installed by the function's
	 * EH prolog. Returns null when anything fails positive identification:
	 * the thunk is not {@code mov eax, imm32}, the record's magic or counts
	 * are implausible, or a table read leaves the mapped image.
	 */
	static EhModel resolve(RecoverySession session, long thunkAddress) {
		try {
			Program program = session.program;
			Memory memory = program.getMemory();
			Address thunk = address(program, thunkAddress);
			if ((memory.getByte(thunk) & 0xff) != 0xb8) {
				return null;
			}
			Address funcInfo = address(program, Integer.toUnsignedLong(
				memory.getInt(thunk.add(1))));
			int magic = memory.getInt(funcInfo);
			boolean known = false;
			for (int candidate : FUNC_INFO_MAGICS) {
				known |= magic == candidate;
			}
			if (!known) {
				return null;
			}
			int maxState = memory.getInt(funcInfo.add(4));
			long unwindMap = Integer.toUnsignedLong(memory.getInt(funcInfo.add(8)));
			int tryBlockCount = memory.getInt(funcInfo.add(12));
			if (maxState < 0 || maxState >= 4096 || tryBlockCount < 0 ||
				tryBlockCount >= 512) {
				return null;
			}
			List<UnwindState> states = new ArrayList<>();
			for (int state = 0; state < maxState; state++) {
				Address entry = address(program, unwindMap).add(state * 8L);
				int toState = memory.getInt(entry);
				long action = Integer.toUnsignedLong(memory.getInt(entry.add(4)));
				Address funclet = action == 0 ? null : address(program, action);
				long[] decoded = funclet == null ? null : decodeObjectFunclet(memory, funclet);
				Function destructor = null;
				Long objectOffset = null;
				if (decoded != null) {
					objectOffset = decoded[0];
					destructor = program.getFunctionManager()
							.getFunctionAt(address(program, decoded[1]));
					if (destructor == null ||
						session.sourceKey(destructor).kind() != SourceKind.DESTRUCTOR ||
						!(destructor.getParentNamespace() instanceof GhidraClass)) {
						objectOffset = null;
						destructor = null;
					}
				}
				states.add(new UnwindState(state, toState, funclet, objectOffset, destructor));
			}
			return new EhModel(funcInfo, maxState, tryBlockCount, states);
		}
		catch (Exception e) {
			return null;
		}
	}

	/**
	 * Decode the one funclet shape that names a stack object: a direct
	 * {@code lea ecx, [ebp+disp]} followed by a tail {@code jmp} (or
	 * {@code call}) of the destructor. Returns {ebpOffset, target} or null.
	 * Pointer loads, vector helpers, and anything longer decline; those
	 * cleanups do not place a typed object on the frame.
	 */
	private static long[] decodeObjectFunclet(Memory memory, Address funclet)
			throws Exception {
		byte[] raw = new byte[12];
		int available = memory.getBytes(funclet, raw);
		long ebpOffset;
		int branch;
		if (available >= 8 && (raw[0] & 0xff) == 0x8d && (raw[1] & 0xff) == 0x4d) {
			ebpOffset = raw[2]; // sign-extended disp8
			branch = 3;
		}
		else if (available >= 11 && (raw[0] & 0xff) == 0x8d && (raw[1] & 0xff) == 0x8d) {
			ebpOffset = littleInt(raw, 2);
			branch = 6;
		}
		else {
			return null;
		}
		int opcode = raw[branch] & 0xff;
		if (opcode != 0xe9 && opcode != 0xe8) {
			return null;
		}
		long relative = littleInt(raw, branch + 1);
		long target = funclet.getOffset() + branch + 5 + relative;
		return new long[] {ebpOffset, target};
	}

	private static long littleInt(byte[] raw, int offset) {
		return (raw[offset] & 0xffL) | (raw[offset + 1] & 0xffL) << 8 |
			(raw[offset + 2] & 0xffL) << 16 | (long) raw[offset + 3] << 24;
	}

	private static Address address(Program program, long offset) {
		return program.getAddressFactory().getDefaultAddressSpace().getAddress(offset);
	}
}
