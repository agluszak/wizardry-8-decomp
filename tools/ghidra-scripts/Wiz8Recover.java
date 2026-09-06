// Recover Wizardry 8 function bodies from the reviewed program.
// @category Wizardry8

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import wiz8.recovery.RecoveryEngine;
import wiz8.recovery.RecoverySourceIndex;
import wiz8.recovery.SourceCandidate;
import wiz8.recovery.SourceHints;
import wiz8.recovery.SourceKind;

public class Wiz8Recover extends GhidraScript {
	@Override
	protected void run() throws Exception {
		Arguments arguments = Arguments.parse(getScriptArgs());
		RecoverySourceIndex source = sourceIndex(arguments.sourceIndex(), arguments.target());
		long[] entries = arguments.allFunctions()
			? allFunctions() : resolveEntries(arguments.selections());
		RecoveryEngine.FunctionExport[] packets = RecoveryEngine.exportFunctionPackets(
			currentProgram, entries, source, arguments.explain(), monitor);
		JsonObject result = result(entries, packets, source, arguments.explain(), arguments.includeBody());
		Path parent = arguments.output().toAbsolutePath().getParent();
		if (parent != null) Files.createDirectories(parent);
		Gson gson = new GsonBuilder().setPrettyPrinting().disableHtmlEscaping().create();
		Files.writeString(arguments.output(), gson.toJson(result) + "\n", StandardCharsets.UTF_8);
		println(arguments.output().toAbsolutePath().toString());
	}

	private long[] resolveEntries(List<String> selections) {
		Set<Long> entries = new LinkedHashSet<>();
		for (String selection : selections) {
			String[] bounds = selection.split(":", -1);
			long start = Long.decode(bounds[0]);
			if (bounds.length == 1) {
				Address address = currentProgram.getAddressFactory().getDefaultAddressSpace()
					.getAddress(start);
				Function function = currentProgram.getFunctionManager().getFunctionContaining(address);
				if (function == null) throw new IllegalArgumentException(
					"no function containing " + selection);
				entries.add(function.getEntryPoint().getOffset());
				continue;
			}
			long end = Long.decode(bounds[1]);
			if (end < start) throw new IllegalArgumentException(
				"range end precedes start: " + selection);
			var space = currentProgram.getAddressFactory().getDefaultAddressSpace();
			AddressSet range = new AddressSet(space.getAddress(start), space.getAddress(end));
			for (Function function : currentProgram.getFunctionManager().getFunctions(range, true)) {
				if (range.contains(function.getEntryPoint())) {
					entries.add(function.getEntryPoint().getOffset());
				}
			}
		}
		if (entries.isEmpty()) throw new IllegalArgumentException("no function entries selected");
		return entries.stream().mapToLong(Long::longValue).toArray();
	}

	private long[] allFunctions() {
		Set<Long> entries = new LinkedHashSet<>();
		for (Function function : currentProgram.getFunctionManager().getFunctions(true)) {
			if (!function.isExternal()) entries.add(function.getEntryPoint().getOffset());
		}
		return entries.stream().mapToLong(Long::longValue).toArray();
	}

	private static RecoverySourceIndex sourceIndex(Path path, String target) throws Exception {
		Map<Long, SourceHints> functions = new LinkedHashMap<>();
		Map<String, SourceCandidate> declarations = new LinkedHashMap<>();
		try (var reader = Files.newBufferedReader(path, StandardCharsets.UTF_8)) {
			JsonObject document = JsonParser.parseReader(reader).getAsJsonObject();
			JsonArray array = document.getAsJsonArray("markers");
			for (JsonElement element : array) {
				JsonObject marker = element.getAsJsonObject();
				if (!target.equals(marker.get("target").getAsString())) continue;
				functions.put(marker.get("address").getAsLong(), sourceHints(marker));
			}
			for (JsonElement element : document.getAsJsonArray("declarations")) {
				JsonObject declaration = element.getAsJsonObject();
				String sourceFile = declaration.get("source_file").getAsString();
				if (!belongsToTarget(sourceFile, target)) continue;
				SourceKind sourceKind = sourceKind("UNKNOWN", declaration);
				if (sourceKind == SourceKind.LIBRARY_ENTITY) continue;
				String semanticId = declaration.get("semantic_id").getAsString();
				declarations.putIfAbsent(semanticId, new SourceCandidate(
					semanticId,
					declaration.get("qualified_name").getAsString(),
					declaration.get("source_signature").getAsString(),
					sourceKind, declaration.getAsJsonArray("parameter_types").size(),
					sourceFile, declaration.get("line").getAsInt()));
			}
		}
		Set<String> established = new LinkedHashSet<>();
		for (SourceHints hints : functions.values()) {
			if (!hints.semanticId().isBlank()) established.add(hints.semanticId());
		}
		for (String semanticId : established) declarations.remove(semanticId);
		return new RecoverySourceIndex(target, functions,
			new ArrayList<>(declarations.values()));
	}

