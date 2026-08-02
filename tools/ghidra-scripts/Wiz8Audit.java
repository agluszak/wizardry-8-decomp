// Run focused read-only audits against live Ghidra objects.
// @category Wizardry8

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.data.Pointer;
import ghidra.program.model.data.Structure;
import ghidra.program.model.data.TypeDef;
import ghidra.program.model.listing.Function;

public class Wiz8Audit extends GhidraScript {
	@Override
	protected void run() throws Exception {
		Arguments arguments = Arguments.parse(getScriptArgs());
		JsonObject result = switch (arguments.audit()) {
			case "source-layouts" -> sourceLayouts(arguments.sourceIndex());
			case "function-inventory" -> functionInventory();
			case "class-fields" -> classFields(arguments.classes());
			case "function-exists" -> functionExists(arguments.entries());
			case "lifecycle-symbols" -> lifecycleSymbols(arguments.classes());
			default -> throw new IllegalArgumentException("unknown audit " + arguments.audit());
		};
		Path parent = arguments.output().toAbsolutePath().getParent();
		if (parent != null) Files.createDirectories(parent);
		String json = new GsonBuilder().setPrettyPrinting().disableHtmlEscaping().create()
			.toJson(result);
		Files.writeString(arguments.output(), json + "\n", StandardCharsets.UTF_8);
		println(arguments.output().toAbsolutePath().toString());
	}

	private JsonObject functionInventory() {
		JsonArray functions = new JsonArray();
		for (Function function : currentProgram.getFunctionManager().getFunctions(true)) {
			if (function.isExternal()) continue;
			JsonObject item = new JsonObject();
			item.addProperty("entry", address(function.getEntryPoint().getOffset()));
			item.addProperty("name", function.getName(true));
			functions.add(item);
		}
		JsonObject result = new JsonObject();
		result.addProperty("schema", "wiz8.function-inventory");
		result.add("functions", functions);
		return result;
	}

	private JsonObject classFields(List<String> names) {
		Map<String, DataType> dataTypes = dataTypes();
		JsonArray classes = new JsonArray();
		for (String name : names) {
			JsonObject item = new JsonObject();
			item.addProperty("name", name);
			JsonArray fields = new JsonArray();
			Structure structure = structure(dataTypes.get("/wiz8/classes/" + name));
			if (structure != null) {
				for (DataTypeComponent component : structure.getDefinedComponents()) {
					if (component.getFieldName() == null) continue;
					JsonObject field = new JsonObject();
					field.addProperty("field", component.getFieldName());
					field.addProperty("offset", component.getOffset());
					field.addProperty("length", component.getLength());
					field.addProperty("type", component.getDataType().getDisplayName());
					fields.add(field);
				}
			}
			item.add("fields", fields);
			classes.add(item);
		}
		JsonObject result = new JsonObject();
		result.addProperty("schema", "wiz8.class-fields");
		result.add("classes", classes);
		return result;
	}

	private JsonObject functionExists(List<Long> entries) {
		JsonArray missing = new JsonArray();
		var space = currentProgram.getAddressFactory().getDefaultAddressSpace();
		for (long entry : entries) {
			if (currentProgram.getFunctionManager().getFunctionAt(space.getAddress(entry)) == null) {
				missing.add(address(entry));
			}
		}
		JsonObject result = new JsonObject();
		result.addProperty("schema", "wiz8.function-existence-audit");
		result.addProperty("ok", missing.isEmpty());
		result.add("missing", missing);
		return result;
	}

	private JsonObject lifecycleSymbols(List<String> classes) {
		JsonArray vtables = new JsonArray();
		var symbols = currentProgram.getSymbolTable().getAllSymbols(true);
		while (symbols.hasNext()) {
			String name = symbols.next().getName(true);
			if (!name.toLowerCase().contains("vftable")) continue;
			if (classes.stream().noneMatch(value -> name.contains(value + "::"))) continue;
			vtables.add(name);
		}
		JsonObject result = new JsonObject();
		result.addProperty("schema", "wiz8.lifecycle-symbols");
		result.add("vtables", vtables);
		return result;
	}

