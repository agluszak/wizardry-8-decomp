// Emit the source-index records for one translation unit by walking libclang
// cursors, instead of serialising the whole Clang AST to JSON and rebuilding it
// in Python.
//
// The measurement that motivates this: clang's own `-ast-dump=json` costs under
// 0.4s for a unit here, but produces ~32 MB, and the 147 units together came to
// 3.7 GB. Loading that back cost ~23s of json.loads and ~35s of tree walking -
// roughly fifty times what compiling the same sources costs. Almost all of it is
// declarations from VC6, Windows and CRT headers that the index discards: on one
// unit, 85 of 492 top-level declarations carried 84% of the bytes.
//
// Walking cursors skips the serialisation entirely, filters by file before any
// work is done, and skips function bodies at parse time because the index binds
// markers to declarations and never looks at a statement.
//
// Output is one JSON object per line: `{"record":"declaration",...}` or
// `{"record":"class",...}`, with the same fields the JSON path produced.

#include <clang-c/Index.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

std::string take(CXString value) {
    const char* text = clang_getCString(value);
    std::string result = text ? text : "";
    clang_disposeString(value);
    return result;
}

// The repository is mounted at /repo inside the container; everything else is
// toolchain and is not part of the index.
bool inRepository(const std::string& path) { return path.compare(0, 6, "/repo/") == 0; }

std::string relative(const std::string& path) {
    return inRepository(path) ? path.substr(6) : path;
}

struct Location {
    std::string file;
    unsigned line = 0;
    unsigned endLine = 0;
};

Location locate(CXCursor cursor) {
    Location location;
    CXSourceRange extent = clang_getCursorExtent(cursor);
    CXFile file;
    unsigned line = 0, column = 0, offset = 0;
    clang_getFileLocation(clang_getRangeStart(extent), &file, &line, &column, &offset);
    if (file) location.file = take(clang_getFileName(file));
    location.line = line;
    clang_getFileLocation(clang_getRangeEnd(extent), &file, &line, &column, &offset);
    location.endLine = line;
    return location;
}

// Qualified name, built from the semantic parents so it matches the spelling the
// index uses for nested and templated entities.
std::string qualified(CXCursor cursor) {
    std::vector<std::string> parts;
    for (CXCursor node = cursor; !clang_Cursor_isNull(node); node = clang_getCursorSemanticParent(node)) {
        CXCursorKind kind = clang_getCursorKind(node);
        if (kind == CXCursor_TranslationUnit) break;
        std::string name = take(clang_getCursorSpelling(node));
        if (name.empty()) break;
        if (node.kind != cursor.kind || parts.empty()) parts.insert(parts.begin(), name);
        else parts.insert(parts.begin(), name);
    }
    std::string result;
    for (size_t index = 0; index < parts.size(); ++index) {
        if (index) result += "::";
        result += parts[index];
    }
    return result;
}

std::string escape(const std::string& value) {
    std::string result;
    for (char character : value) {
        switch (character) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            default:
                if (static_cast<unsigned char>(character) < 0x20) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof buffer, "\\u%04x", character);
                    result += buffer;
                } else {
                    result += character;
                }
        }
    }
    return result;
}

void field(std::string& out, const char* name, const std::string& value, bool first = false) {
    if (!first) out += ",";
    out += "\"";
    out += name;
    out += "\":\"";
    out += escape(value);
    out += "\"";
}

void rawField(std::string& out, const char* name, const std::string& value) {
    out += ",\"";
    out += name;
    out += "\":";
    out += value;
}

const char* conventionName(CXCallingConv convention) {
    switch (convention) {
        case CXCallingConv_C: return "__cdecl";
        case CXCallingConv_X86StdCall: return "__stdcall";
        case CXCallingConv_X86FastCall: return "__fastcall";
        case CXCallingConv_X86ThisCall: return "__thiscall";
        default: return "";
    }
}

const char* declarationKind(CXCursorKind kind) {
    switch (kind) {
        case CXCursor_Constructor: return "constructor";
        case CXCursor_Destructor: return "destructor";
        case CXCursor_CXXMethod: return "method";
        case CXCursor_FunctionDecl: return "function";
        case CXCursor_FunctionTemplate: return "function-template";
        default: return "";
    }
}

void emitDeclaration(CXCursor cursor, const Location& location) {
    CXType type = clang_getCursorType(cursor);
    std::string out = "{\"record\":\"declaration\"";
    field(out, "semantic_id", take(clang_Cursor_getMangling(cursor)));
    field(out, "qualified_name", qualified(cursor));
    field(out, "semantic_kind", declarationKind(clang_getCursorKind(cursor)));
    field(out, "calling_convention", conventionName(clang_getFunctionTypeCallingConv(type)));
    field(out, "return_type", take(clang_getTypeSpelling(clang_getResultType(type))));

    std::string parameters = "[";
    int count = clang_getNumArgTypes(type);
    for (int index = 0; index < count; ++index) {
        if (index) parameters += ",";
        parameters += "\"" + escape(take(clang_getTypeSpelling(clang_getArgType(type, index)))) + "\"";
    }
    parameters += "]";
    rawField(out, "parameter_types", parameters);

    CXCursor parent = clang_getCursorSemanticParent(cursor);
    bool isMember = !clang_Cursor_isNull(parent) &&
                    (parent.kind == CXCursor_ClassDecl || parent.kind == CXCursor_StructDecl);
    field(out, "owning_class", isMember ? qualified(parent) : "");
    rawField(out, "has_this",
             (isMember && !clang_CXXMethod_isStatic(cursor)) ? "true" : "false");
    rawField(out, "is_virtual", clang_CXXMethod_isVirtual(cursor) ? "true" : "false");
    field(out, "source_file", relative(location.file));
    rawField(out, "line", std::to_string(location.line));
    rawField(out, "end_line", std::to_string(location.endLine));
    rawField(out, "is_definition", clang_isCursorDefinition(cursor) ? "true" : "false");
    out += "}";
    std::printf("%s\n", out.c_str());
}

