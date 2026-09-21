#include "clang-tidy/ClangTidyCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace clang::tidy::wiz8 {
namespace {

struct LineRange {
    unsigned first;
    unsigned last;
};

struct FileRanges {
    std::string file;
    std::vector<LineRange> ranges;
};

class AddedLineFilter {
public:
    AddedLineFilter()
    {
        const char* raw = std::getenv("WIZ8_REDUNDANT_CAST_LINES");
        // Only an explicit "*" opts into a full-corpus audit. Unset/empty means
        // no lines are gated: the wrapper is responsible for publishing the
        // changed-line filter, and a missing VCS checkout must not suddenly
        // treat the entire recovered corpus as new debt.
        if (raw == nullptr || raw[0] == '\0') {
            return;
        }
        if (llvm::StringRef(raw) == "*") {
            all_lines_ = true;
            return;
        }
        parse(raw);
    }

    bool contains(llvm::StringRef file, unsigned line) const
    {
        if (all_lines_) {
            return true;
        }
        for (const FileRanges& entry : files_) {
            if (file != entry.file) {
                continue;
            }
            for (const LineRange& range : entry.ranges) {
                if (line >= range.first && line <= range.last) {
                    return true;
                }
            }
            return false;
        }
        return false;
    }

private:
    void parse(llvm::StringRef raw)
    {
        llvm::SmallVector<llvm::StringRef, 16> entries;
        raw.split(entries, ';', -1, false);
        for (llvm::StringRef entry : entries) {
            std::pair<llvm::StringRef, llvm::StringRef> split = entry.split('@');
            if (split.first.empty() || split.second.empty()) {
                continue;
            }
            FileRanges file_ranges;
            file_ranges.file = split.first.str();
            llvm::SmallVector<llvm::StringRef, 16> ranges;
            split.second.split(ranges, ',', -1, false);
            for (llvm::StringRef range_text : ranges) {
                std::pair<llvm::StringRef, llvm::StringRef> bounds = range_text.split('-');
                unsigned first = 0;
                unsigned last = 0;
                if (bounds.first.getAsInteger(10, first)) {
                    continue;
                }
                if (bounds.second.empty()) {
                    last = first;
                } else if (bounds.second.getAsInteger(10, last)) {
                    continue;
                }
                if (first == 0 || last < first) {
                    continue;
                }
                file_ranges.ranges.push_back({first, last});
            }
            if (!file_ranges.ranges.empty()) {
                files_.push_back(std::move(file_ranges));
            }
        }
    }

    bool all_lines_ = false;
    std::vector<FileRanges> files_;
};

static QualType canonical(QualType type)
{
    return type.getCanonicalType().getUnqualifiedType();
}

static int floating_rank(QualType type)
{
    const auto* builtin = dyn_cast<BuiltinType>(canonical(type).getTypePtr());
    if (builtin == nullptr) {
        return 0;
    }
    switch (builtin->getKind()) {
    case BuiltinType::Float:
        return 1;
    case BuiltinType::Double:
        return 2;
    case BuiltinType::LongDouble:
        return 3;
    default:
        return 0;
    }
}

static bool is_value_preserving_floating_conversion(QualType source, QualType target)
{
    const int source_rank = floating_rank(source);
    const int target_rank = floating_rank(target);
    return source_rank != 0 && target_rank > source_rank;
}

static bool is_implicit_floating_operand_conversion(QualType source, QualType target)
{
    target = canonical(target);
    source = canonical(source);
    if (floating_rank(target) == 0) {
        return false;
    }
    if (source->isIntegerType()) {
        return true;
    }
    const int source_rank = floating_rank(source);
    return source_rank != 0 && source_rank <= floating_rank(target);
}

static const Stmt* semantic_parent(const Stmt* node, ASTContext& context)
{
    const Stmt* current = node;
    for (;;) {
        const auto parents = context.getParents(*current);
        if (parents.size() != 1) {
            return nullptr;
        }
        const Stmt* parent = parents[0].get<Stmt>();
        if (parent == nullptr) {
            return nullptr;
        }
        if (isa<ParenExpr>(parent) || isa<ImplicitCastExpr>(parent)) {
            current = parent;
            continue;
        }
        return parent;
    }
}

static const FunctionDecl* enclosing_function(const Stmt* node, ASTContext& context)
{
    DynTypedNode current = DynTypedNode::create(*node);
    for (unsigned depth = 0; depth != 64; ++depth) {
        const auto parents = context.getParents(current);
        if (parents.size() != 1) {
            return nullptr;
        }
        if (const auto* function = parents[0].get<FunctionDecl>()) {
            return function;
        }
        current = parents[0];
    }
    return nullptr;
}

static bool is_in_template_instantiation(const Stmt* node, ASTContext& context)
{
    const FunctionDecl* function = enclosing_function(node, context);
    return function != nullptr && function->getTemplatedKind() != FunctionDecl::TK_NonTemplate;
}

static const Expr* other_operand(const BinaryOperator* binary, const ExplicitCastExpr* cast)
{
    const Expr* lhs = binary->getLHS()->IgnoreParenImpCasts();
    const Expr* rhs = binary->getRHS()->IgnoreParenImpCasts();
    if (lhs == cast) {
        return rhs;
    }
    if (rhs == cast) {
        return lhs;
    }
    return nullptr;
}

static bool is_right_operand(const BinaryOperator* binary, const ExplicitCastExpr* cast)
{
    return binary->getRHS()->IgnoreParenImpCasts() == cast;
}

static bool is_direct_non_overloaded_call(const CallExpr* call, const FunctionDecl* function)
{
    if (function == nullptr || function->getTemplatedKind() != FunctionDecl::TK_NonTemplate) {
        return false;
    }
    const Expr* callee = call->getCallee()->IgnoreParenImpCasts();
    if (const auto* reference = dyn_cast<DeclRefExpr>(callee)) {
        if (reference->getDecl() != function || reference->getFoundDecl() != function) {
            return false;
        }
    } else if (const auto* member = dyn_cast<MemberExpr>(callee)) {
        if (member->getMemberDecl() != function) {
            return false;
        }
    } else {
        return false;
    }

    llvm::SmallPtrSet<const FunctionDecl*, 4> functions;
    for (const NamedDecl* declaration : function->getDeclContext()->lookup(function->getDeclName())) {
        if (const auto* other = dyn_cast<FunctionDecl>(declaration)) {
            functions.insert(other->getCanonicalDecl());
        } else if (isa<FunctionTemplateDecl>(declaration) || isa<UsingShadowDecl>(declaration)) {
            return false;
        }
    }
    return functions.size() == 1;
}

static std::string repository_relative_path(const SourceManager& sources, SourceLocation location)
{
    std::string path = sources.getFilename(location).str();
    constexpr char repo_prefix[] = "/repo/";
    if (path.rfind(repo_prefix, 0) == 0) {
        path.erase(0, sizeof(repo_prefix) - 1);
    }
    return path;
}

class RedundantScalarCastCheck final : public ClangTidyCheck {
public:
    RedundantScalarCastCheck(llvm::StringRef name, ClangTidyContext* context)
        : ClangTidyCheck(name, context)
    {
    }

