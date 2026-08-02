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

public class Wiz8Recover extends GhidraScript {
	@Override
	protected void run() throws Exception {
		Arguments arguments = Arguments.parse(getScriptArgs());
		Map<Long, JsonObject> source = sourceMarkers(arguments.sourceIndex());
		long[] entries = arguments.allFunctions()
			? allFunctions() : resolveEntries(arguments.selections());
		String[][] sourceHints = new String[entries.length][];
		for (int i = 0; i < entries.length; i++) {
			sourceHints[i] = sourceHints(source.get(entries[i]));
		}
		RecoveryEngine.FunctionExport[] packets = RecoveryEngine.exportFunctionPackets(
			currentProgram, entries, sourceHints, arguments.explain(), monitor);
		JsonObject result = result(entries, packets, arguments.explain());
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

	private static Map<Long, JsonObject> sourceMarkers(Path path) throws Exception {
		Map<Long, JsonObject> markers = new LinkedHashMap<>();
		try (var reader = Files.newBufferedReader(path, StandardCharsets.UTF_8)) {
			JsonArray array = JsonParser.parseReader(reader).getAsJsonObject()
				.getAsJsonArray("markers");
			for (JsonElement element : array) {
				JsonObject marker = element.getAsJsonObject();
				markers.put(marker.get("address").getAsLong(), marker);
			}
		}
		return markers;
	}

	private static String[] sourceHints(JsonObject marker) {
		if (marker == null || !marker.has("declaration") || marker.get("declaration").isJsonNull()) {
			return new String[0];
		}
		List<String> hints = new ArrayList<>();
		String markerKind = marker.has("marker_kind")
			? marker.get("marker_kind").getAsString() : "UNKNOWN";
		hints.add("marker=" + markerKind);
		JsonObject declaration = marker.getAsJsonObject("declaration");
		String semanticKind = declaration.has("semantic_kind")
			? declaration.get("semantic_kind").getAsString() : "";
		String sourceKind;
		if (markerKind.equals("TEMPLATE")) sourceKind = "TEMPLATE_MEMBER";
		else if (markerKind.equals("LIBRARY")) sourceKind = "LIBRARY_ENTITY";
		else if (semanticKind.equals("constructor")) sourceKind = "CONSTRUCTOR";
		else if (semanticKind.equals("destructor")) sourceKind = "DESTRUCTOR";
		else if (declaration.has("owning_class") &&
				!declaration.get("owning_class").isJsonNull()) sourceKind = "MEMBER_FUNCTION";
		else sourceKind = "FREE_FUNCTION";
		hints.add("source=" + sourceKind);
		if (declaration.has("source_signature")) {
			hints.add("signature=" + declaration.get("source_signature").getAsString());
		}
		if (declaration.has("semantic_id")) {
			hints.add("semantic=" + declaration.get("semantic_id").getAsString());
		}
		if (marker.has("source_file")) {
			hints.add("file=" + marker.get("source_file").getAsString());
		}
		if (declaration.has("parameter_reference_forms")) {
			JsonArray forms = declaration.getAsJsonArray("parameter_reference_forms");
			if (!forms.isEmpty()) {
				for (int i = 0; i < forms.size(); i++) {
					JsonElement form = forms.get(i);
					String kind = form.isJsonObject() && form.getAsJsonObject().has("kind")
						? form.getAsJsonObject().get("kind").getAsString() : "value";
					hints.add("reference=" + kind);
				}
				return hints.toArray(String[]::new);
			}
		}
		int count = declaration.has("parameter_references")
			? declaration.getAsJsonArray("parameter_references").size() : 0;
		for (int i = 0; i < count; i++) hints.add("reference=value");
		return hints.toArray(String[]::new);
	}

	private JsonObject result(long[] entries, RecoveryEngine.FunctionExport[] packets,
			boolean explain) {
		JsonObject result = new JsonObject();
		result.addProperty("program", currentProgram.getName());
		JsonArray functions = new JsonArray();
		JsonArray exports = new JsonArray();
		StringBuilder text = new StringBuilder();
		for (int i = 0; i < packets.length; i++) {
			RecoveryEngine.FunctionExport packet = packets[i];
			JsonObject item = packet(entries[i], packet, explain);
			Function listed = currentProgram.getFunctionManager().getFunctionAt(
				currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(entries[i]));
			if (listed != null) {
				item.addProperty("name", listed.getName(true));
				item.addProperty("namespace", listed.getParentNamespace().getName(true));
			}
			exports.add(item);
			JsonObject function = new JsonObject();
			function.addProperty("entry", address(entries[i]));
			function.addProperty("kind", packet.getEmissionKind());
			functions.add(function);
			if (text.length() > 0) text.append('\n');
			text.append(packet.getText());
		}
		result.add("functions", functions);
		result.add("exports", exports);
		result.addProperty("text", text.toString());
		return result;
	}

	private static JsonObject packet(long entry, RecoveryEngine.FunctionExport packet,
			boolean explain) {
		JsonObject item = new JsonObject();
		item.addProperty("entry", address(entry));
		item.addProperty("text", packet.getText());
		item.addProperty("body", packet.getBody());
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

	private record Arguments(Path sourceIndex, Path output, boolean explain, boolean allFunctions,
			List<String> selections) {
		static Arguments parse(String[] args) {
			Path sourceIndex = null;
			Path output = null;
			boolean explain = false;
			boolean allFunctions = false;
			List<String> selections = new ArrayList<>();
			for (int i = 0; i < args.length; i++) {
				switch (args[i]) {
					case "--source-index" -> sourceIndex = Path.of(args[++i]);
					case "--output" -> output = Path.of(args[++i]);
					case "--explain" -> explain = true;
					case "--all-functions" -> allFunctions = true;
					default -> selections.add(args[i]);
				}
			}
			if (sourceIndex == null) throw new IllegalArgumentException("--source-index is required");
			if (output == null) throw new IllegalArgumentException("--output is required");
			if (selections.isEmpty() && !allFunctions) {
				throw new IllegalArgumentException("select at least one function");
			}
			return new Arguments(sourceIndex, output, explain, allFunctions,
				List.copyOf(selections));
		}
	}
}
