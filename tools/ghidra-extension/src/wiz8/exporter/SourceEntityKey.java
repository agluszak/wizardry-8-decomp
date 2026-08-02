package wiz8.exporter;

import java.util.ArrayList;
import java.util.List;

import ghidra.program.model.data.DataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.Parameter;

/**
 * Identity of one source-level callable, independent of how many binary
 * emissions implement it.  The formal signature deliberately excludes auto
 * parameters: hidden {@code this}, return storage, and most-derived flags are
 * ABI mechanics, not distinct source entities.
 */
public record SourceEntityKey(GhidraClass owner, SourceKind kind,
		String formalSignature) {

	static SourceEntityKey of(Function emission, Function canonical, SourceKind kind) {
		Function identity = canonical != null ? canonical : emission;
		GhidraClass owner = identity.getParentNamespace() instanceof GhidraClass clazz
			? clazz : emission.getParentNamespace() instanceof GhidraClass clazz ? clazz : null;
		String ownerName = owner == null ? "" : owner.getName(true);
		String name = switch (kind) {
			case CONSTRUCTOR -> owner == null ? identity.getName() : owner.getName();
			case DESTRUCTOR -> "~" + (owner == null ? identity.getName() : owner.getName());
			default -> identity.getName();
		};
		List<String> parameters = new ArrayList<>();
		if (kind != SourceKind.DESTRUCTOR) {
			for (Parameter parameter : identity.getParameters()) {
				if (!parameter.isAutoParameter()) {
					parameters.add(typeIdentity(parameter.getFormalDataType()));
				}
			}
		}
		String qualified = ownerName.isEmpty() ? name : ownerName + "::" + name;
		return new SourceEntityKey(owner, kind,
			qualified + "(" + String.join(",", parameters) + ")");
	}

	private static String typeIdentity(DataType type) {
		if (type == null) {
			return "<unknown>";
		}
		String path = type.getPathName();
		return path == null || path.isBlank() ? type.getName() : path;
	}
}