	private static boolean belongsToTarget(String sourceFile, String target) {
		String root = target.equals("SURRENDER") ? "surrender" : "wiz8";
		return sourceFile.startsWith("src/" + root + "/") ||
			sourceFile.startsWith("include/" + root + "/");
	}

	private static SourceHints sourceHints(JsonObject marker) {
		if (marker == null || !marker.has("declaration") || marker.get("declaration").isJsonNull()) {
			return SourceHints.NONE;
		}
		String markerKind = marker.has("marker_kind")
			? marker.get("marker_kind").getAsString() : "UNKNOWN";
		JsonObject declaration = marker.getAsJsonObject("declaration");
		String sourceKind = sourceKind(markerKind, declaration).name();
		String signature = declaration.has("source_signature")
			? declaration.get("source_signature").getAsString() : "";
		String semantic = declaration.has("semantic_id")
			? declaration.get("semantic_id").getAsString() : "";
		String sourceFile = marker.has("source_file")
			? marker.get("source_file").getAsString() : "";
		List<String> references = new ArrayList<>();
		if (declaration.has("parameter_reference_forms")) {
			JsonArray forms = declaration.getAsJsonArray("parameter_reference_forms");
			if (!forms.isEmpty()) {
				for (int i = 0; i < forms.size(); i++) {
					JsonElement form = forms.get(i);
					String kind = form.isJsonObject() && form.getAsJsonObject().has("kind")
						? form.getAsJsonObject().get("kind").getAsString() : "value";
					references.add(kind);
				}
				return SourceHints.of(markerKind, sourceKind, signature, semantic,
					references.toArray(String[]::new), sourceFile);
			}
		}
		int count = declaration.has("parameter_references")
			? declaration.getAsJsonArray("parameter_references").size() : 0;
		for (int i = 0; i < count; i++) references.add("value");
		return SourceHints.of(markerKind, sourceKind, signature, semantic,
			references.toArray(String[]::new), sourceFile);
	}

	private static SourceKind sourceKind(String markerKind, JsonObject declaration) {
		String semanticKind = declaration.has("semantic_kind")
			? declaration.get("semantic_kind").getAsString() : "";
		if (markerKind.equals("TEMPLATE")) return SourceKind.TEMPLATE_MEMBER;
		if (markerKind.equals("LIBRARY")) return SourceKind.LIBRARY_ENTITY;
		if (semanticKind.equals("constructor")) return SourceKind.CONSTRUCTOR;
		if (semanticKind.equals("destructor")) return SourceKind.DESTRUCTOR;
		if (declaration.has("owning_class") &&
			!declaration.get("owning_class").isJsonNull()) return SourceKind.MEMBER_FUNCTION;
		return SourceKind.FREE_FUNCTION;
	}

	private JsonObject result(long[] entries, RecoveryEngine.FunctionExport[] packets,
			RecoverySourceIndex source,
			boolean explain, boolean includeBody) {
		JsonObject result = new JsonObject();
		result.addProperty("program", currentProgram.getName());
		JsonArray exports = new JsonArray();
		for (int i = 0; i < packets.length; i++) {
			RecoveryEngine.FunctionExport packet = packets[i];
			JsonObject item = packet(entries[i], packet, explain, includeBody);
			Function listed = currentProgram.getFunctionManager().getFunctionAt(
				currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(entries[i]));
			if (listed != null) {
				item.addProperty("name", listed.getName(true));
				item.addProperty("namespace", listed.getParentNamespace().getName(true));
				JsonArray candidates = new JsonArray();
				SourceKind kind = SourceKind.valueOf(packet.getSourceKind().toUpperCase());
				for (SourceCandidate candidate : source.candidates(listed, kind)) {
					JsonObject value = new JsonObject();
					value.addProperty("semantic_id", candidate.semanticId());
					value.addProperty("qualified_name", candidate.qualifiedName());
					value.addProperty("source_signature", candidate.sourceSignature());
					value.addProperty("source_file", candidate.sourceFile());
					value.addProperty("line", candidate.line());
					value.addProperty("evidence", "exact qualified name and parameter count");
					candidates.add(value);
				}
				item.add("source_candidates", candidates);
			}
			exports.add(item);
		}
		result.add("exports", exports);
		return result;
	}

