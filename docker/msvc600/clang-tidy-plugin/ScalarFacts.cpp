#include "ScalarFacts.h"

#include "clang-tidy/ClangTidyCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "clang/AST/ExprCXX.h"
#include "clang/AST/TypeLoc.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Attr.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/SHA256.h"

#include <unordered_set>

namespace clang::tidy::wiz8::scalar {
namespace {

// This visitor records source observations, never recovered type conclusions.
// Arithmetic and conversions remain uses/barriers, rather than copy edges.
class ScalarFactVisitor final : public RecursiveASTVisitor<ScalarFactVisitor> {
public:
    ScalarFactVisitor(ASTContext& context, FactWriter& writer) : context_(context), names_(context), writer_(writer)
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
            field_reference(initializer->getMember(), initializer->getSourceLocation());
            transfer(initializer->getMember(), initializer->getInit(), "initializer",
                     initializer->getSourceLocation());
        }
        return RecursiveASTVisitor<ScalarFactVisitor>::TraverseConstructorInitializer(initializer);
    }

    bool VisitVarDecl(VarDecl* variable)
    {
        declare(variable);
        if (variable->hasInit()) {
            if (array_type(variable) != nullptr)
                array_initializer(variable, variable->getInit());
            else if (variable->getType()->isRecordType())
                record_initializer(variable->getType(), variable->getInit());
            else
                transfer(variable, variable->getInit(), "initializer", variable->getLocation());
        }
        if (variable->getType()->isReferenceType() && variable->hasInit())
            escape(variable->getInit(), "reference binding", variable->getLocation());
        return true;
    }

    bool VisitRecordDecl(RecordDecl* record)
    {
        if (!record->isCompleteDefinition() || record->isDependentType() || record->isInvalidDecl())
            return true;
        const auto point = source_point(context_.getSourceManager(), record->getLocation());
        if (!point)
            return true;
        const auto key = owner_key(record);
        if (!records_.insert(key).second)
            return true;
        const auto& layout = context_.getASTRecordLayout(record);
        // #pragma pack is an explicit authored source control, distinct from
        // the compiled layout it produces.
        const auto* packing = record->getAttr<MaxFieldAlignmentAttr>();
        bool simple = !record->isUnion();
        if (const auto* cxx = dyn_cast<CXXRecordDecl>(record))
            simple = simple && cxx->getNumBases() == 0 && !cxx->isPolymorphic();
        unsigned index = 0;
        uint64_t natural = 0, alignment = 1;
        // `int a, b;` shares one declaration statement; only a sole declarator
        // owns a removable field-declaration span.
        std::unordered_map<unsigned, unsigned> declarators;
        for (const auto* field : record->fields())
            ++declarators[field->getBeginLoc().getRawEncoding()];
        for (const auto* field : record->fields()) {
            const auto type = field->getType();
            if (field->isBitField() || type->isIncompleteType() || type->isDependentType()) {
                simple = false;
                ++index;
                continue;
            }
            const auto size = context_.getTypeSize(type);
            const auto align = context_.getTypeAlign(type);
            natural = ((natural + align - 1) / align) * align + size;
            alignment = std::max<uint64_t>(alignment, align);
            if (field->hasAttrs())
                simple = false;
            writer_.fact({"RF", key, owner_key(field), field->getNameAsString(),
                          std::to_string(layout.getFieldOffset(index++)), std::to_string(size),
                          std::to_string(align), type.getCanonicalType().getAsString()});
            if (declarators[field->getBeginLoc().getRawEncoding()] == 1)
                declaration_span(field);
        }
        natural = ((natural + alignment - 1) / alignment) * alignment;
        writer_.fact({"REC", key, point.file, std::to_string(point.line),
                      record->getQualifiedNameAsString(),
                      std::to_string(layout.getSize().getQuantity() * 8),
                      std::to_string(layout.getAlignment().getQuantity() * 8),
                      simple ? std::to_string(natural) : "unknown",
                      std::to_string(packing == nullptr ? 0 : packing->getAlignment())});
        return true;
    }

    bool VisitFieldDecl(FieldDecl* field)
    {
        declare(field);
        if (field->hasInClassInitializer()) {
            if (array_type(field) != nullptr)
                array_initializer(field, field->getInClassInitializer());
            else
                transfer(field, field->getInClassInitializer(), "initializer",
                         field->getLocation());
        }
        return true;
    }

    bool VisitFunctionDecl(FunctionDecl* function)
    {
        const auto* canonical = function->getCanonicalDecl();
        const auto point = source_point(context_.getSourceManager(), canonical->getLocation());
        if (const auto* prototype = function->getType()->getAs<FunctionProtoType>();
            point && prototype != nullptr && !function->isImplicit() &&
            !function->isDependentContext() && !function->getDescribedFunctionTemplate())
            writer_.fact({"FN", owner_key(canonical), function->getQualifiedNameAsString(),
                          std::to_string(prototype->getNumParams()),
                          std::to_string(static_cast<unsigned>(prototype->getCallConv())),
                          prototype->isVariadic() ? "1" : "0", names_.getName(canonical)});
        declare(function);
        for (const auto* parameter : function->parameters())
            declare(parameter);
        if (function->doesThisDeclarationHaveABody()) {
            // Every collected body, including void and record-returning functions.
            if (point)
                writer_.fact({"HB", owner_key(canonical)});
            event("H", function, "body", function->getLocation());
            for (const auto* parameter : function->parameters())
                event("H", parameter, "body", function->getLocation());
        }
        if (const auto* method = dyn_cast<CXXMethodDecl>(function)) {
            if (method->isVirtual())
                escape_signature(function, "virtual slot", function->getLocation());
            // Override topology lets the solver decide whether a virtual call
            // can only reach collected bodies.
            if (point && method->isVirtual() && method == method->getCanonicalDecl()) {
                if (method->isPureVirtual())
                    writer_.fact({"PV", owner_key(canonical)});
                for (const auto* overridden : method->overridden_methods())
                    writer_.fact({"OV", owner_key(overridden->getCanonicalDecl()),
                                  owner_key(canonical)});
            }
        }
        return true;
    }