	private JsonObject sourceLayouts(Path sourceIndex) throws Exception {
		JsonArray failures = new JsonArray();
		JsonObject checks = new JsonObject();
		int classes = 0;
		int sourceFields = 0;
		int fields = 0;
		int bases = 0;
		Map<String, DataType> dataTypes = dataTypes();
		JsonArray sourceClasses;
		try (var reader = Files.newBufferedReader(sourceIndex, StandardCharsets.UTF_8)) {
			sourceClasses = JsonParser.parseReader(reader).getAsJsonObject().getAsJsonArray("classes");
		}
		int layoutOwned = 0;
		for (JsonElement element : sourceClasses) {
			JsonObject sourceClass = element.getAsJsonObject();
			if (!sourceClass.has("asserted_size") || sourceClass.get("asserted_size").isJsonNull()) {
				continue;
			}
			String sourceFile = string(sourceClass, "source_file");
			if (!sourceFile.startsWith("src/wiz8/") && !sourceFile.startsWith("include/wiz8/")) {
				continue;
			}
			layoutOwned++;
			String name = sourceClass.get("qualified_name").getAsString();
			Structure rebuilt = structure(dataTypes.get("/" + name));
			if (rebuilt == null) {
				failures.add(failure("missing-pdb-class", name, null));
				continue;
			}
			classes++;
			int expectedSize = sourceClass.get("asserted_size").getAsInt();
			if (rebuilt.getLength() != expectedSize) {
				JsonObject failure = failure("size", name, null);
				failure.addProperty("expected", expectedSize);
				failure.addProperty("actual", rebuilt.getLength());
				failures.add(failure);
			}

			Set<String> actualBases = new HashSet<>();
			for (DataTypeComponent component : rebuilt.getDefinedComponents()) {
				if (isBase(component)) actualBases.add(baseTypeName(component.getDataType()));
			}
			for (JsonElement base : sourceClass.getAsJsonArray("bases")) {
				bases++;
				String expected = base.getAsString();
				if (!actualBases.contains(expected)) {
					JsonObject failure = failure("base", name, null);
					failure.addProperty("expected", expected);
					JsonArray actual = new JsonArray();
					actualBases.stream().sorted().forEach(actual::add);
					failure.add("actual", actual);
					failures.add(failure);
				}
			}

			Map<String, DataTypeComponent> rebuiltFields = new HashMap<>();
			for (DataTypeComponent component : rebuilt.getDefinedComponents()) {
				String field = component.getFieldName();
				if (field != null && !field.equals("vftable") && !isBase(component)) {
					rebuiltFields.put(field, component);
				}
			}
			for (JsonElement fieldElement : sourceClass.getAsJsonArray("fields")) {
				sourceFields++;
				JsonObject sourceField = fieldElement.getAsJsonObject();
				String fieldName = sourceField.get("name").getAsString();
				DataTypeComponent actual = rebuiltFields.get(fieldName);
				if (actual == null) {
					failures.add(failure("missing-pdb-field", name, fieldName));
					continue;
				}
				int expectedDepth = sourcePointerDepth(sourceField.get("type").getAsString());
				int actualDepth = pointerDepth(actual.getDataType());
				if (expectedDepth > 0 && expectedDepth != actualDepth) {
					JsonObject failure = failure("source-field-pointer-depth", name, fieldName);
					failure.addProperty("expected", expectedDepth);
					failure.addProperty("actual", actualDepth);
					failures.add(failure);
				}
			}

			Structure original = structure(dataTypes.get("/wiz8/classes/" + name));
			if (original == null) {
				failures.add(failure("missing-ghidra-class", name, null));
				continue;
			}
			List<FieldSpan> sourceSpans = new ArrayList<>();
			flatten(rebuilt, 0, sourceSpans);
			for (DataTypeComponent component : original.getDefinedComponents()) {
				String fieldName = component.getFieldName();
				if (fieldName == null || fieldName.isBlank()) continue;
				int start = component.getOffset();
				int end = start + component.getLength();
				List<FieldSpan> covering = sourceSpans.stream()
					.filter(span -> span.offset() < end && span.end() > start).toList();
				Set<Integer> covered = new HashSet<>();
				for (FieldSpan span : covering) {
					for (int offset = Math.max(start, span.offset());
							offset < Math.min(end, span.end()); offset++) covered.add(offset);
				}
				if (covered.size() != component.getLength()) {
					failures.add(failure("missing-field", name, fieldName));
					continue;
				}
				fields++;
				if (fieldName.contains("vptr")) continue;
				List<FieldSpan> exact = covering.stream()
					.filter(span -> span.offset() == start && span.length() == component.getLength())
					.toList();
				int expectedDepth = pointerDepth(component.getDataType());
				int actualDepth = exact.isEmpty() ? 0 : pointerDepth(exact.get(0).type());
				if (expectedDepth > 0 && actualDepth > 0 && expectedDepth != actualDepth) {
					JsonObject failure = failure("field", name, fieldName);
					failure.addProperty("expected_pointer_depth", expectedDepth);
					JsonArray actualTypes = new JsonArray();
					exact.forEach(span -> actualTypes.add(span.type().getDisplayName()));
					failure.add("actual_types", actualTypes);
					failures.add(failure);
				}
			}
		}
		checks.addProperty("classes", classes);
		checks.addProperty("source_fields", sourceFields);
		checks.addProperty("fields", fields);
		checks.addProperty("bases", bases);
		JsonObject result = new JsonObject();
		result.addProperty("schema", "wiz8.source-layout-audit");
		result.addProperty("ok", failures.isEmpty());
		result.addProperty("layout_owned_classes", layoutOwned);
		result.add("checks", checks);
		result.addProperty("failure_count", failures.size());
		result.add("failures", failures);
		return result;
	}

