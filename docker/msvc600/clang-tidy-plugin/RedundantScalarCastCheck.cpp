#include "clang-tidy/ClangTidyCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include <cstdlib>
#include <set>
#include <string>
#include <tuple>
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

static constexpr llvm::StringLiteral project_roots[] = {
    "src/wiz8/",          "include/wiz8/",           "src/surrender/",
    "include/surrender/", "src/srext_jpegimporter/", "src/srext_unzip/",
};

static std::string normalized_repository_path(llvm::StringRef input)
{
    std::string path = input.str();
    for (char& character : path) {
        if (character == '\\') {
            character = '/';
        }
    }
    for (llvm::StringRef root : project_roots) {
        if (llvm::StringRef(path).starts_with(root)) {
            return path;
        }
        const std::string marker = "/" + root.str();
        const size_t position = path.find(marker);
        if (position != std::string::npos) {
            return path.substr(position + 1);
        }
    }
    return path;
}

static bool is_project_source_path(llvm::StringRef path)
{
    for (llvm::StringRef root : project_roots) {
        if (path.starts_with(root)) {
            return true;
        }
    }
    return false;
}

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
        const std::string normalized_file = normalized_repository_path(file);
        for (const FileRanges& entry : files_) {
            if (normalized_file != entry.file) {
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
            file_ranges.file = normalized_repository_path(split.first);
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
    for (const NamedDecl* declaration :
         function->getDeclContext()->lookup(function->getDeclName())) {
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
    const PresumedLoc presumed = sources.getPresumedLoc(location);
    if (presumed.isValid()) {
        return normalized_repository_path(presumed.getFilename());
    }
    return normalized_repository_path(sources.getFilename(location));
}

// Removing a C-style cast leaves its operand in place: the operand of `(T)e` is
// already a cast-expression, so it parses identically without the prefix. A
// static_cast keeps its parentheses unless the operand is a primary expression.
static std::vector<FixItHint> removal_fix(const ExplicitCastExpr* cast,
                                          const SourceManager& sources, const LangOptions& language,
                                          bool whole_argument = false)
{
    std::vector<FixItHint> hints;
    const Expr* operand = cast->getSubExprAsWritten();
    if (operand == nullptr || cast->getBeginLoc().isMacroID() ||
        operand->getBeginLoc().isMacroID() || cast->getEndLoc().isMacroID()) {
        return hints;
    }
    if (isa<CStyleCastExpr>(cast)) {
        // A whole call argument needs no grouping parentheses once the cast is gone.
        if (const auto* group = dyn_cast<ParenExpr>(operand);
            group != nullptr && whole_argument && !group->getRParen().isMacroID()) {
            hints.push_back(FixItHint::CreateRemoval(CharSourceRange::getCharRange(
                cast->getBeginLoc(), group->getSubExpr()->getBeginLoc())));
            hints.push_back(FixItHint::CreateRemoval(
                CharSourceRange::getTokenRange(group->getRParen(), group->getRParen())));
            return hints;
        }
        hints.push_back(FixItHint::CreateRemoval(
            CharSourceRange::getCharRange(cast->getBeginLoc(), operand->getBeginLoc())));
        return hints;
    }
    if (isa<CXXStaticCastExpr>(cast)) {
        const Expr* bare = operand->IgnoreParens();
        const bool primary = isa<DeclRefExpr>(bare) || isa<MemberExpr>(bare) ||
                             isa<CallExpr>(bare) || isa<ArraySubscriptExpr>(bare) ||
                             isa<IntegerLiteral>(bare) || isa<FloatingLiteral>(bare);
        const CharSourceRange text = CharSourceRange::getTokenRange(operand->getSourceRange());
        std::string spelling = Lexer::getSourceText(text, sources, language).str();
        if (spelling.empty()) {
            return hints;
        }
        hints.push_back(
            FixItHint::CreateReplacement(CharSourceRange::getTokenRange(cast->getSourceRange()),
                                         primary ? spelling : "(" + spelling + ")"));
    }
    return hints;
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
            !target->isAnyCharacterType() &&
            context.getTypeSize(source) == context.getTypeSize(target) &&
            source->isSignedIntegerType() == target->isSignedIntegerType()) {
            if (const auto* outer = dyn_cast_or_null<ExplicitCastExpr>(parent)) {
                if (outer->getType()->isArithmeticType()) {
                    diag(cast->getBeginLoc(),
                         "intermediate cast from %0 to %1 is redundant; both types have the same "
                         "signedness and width before the enclosing conversion")
                        << source << target << removal_fix(cast, sources, context.getLangOpts());
                    return;
                }
            }
        }

        if (const auto* binary = dyn_cast_or_null<BinaryOperator>(parent)) {
            if (binary->isAdditiveOp() || binary->isMultiplicativeOp() ||
                binary->isComparisonOp()) {
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
                        << source << target << removal_fix(cast, sources, context.getLangOpts());
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
                if (!context.hasSameType(canonical(function->getParamDecl(index)->getType()),
                                         target)) {
                    return;
                }
                diag(cast->getBeginLoc(), "explicit cast from %0 to %1 is redundant; this "
                                          "non-overloaded parameter already "
                                          "performs the value-preserving conversion")
                    << source << target << removal_fix(cast, sources, context.getLangOpts(), true);
                return;
            }
            if (function->isVariadic() && floating_rank(source) == 1 &&
                floating_rank(target) == 2) {
                diag(cast->getBeginLoc(),
                     "explicit cast from float to double is redundant; variadic argument promotion "
                     "already performs this conversion")
                    << removal_fix(cast, sources, context.getLangOpts(), true);
                return;
            }
        }
    }

private:
    AddedLineFilter added_lines_;
};

