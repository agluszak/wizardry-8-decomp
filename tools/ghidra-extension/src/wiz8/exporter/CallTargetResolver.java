package wiz8.exporter;

import java.util.IdentityHashMap;
import java.util.Map;

import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Program;
import ghidra.program.model.listing.FunctionTag;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.pcode.PcodeOp;
import ghidra.program.model.pcode.Varnode;

/** Canonical target and source ownership for one direct call. */
final class CallTargetResolver {
	private final Map<Function, Target> cache = new IdentityHashMap<>();

	/** Direct function or unique external function referenced by an import pointer. */
	Function referencedAt(Program program, Address address) {
		Function direct = program.getFunctionManager().getFunctionAt(address);
		if (direct != null) {
			return direct;
		}
		Function match = null;
		for (var reference : program.getReferenceManager().getReferencesFrom(address)) {
			Function candidate = program.getFunctionManager()
				.getFunctionAt(reference.getToAddress());
			if (candidate == null) {
				continue;
			}
			if (match != null && !match.equals(candidate)) {
				return null;
			}
			match = candidate;
		}
		return match;
	}
	enum Origin {
		FIRST_PARTY, SURRENDER, SGP, MSVC_RUNTIME, PLATFORM,
		ZLIB, JPEG, INFO_ZIP, IMPORT, LIBRARY, UNKNOWN
	}
	enum ThunkKind {
		NONE, IMPORT, THIS_ADJUSTOR, RETURN_ADJUSTOR,
		THIS_AND_RETURN_ADJUSTOR, UNKNOWN
	}

	record Target(Function referenced, Function canonical, Origin origin,
			ThunkKind thunkKind, long thisAdjustment, long returnAdjustment,
			String evidence) {
		boolean safelyErasable() {
			return thunkKind == ThunkKind.NONE || thunkKind == ThunkKind.IMPORT;
		}
	}

	Target resolve(Function referenced) {
		if (referenced == null) {
			return null;
		}
		return cache.computeIfAbsent(referenced, this::resolveUncached);
	}

	private Target resolveUncached(Function referenced) {
		Function canonical = referenced;
		ThunkKind thunkKind = ThunkKind.NONE;
		if (referenced.isThunk()) {
			Function resolved = referenced.getThunkedFunction(true);
			if (resolved != null) {
				canonical = resolved;
				if (!resolved.equals(referenced)) {
					thunkKind = resolved.isExternal() ? ThunkKind.IMPORT : ThunkKind.UNKNOWN;
				}
			}
		}
		ThunkAdjustment adjustment = thunkKind == ThunkKind.IMPORT ||
			thunkKind == ThunkKind.NONE ? new ThunkAdjustment(thunkKind, 0, 0)
				: thunkAdjustment(referenced);
		thunkKind = adjustment.kind;
		Origin tagged = taggedOrigin(referenced);
		if (tagged == Origin.UNKNOWN && canonical != referenced) {
			tagged = taggedOrigin(canonical);
		}
		if (tagged != Origin.UNKNOWN) {
			return new Target(referenced, canonical, tagged, thunkKind,
				adjustment.thisAdjustment, adjustment.returnAdjustment,
				"reviewed Ghidra origin tag");
		}
		if (canonical.isExternal()) {
			String library = canonical.getExternalLocation() == null ? ""
				: canonical.getExternalLocation().getLibraryName();
			return new Target(referenced, canonical, libraryOrigin(library),
				thunkKind == ThunkKind.NONE ? ThunkKind.IMPORT : thunkKind,
				adjustment.thisAdjustment, adjustment.returnAdjustment,
				"Ghidra external location " + library);
		}
		String identity = (canonical.getParentNamespace().getName(true) + "::" +
			canonical.getName()).toLowerCase();
		Origin origin = identityOrigin(identity);
		return new Target(referenced, canonical, origin, thunkKind,
			adjustment.thisAdjustment, adjustment.returnAdjustment,
			origin == Origin.UNKNOWN ? "unclassified internal function"
				: "centralized symbol-identity codec");
	}

