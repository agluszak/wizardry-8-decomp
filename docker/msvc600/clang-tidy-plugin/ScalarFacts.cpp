#include "ScalarFacts.h"

#include "clang-tidy/ClangTidyCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallString.h"

namespace clang::tidy::wiz8::scalar {
namespace {

// This visitor records source observations, never recovered type conclusions.
// Arithmetic and conversions remain uses/barriers, rather than copy edges.
class ScalarFactVisitor final : public RecursiveASTVisitor<ScalarFactVisitor> {
public:
    ScalarFactVisitor(ASTContext& context, FactWriter& writer) : context_(context), writer_(writer)
    {
    }

    bool TraverseDecl(Decl* declaration)
    {
        auto* previous = function_;
        // RAV dispatches methods, constructors and conversions through their
        // own Traverse*Decl entry points, not TraverseFunctionDecl.
        if (auto* function = dyn_cast_or_null<FunctionDecl>(declaration))
            function_ = function;
        const bool result = RecursiveASTVisitor<ScalarFactVisitor>::TraverseDecl(declaration);
        function_ = previous;
        return result;
    }

    bool TraverseLambdaExpr(LambdaExpr* lambda)
    {
        auto* previous = function_;
        function_ = lambda->getCallOperator();
        const bool result = RecursiveASTVisitor<ScalarFactVisitor>::TraverseLambdaExpr(lambda);
        function_ = previous;
        return result;
    }

    bool TraverseConstructorInitializer(CXXCtorInitializer* initializer)
    {
        if (initializer != nullptr && initializer->isMemberInitializer()) {
            transfer(initializer->getMember(), initializer->getInit(), "initializer",
                     initializer->getSourceLocation());
        }
        return RecursiveASTVisitor<ScalarFactVisitor>::TraverseConstructorInitializer(initializer);
    }

    bool VisitVarDecl(VarDecl* variable)
    {
        declare(variable);
        if (variable->hasInit())
            transfer(variable, variable->getInit(), "initializer", variable->getLocation());
        if (variable->getType()->isReferenceType() && variable->hasInit())
            escape(variable->getInit(), "reference binding", variable->getLocation());
        return true;
    }

    bool VisitFieldDecl(FieldDecl* field)
    {
        declare(field);
        if (field->hasInClassInitializer())
            transfer(field, field->getInClassInitializer(), "initializer", field->getLocation());
        return true;
    }

    bool VisitFunctionDecl(FunctionDecl* function)
    {
        declare(function);
        for (const auto* parameter : function->parameters())
            declare(parameter);
        if (function->doesThisDeclarationHaveABody()) {
            event("H", function, "body", function->getLocation());
            for (const auto* parameter : function->parameters())
                event("H", parameter, "body", function->getLocation());
        }
        if (const auto* method = dyn_cast<CXXMethodDecl>(function)) {
            if (method->isVirtual())
                escape_signature(function, "virtual slot", function->getLocation());
        }
        return true;
    }

    bool VisitDeclRefExpr(DeclRefExpr* reference)
    {
        if (const auto* function = dyn_cast<FunctionDecl>(reference->getDecl())) {
            if (!direct_callees_.count(reference))
                escape_signature(function, "function pointer", reference->getLocation());
        }
        return true;
    }

    bool VisitReturnStmt(ReturnStmt* statement)
    {
        if (function_ != nullptr)
            transfer(function_, statement->getRetValue(), "return", statement->getBeginLoc());
        return true;
    }

    bool VisitBinaryOperator(BinaryOperator* binary)
    {
        if (binary->getOpcode() == BO_Assign) {
            transfer(resolve(binary->getLHS()), binary->getRHS(), "assignment",
                     binary->getOperatorLoc());
        } else {
            use(binary->getLHS(), binary->getOpcodeStr().str(), binary->getOperatorLoc());
            use(binary->getRHS(), binary->getOpcodeStr().str(), binary->getOperatorLoc());
            if (binary->isCompoundAssignmentOp())
                event("A", resolve(binary->getLHS()), "compound write", binary->getOperatorLoc());
        }
        return true;
    }

    bool VisitUnaryOperator(UnaryOperator* unary)
    {
        if (unary->getOpcode() == UO_AddrOf)
            escape(unary->getSubExpr(), "address taken", unary->getOperatorLoc());
        else
            use(unary->getSubExpr(), UnaryOperator::getOpcodeStr(unary->getOpcode()).str(),
                unary->getOperatorLoc());
        return true;
    }

    bool VisitArraySubscriptExpr(ArraySubscriptExpr* subscript)
    {
        use(subscript->getIdx(), "index", subscript->getExprLoc());
        return true;
    }

    bool VisitSwitchStmt(SwitchStmt* statement)
    {
        use(statement->getCond(), "switch", statement->getSwitchLoc());
        return true;
    }