struct ProjectRecordType {
    const RecordDecl* record = nullptr;
    unsigned pointer_depth = 0;
    bool reference = false;
};

static bool is_project_record(const RecordDecl* record, const SourceManager& sources)
{
    if (record == nullptr) {
        return false;
    }
    const RecordDecl* declaration = record->getDefinition();
    if (declaration == nullptr) {
        declaration = dyn_cast<RecordDecl>(record->getCanonicalDecl());
    }
    if (declaration == nullptr) {
        return false;
    }
    SourceLocation location = sources.getSpellingLoc(declaration->getLocation());
    if (location.isInvalid() || sources.isInSystemHeader(location)) {
        return false;
    }
    return is_project_source_path(repository_relative_path(sources, location));
}

static ProjectRecordType project_record_type(QualType type, const SourceManager& sources)
{
    ProjectRecordType result;
    type = type.getCanonicalType();
    if (const auto* reference = dyn_cast<ReferenceType>(type.getTypePtr())) {
        result.reference = true;
        type = reference->getPointeeType().getCanonicalType();
    }
    while (type->isPointerType()) {
        ++result.pointer_depth;
        type = type->getPointeeType().getCanonicalType();
    }

    const auto* record_type = type->getAs<RecordType>();
    if (record_type == nullptr) {
        return {};
    }
    const RecordDecl* record = record_type->getDecl();
    if (!is_project_record(record, sources)) {
        return {};
    }
    result.record = dyn_cast<RecordDecl>(record->getCanonicalDecl());
    return result;
}

static ProjectRecordType project_record_expression(const Expr* expression,
                                                   const SourceManager& sources)
{
    if (expression == nullptr) {
        return {};
    }
    ProjectRecordType result = project_record_type(expression->getType(), sources);
    if (result.record != nullptr && result.pointer_depth == 0 && !result.reference &&
        expression->isGLValue()) {
        result.reference = true;
    }
    return result;
}