    bool VisitDeclRefExpr(DeclRefExpr* reference)
    {
        if (array_type(reference->getDecl()) != nullptr) {
            declare(reference->getDecl());
            array_use(reference->getDecl(), "reference", reference->getLocation());
        }
        if (const auto* function = dyn_cast<FunctionDecl>(reference->getDecl())) {
            if (!direct_callees_.count(reference))
                escape_signature(function, "function pointer", reference->getLocation());
        }
        return true;
    }

    bool VisitMemberExpr(MemberExpr* member)
    {
        if (const auto* field = dyn_cast<FieldDecl>(member->getMemberDecl()))
            field_reference(field, member->getMemberLoc());
        return true;
    }

    bool VisitOffsetOfExpr(OffsetOfExpr* offset)
    {
        for (unsigned index = 0; index < offset->getNumComponents(); ++index)
            if (offset->getComponent(index).getKind() == OffsetOfNode::Field)
                field_reference(offset->getComponent(index).getField(), offset->getBeginLoc());
        return true;
    }

    bool VisitDesignatedInitExpr(DesignatedInitExpr* initializer)
    {
        for (const auto& designator : initializer->designators())
            if (designator.isFieldDesignator() && designator.getFieldDecl() != nullptr)
                field_reference(designator.getFieldDecl(), initializer->getBeginLoc());
        return true;
    }