	/** Prove constant ECX/EAX adjustments from the thunk's instruction P-code. */
	private static ThunkAdjustment thunkAdjustment(Function thunk) {
		long thisAdjustment = 0;
		long returnAdjustment = 0;
		boolean adjustsThis = false;
		boolean adjustsReturn = false;
		var instructions = thunk.getProgram().getListing().getInstructions(thunk.getBody(), true);
		while (instructions.hasNext()) {
			Instruction instruction = instructions.next();
			for (PcodeOp op : instruction.getPcode()) {
				if (op.getOutput() == null || op.getNumInputs() < 2 ||
					(op.getOpcode() != PcodeOp.INT_ADD && op.getOpcode() != PcodeOp.INT_SUB)) {
					continue;
				}
				Varnode constant = op.getInput(1).isConstant() ? op.getInput(1)
					: op.getOpcode() == PcodeOp.INT_ADD && op.getInput(0).isConstant()
						? op.getInput(0) : null;
				Varnode source = constant == op.getInput(0) ? op.getInput(1) : op.getInput(0);
				if (constant == null || !sameRegister(thunk, source, op.getOutput())) {
					continue;
				}
				var register = thunk.getProgram().getRegister(op.getOutput());
				if (register == null) {
					continue;
				}
				long delta = signed(constant);
				if (op.getOpcode() == PcodeOp.INT_SUB) {
					delta = -delta;
				}
				if (register.getName().equalsIgnoreCase("ECX")) {
					thisAdjustment += delta;
					adjustsThis = true;
				}
				else if (register.getName().equalsIgnoreCase("EAX")) {
					returnAdjustment += delta;
					adjustsReturn = true;
				}
			}
		}
		ThunkKind kind = adjustsThis && adjustsReturn ? ThunkKind.THIS_AND_RETURN_ADJUSTOR
			: adjustsThis ? ThunkKind.THIS_ADJUSTOR
				: adjustsReturn ? ThunkKind.RETURN_ADJUSTOR : ThunkKind.UNKNOWN;
		return new ThunkAdjustment(kind, thisAdjustment, returnAdjustment);
	}

	private static boolean sameRegister(Function function, Varnode left, Varnode right) {
		var leftRegister = function.getProgram().getRegister(left);
		var rightRegister = function.getProgram().getRegister(right);
		return leftRegister != null && leftRegister.equals(rightRegister);
	}

	private static long signed(Varnode constant) {
		int bits = Math.min(64, Math.max(1, constant.getSize() * 8));
		long value = constant.getOffset();
		if (bits < 64 && (value & (1L << (bits - 1))) != 0) {
			value |= -1L << bits;
		}
		return value;
	}

	private record ThunkAdjustment(ThunkKind kind, long thisAdjustment,
			long returnAdjustment) {
	}

	private static Origin taggedOrigin(Function function) {
		Origin found = null;
		for (FunctionTag tag : function.getTags()) {
			String name = tag.getName();
			if (!name.startsWith("wiz8:origin:")) {
				continue;
			}
			Origin parsed = switch (name.substring("wiz8:origin:".length())) {
				case "first-party" -> Origin.FIRST_PARTY;
				case "library" -> Origin.LIBRARY;
				case "surrender" -> Origin.SURRENDER;
				case "sgp" -> Origin.SGP;
				case "msvc-crt" -> Origin.MSVC_RUNTIME;
				case "zlib" -> Origin.ZLIB;
				case "jpeg" -> Origin.JPEG;
				case "info-zip" -> Origin.INFO_ZIP;
				case "win32", "directx", "mfc", "platform" -> Origin.PLATFORM;
				case "import" -> Origin.IMPORT;
				default -> Origin.UNKNOWN;
			};
			if (found != null && found != parsed) {
				throw new IllegalStateException("conflicting origin tags on " +
					function.getName(true));
			}
			found = parsed;
		}
		return found == null ? Origin.UNKNOWN : found;
	}

	private static Origin libraryOrigin(String library) {
		String name = library == null ? "" : library.toLowerCase();
		if (name.contains("zlib")) return Origin.ZLIB;
		if (name.contains("jpeg")) return Origin.JPEG;
		if (name.contains("zip")) return Origin.INFO_ZIP;
		if (name.contains("sgp")) return Origin.SGP;
		if (name.contains("msv") || name.contains("crt")) return Origin.MSVC_RUNTIME;
		if (name.endsWith(".dll") || !name.isEmpty()) return Origin.PLATFORM;
		return Origin.IMPORT;
	}

	private static Origin identityOrigin(String identity) {
		if (identity.contains("surrender") || identity.contains("::sr")) {
			return Origin.SURRENDER;
		}
		if (identity.contains("sgp")) return Origin.SGP;
		if (identity.contains("zlib")) return Origin.ZLIB;
		if (identity.contains("jpeg")) return Origin.JPEG;
		if (identity.contains("infozip") || identity.contains("info_zip")) {
			return Origin.INFO_ZIP;
		}
		String leaf = identity.substring(identity.lastIndexOf(':') + 1);
		if (leaf.matches("__(?:a?u?ll(?:mul|div|rem|shr)|alloca_probe).*")) {
			return Origin.MSVC_RUNTIME;
		}
		return Origin.UNKNOWN;
	}
}