static const Expr* strip_expression_wrappers(const Expr* expression)
{
    while (expression != nullptr) {
        if (const auto* paren = dyn_cast<ParenExpr>(expression)) {
            expression = paren->getSubExpr();
        } else if (const auto* implicit = dyn_cast<ImplicitCastExpr>(expression)) {
            expression = implicit->getSubExpr();
        } else if (const auto* cleanups = dyn_cast<ExprWithCleanups>(expression)) {
            expression = cleanups->getSubExpr();
        } else if (const auto* bound = dyn_cast<CXXBindTemporaryExpr>(expression)) {
            expression = bound->getSubExpr();
        } else {
            break;
        }
    }
    return expression;
}

static bool is_direct_variable_reference(const Expr* expression, const VarDecl* variable)
{
    expression = strip_expression_wrappers(expression);
    const auto* reference = dyn_cast_or_null<DeclRefExpr>(expression);
    return reference != nullptr && reference->getDecl() == variable;
}

class VariableReferenceFinder final : public RecursiveASTVisitor<VariableReferenceFinder> {
public:
    explicit VariableReferenceFinder(const VarDecl* variable) : variable_(variable) {}

    bool VisitDeclRefExpr(const DeclRefExpr* reference)
    {
        found_ = found_ || reference->getDecl() == variable_;
        return true;
    }

    bool found() const
    {
        return found_;
    }

private:
    const VarDecl* variable_;
    bool found_ = false;
};

static bool references_variable(const Expr* expression, const VarDecl* variable)
{
    if (expression == nullptr) {
        return false;
    }
    VariableReferenceFinder finder(variable);
    finder.TraverseStmt(const_cast<Expr*>(expression));
    return finder.found();
}

class LocalDefinitionSafety final : public RecursiveASTVisitor<LocalDefinitionSafety> {
public:
    explicit LocalDefinitionSafety(const VarDecl* variable) : variable_(variable) {}

    bool VisitBinaryOperator(const BinaryOperator* binary)
    {
        if (!binary->isAssignmentOp()) {
            return true;
        }
        if (is_direct_variable_reference(binary->getLHS(), variable_)) {
            modified_ = true;
        }
        if (references_variable(binary->getRHS(), variable_)) {
            const auto* lhs_reference =
                dyn_cast_or_null<DeclRefExpr>(strip_expression_wrappers(binary->getLHS()));
            const auto* lhs_variable =
                lhs_reference == nullptr ? nullptr : dyn_cast<VarDecl>(lhs_reference->getDecl());
            if (lhs_variable == nullptr || !lhs_variable->isLocalVarDecl()) {
                escaped_ = true;
            }
        }
        return true;
    }

    bool VisitUnaryOperator(const UnaryOperator* unary)
    {
        if (unary->isIncrementDecrementOp() &&
            is_direct_variable_reference(unary->getSubExpr(), variable_)) {
            modified_ = true;
        }
        if (unary->getOpcode() == UO_AddrOf &&
            is_direct_variable_reference(unary->getSubExpr(), variable_)) {
            escaped_ = true;
        }
        return true;
    }

    bool VisitCallExpr(const CallExpr* call)
    {
        for (const Expr* argument : call->arguments()) {
            escaped_ = escaped_ || references_variable(argument, variable_);
        }
        return true;
    }

    bool VisitReturnStmt(const ReturnStmt* statement)
    {
        escaped_ = escaped_ || is_direct_variable_reference(statement->getRetValue(), variable_);
        return true;
    }

    bool VisitLambdaExpr(const LambdaExpr* expression)
    {
        for (const LambdaCapture& capture : expression->captures()) {
            if (capture.capturesVariable() && capture.getCapturedVar() == variable_) {
                escaped_ = true;
            }
        }
        return true;
    }

    bool safe() const
    {
        return !modified_ && !escaped_;
    }

private:
    const VarDecl* variable_;
    bool modified_ = false;
    bool escaped_ = false;
};