struct ClassVisitState {
    std::string bases;
    std::string fields;
    std::string virtuals;
};

CXChildVisitResult visitClassMember(CXCursor cursor, CXCursor, CXClientData data) {
    ClassVisitState& state = *static_cast<ClassVisitState*>(data);
    switch (clang_getCursorKind(cursor)) {
        case CXCursor_CXXBaseSpecifier: {
            if (!state.bases.empty()) state.bases += ",";
            state.bases += "\"" + escape(take(clang_getTypeSpelling(clang_getCursorType(cursor)))) + "\"";
            break;
        }
        case CXCursor_FieldDecl: {
            Location where = locate(cursor);
            if (!state.fields.empty()) state.fields += ",";
            state.fields += "{\"name\":\"" + escape(take(clang_getCursorSpelling(cursor))) +
                            "\",\"type\":\"" +
                            escape(take(clang_getTypeSpelling(clang_getCursorType(cursor)))) +
                            "\",\"source_file\":\"" + escape(relative(where.file)) +
                            "\",\"line\":" + std::to_string(where.line) + "}";
            break;
        }
        case CXCursor_CXXMethod:
        case CXCursor_Destructor: {
            if (clang_CXXMethod_isVirtual(cursor)) {
                if (!state.virtuals.empty()) state.virtuals += ",";
                state.virtuals += "\"" + escape(take(clang_Cursor_getMangling(cursor))) + "\"";
            }
            break;
        }
        default:
            break;
    }
    return CXChildVisit_Continue;
}

void emitClass(CXCursor cursor, const Location& location) {
    ClassVisitState state;
    clang_visitChildren(cursor, visitClassMember, &state);
    std::string out = "{\"record\":\"class\"";
    field(out, "semantic_id", "record:" + qualified(cursor));
    field(out, "qualified_name", qualified(cursor));
    rawField(out, "bases", "[" + state.bases + "]");
    rawField(out, "fields", "[" + state.fields + "]");
    rawField(out, "virtual_declarations", "[" + state.virtuals + "]");
    field(out, "source_file", relative(location.file));
    rawField(out, "line", std::to_string(location.line));
    rawField(out, "end_line", std::to_string(location.endLine));
    out += "}";
    std::printf("%s\n", out.c_str());
}

CXChildVisitResult visit(CXCursor cursor, CXCursor, CXClientData) {
    Location location = locate(cursor);
    // Filtering here is the whole point: a toolchain header costs one string
    // compare rather than megabytes of serialised nodes.
    if (location.file.empty() || !inRepository(location.file)) return CXChildVisit_Continue;

    switch (clang_getCursorKind(cursor)) {
        case CXCursor_FunctionDecl:
        case CXCursor_CXXMethod:
        case CXCursor_Constructor:
        case CXCursor_Destructor:
            emitDeclaration(cursor, location);
            return CXChildVisit_Continue;
        case CXCursor_ClassDecl:
        case CXCursor_StructDecl:
            if (clang_isCursorDefinition(cursor)) emitClass(cursor, location);
            return CXChildVisit_Recurse;
        case CXCursor_Namespace:
            return CXChildVisit_Recurse;
        default:
            return CXChildVisit_Recurse;
    }
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: indexer <clang arguments...>\n");
        return 2;
    }
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = nullptr;
    // Function bodies are never read by the index, and skipping them is the one
    // saving that happens inside the parser rather than after it.
    unsigned options = CXTranslationUnit_SkipFunctionBodies |
                       CXTranslationUnit_DetailedPreprocessingRecord;
    // FullArgv, because the compile database records clang-cl command lines and
    // the driver has to be argv[0] for cl mode to be selected the same way the
    // real build selects it.
    CXErrorCode status = clang_parseTranslationUnit2FullArgv(index, nullptr, argv + 1, argc - 1,
                                                             nullptr, 0, options, &unit);
    if (status != CXError_Success || !unit) {
        std::fprintf(stderr, "indexer: clang failed to parse (%d)\n", status);
        return 1;
    }
    unsigned diagnostics = clang_getNumDiagnostics(unit);
    for (unsigned index_ = 0; index_ < diagnostics; ++index_) {
        CXDiagnostic diagnostic = clang_getDiagnostic(unit, index_);
        if (clang_getDiagnosticSeverity(diagnostic) >= CXDiagnostic_Error) {
            std::fprintf(stderr, "%s\n",
                         take(clang_formatDiagnostic(diagnostic, CXDiagnostic_DisplaySourceLocation))
                             .c_str());
        }
        clang_disposeDiagnostic(diagnostic);
    }
    clang_visitChildren(clang_getTranslationUnitCursor(unit), visit, nullptr);
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
    return 0;
}