    bool VisitCallExpr(CallExpr* call)
    {
        if (const auto* reference = dyn_cast<DeclRefExpr>(call->getCallee()->IgnoreParenImpCasts()))
            direct_callees_.insert(reference);
        const auto* function = call->getDirectCallee();
        for (unsigned index = 0; index < call->getNumArgs(); ++index) {
            const Expr* argument = call->getArg(index);
            if (function == nullptr || index >= function->getNumParams()) {
                escape(argument, "indirect or variadic call", argument->getExprLoc());
                continue;
            }
            const auto* parameter = function->getParamDecl(index);
            transfer(parameter, argument, "argument", argument->getExprLoc());
            if (parameter->getType()->isReferenceType() || parameter->getType()->isPointerType()) {
                escape(argument, "indirect storage argument", argument->getExprLoc());
                escape_record(argument->IgnoreParenImpCasts()->getType(), argument->getExprLoc());
            } else if (writer_.key(parameter).empty()) {
                escape(argument, "unmodeled ABI parameter", argument->getExprLoc());
            }
        }
        return true;
    }

    bool VisitCXXConstructExpr(CXXConstructExpr* construct)
    {
        const auto* constructor = construct->getConstructor();
        for (unsigned index = 0; constructor != nullptr && index < construct->getNumArgs();
             ++index) {
            const auto* argument = construct->getArg(index);
            if (index >= constructor->getNumParams()) {
                escape(argument, "variadic construction", argument->getExprLoc());
                continue;
            }
            const auto* parameter = constructor->getParamDecl(index);
            transfer(parameter, argument, "argument", argument->getExprLoc());
            if (parameter->getType()->isReferenceType() || parameter->getType()->isPointerType())
                escape(argument, "indirect construction argument", argument->getExprLoc());
        }
        return true;
    }

private:
    const NamedDecl* resolve(const Expr* expression) const
    {
        if (expression == nullptr)
            return nullptr;
        expression = expression->IgnoreParenImpCasts();
        if (const auto* reference = dyn_cast<DeclRefExpr>(expression))
            return canonical_scalar_declaration(dyn_cast<ValueDecl>(reference->getDecl()));
        if (const auto* member = dyn_cast<MemberExpr>(expression))
            return canonical_scalar_declaration(member->getMemberDecl());
        if (const auto* call = dyn_cast<CallExpr>(expression))
            return canonical_scalar_declaration(call->getDirectCallee());
        return nullptr;
    }

    void declare(const NamedDecl* declaration)
    {
        if (const auto* candidate = canonical_scalar_declaration(declaration))
            writer_.declaration(candidate);
    }

    void event(const char* tag, const NamedDecl* declaration, const std::string& detail,
               SourceLocation location)
    {
        if (declaration == nullptr)
            return;
        declare(declaration);
        const std::string key = writer_.key(declaration);
        const auto point = source_point(context_.getSourceManager(), location);
        if (!key.empty() && point)
            writer_.fact({tag, key, detail, point.file, std::to_string(point.line),
                          std::to_string(point.column)});
    }

    void use(const Expr* expression, const std::string& operation, SourceLocation location)
    {
        if (expression == nullptr)
            return;
        if (const auto* declaration = resolve(expression)) {
            event("U", declaration, operation, location);
            return;
        }
        expression = expression->IgnoreParenImpCasts();
        if (const auto* cast = dyn_cast<ExplicitCastExpr>(expression))
            use(cast->getSubExpr(), "cast:" + operation, location);
    }

    void escape(const Expr* expression, const std::string& reason, SourceLocation location)
    {
        if (expression == nullptr)
            return;
        if (const auto* declaration = resolve(expression)) {
            event("A", declaration, reason, location);
            return;
        }
        expression = expression->IgnoreParenImpCasts();
        if (const auto* unary = dyn_cast<UnaryOperator>(expression))
            escape(unary->getSubExpr(), reason, location);
        if (const auto* cast = dyn_cast<ExplicitCastExpr>(expression))
            escape(cast->getSubExpr(), reason, location);
    }

    void escape_record(QualType type, SourceLocation location)
    {
        type = type.getCanonicalType();
        if (type->isPointerType() || type->isReferenceType())
            type = type->getPointeeType();
        const auto* record = type->getAs<RecordType>();
        if (record == nullptr || record->getDecl()->getDefinition() == nullptr)
            return;
        for (const auto* field : record->getDecl()->getDefinition()->fields()) {
            event("A", field, "aggregate storage argument", location);
            if (field->getType()->isRecordType())
                escape_record(field->getType(), location);
        }
    }

    void escape_signature(const FunctionDecl* function, const std::string& reason,
                          SourceLocation location)
    {
        event("A", function, reason, location);
        for (const auto* parameter : function->parameters())
            event("A", parameter, reason, location);
    }