    bool VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr* trait)
    {
        if (trait->getKind() != UETT_SizeOf || trait->isArgumentType())
            return true;
        const auto* argument = trait->getArgumentExpr()->IgnoreParens();
        if (const auto* array = array_owner(argument)) {
            declare(array);
            // sizeof(array) depends on the extent; sizeof(array[0]) only on the element.
            array_use(array,
                      isa<DeclRefExpr>(argument) || isa<MemberExpr>(argument) ? "sizeof"
                                                                               : "sizeof-element",
                      trait->getExprLoc());
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
            operand(binary->getLHS(), binary->getRHS(), binary->getOpcodeStr().str(),
                    binary->getOperatorLoc());
            operand(binary->getRHS(), binary->getLHS(), "rhs:" + binary->getOpcodeStr().str(),
                    binary->getOperatorLoc());
            if (binary->isCompoundAssignmentOp())
                event("A", resolve(binary->getLHS()), "compound write", binary->getOperatorLoc());
        }
        return true;
    }

    bool VisitUnaryOperator(UnaryOperator* unary)
    {
        if (unary->getOpcode() == UO_AddrOf) {
            if (!callback_addresses_.count(unary))
                escape(unary->getSubExpr(), "address taken", unary->getOperatorLoc());
        } else
            use(unary->getSubExpr(), UnaryOperator::getOpcodeStr(unary->getOpcode()).str(),
                unary->getOperatorLoc());
        return true;
    }

    bool VisitArraySubscriptExpr(ArraySubscriptExpr* subscript)
    {
        use(subscript->getIdx(), "index", subscript->getExprLoc());
        if (const auto* array = array_owner(subscript->getBase())) {
            declare(array);
            array_use(array, "indexed", subscript->getExprLoc());
            Expr::EvalResult result;
            if (subscript->getIdx()->EvaluateAsInt(result, context_) && !result.HasSideEffects)
                array_use(array, "index:" + llvm::toString(result.Val.getInt(), 10),
                          subscript->getExprLoc());
        }
        return true;
    }

    bool VisitSwitchStmt(SwitchStmt* statement)
    {
        use(statement->getCond(), "switch", statement->getSwitchLoc());
        for (const SwitchCase* item = statement->getSwitchCaseList(); item != nullptr;
             item = item->getNextSwitchCase()) {
            if (const auto* label = dyn_cast<CaseStmt>(item))
                operand(statement->getCond(), label->getLHS(), "case", label->getCaseLoc());
        }
        return true;
    }

    bool VisitCallExpr(CallExpr* call)
    {
        if (const auto* reference = dyn_cast<DeclRefExpr>(call->getCallee()->IgnoreParenImpCasts()))
            direct_callees_.insert(reference);
        const auto* function = call->getDirectCallee();
        const auto* slot = function == nullptr ? resolve(call->getCallee()) : nullptr;
        if (slot != nullptr)
            callback_signature(slot, call->getExprLoc());
        if (const auto* member = dyn_cast<CXXMemberCallExpr>(call)) {
            const Expr* receiver = member->getImplicitObjectArgument();
            // Virtual dispatch is resolved against the collected override closure.
            if (receiver != nullptr)
                escape_record(receiver->getType(), call->getExprLoc(), member->getMethodDecl());
        }
        for (unsigned index = 0; index < call->getNumArgs(); ++index) {
            const Expr* argument = call->getArg(index);
            if (function == nullptr && slot != nullptr) {
                const auto* prototype = callback_type(node_type(slot));
                if (prototype != nullptr && index < prototype->getNumParams()) {
                    transfer_key(callback_key(slot, index), argument, "callback-argument",
                                 argument->getExprLoc());
                    // Indirect pointee mutation is distinct from passing a pointer value.
                    if (prototype->getParamType(index)->isReferenceType())
                        escape(argument, "indirect storage argument", argument->getExprLoc());
                    continue;
                }
            }
            if (function == nullptr || index >= function->getNumParams()) {
                escape(argument, "indirect or variadic call", argument->getExprLoc());
                continue;
            }
            const auto* parameter = function->getParamDecl(index);
            transfer(parameter, argument, "argument", argument->getExprLoc());
            if (parameter->getType()->isReferenceType() || parameter->getType()->isPointerType()) {
                escape(argument, "indirect storage argument", argument->getExprLoc());
                escape_record(argument->IgnoreParenImpCasts()->getType(), argument->getExprLoc(),
                              function);
                if (const auto* array = array_owner(argument)) {
                    declare(array);
                    event("A", array, "array storage argument", argument->getExprLoc());
                    array_use(array, "call:" + function->getNameAsString(), argument->getExprLoc());
                    // `names[i]` of a pointer array passes an element value, and
                    // `rows[i]` of a 2-D array an inner array: neither is this
                    // array's own character storage.
                    const auto element = array_type(array)->getElementType();
                    if (!element->isPointerType() && !element->isArrayType())
                        storage_consumer(array, function, call, index, argument->getExprLoc());
                }
            } else if (node_key(parameter).empty()) {
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
        if (const auto* reference = dyn_cast<DeclRefExpr>(expression)) {
            // A function address is not the function's return value.
            if (isa<FunctionDecl>(reference->getDecl()))
                return nullptr;
            return array_type(reference->getDecl()) == nullptr
                       ? canonical_scalar_declaration(dyn_cast<ValueDecl>(reference->getDecl()))
                       : nullptr;
        }
        if (const auto* member = dyn_cast<MemberExpr>(expression)) {
            if (isa<FunctionDecl>(member->getMemberDecl()))
                return nullptr;
            return array_type(member->getMemberDecl()) == nullptr
                       ? canonical_scalar_declaration(member->getMemberDecl())
                       : nullptr;
        }
        if (const auto* call = dyn_cast<CallExpr>(expression))
            return canonical_scalar_declaration(call->getDirectCallee());
        if (const auto* indexed = dyn_cast<ArraySubscriptExpr>(expression))
            return node_declaration(array_owner(indexed->getBase()));
        return nullptr;
    }

    void array_use(const NamedDecl* owner, const std::string& role, SourceLocation location)
    {
        const auto point = source_point(context_.getSourceManager(), location);
        if (point && !owner_key(owner).empty())
            writer_.fact({"AU", owner_key(owner), role, point.file, std::to_string(point.line),
                          std::to_string(point.column)});
    }

    void record_initializer(QualType type, const Expr* value)
    {
        if (value == nullptr)
            return;
        const auto* list = dyn_cast<InitListExpr>(value->IgnoreParenImpCasts());
        const auto* record = type->getAs<RecordType>();
        if (list == nullptr || record == nullptr || record->getDecl()->isUnion() ||
            record->getDecl()->getDefinition() == nullptr)
            return;
        if (const auto* cxx = dyn_cast<CXXRecordDecl>(record->getDecl()->getDefinition()))
            if (cxx->getNumBases() != 0)
                return; // Base initializers are not field initializers.
        if (!list->isSemanticForm() && list->getSemanticForm() != nullptr)
            list = list->getSemanticForm();
        // Positional initialization depends on the complete declared member list.
        array_use(record->getDecl()->getDefinition(), "aggregate-initializer", list->getBeginLoc());
        unsigned index = 0;
        for (const auto* field : record->getDecl()->getDefinition()->fields()) {
            if (index >= list->getNumInits())
                break;
            const auto* init = list->getInit(index++);
            if (array_type(field) != nullptr)
                array_initializer(field, init);
            else if (field->getType()->isRecordType())
                record_initializer(field->getType(), init);
            else
                transfer(field, init, "record-initializer", init->getExprLoc());
        }
    }

    void array_initializer(const NamedDecl* owner, const Expr* value)
    {
        const auto* array = array_type(owner);
        if (array == nullptr || value == nullptr)
            return;
        value = value->IgnoreParenImpCasts();
        if (const auto* literal = dyn_cast<StringLiteral>(value)) {
            array_use(owner, "string-initializer", value->getExprLoc());
            const char* kind = literal->isWide()                         ? "wchar_t"
                               : literal->isOrdinary() || literal->isUTF8() ? "char"
                                                                          : "other";
            character(owner, kind, "literal", value->getExprLoc());
            character(owner, "units:" + std::to_string(literal->getLength() + 1), "literal",
                      value->getExprLoc());
            return;
        }
        if (const auto* list = dyn_cast<InitListExpr>(value)) {
            for (const auto* init : list->inits()) {
                if (array->getElementType()->isRecordType())
                    record_initializer(array->getElementType(), init);
                else
                    transfer(owner, init, "array-initializer", init->getExprLoc());
            }
            if (const auto* filler = list->getArrayFiller()) {
                if (isa<ImplicitValueInitExpr>(filler) && !node_key(owner).empty()) {
                    const auto point =
                        source_point(context_.getSourceManager(), value->getExprLoc());
                    if (point)
                        writer_.fact({"K", node_key(owner), "0", point.file,
                                      std::to_string(point.line), std::to_string(point.column)});
                } else if (array->getElementType()->isScalarType())
                    transfer(owner, filler, "array-filler", value->getExprLoc());
            }
        } else {
            event("A", owner, "unmodeled array initializer", value->getExprLoc());
        }
    }

    const ConstantArrayType* array_type(const NamedDecl* declaration) const
    {
        const auto* value = dyn_cast_or_null<ValueDecl>(declaration);
        if (value == nullptr || value->isInvalidDecl() || value->getType()->isDependentType())
            return nullptr;
        return context_.getAsConstantArrayType(value->getType());
    }

    const NamedDecl* array_owner(const Expr* expression) const
    {
        if (expression == nullptr)
            return nullptr;
        expression = expression->IgnoreParenImpCasts();
        if (const auto* cast = dyn_cast<ExplicitCastExpr>(expression))
            return array_owner(cast->getSubExpr());
        if (const auto* unary = dyn_cast<UnaryOperator>(expression))
            return array_owner(unary->getSubExpr());
        const NamedDecl* result = nullptr;
        if (const auto* reference = dyn_cast<DeclRefExpr>(expression))
            result = reference->getDecl();
        if (const auto* member = dyn_cast<MemberExpr>(expression))
            result = member->getMemberDecl();
        if (const auto* indexed = dyn_cast<ArraySubscriptExpr>(expression))
            return array_owner(indexed->getBase());
        if (array_type(result) == nullptr)
            return nullptr;
        if (const auto* variable = dyn_cast<VarDecl>(result))
            return variable->getCanonicalDecl();
        if (const auto* field = dyn_cast<FieldDecl>(result)) {
            if (!field->isBitField() && !cast<RecordDecl>(field->getDeclContext())->isUnion())
                return field;
        }
        return nullptr;
    }

    const NamedDecl* node_declaration(const NamedDecl* declaration) const
    {
        if (declaration == nullptr || declaration->getDeclContext()->isDependentContext())
            return nullptr;
        if (const auto* array = array_type(declaration)) {
            if (array->getElementType()->isScalarType() &&
                !array->getElementType()->isDependentType()) {
                if (const auto* variable = dyn_cast<VarDecl>(declaration))
                    return variable->getCanonicalDecl();
                if (const auto* field = dyn_cast<FieldDecl>(declaration))
                    if (!field->isBitField() &&
                        !cast<RecordDecl>(field->getDeclContext())->isUnion())
                        return field;
            }
            return nullptr;
        }
        return canonical_scalar_declaration(declaration);
    }

    QualType node_type(const NamedDecl* declaration) const
    {
        if (const auto* array = array_type(declaration))
            return array->getElementType();
        return declaration_type(declaration);
    }

    std::string owner_key(const NamedDecl* declaration) const
    {
        if (declaration == nullptr)
            return {};
        const auto point = source_point(context_.getSourceManager(), declaration->getLocation());
        if (!point)
            return {};
        return point.file + ":" + std::to_string(point.line) + ":" + std::to_string(point.column) +
               ":" + candidate_kind(declaration) + ":" + candidate_key_name(declaration);
    }

    std::string node_key(const NamedDecl* declaration) const
    {
        if (array_type(declaration) != nullptr) {
            const auto* candidate = node_declaration(declaration);
            return candidate == nullptr ? "" : owner_key(candidate) + "::element";
        }
        return writer_.key(declaration);
    }

    void declare(const NamedDecl* declaration)
    {
        if (declaration == nullptr || declaration->getDeclContext()->isDependentContext())
            return;
        if (const auto* array = array_type(declaration)) {
            source_span(declaration);
            if (const auto* variable = dyn_cast<VarDecl>(declaration))
                declaration = variable->getCanonicalDecl();
            const auto key = owner_key(declaration);
            const auto point =
                source_point(context_.getSourceManager(), declaration->getLocation());
            if (point && !key.empty() && arrays_.insert(key).second) {
                writer_.fact({"ARR", key, point.file, std::to_string(point.line),
                              std::to_string(point.column), declaration->getQualifiedNameAsString(),
                              array->getElementType().getAsString(),
                              std::to_string(array->getSize().getLimitedValue()),
                              std::to_string(context_.getTypeSize(array->getElementType())),
                              node_key(declaration)});
                const auto element = node_key(declaration);
                if (!element.empty()) {
                    signature_node(element, array->getElementType(), "array-element", point);
                    source_span(declaration);
                }
            }
            callback_signature(declaration, declaration->getLocation());
            linkage(key, declaration);
            // Keep legacy fixed-byte-array declarations for the unchanged bool client.
            writer_.declaration(declaration);
            return;
        }
        source_span(declaration);
        if (const auto* candidate = canonical_scalar_declaration(declaration)) {
            writer_.declaration(candidate);
            type_fact(node_key(candidate), node_type(candidate));
            linkage(node_key(candidate), candidate);
        }
        callback_signature(declaration, declaration->getLocation());
    }

    void source_span(const NamedDecl* declaration)
    {
        if (declaration == nullptr || !span_declarations_.insert(declaration).second)
            return;
        const auto* declarator = dyn_cast_or_null<DeclaratorDecl>(declaration);
        if (declarator == nullptr || declarator->getTypeSourceInfo() == nullptr ||
            node_declaration(declaration) == nullptr)
            return;
        TypeLoc type = declarator->getTypeSourceInfo()->getTypeLoc();
        std::string component = isa<ParmVarDecl>(declaration) ? "parameter-type" : "type";
        if (isa<FunctionDecl>(declarator)) {
            auto function = type.getAs<FunctionTypeLoc>();
            if (function.isNull())
                return;
            type = function.getReturnLoc();
            component = "return-type";
        }
        component_span(node_key(declaration), type, component);
    }

    void component_span(const std::string& key, TypeLoc type, const std::string& component)
    {
        if (key.empty() || type.isNull())
            return;
        type = type.getUnqualifiedLoc();
        if (auto attributed = type.getAs<AttributedTypeLoc>(); !attributed.isNull()) {
            component_span(key, attributed.getModifiedLoc(), component);
            return;
        }
        if (auto elaborated = type.getAs<ElaboratedTypeLoc>(); !elaborated.isNull()) {
            component_span(key, elaborated.getNamedTypeLoc(), component);
            return;
        }
        if (auto paren = type.getAs<ParenTypeLoc>(); !paren.isNull()) {
            component_span(key, paren.getInnerLoc(), component);
            return;
        }
        if (auto array = type.getAs<ArrayTypeLoc>(); !array.isNull()) {
            if (const auto* size = array.getSizeExpr())
                protected_span(key, "array-extent", size->getSourceRange());
            component_span(key, array.getElementLoc(), "array-element");
            return;
        }
        if (auto alias = type.getAs<TypedefTypeLoc>();
            !alias.isNull() && callback_type(type.getType()) != nullptr) {
            // All collected uses of this typedef own the same protected atoms.
            // The patcher rejects a change unless every owner agrees.
            const auto* info = alias.getTypedefNameDecl()->getTypeSourceInfo();
            if (info != nullptr)
                component_span(key, info->getTypeLoc(), component);
            return;
        }
        if (auto pointer = type.getAs<PointerTypeLoc>(); !pointer.isNull()) {
            component_span(key, pointer.getPointeeLoc(), component + ":pointee");
            return;
        }
        if (auto function = type.getAs<FunctionProtoTypeLoc>(); !function.isNull()) {
            component_span(key + "::callback-return", function.getReturnLoc(), "callback-return");
            for (unsigned index = 0; index < function.getNumParams(); ++index) {
                const auto* parameter = function.getParam(index);
                if (parameter != nullptr && parameter->getTypeSourceInfo() != nullptr)
                    component_span(key + "::callback-arg#" + std::to_string(index),
                                   parameter->getTypeSourceInfo()->getTypeLoc(),
                                   "callback-param#" + std::to_string(index));
            }
            return;
        }
        if (!type.getAs<BuiltinTypeLoc>().isNull() || !type.getAs<TypedefTypeLoc>().isNull() ||
            !type.getAs<EnumTypeLoc>().isNull())
            protected_span(key, component, type.getSourceRange());
    }

    void protected_span(const std::string& key, const std::string& component, SourceRange range)
    {
        if (range.getBegin().isMacroID() || range.getEnd().isMacroID())
            return;
        protected_range(key, component, range.getBegin(),
                        Lexer::getLocForEndOfToken(range.getEnd(), 0, context_.getSourceManager(),
                                                   context_.getLangOpts()));
    }

    // A complete `T name[N];` member declaration, through its semicolon.
    void declaration_span(const FieldDecl* field)
    {
        const SourceRange range = field->getSourceRange();
        if (range.getBegin().isMacroID() || range.getEnd().isMacroID())
            return;
        protected_range(owner_key(field), "field-declaration", range.getBegin(),
                        Lexer::findLocationAfterToken(range.getEnd(), tok::semi,
                                                      context_.getSourceManager(),
                                                      context_.getLangOpts(), false));
    }

    void protected_range(const std::string& key, const std::string& component,
                         SourceLocation begin, SourceLocation end)
    {
        auto& sources = context_.getSourceManager();
        const auto point = source_point(sources, begin);
        if (key.empty() || !point || end.isInvalid() ||
            sources.getFileID(begin) != sources.getFileID(end))
            return;
        const auto buffer = sources.getBufferData(sources.getFileID(begin));
        const auto offset = sources.getFileOffset(begin);
        const auto length = sources.getFileOffset(end) - offset;
        const auto file_id = sources.getFileID(begin).getHashValue();
        auto hash = source_hashes_.find(file_id);
        if (hash == source_hashes_.end()) {
            const auto digest = llvm::SHA256::hash(llvm::ArrayRef<uint8_t>(
                reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size()));
            hash = source_hashes_.emplace(file_id, llvm::toHex(digest, true)).first;
        }
        writer_.fact({"L", key, component, point.file, std::to_string(offset),
                      std::to_string(length), buffer.substr(offset, length).str(), hash->second});
    }

    void field_reference(const FieldDecl* field, SourceLocation location)
    {
        const auto point = source_point(context_.getSourceManager(), location);
        const auto key = owner_key(field);
        if (point && !key.empty())
            writer_.fact({"FR", key, point.file, std::to_string(point.line),
                          std::to_string(point.column)});
    }

    // Source-side boundary facts: who outside the collected corpus may name this entity.
    void linkage(const std::string& key, const NamedDecl* declaration)
    {
        const auto* variable = dyn_cast<VarDecl>(declaration);
        if (key.empty() || (!isa<FunctionDecl>(declaration) &&
                            (variable == nullptr || !variable->hasGlobalStorage() ||
                             variable->isStaticLocal())))
            return;
        std::string dll = "none";
        for (const auto* redeclaration : declaration->redecls()) {
            if (redeclaration->hasAttr<DLLImportAttr>())
                dll = "import";
            else if (redeclaration->hasAttr<DLLExportAttr>())
                dll = "export";
        }
        if (const auto* record = dyn_cast<CXXRecordDecl>(declaration->getDeclContext())) {
            if (record->hasAttr<DLLImportAttr>())
                dll = "import";
            else if (record->hasAttr<DLLExportAttr>())
                dll = "export";
        }
        writer_.fact({"LK", key, declaration->isExternallyVisible() ? "external" : "internal", dll});
    }

    // Character semantics come from a pointee contract; a sugared VC6
    // `wchar_t`/`WCHAR` typedef of unsigned short still names wide text.
    std::string character_kind(QualType pointee) const
    {
        for (QualType step = pointee;;) {
            if (const auto* alias = dyn_cast<TypedefType>(step.getTypePtr())) {
                const auto name = alias->getDecl()->getName();
                if (name == "wchar_t" || name == "WCHAR")
                    return "wchar_t";
            }
            const QualType next = step.getSingleStepDesugaredType(context_);
            if (next == step)
                break;
            step = next;
        }
        const auto canonical = pointee.getCanonicalType().getUnqualifiedType();
        if (canonical->isVoidType())
            return "raw-byte";
        if (const auto* builtin = canonical->getAs<BuiltinType>()) {
            switch (builtin->getKind()) {
            case BuiltinType::Char_S:
            case BuiltinType::Char_U:
                return "char";
            case BuiltinType::WChar_S:
            case BuiltinType::WChar_U:
                return "wchar_t";
            case BuiltinType::SChar:
            case BuiltinType::UChar:
                return "raw-byte";
            default:
                break;
            }
        }
        return "type:" + canonical.getAsString();
    }

    void character(const NamedDecl* array, const std::string& kind, const std::string& role,
                   SourceLocation location)
    {
        const auto point = source_point(context_.getSourceManager(), location);
        const auto key = owner_key(array);
        if (point && !key.empty())
            writer_.fact({"CH", key, kind, role, point.file, std::to_string(point.line),
                          std::to_string(point.column)});
    }

    // Storage passed to a callee: its pointee contract and any constant byte
    // count. System-header APIs are external contracts; project callees are
    // reconstruction and only constrain coherent recovery.
    void storage_consumer(const NamedDecl* array, const FunctionDecl* function,
                          const CallExpr* call, unsigned index, SourceLocation location)
    {
        const auto& sources = context_.getSourceManager();
        const auto* canonical = function->getCanonicalDecl();
        const bool external = sources.isInSystemHeader(sources.getExpansionLoc(canonical->getLocation()));
        const std::string role = std::string(external ? "api:" : "call:") +
                                 function->getQualifiedNameAsString() + "#" + std::to_string(index);
        const auto parameter_type = function->getParamDecl(index)->getType();
        if (parameter_type->isPointerType())
            character(array, character_kind(parameter_type->getPointeeType()), role, location);
        for (unsigned other = 0; other < call->getNumArgs() && other < function->getNumParams();
             ++other) {
            const auto* parameter = function->getParamDecl(other);
            const auto name = parameter->getName().lower();
            bool size_type = false;
            for (QualType step = parameter->getType();;) {
                if (const auto* alias = dyn_cast<TypedefType>(step.getTypePtr()))
                    size_type = size_type || alias->getDecl()->getName() == "size_t";
                const QualType next = step.getSingleStepDesugaredType(context_);
                if (next == step)
                    break;
                step = next;
            }
            if (!parameter->getType()->isUnsignedIntegerType() ||
                (!size_type && name.find("byte") == std::string::npos &&
                 name.find("size") == std::string::npos && name.find("len") == std::string::npos))
                continue;
            Expr::EvalResult result;
            if (call->getArg(other)->EvaluateAsInt(result, context_) && !result.HasSideEffects)
                character(array, "bytes:" + llvm::toString(result.Val.getInt(), 10), role,
                          location);
        }
    }

    void type_fact(const std::string& key, QualType type)
    {
        if (key.empty() || type->isDependentType() || type->isIncompleteType() ||
            !typed_nodes_.insert(key).second)
            return;
        const auto canonical = type.getCanonicalType();
        auto spelled = type;
        while (!isa<TypedefType>(spelled.getTypePtr()) &&
               spelled != spelled.getSingleStepDesugaredType(context_))
            spelled = spelled.getSingleStepDesugaredType(context_);
        const auto* alias = dyn_cast<TypedefType>(spelled.getTypePtr());
        writer_.fact({"T", key, canonical.getAsString(),
                      alias == nullptr ? "" : alias->getDecl()->getQualifiedNameAsString(),
                      canonical->isPointerType() && !canonical->getPointeeType()->isFunctionType()
                          ? canonical->getPointeeType().getAsString()
                          : ""});
    }

    void operand(const Expr* value, const Expr* constant, const std::string& operation,
                 SourceLocation location)
    {
        const auto* declaration = resolve(value);
        if (declaration == nullptr)
            return;
        Expr::EvalResult result;
        if (!constant->EvaluateAsInt(result, context_) || result.HasSideEffects)
            return;
        const auto point = source_point(context_.getSourceManager(), location);
        if (!point)
            return;
        declare(declaration);
        llvm::SmallString<32> text;
        result.Val.getInt().toString(text, 10);
        writer_.fact({"O", node_key(declaration), operation, text.str().str(), point.file,
                      std::to_string(point.line), std::to_string(point.column)});
    }

    const FunctionProtoType* callback_type(QualType type) const
    {
        if (!type->isPointerType())
            return nullptr;
        return type->getPointeeType()->getAs<FunctionProtoType>();
    }

    std::string callback_key(const NamedDecl* slot, unsigned index) const
    {
        const auto key = node_key(slot);
        return key.empty() ? "" : key + "::callback-arg#" + std::to_string(index);
    }

    std::string callback_return(const NamedDecl* slot) const
    {
        const auto key = node_key(slot);
        return key.empty() ? "" : key + "::callback-return";
    }

    void signature_node(const std::string& key, QualType type, const char* kind,
                        const SourcePoint& point)
    {
        if (!type->isScalarType() || type->isDependentType() || type->isIncompleteType())
            return;
        writer_.fact({"D", key, point.file, std::to_string(point.line),
                      std::to_string(point.column), kind, key, "0", "0",
                      std::to_string(context_.getTypeSize(type)), observed_signedness(type),
                      observed_domain(type), type.getAsString()});
        type_fact(key, type);
    }

    void callback_signature(const NamedDecl* slot, SourceLocation location)
    {
        const auto* prototype = callback_type(node_type(slot));
        const auto point = source_point(context_.getSourceManager(), slot->getLocation());
        if (prototype == nullptr || !point || node_key(slot).empty() ||
            !callback_slots_.insert(node_key(slot)).second)
            return;
        declare(slot);
        writer_.fact({"J", node_key(slot), std::to_string(prototype->getNumParams()),
                      std::to_string(static_cast<unsigned>(prototype->getCallConv())),
                      prototype->isVariadic() ? "1" : "0"});
        signature_node(callback_return(slot), prototype->getReturnType(), "callback-return", point);
        for (unsigned index = 0; index < prototype->getNumParams(); ++index)
            signature_node(callback_key(slot, index), prototype->getParamType(index),
                           "callback-parameter", point);
    }

    void edge(const std::string& target, const std::string& source, const char* role,
              SourceLocation location)
    {
        const auto point = source_point(context_.getSourceManager(), location);
        if (point && !target.empty() && !source.empty())
            writer_.fact({"F", target, source, role, point.file, std::to_string(point.line),
                          std::to_string(point.column)});
    }

    void transfer_key(const std::string& target, const Expr* value, const char* role,
                      SourceLocation location)
    {
        if (value == nullptr)
            return;
        value = value->IgnoreParenImpCasts();
        if (const auto* source = resolve(value)) {
            declare(source);
            edge(target, node_key(source), role, location);
        } else if (const auto* call = dyn_cast<CallExpr>(value)) {
            if (const auto* slot = resolve(call->getCallee())) {
                callback_signature(slot, location);
                edge(target, callback_return(slot), role, location);
            }
        } else {
            Expr::EvalResult result;
            const auto point = source_point(context_.getSourceManager(), location);
            if (point && value->EvaluateAsInt(result, context_) && !result.HasSideEffects) {
                llvm::SmallString<32> text;
                result.Val.getInt().toString(text, 10);
                writer_.fact({"K", target, text.str().str(), point.file, std::to_string(point.line),
                              std::to_string(point.column)});
            } else if (point) {
                writer_.fact({"A", target, "unmodeled callback producer", point.file,
                              std::to_string(point.line), std::to_string(point.column)});
            }
        }
    }

    bool bind_callback(const NamedDecl* target, const Expr* value, SourceLocation location)
    {
        const auto* prototype = callback_type(node_type(target));
        if (prototype == nullptr)
            return false;
        const Expr* expression = value->IgnoreParenImpCasts();
        const UnaryOperator* callback_address = nullptr;
        if (const auto* address = dyn_cast<UnaryOperator>(expression)) {
            if (address->getOpcode() == UO_AddrOf) {
                callback_address = address;
                expression = address->getSubExpr()->IgnoreParenImpCasts();
            }
        }
        const auto* reference = dyn_cast<DeclRefExpr>(expression);
        const auto* function =
            reference == nullptr ? nullptr : dyn_cast<FunctionDecl>(reference->getDecl());
        if (function == nullptr || function->getType()->getAs<FunctionProtoType>() == nullptr)
            return false;
        // This exact expression is represented by the callback ABI graph.
        // Other address-taking/reference sites still escape the implementation.
        direct_callees_.insert(reference);
        if (callback_address != nullptr)
            callback_addresses_.insert(callback_address);
        callback_signature(target, location);
        declare(function);
        const auto point =
            source_point(context_.getSourceManager(), function->getCanonicalDecl()->getLocation());
        if (!point)
            return false;
        const std::string identity = point.file + ":" + std::to_string(point.line) + ":" +
                                     std::to_string(point.column) +
                                     ":callback:" + function->getQualifiedNameAsString();
        writer_.fact({"C", node_key(target), identity, std::to_string(function->getNumParams()),
                      std::to_string(static_cast<unsigned>(
                          function->getType()->getAs<FunctionProtoType>()->getCallConv())),
                      function->isVariadic() ? "1" : "0"});
        edge(callback_return(target), node_key(function), "callback-return", location);
        for (unsigned index = 0; index < function->getNumParams(); ++index) {
            declare(function->getParamDecl(index));
            edge(node_key(function->getParamDecl(index)), callback_key(target, index),
                 "callback-parameter", location);
        }
        return true;
    }

    void event(const char* tag, const NamedDecl* declaration, const std::string& detail,
               SourceLocation location)
    {
        if (declaration == nullptr)
            return;
        declare(declaration);
        const std::string key = node_key(declaration);
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
        if (const auto* array = array_owner(expression)) {
            declare(array);
            event("A", array, reason, location);
            array_use(array, "escape:" + reason, location);
            return;
        }
        expression = expression->IgnoreParenImpCasts();
        if (const auto* unary = dyn_cast<UnaryOperator>(expression))
            escape(unary->getSubExpr(), reason, location);
        if (const auto* cast = dyn_cast<ExplicitCastExpr>(expression))
            escape(cast->getSubExpr(), reason, location);
    }

    // Storage handed to a direct callee escapes only if that callee's body is
    // not collected anywhere; the solver resolves `@callee` against HB facts.
    void escape_record(QualType type, SourceLocation location,
                       const FunctionDecl* callee = nullptr)
    {
        type = type.getCanonicalType();
        if (type->isPointerType() || type->isReferenceType())
            type = type->getPointeeType();
        const auto* record = type->getAs<RecordType>();
        if (record == nullptr || record->getDecl()->getDefinition() == nullptr)
            return;
        if (const auto* cxx = dyn_cast<CXXRecordDecl>(record->getDecl()->getDefinition())) {
            for (const auto& base : cxx->bases())
                escape_record(base.getType(), location, callee);
        }
        const auto* method = dyn_cast_or_null<CXXMethodDecl>(callee);
        const std::string detail =
            "aggregate storage argument" +
            (callee == nullptr ? std::string()
                               : (method != nullptr && method->isVirtual() ? "@virtual:" : "@") +
                                     owner_key(callee->getCanonicalDecl()));
        for (const auto* field : record->getDecl()->getDefinition()->fields()) {
            event("A", field, detail, location);
            if (field->getType()->isRecordType())
                escape_record(field->getType(), location, callee);
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
        target = node_declaration(target);
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
        if (bind_callback(target, value, location))
            return;
        if (const auto* call = dyn_cast<CallExpr>(value)) {
            if (call->getDirectCallee() == nullptr) {
                if (const auto* slot = resolve(call->getCallee())) {
                    callback_signature(slot, location);
                    edge(node_key(target), callback_return(slot), role, location);
                    return;
                }
            }
        }
        if (const auto* cast = dyn_cast<ExplicitCastExpr>(value)) {
            if (const auto* source = resolve(cast->getSubExpr())) {
                declare(source);
                writer_.fact({"V", node_key(target), node_key(source),
                              cast->getSubExpr()->getType().getCanonicalType().getAsString(),
                              cast->getType().getCanonicalType().getAsString()});
            }
            event("A", target, "explicit conversion", location);
            escape(cast->getSubExpr(), "explicit conversion", location);
            if (const auto* source = resolve(cast->getSubExpr())) {
                declare(source);
                const auto point = source_point(context_.getSourceManager(), location);
                const auto from = node_key(source);
                const auto to = node_key(target);
                if (point && !from.empty() && !to.empty()) {
                    writer_.fact({"F", to, from, "explicit-conversion", point.file,
                                  std::to_string(point.line), std::to_string(point.column)});
                }
            } else {
                event("A", target, "unmodeled conversion producer", location);
            }
            return;
        }
        const auto point = source_point(context_.getSourceManager(), location);
        if (!point)
            return;
        if (const auto* source = resolve(value)) {
            declare(source);
            const auto from = node_key(source);
            const auto to = node_key(target);
            if (from.empty() || to.empty()) {
                event("A", target, "unmodeled value producer", location);
                return;
            }
            // Width/domain changes are edge semantics, classified per property
            // by the solver from both endpoint declarations.
            writer_.fact({"F", to, from, role, point.file, std::to_string(point.line),
                          std::to_string(point.column)});
            return;
        }
        if (value->getType()->isBooleanType()) {
            event("G", target, "bool", location);
            return;
        }
        if (value->isNullPointerConstant(context_, Expr::NPC_ValueDependentIsNotNull) &&
            node_type(target)->isPointerType()) {
            writer_.fact({"K", node_key(target), "0", point.file, std::to_string(point.line),
                          std::to_string(point.column)});
            return;
        }
        Expr::EvalResult result;
        if (value->EvaluateAsInt(result, context_) && !result.HasSideEffects) {
            const auto integer = result.Val.getInt();
            llvm::SmallString<32> text;
            integer.toString(text, 10);
            writer_.fact({"K", node_key(target), text.str().str(), point.file,
                          std::to_string(point.line), std::to_string(point.column)});
        } else {
            // Record the producer separately: its current source result type is
            // not independent evidence about retail signedness or storage width.
            event("A", target, "unmodeled expression producer", location);
        }
    }

    ASTContext& context_;
    ASTNameGenerator names_;
    FactWriter& writer_;
    FunctionDecl* function_ = nullptr;
    llvm::SmallPtrSet<const DeclRefExpr*, 32> direct_callees_;
    llvm::SmallPtrSet<const UnaryOperator*, 32> callback_addresses_;
    llvm::SmallPtrSet<const NamedDecl*, 32> span_declarations_;
    std::unordered_map<unsigned, std::string> source_hashes_;
    std::unordered_set<std::string> typed_nodes_;
    std::unordered_set<std::string> arrays_;
    std::unordered_set<std::string> records_;
    std::unordered_set<std::string> callback_slots_;
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