	private Map<String, DataType> dataTypes() {
		Map<String, DataType> result = new HashMap<>();
		var iterator = currentProgram.getDataTypeManager().getAllDataTypes();
		while (iterator.hasNext()) {
			DataType dataType = iterator.next();
			result.put(dataType.getPathName(), dataType);
		}
		return result;
	}

	private static void flatten(Structure structure, int origin, List<FieldSpan> result) {
		for (DataTypeComponent component : structure.getDefinedComponents()) {
			int offset = origin + component.getOffset();
			Structure base = structure(component.getDataType());
			if (isBase(component) && base != null) flatten(base, offset, result);
			else result.add(new FieldSpan(offset, component.getLength(), component.getDataType()));
		}
	}

	private static Structure structure(DataType type) {
		DataType resolved = unwrap(type);
		return resolved instanceof Structure value ? value : null;
	}

	private static DataType unwrap(DataType type) {
		DataType current = type;
		while (current instanceof TypeDef typeDef) current = typeDef.getBaseDataType();
		return current;
	}

	private static int pointerDepth(DataType type) {
		int depth = 0;
		DataType current = unwrap(type);
		while (current instanceof Pointer pointer) {
			depth++;
			current = unwrap(pointer.getDataType());
		}
		return depth;
	}

	private static int sourcePointerDepth(String spelling) {
		int depth = 0;
		String current = spelling.stripTrailing();
		while (current.endsWith("*")) {
			depth++;
			current = current.substring(0, current.length() - 1).stripTrailing();
		}
		return depth;
	}

	private static boolean isBase(DataTypeComponent component) {
		String field = component.getFieldName();
		return field != null && (field.equals("base") || field.startsWith("base_"));
	}

	private static String baseTypeName(DataType type) {
		return unwrap(type).getDisplayName();
	}

	private static String string(JsonObject object, String property) {
		return object.has(property) && !object.get(property).isJsonNull()
			? object.get(property).getAsString() : "";
	}

	private static String address(long value) {
		return String.format("0x%08x", value);
	}

	private static JsonObject failure(String kind, String className, String field) {
		JsonObject result = new JsonObject();
		result.addProperty("kind", kind);
		result.addProperty("class", className);
		if (field != null) result.addProperty("field", field);
		return result;
	}

	private record FieldSpan(int offset, int length, DataType type) {
		int end() { return offset + length; }
	}

	private record Arguments(String audit, Path sourceIndex, Path output, List<String> classes,
			List<Long> entries) {
		static Arguments parse(String[] args) {
			String audit = null;
			Path sourceIndex = null;
			Path output = null;
			List<String> classes = new ArrayList<>();
			List<Long> entries = new ArrayList<>();
			for (int i = 0; i < args.length; i++) {
				switch (args[i]) {
					case "--audit" -> audit = args[++i];
					case "--source-index" -> sourceIndex = Path.of(args[++i]);
					case "--output" -> output = Path.of(args[++i]);
					case "--class" -> classes.add(args[++i]);
					case "--entry" -> entries.add(Long.decode(args[++i]));
					default -> throw new IllegalArgumentException("unknown argument " + args[i]);
				}
			}
			if (audit == null) throw new IllegalArgumentException("--audit is required");
			if (audit.equals("source-layouts") && sourceIndex == null) {
				throw new IllegalArgumentException("--source-index is required");
			}
			if (output == null) throw new IllegalArgumentException("--output is required");
			return new Arguments(audit, sourceIndex, output, List.copyOf(classes),
				List.copyOf(entries));
		}
	}
}