    void registerMatchers(ast_matchers::MatchFinder* finder) override
    {
        finder->addMatcher(ast_matchers::explicitCastExpr().bind("cast"), this);
    }

    void check(const ast_matchers::MatchFinder::MatchResult& result) override
    {
        const auto* cast = result.Nodes.getNodeAs<ExplicitCastExpr>("cast");
        if (cast == nullptr || result.Context == nullptr || result.SourceManager == nullptr) {
            return;
        }
        if (!isa<CStyleCastExpr>(cast) && !isa<CXXStaticCastExpr>(cast)) {
            return;
        }
        if (cast->getBeginLoc().isMacroID()) {
            return;
        }

        SourceManager& sources = *result.SourceManager;
        SourceLocation location = sources.getExpansionLoc(cast->getBeginLoc());
        if (location.isInvalid() || sources.isInSystemHeader(location)) {
            return;
        }
        const std::string file = repository_relative_path(sources, location);
        const unsigned line = sources.getSpellingLineNumber(location);
        if (!added_lines_.contains(file, line)) {
            return;
        }

        ASTContext& context = *result.Context;
        if (is_in_template_instantiation(cast, context)) {
            return;
        }

        const Expr* source_expr = cast->getSubExpr()->IgnoreParenImpCasts();
        QualType source = canonical(source_expr->getType());
        QualType target = canonical(cast->getType());
        if (!source->isArithmeticType() || !target->isArithmeticType()) {
            return;
        }

        const Stmt* parent = semantic_parent(cast, context);

        if (source->isIntegerType() && target->isIntegerType() && !source->isBooleanType() &&
            !target->isBooleanType() && !source->isAnyCharacterType() &&
            !target->isAnyCharacterType() && context.getTypeSize(source) == context.getTypeSize(target) &&
            source->isSignedIntegerType() == target->isSignedIntegerType()) {
            if (const auto* outer = dyn_cast_or_null<ExplicitCastExpr>(parent)) {
                if (outer->getType()->isArithmeticType()) {
                    diag(cast->getBeginLoc(),
                         "intermediate cast from %0 to %1 is redundant; both types have the same "
                         "signedness and width before the enclosing conversion")
                        << source << target;
                    return;
                }
            }
        }

        if (const auto* binary = dyn_cast_or_null<BinaryOperator>(parent)) {
            if (binary->isAdditiveOp() || binary->isMultiplicativeOp() || binary->isComparisonOp()) {
                const Expr* other = other_operand(binary, cast);
                if (other != nullptr && context.hasSameType(canonical(other->getType()), target) &&
                    is_implicit_floating_operand_conversion(source, target)) {
                    // If both operands carry the same explicit conversion, either cast is
                    // individually removable but deleting both could change integer arithmetic.
                    // Report only the left-hand one; the right-hand cast then remains as the
                    // single source-level conversion that establishes the floating operation.
                    if (is_right_operand(binary, cast)) {
                        if (const auto* other_cast = dyn_cast<ExplicitCastExpr>(other)) {
                            if (context.hasSameType(canonical(other_cast->getType()), target)) {
                                return;
                            }
                        }
                    }
                    diag(cast->getBeginLoc(),
                         "explicit cast from %0 to %1 is redundant; the surrounding floating-point "
                         "operation already performs this conversion")
                        << source << target;
                    return;
                }
            }
        }

        const auto* call = dyn_cast_or_null<CallExpr>(parent);
        if (call == nullptr || !is_value_preserving_floating_conversion(source, target)) {
            return;
        }
        const FunctionDecl* function = call->getDirectCallee();
        if (!is_direct_non_overloaded_call(call, function)) {
            return;
        }
        for (unsigned index = 0; index < call->getNumArgs(); ++index) {
            if (call->getArg(index)->IgnoreParenImpCasts() != cast) {
                continue;
            }
            if (index < function->getNumParams()) {
                if (!context.hasSameType(canonical(function->getParamDecl(index)->getType()), target)) {
                    return;
                }
                diag(cast->getBeginLoc(),
                     "explicit cast from %0 to %1 is redundant; this non-overloaded parameter already "
                     "performs the value-preserving conversion")
                    << source << target;
                return;
            }
            if (function->isVariadic() && floating_rank(source) == 1 && floating_rank(target) == 2) {
                diag(cast->getBeginLoc(),
                     "explicit cast from float to double is redundant; variadic argument promotion "
                     "already performs this conversion");
                return;
            }
        }
    }

private:
    AddedLineFilter added_lines_;
};

struct ProjectRecordPointer {
    const RecordDecl* record = nullptr;
    unsigned pointer_depth = 0;
};

static ProjectRecordPointer project_record_pointer(QualType type)
{
    ProjectRecordPointer result;
    type = canonical(type);
    while (type->isPointerType()) {
        ++result.pointer_depth;
        type = canonical(type->getPointeeType());
    }
    if (result.pointer_depth == 0) {
        return {};
    }

    const auto* record_type = type->getAs<RecordType>();
    if (record_type == nullptr) {
        return {};
    }
    const RecordDecl* record = record_type->getDecl();
    if (const RecordDecl* definition = record->getDefinition()) {
        record = definition;
    }
    const llvm::StringRef name = record->getName();
    if (!name.starts_with("W8") && !name.starts_with("sr") && !name.starts_with("st")) {
        return {};
    }
    result.record = dyn_cast<RecordDecl>(record->getCanonicalDecl());
    return result;
}

class ProjectRecordReinterpretCastCheck final : public ClangTidyCheck {
public:
    ProjectRecordReinterpretCastCheck(llvm::StringRef name, ClangTidyContext* context)
        : ClangTidyCheck(name, context)
    {
    }