static bool has_single_local_definition(const VarDecl* variable, const FunctionDecl* function,
                                        llvm::DenseMap<const VarDecl*, bool>& cache)
{
    const auto cached = cache.find(variable);
    if (cached != cache.end()) {
        return cached->second;
    }
    if (isa<ParmVarDecl>(variable) || !variable->isLocalVarDecl() || variable->hasGlobalStorage() ||
        variable->getInit() == nullptr || function == nullptr || function->getBody() == nullptr) {
        cache[variable] = false;
        return false;
    }
    LocalDefinitionSafety safety(variable);
    safety.TraverseStmt(const_cast<Stmt*>(function->getBody()));
    cache[variable] = safety.safe();
    return safety.safe();
}

static bool is_byte_pointer(QualType type)
{
    type = canonical(type);
    if (!type->isPointerType()) {
        return false;
    }
    while (type->isPointerType()) {
        type = canonical(type->getPointeeType());
    }
    const auto* builtin = dyn_cast<BuiltinType>(type.getTypePtr());
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

static bool is_erased_pointer_type(QualType type)
{
    type = canonical(type);
    if (!type->isPointerType()) {
        return false;
    }
    const QualType original = type;
    while (type->isPointerType()) {
        type = canonical(type->getPointeeType());
    }
    return type->isVoidType() || is_byte_pointer(original);
}

struct RecordProvenance {
    ProjectRecordType origin;
    SourceLocation erasure;
    bool unresolved = false;
};

static void set_erasure_site(RecordProvenance& provenance, SourceLocation location)
{
    if (provenance.origin.record != nullptr && provenance.erasure.isInvalid()) {
        provenance.erasure = location;
    }
}

static SourceLocation explicit_cast_site(const ExplicitCastExpr* cast)
{
    if (const auto* named = dyn_cast<CXXNamedCastExpr>(cast)) {
        return named->getOperatorLoc();
    }
    if (const auto* c_style = dyn_cast<CStyleCastExpr>(cast)) {
        return c_style->getLParenLoc();
    }
    return cast->getBeginLoc();
}

static RecordProvenance trace_record_provenance(const Expr* expression,
                                                const SourceManager& sources,
                                                const FunctionDecl* function,
                                                llvm::DenseMap<const VarDecl*, bool>& local_cache,
                                                llvm::SmallPtrSetImpl<const VarDecl*>& visited,
                                                unsigned depth = 0)
{
    if (expression == nullptr || depth >= 32) {
        return {};
    }

    if (const auto* paren = dyn_cast<ParenExpr>(expression)) {
        return trace_record_provenance(paren->getSubExpr(), sources, function, local_cache, visited,
                                       depth + 1);
    }
    if (const auto* cleanups = dyn_cast<ExprWithCleanups>(expression)) {
        return trace_record_provenance(cleanups->getSubExpr(), sources, function, local_cache,
                                       visited, depth + 1);
    }
    if (const auto* bound = dyn_cast<CXXBindTemporaryExpr>(expression)) {
        return trace_record_provenance(bound->getSubExpr(), sources, function, local_cache, visited,
                                       depth + 1);
    }
    if (const auto* implicit = dyn_cast<ImplicitCastExpr>(expression)) {
        RecordProvenance provenance = trace_record_provenance(
            implicit->getSubExpr(), sources, function, local_cache, visited, depth + 1);
        if (is_erased_pointer_type(implicit->getType())) {
            set_erasure_site(provenance, implicit->getExprLoc());
        }
        return provenance;
    }
    if (const auto* explicit_cast = dyn_cast<ExplicitCastExpr>(expression)) {
        RecordProvenance provenance = trace_record_provenance(
            explicit_cast->getSubExpr(), sources, function, local_cache, visited, depth + 1);
        if (is_erased_pointer_type(explicit_cast->getType())) {
            set_erasure_site(provenance, explicit_cast_site(explicit_cast));
        }
        return provenance;
    }
    if (const auto* reference = dyn_cast<DeclRefExpr>(expression)) {
        if (const auto* variable = dyn_cast<VarDecl>(reference->getDecl())) {
            const FunctionDecl* owner = function;
            if (owner == nullptr) {
                owner = dyn_cast<FunctionDecl>(variable->getDeclContext());
            }
            const bool is_local = !isa<ParmVarDecl>(variable) && variable->isLocalVarDecl() &&
                                  !variable->hasGlobalStorage();
            const bool has_one_definition =
                is_local && has_single_local_definition(variable, owner, local_cache);
            if (has_one_definition && visited.insert(variable).second) {
                RecordProvenance provenance = trace_record_provenance(
                    variable->getInit(), sources, owner, local_cache, visited, depth + 1);
                visited.erase(variable);
                if (provenance.origin.record != nullptr || provenance.unresolved) {
                    return provenance;
                }
            }
            if (is_local && is_erased_pointer_type(variable->getType()) && !has_one_definition) {
                return {{}, SourceLocation(), true};
            }
            return {project_record_expression(expression, sources), SourceLocation()};
        }
        return {project_record_expression(expression, sources), SourceLocation()};
    }

    return {project_record_expression(expression, sources), SourceLocation()};
}

static bool is_inheritance_cast_kind(CastKind kind)
{
    return kind == CK_DerivedToBase || kind == CK_BaseToDerived;
}

static bool has_matching_indirection(const ProjectRecordType& source,
                                     const ProjectRecordType& target)
{
    return source.pointer_depth == target.pointer_depth && source.reference == target.reference &&
           (source.reference ? source.pointer_depth == 0 : source.pointer_depth == 1);
}

static bool is_ordinary_inheritance_static_cast(const ExplicitCastExpr* expression,
                                                const SourceManager& sources,
                                                const ProjectRecordType& source,
                                                const ProjectRecordType& target)
{
    const auto* cast = dyn_cast<CXXStaticCastExpr>(expression);
    if (cast == nullptr || source.record == nullptr || target.record == nullptr ||
        !has_matching_indirection(source, target)) {
        return false;
    }
    if (is_inheritance_cast_kind(cast->getCastKind())) {
        return true;
    }

    // A no-adjustment derived-to-base static_cast is represented by an outer
    // CK_NoOp around the implicit CK_DerivedToBase conversion.
    const Expr* nested = cast->getSubExpr();
    while (const auto* conversion = dyn_cast<ImplicitCastExpr>(nested)) {
        if (is_inheritance_cast_kind(conversion->getCastKind())) {
            const ProjectRecordType converted_source =
                project_record_expression(conversion->getSubExpr(), sources);
            const ProjectRecordType converted_target =
                project_record_expression(conversion, sources);
            if (converted_source.record == source.record &&
                converted_target.record == target.record &&
                has_matching_indirection(converted_source, converted_target)) {
                return true;
            }
        }
        nested = conversion->getSubExpr();
    }
    return false;
}

static std::string record_name(const ProjectRecordType& type)
{
    if (type.record == nullptr) {
        return "<unknown>";
    }
    return type.record->getQualifiedNameAsString();
}

static std::string record_indirection(const ProjectRecordType& type)
{
    return std::string(type.pointer_depth, '*') + (type.reference ? "&" : "");
}

class ProjectRecordReinterpretCastCheck final : public ClangTidyCheck {
public:
    ProjectRecordReinterpretCastCheck(llvm::StringRef name, ClangTidyContext* context)
        : ClangTidyCheck(name, context)
    {
    }

    void registerMatchers(ast_matchers::MatchFinder* finder) override
    {
        finder->addMatcher(ast_matchers::explicitCastExpr().bind("record-cast"), this);
    }

    void check(const ast_matchers::MatchFinder::MatchResult& result) override
    {
        const auto* cast = result.Nodes.getNodeAs<ExplicitCastExpr>("record-cast");
        if (cast == nullptr || result.Context == nullptr || result.SourceManager == nullptr) {
            return;
        }
        if (!isa<CXXReinterpretCastExpr>(cast) && !isa<CXXStaticCastExpr>(cast) &&
            !isa<CStyleCastExpr>(cast)) {
            return;
        }

        SourceManager& sources = *result.SourceManager;
        const SourceLocation spelling_location = sources.getSpellingLoc(cast->getBeginLoc());
        const SourceLocation expansion_location = sources.getExpansionLoc(cast->getBeginLoc());
        if (spelling_location.isInvalid() || sources.isInSystemHeader(spelling_location)) {
            return;
        }
        const std::string spelling_file = repository_relative_path(sources, spelling_location);
        if (!is_project_source_path(spelling_file)) {
            return;
        }
        const unsigned spelling_line = sources.getSpellingLineNumber(spelling_location);
        const std::string expansion_file = repository_relative_path(sources, expansion_location);
        const unsigned expansion_line = sources.getSpellingLineNumber(expansion_location);
        if (!added_lines_.contains(spelling_file, spelling_line) &&
            !added_lines_.contains(expansion_file, expansion_line)) {
            return;
        }

        ASTContext& context = *result.Context;
        const FunctionDecl* function = enclosing_function(cast, context);
        llvm::SmallPtrSet<const VarDecl*, 8> visited;
        RecordProvenance provenance = trace_record_provenance(cast->getSubExpr(), sources, function,
                                                              local_definition_cache_, visited);
        const ProjectRecordType source = provenance.origin;
        const ProjectRecordType target = project_record_expression(cast, sources);
        if (target.record == nullptr) {
            return;
        }
        if (source.record == nullptr) {
            if (provenance.unresolved) {
                const std::string target_name = record_name(target);
                diag(cast->getBeginLoc(),
                     "source record provenance for conversion to repository record '%0' is "
                     "unresolved because the local erased pointer has ambiguous definitions or "
                     "escapes its function")
                    << target_name;
            }
            return;
        }
        if (source.record == target.record && source.pointer_depth == target.pointer_depth &&
            source.reference == target.reference) {
            return;
        }
        if (provenance.erasure.isInvalid() &&
            is_ordinary_inheritance_static_cast(cast, sources, source, target)) {
            return;
        }

        const std::string source_name = record_name(source);
        const std::string target_name = record_name(target);
        const std::string source_indirection = record_indirection(source);
        const std::string target_indirection = record_indirection(target);
        const auto site = sources.getPresumedLoc(spelling_location);
        const std::string site_file = spelling_file;
        const unsigned site_line = site.isValid() ? site.getLine() : spelling_line;
        const unsigned site_column = site.isValid() ? site.getColumn() : 0;
        const FindingKey key{site_file,
                             site_line,
                             site_column,
                             source_name,
                             source.pointer_depth,
                             source.reference,
                             target_name,
                             target.pointer_depth,
                             target.reference};
        if (!findings_.insert(key).second) {
            return;
        }

        diag(cast->getBeginLoc(),
             "conversion from repository record '%0' to repository record '%1' crosses "
             "incompatible source types (%0%2 to %1%3); repair the owning declaration")
            << source_name << target_name << source_indirection << target_indirection;
        if (provenance.erasure.isValid()) {
            diag(provenance.erasure,
                 "source record '%0%1' is erased here before conversion to '%2%3'",
                 DiagnosticIDs::Note)
                << source_name << source_indirection << target_name << target_indirection;
        }
    }

private:
    using FindingKey = std::tuple<std::string, unsigned, unsigned, std::string, unsigned, bool,
                                  std::string, unsigned, bool>;

    AddedLineFilter added_lines_;
    llvm::DenseMap<const VarDecl*, bool> local_definition_cache_;
    std::set<FindingKey> findings_;
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

static ClangTidyModuleRegistry::Add<WizardryTidyModule>
    module("wiz8-module", "Adds Wizardry reconstruction checks.");

} // namespace
} // namespace clang::tidy::wiz8