    void transfer(const NamedDecl* target, const Expr* value, const char* role,
                  SourceLocation location)
    {
        target = canonical_scalar_declaration(target);
        if (target == nullptr)
            return;
        declare(target);
        if (value == nullptr) {
            event("A", target, "missing value", location);
            return;
        }
        value = value->IgnoreParenImpCasts();
        if (const auto* cleanup = dyn_cast<ExprWithCleanups>(value)) {
            transfer(target, cleanup->getSubExpr(), role, location);
            return;
        }
        if (const auto* argument = dyn_cast<CXXDefaultArgExpr>(value)) {
            transfer(target, argument->getExpr(), role, location);
            return;
        }
        if (const auto* conditional = dyn_cast<ConditionalOperator>(value)) {
            transfer(target, conditional->getTrueExpr(), role, location);
            transfer(target, conditional->getFalseExpr(), role, location);
            return;
        }
        if (const auto* cast = dyn_cast<ExplicitCastExpr>(value)) {
            event("A", target, "explicit conversion", location);
            escape(cast->getSubExpr(), "explicit conversion", location);
            if (const auto* source = resolve(cast->getSubExpr())) {
                declare(source);
                const auto point = source_point(context_.getSourceManager(), location);
                const auto from = writer_.key(source);
                const auto to = writer_.key(target);
                if (point && !from.empty() && !to.empty()) {
                    writer_.fact({"F", to, from, "explicit-conversion", point.file,
                                  std::to_string(point.line), std::to_string(point.column)});
                }
            }
            return;
        }
        const auto point = source_point(context_.getSourceManager(), location);
        if (!point)
            return;
        if (const auto* source = resolve(value)) {
            declare(source);
            const auto from = writer_.key(source);
            const auto to = writer_.key(target);
            if (from.empty() || to.empty()) {
                event("A", target, "unmodeled value producer", location);
                return;
            }
            writer_.fact({"F", to, from, role, point.file, std::to_string(point.line),
                          std::to_string(point.column)});
            if (context_.getTypeSize(declaration_type(source)) !=
                context_.getTypeSize(declaration_type(target))) {
                event("A", target, "width-changing conversion", location);
                event("A", source, "width-changing conversion", location);
            }
            return;
        }
        if (value->getType()->isBooleanType()) {
            event("G", target, "bool", location);
            return;
        }
        Expr::EvalResult result;
        if (value->EvaluateAsInt(result, context_) && !result.HasSideEffects) {
            const auto integer = result.Val.getInt();
            llvm::SmallString<32> text;
            integer.toString(text, 10);
            writer_.fact({"K", writer_.key(target), text.str().str(), point.file,
                          std::to_string(point.line), std::to_string(point.column)});
        } else {
            // Record the producer separately: its current source result type is
            // not independent evidence about retail signedness or storage width.
            event("A", target, "unmodeled expression producer", location);
        }
    }

    ASTContext& context_;
    FactWriter& writer_;
    FunctionDecl* function_ = nullptr;
    llvm::SmallPtrSet<const DeclRefExpr*, 32> direct_callees_;
};
} // namespace

void collect_scalar_facts(ASTContext& context, const TranslationUnitDecl* translation_unit,
                          FactWriter& writer)
{
    ScalarFactVisitor visitor(context, writer);
    visitor.TraverseDecl(const_cast<TranslationUnitDecl*>(translation_unit));
}

namespace {
class ScalarFactsCheck final : public ClangTidyCheck {
public:
    ScalarFactsCheck(llvm::StringRef name, ClangTidyContext* context)
        : ClangTidyCheck(name, context)
    {
    }

    void registerMatchers(ast_matchers::MatchFinder* finder) override
    {
        finder->addMatcher(ast_matchers::translationUnitDecl().bind("translation_unit"), this);
    }

    void check(const ast_matchers::MatchFinder::MatchResult& result) override
    {
        context_ = result.Context;
        translation_unit_ = result.Nodes.getNodeAs<TranslationUnitDecl>("translation_unit");
    }

    void onEndOfTranslationUnit() override
    {
        if (context_ != nullptr && translation_unit_ != nullptr) {
            FactWriter writer(*context_, translation_unit_);
            if (writer.enabled())
                collect_scalar_facts(*context_, translation_unit_, writer);
        }
        context_ = nullptr;
        translation_unit_ = nullptr;
    }

private:
    ASTContext* context_ = nullptr;
    const TranslationUnitDecl* translation_unit_ = nullptr;
};

class ScalarFactsModule final : public ClangTidyModule {
public:
    void addCheckFactories(ClangTidyCheckFactories& factories) override
    {
        factories.registerCheck<ScalarFactsCheck>("wiz8-scalar-facts");
    }
};
static ClangTidyModuleRegistry::Add<ScalarFactsModule>
    module("wiz8-scalar-facts-module", "Collects whole-program scalar source facts.");
} // namespace
} // namespace clang::tidy::wiz8::scalar