    void registerMatchers(ast_matchers::MatchFinder* finder) override
    {
        finder->addMatcher(ast_matchers::cxxReinterpretCastExpr().bind("record-cast"), this);
    }

    void check(const ast_matchers::MatchFinder::MatchResult& result) override
    {
        const auto* cast = result.Nodes.getNodeAs<CXXReinterpretCastExpr>("record-cast");
        if (cast == nullptr || result.Context == nullptr || result.SourceManager == nullptr) {
            return;
        }
        if (cast->getBeginLoc().isMacroID()) {
            return;
        }

        SourceManager& sources = *result.SourceManager;
        SourceLocation location = sources.getExpansionLoc(cast->getBeginLoc());
        if (location.isInvalid() || sources.isInSystemHeader(location)) {
            return;
        }
        const std::string file = repository_relative_path(sources, location);
        const unsigned line = sources.getSpellingLineNumber(location);
        if (!added_lines_.contains(file, line)) {
            return;
        }

        ASTContext& context = *result.Context;
        if (is_in_template_instantiation(cast, context)) {
            return;
        }

        const Expr* source_expr = cast->getSubExpr()->IgnoreParenImpCasts();
        const QualType source_type = canonical(source_expr->getType());
        const QualType target_type = canonical(cast->getType());
        const ProjectRecordPointer source = project_record_pointer(source_type);
        const ProjectRecordPointer target = project_record_pointer(target_type);
        if (source.record == nullptr || target.record == nullptr) {
            return;
        }
        if (source.record == target.record && source.pointer_depth == target.pointer_depth) {
            return;
        }

        diag(cast->getBeginLoc(),
             "reinterpret_cast from modeled project record pointer %0 to %1 hides a "
             "source-model disagreement; fix the owning type or recovered prototype instead")
            << source_type << target_type;
    }

private:
    AddedLineFilter added_lines_;
};

class WizardryTidyModule final : public ClangTidyModule {
public:
    void addCheckFactories(ClangTidyCheckFactories& factories) override
    {
        factories.registerCheck<RedundantScalarCastCheck>("wiz8-redundant-scalar-cast");
        factories.registerCheck<ProjectRecordReinterpretCastCheck>(
            "wiz8-project-record-reinterpret-cast");
    }
};

static ClangTidyModuleRegistry::Add<WizardryTidyModule> module(
    "wiz8-module", "Adds Wizardry reconstruction checks.");

} // namespace
} // namespace clang::tidy::wiz8
