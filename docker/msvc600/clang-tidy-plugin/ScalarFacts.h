#pragma once

#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/StringRef.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <vector>

namespace clang::tidy::wiz8::scalar {
struct SourcePoint {
    std::string file;
    unsigned line = 0;
    unsigned column = 0;

    explicit operator bool() const
    {
        return !file.empty() && line != 0;
    }
};

inline std::string repository_relative_path(const SourceManager& sources, SourceLocation location)
{
    location = sources.getExpansionLoc(location);
    std::string path = sources.getFilename(location).str();
    constexpr char repo_prefix[] = "/repo/";
    if (path.rfind(repo_prefix, 0) == 0) {
        path.erase(0, sizeof(repo_prefix) - 1);
    }
    return path;
}

inline SourcePoint source_point(const SourceManager& sources, SourceLocation location)
{
    location = sources.getExpansionLoc(location);
    if (location.isInvalid() || location.isMacroID() || sources.isInSystemHeader(location)) {
        return {};
    }
    SourcePoint point;
    point.file = repository_relative_path(sources, location);
    point.line = sources.getSpellingLineNumber(location);
    point.column = sources.getSpellingColumnNumber(location);
    return point;
}

inline QualType declaration_type(const NamedDecl* declaration)
{
    if (const auto* function = dyn_cast<FunctionDecl>(declaration)) {
        return function->getReturnType();
    }
    return cast<ValueDecl>(declaration)->getType();
}

// Fixed unsigned-byte fields have one indexed byte-domain owner. Other arrays
// remain outside scalar inference (strings, pointers, records and typedef APIs).
inline const ConstantArrayType* fixed_byte_array(const NamedDecl* declaration)
{
    const auto* field = dyn_cast_or_null<FieldDecl>(declaration);
    if (field == nullptr || field->isBitField() ||
        cast<RecordDecl>(field->getDeclContext())->isUnion())
        return nullptr;
    const auto* array = dyn_cast<ConstantArrayType>(field->getType().getTypePtr());
    if (array == nullptr || array->getSize().isZero())
        return nullptr;
    const auto* element = dyn_cast<BuiltinType>(array->getElementType().getTypePtr());
    return element != nullptr && element->getKind() == BuiltinType::UChar ? array : nullptr;
}

inline const NamedDecl* canonical_scalar_declaration(const NamedDecl* declaration)
{
    if (declaration == nullptr || declaration->isImplicit())
        return nullptr;
    if (!isa<ValueDecl>(declaration))
        return nullptr;
    const QualType type = declaration_type(declaration);
    // Uninstantiated member-pointer types have no concrete MSVC layout.
    // Asking ASTContext for their width can dereference a missing class.
    if (declaration->isInvalidDecl() || type->isDependentType() || type->isIncompleteType() ||
        (!type->isScalarType() && fixed_byte_array(declaration) == nullptr))
        return nullptr;
    if (const auto* parameter = dyn_cast<ParmVarDecl>(declaration)) {
        const auto* function = dyn_cast<FunctionDecl>(parameter->getDeclContext());
        if (function == nullptr || function->isVariadic() ||
            function->getTemplatedKind() != FunctionDecl::TK_NonTemplate)
            return nullptr;
        const auto* owner = function->getCanonicalDecl();
        const unsigned index = parameter->getFunctionScopeIndex();
        return index < owner->getNumParams() ? owner->getParamDecl(index) : nullptr;
    }
    if (const auto* function = dyn_cast<FunctionDecl>(declaration)) {
        if (function->isVariadic() || function->getTemplatedKind() != FunctionDecl::TK_NonTemplate)
            return nullptr;
        return function->getCanonicalDecl();
    }
    if (const auto* variable = dyn_cast<VarDecl>(declaration))
        return variable->getCanonicalDecl();
    if (isa<EnumConstantDecl>(declaration))
        return declaration;
    if (const auto* field = dyn_cast<FieldDecl>(declaration)) {
        const auto* record = cast<RecordDecl>(field->getDeclContext());
        if (field->isBitField() || record->isUnion())
            return nullptr;
        return field->getCanonicalDecl();
    }
    return nullptr;
}

inline bool is_plain_byte(const NamedDecl* declaration)
{
    if (fixed_byte_array(declaration) != nullptr)
        return true;
    const auto* builtin = dyn_cast<BuiltinType>(
        declaration_type(declaration).getCanonicalType().getUnqualifiedType().getTypePtr());
    if (builtin == nullptr)
        return false;
    switch (builtin->getKind()) {
    case BuiltinType::Char_S:
    case BuiltinType::Char_U:
    case BuiltinType::SChar:
    case BuiltinType::UChar:
        return true;
    default:
        return false;
    }
}
inline std::string candidate_kind(const NamedDecl* declaration)
{
    if (isa<FunctionDecl>(declaration)) {
        return "function";
    }
    if (isa<FieldDecl>(declaration)) {
        return "field";
    }
    if (isa<ParmVarDecl>(declaration)) {
        return "parameter";
    }
    return "variable";
}

inline std::string candidate_name(const NamedDecl* declaration)
{
    const auto* parameter = dyn_cast<ParmVarDecl>(declaration);
    if (parameter == nullptr) {
        return declaration->getNameAsString();
    }
    // Canonical declarations may omit parameter names; prefer any redeclaration
    // that spells one so the inventory stays readable.
    const auto* function = cast<FunctionDecl>(parameter->getDeclContext());
    const unsigned index = parameter->getFunctionScopeIndex();
    std::string name = parameter->getNameAsString();
    for (const FunctionDecl* redeclaration : function->redecls()) {
        if (!name.empty()) {
            break;
        }
        if (index < redeclaration->getNumParams()) {
            name = redeclaration->getParamDecl(index)->getNameAsString();
        }
    }
    if (name.empty()) {
        name = "#" + std::to_string(index);
    }
    return function->getQualifiedNameAsString() + "::" + name;
}

// Redeclarations visible to a TU may spell different parameter names, so the
// cross-TU identity of a parameter uses only its function and position.
inline std::string candidate_key_name(const NamedDecl* declaration)
{
    const auto* parameter = dyn_cast<ParmVarDecl>(declaration);
    if (parameter == nullptr) {
        return declaration->getNameAsString();
    }
    const auto* function = cast<FunctionDecl>(parameter->getDeclContext());
    return function->getQualifiedNameAsString() + "::#" +
           std::to_string(parameter->getFunctionScopeIndex());
}

inline std::string observed_domain(QualType type)
{
    return type->isBooleanType()                                  ? "bool"
           : type->isEnumeralType()                               ? "enum"
           : type->isCharType()                                   ? "character"
           : type->isPointerType() || type->isMemberPointerType() ? "pointer"
           : type->isFloatingType()                               ? "floating"
           : type->isIntegerType()                                ? "integer"
                                                                  : "opaque";
}

inline std::string observed_signedness(QualType type)
{
    return !type->isIntegerType() || type->isBooleanType() ? "irrelevant"
           : type->isUnsignedIntegerType()                 ? "unsigned"
                                                           : "signed";
}

class FactWriter {
public:
    FactWriter(ASTContext& context, const TranslationUnitDecl* translation_unit)
        : context_(context), sources_(context.getSourceManager())
    {
        const char* directory = std::getenv("WIZ8_SCALAR_FACTS_DIR");
        if (directory == nullptr || directory[0] == '\0' || translation_unit == nullptr) {
            return;
        }
        const std::string filename = std::string(directory) + "/facts-" +
                                     std::to_string(static_cast<long long>(getpid())) + ".tsv";
        stream_.open(filename, std::ios::out | std::ios::app);
        const SourcePoint main =
            source_point(sources_, sources_.getLocForStartOfFile(sources_.getMainFileID()));
        if (main)
            emit({"M", main.file});
    }

