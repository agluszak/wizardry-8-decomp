#include "clang-tidy/ClangTidyCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace clang::tidy::wiz8 {
namespace {

struct SourcePoint {
    std::string file;
    unsigned line = 0;
    unsigned column = 0;

    explicit operator bool() const { return !file.empty() && line != 0; }
};

static QualType canonical(QualType type)
{
    return type.getCanonicalType().getUnqualifiedType();
}

static bool is_byte_type(QualType type)
{
    // Infer only declarations written directly as character types. Typedef-backed
    // byte APIs such as SGP BOOLEAN carry source/ABI meaning of their own.
    const Type* source_type = type.getUnqualifiedType().getTypePtrOrNull();
    if (source_type == nullptr || isa<TypedefType>(source_type)) {
        return false;
    }
    const auto* builtin = dyn_cast<BuiltinType>(source_type);
    if (builtin == nullptr) {
        return false;
    }
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

static std::string repository_relative_path(const SourceManager& sources, SourceLocation location)
{
    location = sources.getExpansionLoc(location);
    std::string path = sources.getFilename(location).str();
    constexpr char repo_prefix[] = "/repo/";
    if (path.rfind(repo_prefix, 0) == 0) {
        path.erase(0, sizeof(repo_prefix) - 1);
    }
    return path;
}

static SourcePoint source_point(const SourceManager& sources, SourceLocation location)
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

static std::string normalized_words(llvm::StringRef name)
{
    std::string out;
    out.reserve(name.size() + 8);
    for (size_t index = 0; index < name.size(); ++index) {
        const unsigned char ch = static_cast<unsigned char>(name[index]);
        if (std::isupper(ch) && !out.empty() &&
            (std::islower(static_cast<unsigned char>(out.back())) ||
             std::isdigit(static_cast<unsigned char>(out.back())))) {
            out.push_back('_');
        }
        out.push_back(static_cast<char>(std::tolower(ch)));
    }
    while (out.rfind("m_", 0) == 0 || out.rfind("g_", 0) == 0 || out.rfind("s_", 0) == 0) {
        out.erase(0, 2);
    }

    const size_t underscore = out.rfind('_');
    if (underscore != std::string::npos && underscore + 1 < out.size()) {
        const std::string suffix = out.substr(underscore + 1);
        const bool offset_like =
            suffix.size() >= 3 && suffix.size() <= 8 &&
            std::any_of(suffix.begin(), suffix.end(), [](unsigned char value) {
                return std::isdigit(value);
            }) &&
            std::all_of(suffix.begin(), suffix.end(), [](unsigned char value) {
                return std::isxdigit(value);
            });
        if (offset_like) {
            out.erase(underscore);
        }
    }
    return out;
}

static bool bool_like_name(llvm::StringRef name)
{
    const std::string normalized = normalized_words(name);
    llvm::SmallVector<llvm::StringRef, 16> words;
    llvm::StringRef(normalized).split(words, '_', -1, false);
    if (words.empty()) {
        return false;
    }

    // A byte count/mask/id is not a bool merely because every recovered
    // producer currently happens to use 0 or 1 and consumers truth-test it.
    static constexpr llvm::StringLiteral non_boolean_words[] = {
        "bit", "bits", "byte", "bytes", "code", "count", "id", "index", "kind",
        "length", "mask", "mode", "num", "number", "offset", "size", "slot", "status",
        "type",
    };
    for (llvm::StringRef word : words) {
        for (llvm::StringRef non_boolean : non_boolean_words) {
            if (word == non_boolean) {
                return false;
            }
        }
    }

    static constexpr llvm::StringLiteral predicate_words[] = {
        "all", "allows", "any", "both", "can", "contains", "did", "does", "has", "is",
        "needs", "should", "supports", "uses", "wants", "was", "were", "will",
    };
    for (llvm::StringRef word : words) {
        for (llvm::StringRef predicate : predicate_words) {
            if (word == predicate) {
                return true;
            }
        }
    }

    static constexpr llvm::StringLiteral boolean_words[] = {
        "accepted", "active", "alive", "allowed", "available", "changed", "checked",
        "closed", "complete", "completed", "created", "dead", "dirty", "disabled", "done",
        "empty", "enabled", "failed", "finished", "focused", "found", "handled", "hidden",
        "hostile", "hovered", "initialized", "inside", "loaded", "locked", "muted", "occupied",
        "open", "outside", "pending", "prepared", "present", "pressed", "ready", "required",
        "running", "saved", "scrollable", "selected", "settled", "started", "success",
        "successful", "toggled", "valid", "visible", "writing",
    };
    for (llvm::StringRef word : words) {
        for (llvm::StringRef boolean_word : boolean_words) {
            if (word == boolean_word) {
                return true;
            }
        }
    }
    return false;
}

static bool bool_like_function_name(llvm::StringRef name)
{
    const std::string normalized = normalized_words(name);
    llvm::SmallVector<llvm::StringRef, 16> words;
    llvm::StringRef(normalized).split(words, '_', -1, false);
    if (words.empty() || !bool_like_name(name)) {
        return false;
    }

    // An action returning 0/1 is often an old byte status, not an authored
    // C++ predicate. Predicate/state evidence must precede the operation.
    // Thus CanOpenNpcDialogue remains eligible, while OpenRendererWindow,
    // MonsterReadAllCycles and LoadSavedLevelItems do not.
    static constexpr llvm::StringLiteral action_words[] = {
        "check", "clear", "create", "destroy", "init", "initialize", "load", "open",
        "read", "release", "remove", "resize", "save", "set", "take", "update", "write",
    };
    static constexpr llvm::StringLiteral evidence_words[] = {
        "accepted", "active", "alive", "all", "allowed", "allows", "any", "available",
        "both", "can", "changed", "checked", "closed", "complete", "completed", "contains",
        "dead", "did", "dirty", "disabled", "does", "done", "empty", "enabled", "failed",
        "finished", "focused", "found", "handled", "has", "hidden", "hostile", "hovered",
        "inside", "is", "loaded", "locked", "missing", "muted", "needs", "occupied", "outside",
        "pending", "prepared", "present", "pressed", "ready", "required", "running", "saved",
        "scrollable", "selected", "settled", "should", "started", "success", "successful",
        "supports", "toggled", "uses", "valid", "visible", "wants", "was", "were", "will",
        "writing",
    };

    size_t first_action = words.size();
    size_t first_evidence = words.size();
    for (size_t index = 0; index < words.size(); ++index) {
        for (llvm::StringRef action : action_words) {
            if (words[index] == action) {
                first_action = std::min(first_action, index);
            }
        }
        for (llvm::StringRef evidence : evidence_words) {
            if (words[index] == evidence) {
                first_evidence = std::min(first_evidence, index);
            }
        }
    }
    return first_evidence != words.size() &&
           (first_action == words.size() || first_evidence < first_action);
}

static bool bool_like_declaration_name(const NamedDecl* declaration)
{
    if (isa<FunctionDecl>(declaration)) {
        return bool_like_function_name(declaration->getName());
    }
    return bool_like_name(declaration->getName());
}

static bool is_candidate_variable(const ValueDecl* declaration)
{
    if (declaration == nullptr || declaration->isImplicit() || !is_byte_type(declaration->getType())) {
        return false;
    }
    if (isa<ParmVarDecl>(declaration)) {
        return false;
    }
    if (const auto* variable = dyn_cast<VarDecl>(declaration)) {
        return !variable->getType()->isArrayType() && !variable->getType()->isReferenceType();
    }
    if (const auto* field = dyn_cast<FieldDecl>(declaration)) {
        if (field->isBitField() || field->getType()->isArrayType()) {
            return false;
        }
        const auto* record = dyn_cast<RecordDecl>(field->getDeclContext());
        return record == nullptr || !record->isUnion();
    }
    return false;
}

static bool is_candidate_function(const FunctionDecl* function)
{
    return function != nullptr && !function->isImplicit() && !function->isVariadic() &&
           function->getTemplatedKind() == FunctionDecl::TK_NonTemplate &&
           is_byte_type(function->getReturnType());
}

static const NamedDecl* canonical_candidate(const NamedDecl* declaration)
{
    if (const auto* variable = dyn_cast_or_null<VarDecl>(declaration)) {
        if (!is_candidate_variable(variable)) {
            return nullptr;
        }
        return variable->getCanonicalDecl();
    }
    if (const auto* field = dyn_cast_or_null<FieldDecl>(declaration)) {
        if (!is_candidate_variable(field)) {
            return nullptr;
        }
        return field->getCanonicalDecl();
    }
    if (const auto* function = dyn_cast_or_null<FunctionDecl>(declaration)) {
        if (!is_candidate_function(function)) {
            return nullptr;
        }
        return function->getCanonicalDecl();
    }
    return nullptr;
}

static std::string candidate_kind(const NamedDecl* declaration)
{
    if (isa<FunctionDecl>(declaration)) {
        return "function";
    }
    if (isa<FieldDecl>(declaration)) {
        return "field";
    }
    return "variable";
}

class FactWriter {
public:
    FactWriter(ASTContext& context, const TranslationUnitDecl* translation_unit)
        : sources_(context.getSourceManager())
    {
        const char* directory = std::getenv("WIZ8_BOOL_FACTS_DIR");
        if (directory == nullptr || directory[0] == '\0' || translation_unit == nullptr) {
            return;
        }
        const std::string filename =
            std::string(directory) + "/facts-" + std::to_string(static_cast<long long>(getpid())) +
            ".tsv";
        stream_.open(filename, std::ios::out | std::ios::app);
    }

    bool enabled() const { return stream_.is_open(); }

    std::string key(const NamedDecl* declaration) const
    {
        const NamedDecl* canonical_decl = canonical_candidate(declaration);
        if (canonical_decl == nullptr) {
            return {};
        }
        const SourcePoint point = source_point(sources_, canonical_decl->getLocation());
        if (!point) {
            return {};
        }
        return point.file + ":" + std::to_string(point.line) + ":" + std::to_string(point.column) +
               ":" + candidate_kind(canonical_decl) + ":" + canonical_decl->getNameAsString();
    }

    void declaration(const NamedDecl* declaration)
    {
        const NamedDecl* canonical_decl = canonical_candidate(declaration);
        if (canonical_decl == nullptr) {
            return;
        }
        const SourcePoint point = source_point(sources_, canonical_decl->getLocation());
        const std::string declaration_key = key(canonical_decl);
        if (!point || declaration_key.empty()) {
            return;
        }
        emit({"D", declaration_key, point.file, std::to_string(point.line),
              std::to_string(point.column), candidate_kind(canonical_decl),
              canonical_decl->getNameAsString(),
              bool_like_declaration_name(canonical_decl) ? "1" : "0"});
    }

    void write_direct(const NamedDecl* declaration, SourceLocation location)
    {
        write(declaration, {}, location);
    }

    void write(const NamedDecl* declaration, const std::vector<std::string>& dependencies,
               SourceLocation location)
    {
        const std::string declaration_key = key(declaration);
        const SourcePoint point = source_point(sources_, location);
        if (declaration_key.empty() || !point) {
            return;
        }
        std::string joined;
        for (const std::string& dependency : dependencies) {
            if (dependency.empty()) {
                continue;
            }
            if (!joined.empty()) {
                joined.push_back(',');
            }
            joined += dependency;
        }
        emit({"W", declaration_key, joined.empty() ? "D" : "R", joined, point.file,
              std::to_string(point.line), std::to_string(point.column)});
    }

    void invalid_write(const NamedDecl* declaration, SourceLocation location)
    {
        event("X", declaration, location);
    }

    void escape(const NamedDecl* declaration, SourceLocation location)
    {
        event("E", declaration, location);
    }

    void support(const std::string& declaration_key, SourceLocation location)
    {
        const SourcePoint point = source_point(sources_, location);
        if (declaration_key.empty() || !point) {
            return;
        }
        emit({"S", declaration_key, point.file, std::to_string(point.line),
              std::to_string(point.column)});
    }

private:
    void event(llvm::StringRef tag, const NamedDecl* declaration, SourceLocation location)
    {
        const std::string declaration_key = key(declaration);
        const SourcePoint point = source_point(sources_, location);
        if (declaration_key.empty() || !point) {
            return;
        }
        emit({tag.str(), declaration_key, point.file, std::to_string(point.line),
              std::to_string(point.column)});
    }

    void emit(const std::vector<std::string>& fields)
    {
        if (!stream_) {
            return;
        }
        for (size_t index = 0; index < fields.size(); ++index) {
            if (index != 0) {
                stream_ << '\t';
            }
            stream_ << fields[index];
        }
        stream_ << '\n';
    }

    SourceManager& sources_;
    std::ofstream stream_;
};

struct DomainExpr {
    bool possible = false;
    std::vector<std::string> dependencies;

    static DomainExpr direct()
    {
        DomainExpr result;
        result.possible = true;
        return result;
    }
};

static void append_dependencies(std::vector<std::string>& target,
                                const std::vector<std::string>& source)
{
    for (const std::string& dependency : source) {
        if (std::find(target.begin(), target.end(), dependency) == target.end()) {
            target.push_back(dependency);
        }
    }
}

class BoolFactVisitor final : public RecursiveASTVisitor<BoolFactVisitor> {
public:
    explicit BoolFactVisitor(FactWriter& writer)
        : writer_(writer)
    {
    }

    bool TraverseFunctionDecl(FunctionDecl* function)
    {
        FunctionDecl* previous = current_function_;
        current_function_ = function;
        const bool result = RecursiveASTVisitor<BoolFactVisitor>::TraverseFunctionDecl(function);
        current_function_ = previous;
        return result;
    }

    bool TraverseConstructorInitializer(CXXCtorInitializer* initializer)
    {
        if (initializer != nullptr && initializer->isMemberInitializer()) {
            if (FieldDecl* field = initializer->getMember()) {
                if (const NamedDecl* candidate = canonical_candidate(field)) {
                    writer_.declaration(candidate);
                    record_write(candidate, initializer->getInit(), initializer->getSourceLocation());
                }
            }
        }
        return RecursiveASTVisitor<BoolFactVisitor>::TraverseConstructorInitializer(initializer);
    }

    bool VisitVarDecl(VarDecl* variable)
    {
        if (const NamedDecl* candidate = canonical_candidate(variable)) {
            writer_.declaration(candidate);
            if (variable->hasInit()) {
                record_write(candidate, variable->getInit(), variable->getLocation());
            } else if (variable->hasGlobalStorage() &&
                       variable->isThisDeclarationADefinition() != VarDecl::DeclarationOnly) {
                writer_.write_direct(candidate, variable->getLocation());
            }
        }

        if (variable->getType()->isReferenceType() &&
            !variable->getType().getNonReferenceType().isConstQualified() && variable->hasInit()) {
            escape_expression(variable->getInit(), variable->getLocation());
        }
        return true;
    }

    bool VisitFieldDecl(FieldDecl* field)
    {
        if (const NamedDecl* candidate = canonical_candidate(field)) {
            writer_.declaration(candidate);
            if (field->hasInClassInitializer()) {
                record_write(candidate, field->getInClassInitializer(), field->getLocation());
            }
        }
        return true;
    }

    bool VisitFunctionDecl(FunctionDecl* function)
    {
        if (const NamedDecl* candidate = canonical_candidate(function)) {
            writer_.declaration(candidate);
        }
        return true;
    }

    bool VisitReturnStmt(ReturnStmt* statement)
    {
        if (current_function_ == nullptr) {
            return true;
        }
        if (const NamedDecl* candidate = canonical_candidate(current_function_)) {
            const Expr* value = statement->getRetValue();
            if (value == nullptr) {
                writer_.invalid_write(candidate, statement->getBeginLoc());
            } else {
                record_write(candidate, value, statement->getBeginLoc());
            }
            return true;
        }

        QualType return_type = current_function_->getReturnType();
        if (return_type->isReferenceType() &&
            !return_type.getNonReferenceType().isConstQualified() &&
            statement->getRetValue() != nullptr) {
            escape_expression(statement->getRetValue(), statement->getBeginLoc());
        }
        return true;
    }

    bool VisitBinaryOperator(BinaryOperator* binary)
    {
        const BinaryOperatorKind opcode = binary->getOpcode();
        if (opcode == BO_Assign) {
            if (const NamedDecl* target = resolve_candidate(binary->getLHS())) {
                record_write(target, binary->getRHS(), binary->getOperatorLoc());
            }
            return true;
        }

        if (opcode == BO_AndAssign || opcode == BO_OrAssign || opcode == BO_XorAssign) {
            if (const NamedDecl* target = resolve_candidate(binary->getLHS())) {
                DomainExpr rhs = domain(binary->getRHS());
                if (!rhs.possible) {
                    writer_.invalid_write(target, binary->getOperatorLoc());
                } else {
                    const std::string self = writer_.key(target);
                    if (!self.empty() &&
                        std::find(rhs.dependencies.begin(), rhs.dependencies.end(), self) ==
                            rhs.dependencies.end()) {
                        rhs.dependencies.push_back(self);
                    }
                    writer_.write(target, rhs.dependencies, binary->getOperatorLoc());
                }
            }
            return true;
        }

        if (binary->isCompoundAssignmentOp()) {
            if (const NamedDecl* target = resolve_candidate(binary->getLHS())) {
                writer_.invalid_write(target, binary->getOperatorLoc());
            }
        }

        if ((opcode == BO_EQ || opcode == BO_NE) &&
            (is_zero_or_one(binary->getLHS()) || is_zero_or_one(binary->getRHS()))) {
            const Expr* other =
                is_zero_or_one(binary->getLHS()) ? binary->getRHS() : binary->getLHS();
            mark_support(other, binary->getOperatorLoc());
        }
        return true;
    }

    bool VisitUnaryOperator(UnaryOperator* unary)
    {
        if (unary->isIncrementDecrementOp()) {
            if (const NamedDecl* target = resolve_candidate(unary->getSubExpr())) {
                writer_.invalid_write(target, unary->getOperatorLoc());
            }
        } else if (unary->getOpcode() == UO_AddrOf) {
            escape_expression(unary->getSubExpr(), unary->getOperatorLoc());
        }
        return true;
    }

    bool VisitImplicitCastExpr(ImplicitCastExpr* cast)
    {
        if (cast->getCastKind() == CK_IntegralToBoolean) {
            mark_support(cast->getSubExpr(), cast->getExprLoc());
        }
        return true;
    }

    bool VisitCallExpr(CallExpr* call)
    {
        const FunctionDecl* function = call->getDirectCallee();
        if (function == nullptr) {
            return true;
        }

        // Whole-record memset is a real producer for byte fields. This is
        // common in recovered C-style runtime layouts (notably the MGS level
        // block), where no scalar `field = 0` exists for the initial state.
        // Require an exact sizeof(record) so partial/raw buffer clears do not
        // become evidence for unrelated fields.
        if (function->getName() == "memset" && call->getNumArgs() >= 3 &&
            is_zero_or_one(call->getArg(1))) {
            record_aggregate_byte_write(call->getArg(0), call->getArg(2), call->getExprLoc());
        }

        const unsigned count = std::min(call->getNumArgs(), function->getNumParams());
        for (unsigned index = 0; index < count; ++index) {
            if (is_nonconst_reference(function->getParamDecl(index)->getType())) {
                escape_expression(call->getArg(index), call->getArg(index)->getExprLoc());
            }
        }
        return true;
    }

    bool VisitCXXConstructExpr(CXXConstructExpr* construct)
    {
        const CXXConstructorDecl* constructor = construct->getConstructor();
        if (constructor == nullptr) {
            return true;
        }
        const unsigned count = std::min(construct->getNumArgs(), constructor->getNumParams());
        for (unsigned index = 0; index < count; ++index) {
            if (is_nonconst_reference(constructor->getParamDecl(index)->getType())) {
                escape_expression(construct->getArg(index), construct->getArg(index)->getExprLoc());
            }
        }
        return true;
    }

private:
    static bool is_nonconst_reference(QualType type)
    {
        return type->isReferenceType() && !type.getNonReferenceType().isConstQualified();
    }

    static bool is_zero_or_one(const Expr* expression)
    {
        expression = expression->IgnoreParenImpCasts();
        if (const auto* integer = dyn_cast<IntegerLiteral>(expression)) {
            return integer->getValue() == 0 || integer->getValue() == 1;
        }
        if (const auto* character = dyn_cast<CharacterLiteral>(expression)) {
            return character->getValue() == 0 || character->getValue() == 1;
        }
        if (isa<CXXBoolLiteralExpr>(expression)) {
            return true;
        }
        return false;
    }

    static const RecordDecl* exact_memset_record(const Expr* destination, const Expr* size)
    {
        if (destination == nullptr || size == nullptr) {
            return nullptr;
        }

        destination = destination->IgnoreParenImpCasts();
        const QualType destination_type = canonical(destination->getType());
        if (!destination_type->isPointerType()) {
            return nullptr;
        }

        const QualType pointee_type = canonical(destination_type->getPointeeType());
        const auto* record_type = pointee_type->getAs<RecordType>();
        if (record_type == nullptr) {
            return nullptr;
        }

        size = size->IgnoreParenImpCasts();
        const auto* sizeof_expression = dyn_cast<UnaryExprOrTypeTraitExpr>(size);
        if (sizeof_expression == nullptr || sizeof_expression->getKind() != UETT_SizeOf) {
            return nullptr;
        }

        QualType sized_type;
        if (sizeof_expression->isArgumentType()) {
            sized_type = sizeof_expression->getArgumentType();
        } else if (const Expr* argument = sizeof_expression->getArgumentExpr()) {
            sized_type = argument->getType();
        }
        if (sized_type.isNull() || canonical(sized_type) != pointee_type) {
            return nullptr;
        }

        const RecordDecl* record = record_type->getDecl();
        if (const RecordDecl* definition = record->getDefinition()) {
            return definition;
        }
        return record;
    }

    void record_aggregate_byte_write(const Expr* destination, const Expr* size,
                                     SourceLocation location)
    {
        const RecordDecl* record = exact_memset_record(destination, size);
        if (record == nullptr) {
            return;
        }
        for (const FieldDecl* field : record->fields()) {
            if (const NamedDecl* candidate = canonical_candidate(field)) {
                writer_.declaration(candidate);
                writer_.write_direct(candidate, location);
            }
        }
    }

    const NamedDecl* resolve_candidate(const Expr* expression) const
    {
        if (expression == nullptr) {
            return nullptr;
        }
        expression = expression->IgnoreParenImpCasts();
        if (const auto* reference = dyn_cast<DeclRefExpr>(expression)) {
            return canonical_candidate(dyn_cast<NamedDecl>(reference->getDecl()));
        }
        if (const auto* member = dyn_cast<MemberExpr>(expression)) {
            return canonical_candidate(dyn_cast<NamedDecl>(member->getMemberDecl()));
        }
        return nullptr;
    }

    DomainExpr domain(const Expr* expression) const
    {
        if (expression == nullptr) {
            return {};
        }
        expression = expression->IgnoreParenImpCasts();

        if (canonical(expression->getType())->isBooleanType()) {
            return DomainExpr::direct();
        }
        if (is_zero_or_one(expression)) {
            return DomainExpr::direct();
        }

        if (const NamedDecl* declaration = resolve_candidate(expression)) {
            const std::string dependency = writer_.key(declaration);
            if (!dependency.empty()) {
                DomainExpr result = DomainExpr::direct();
                result.dependencies.push_back(dependency);
                return result;
            }
            return {};
        }

        if (const auto* call = dyn_cast<CallExpr>(expression)) {
            if (const FunctionDecl* function = call->getDirectCallee()) {
                if (const NamedDecl* declaration = canonical_candidate(function)) {
                    const std::string dependency = writer_.key(declaration);
                    if (!dependency.empty()) {
                        DomainExpr result = DomainExpr::direct();
                        result.dependencies.push_back(dependency);
                        return result;
                    }
                }
            }
            return {};
        }

        if (const auto* cast = dyn_cast<ExplicitCastExpr>(expression)) {
            QualType target = canonical(cast->getType());
            if (!target->isIntegralOrEnumerationType()) {
                return {};
            }
            return domain(cast->getSubExpr());
        }

        if (const auto* conditional = dyn_cast<ConditionalOperator>(expression)) {
            DomainExpr left = domain(conditional->getTrueExpr());
            DomainExpr right = domain(conditional->getFalseExpr());
            if (!left.possible || !right.possible) {
                return {};
            }
            append_dependencies(left.dependencies, right.dependencies);
            return left;
        }

        if (const auto* binary = dyn_cast<BinaryOperator>(expression)) {
            if (binary->getOpcode() == BO_Comma) {
                return domain(binary->getRHS());
            }
            if (binary->getOpcode() == BO_And || binary->getOpcode() == BO_Or ||
                binary->getOpcode() == BO_Xor) {
                DomainExpr left = domain(binary->getLHS());
                DomainExpr right = domain(binary->getRHS());
                if (!left.possible || !right.possible) {
                    return {};
                }
                append_dependencies(left.dependencies, right.dependencies);
                return left;
            }
        }

        if (const auto* cleanup = dyn_cast<ExprWithCleanups>(expression)) {
            return domain(cleanup->getSubExpr());
        }
        if (const auto* temporary = dyn_cast<CXXBindTemporaryExpr>(expression)) {
            return domain(temporary->getSubExpr());
        }
        if (const auto* constant = dyn_cast<ConstantExpr>(expression)) {
            return domain(constant->getSubExpr());
        }

        return {};
    }

    void record_write(const NamedDecl* target, const Expr* value, SourceLocation location)
    {
        DomainExpr result = domain(value);
        if (!result.possible) {
            writer_.invalid_write(target, location);
            return;
        }
        writer_.write(target, result.dependencies, location);
    }

    void mark_support(const Expr* expression, SourceLocation location)
    {
        DomainExpr result = domain(expression);
        if (!result.possible) {
            return;
        }
        for (const std::string& dependency : result.dependencies) {
            writer_.support(dependency, location);
        }
    }

    void escape_expression(const Expr* expression, SourceLocation location)
    {
        if (const NamedDecl* declaration = resolve_candidate(expression)) {
            writer_.escape(declaration, location);
        }
    }

    FactWriter& writer_;
    FunctionDecl* current_function_ = nullptr;
};

class BoolLikeByteCheck final : public ClangTidyCheck {
public:
    BoolLikeByteCheck(llvm::StringRef name, ClangTidyContext* context)
        : ClangTidyCheck(name, context)
    {
    }

    void registerMatchers(ast_matchers::MatchFinder* finder) override
    {
        finder->addMatcher(ast_matchers::translationUnitDecl().bind("translation_unit"), this);
    }

    void check(const ast_matchers::MatchFinder::MatchResult& result) override
    {
        translation_unit_ = result.Nodes.getNodeAs<TranslationUnitDecl>("translation_unit");
        context_ = result.Context;
    }

    void onEndOfTranslationUnit() override
    {
        if (context_ == nullptr || translation_unit_ == nullptr) {
            return;
        }
        FactWriter writer(*context_, translation_unit_);
        if (writer.enabled()) {
            BoolFactVisitor visitor(writer);
            visitor.TraverseDecl(const_cast<TranslationUnitDecl*>(translation_unit_));
        }
        context_ = nullptr;
        translation_unit_ = nullptr;
    }

private:
    ASTContext* context_ = nullptr;
    const TranslationUnitDecl* translation_unit_ = nullptr;
};

class WizardryBoolTidyModule final : public ClangTidyModule {
public:
    void addCheckFactories(ClangTidyCheckFactories& factories) override
    {
        factories.registerCheck<BoolLikeByteCheck>("wiz8-bool-like-byte");
    }
};

static ClangTidyModuleRegistry::Add<WizardryBoolTidyModule> module(
    "wiz8-bool-module", "Adds Wizardry boolean-domain recovery checks.");

} // namespace
} // namespace clang::tidy::wiz8