	private static JsonObject packet(long entry, RecoveryEngine.FunctionExport packet,
			boolean explain, boolean includeBody) {
		JsonObject item = new JsonObject();
		item.addProperty("entry", address(entry));
		item.addProperty("generated_code", packet.getText());
		if (includeBody) item.addProperty("generated_body", packet.getBody());
		JsonObject recovery = new JsonObject();
		recovery.addProperty("source_kind", packet.getSourceKind());
		recovery.addProperty("source_entity", packet.getSourceEntity());
		recovery.addProperty("emission_kind", packet.getEmissionKind());
		recovery.add("body_owner", nullableAddress(packet.getBodyOwner()));
		recovery.add("canonical_target", nullableAddress(packet.getCanonicalTarget()));
		recovery.addProperty("origin", packet.getOrigin());
		recovery.addProperty("evidence", packet.getEvidence());
		JsonArray passes = new JsonArray();
		for (RecoveryEngine.PassFact fact : packet.getPasses()) {
			JsonObject value = new JsonObject();
			value.addProperty("status", fact.getStatus());
			value.addProperty("pass", fact.getPass());
			value.addProperty("detail", fact.getDetail());
			passes.add(value);
		}
		recovery.add("passes", passes);
		JsonArray defects = new JsonArray();
		for (String defect : packet.getDefects()) defects.add(defect);
		recovery.add("defects", defects);
		if (explain) {
			recovery.add("calls", calls(packet));
		}
		else {
			recovery.add("calls", new JsonArray());
		}
		item.add("recovery", recovery);
		return item;
	}

	private static JsonArray calls(RecoveryEngine.FunctionExport packet) {
		JsonArray calls = new JsonArray();
		for (RecoveryEngine.CallFact fact : packet.getCalls()) {
			JsonObject value = new JsonObject();
			value.add("site", nullableAddress(fact.getSite()));
			value.addProperty("referenced", address(fact.getReferenced()));
			value.addProperty("canonical", address(fact.getCanonical()));
			value.addProperty("referenced_name", fact.getReferencedName());
			value.addProperty("canonical_name", fact.getCanonicalName());
			value.addProperty("origin", fact.getOrigin());
			value.addProperty("thunk_kind", fact.getThunkKind());
			value.addProperty("this_adjustment", fact.getThisAdjustment());
			value.addProperty("return_adjustment", fact.getReturnAdjustment());
			value.addProperty("evidence", fact.getEvidence());
			calls.add(value);
		}
		return calls;
	}

	private static JsonElement nullableAddress(long value) {
		return value < 0 ? com.google.gson.JsonNull.INSTANCE
			: new com.google.gson.JsonPrimitive(address(value));
	}

	private static String address(long value) {
		return String.format("0x%08x", value);
	}

	private record Arguments(Path sourceIndex, String target, Path output, boolean explain, boolean includeBody,
			boolean allFunctions,
			List<String> selections) {
		static Arguments parse(String[] args) {
			Path sourceIndex = null;
			String target = null;
			Path output = null;
			boolean explain = false;
			boolean includeBody = false;
			boolean allFunctions = false;
			List<String> selections = new ArrayList<>();
			for (int i = 0; i < args.length; i++) {
				switch (args[i]) {
					case "--source-index" -> sourceIndex = Path.of(args[++i]);
					case "--target" -> target = args[++i];
					case "--output" -> output = Path.of(args[++i]);
					case "--explain" -> explain = true;
					case "--include-body" -> includeBody = true;
					case "--all-functions" -> allFunctions = true;
					default -> selections.add(args[i]);
				}
			}
			if (sourceIndex == null) throw new IllegalArgumentException("--source-index is required");
			if (target == null) throw new IllegalArgumentException("--target is required");
			if (output == null) throw new IllegalArgumentException("--output is required");
			if (selections.isEmpty() && !allFunctions) {
				throw new IllegalArgumentException("select at least one function");
			}
			return new Arguments(sourceIndex, target, output, explain, includeBody, allFunctions,
				List.copyOf(selections));
		}
	}
}