    bool enabled() const
    {
        return stream_.is_open();
    }

    std::string key(const NamedDecl* declaration) const
    {
        const NamedDecl* canonical_decl = canonical_scalar_declaration(declaration);
        if (canonical_decl == nullptr) {
            return {};
        }
        const SourcePoint point = source_point(sources_, canonical_decl->getLocation());
        if (!point) {
            return {};
        }
        return point.file + ":" + std::to_string(point.line) + ":" + std::to_string(point.column) +
               ":" + candidate_kind(canonical_decl) + ":" + candidate_key_name(canonical_decl);
    }

    void declaration(const NamedDecl* declaration, bool bool_name = false)
    {
        const NamedDecl* canonical_decl = canonical_scalar_declaration(declaration);
        if (canonical_decl == nullptr) {
            return;
        }
        const SourcePoint point = source_point(sources_, canonical_decl->getLocation());
        const std::string declaration_key = key(canonical_decl);
        if (!point || declaration_key.empty()) {
            return;
        }
        const auto previous = declarations_.find(declaration_key);
        if (previous != declarations_.end() && (previous->second || !bool_name)) {
            return;
        }
        declarations_[declaration_key] = bool_name;
        emit({"D", declaration_key, point.file, std::to_string(point.line),
              std::to_string(point.column), candidate_kind(canonical_decl),
              candidate_name(canonical_decl), bool_name ? "1" : "0",
              is_plain_byte(canonical_decl) ? "1" : "0",
              std::to_string(context_.getTypeSize(declaration_type(canonical_decl))),
              observed_signedness(declaration_type(canonical_decl)),
              observed_domain(declaration_type(canonical_decl)),
              declaration_type(canonical_decl).getAsString()});
    }

    void fact(const std::vector<std::string>& fields)
    {
        emit(fields);
    }

private:
    void emit(const std::vector<std::string>& fields)
    {
        if (!stream_) {
            return;
        }
        for (size_t index = 0; index < fields.size(); ++index) {
            if (index != 0) {
                stream_ << '\t';
            }
            // Type spellings and source paths may contain TSV delimiters.
            for (char character : fields[index]) {
                switch (character) {
                case '\\':
                    stream_ << "\\\\";
                    break;
                case '\t':
                    stream_ << "\\t";
                    break;
                case '\n':
                    stream_ << "\\n";
                    break;
                case '\r':
                    stream_ << "\\r";
                    break;
                default:
                    stream_ << character;
                    break;
                }
            }
        }
        stream_ << '\n';
    }

    ASTContext& context_;
    SourceManager& sources_;
    std::ofstream stream_;
    std::unordered_map<std::string, bool> declarations_;
};

void collect_scalar_facts(ASTContext& context, const TranslationUnitDecl* translation_unit,
                          FactWriter& writer);

} // namespace clang::tidy::wiz8::scalar
