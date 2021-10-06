#pragma once 
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/RecordLayout.h"
#include <unordered_map>
#include "Descriptor.h"
#include "CodeGenerator.h"

using namespace clang;

void SplitAnnotation(StringRef AnnotationString, std::vector<std::string>& Annotations)
{
    Annotations.clear();
    size_t PreviousPos = 0;
    for (size_t i = 0; i < AnnotationString.size(); i++)
    {
        if(AnnotationString[i] == ',') {
            std::string Str(AnnotationString.data() + PreviousPos, i - PreviousPos);
            Str.erase(std::remove_if(Str.begin(), Str.end(), isspace), Str.end());
            Annotations.emplace_back(Str);
            PreviousPos = i + 1;
        }
    }
    std::string Str(AnnotationString.data() + PreviousPos, AnnotationString.size() - PreviousPos);
    Str.erase(std::remove_if(Str.begin(), Str.end(), isspace), Str.end());
    Annotations.emplace_back(Str);
    
}

bool FindReflectAnnotation(Decl* CheckedDecl, const char* FoundMarkStr, std::vector<std::string>& ReflectAnnotation) {
    for (auto AttrIterator = CheckedDecl->attr_begin(); AttrIterator < CheckedDecl->attr_end(); AttrIterator++) {
        if ((*AttrIterator)->getKind() == attr::Annotate)
        {
            AnnotateAttr* AnnotateAttrPtr = dyn_cast<AnnotateAttr>(*AttrIterator);
            SplitAnnotation(AnnotateAttrPtr->getAnnotation(), ReflectAnnotation);
            if(ReflectAnnotation.size() > 0 && ReflectAnnotation[0] == FoundMarkStr){
                return true;
            }
        }
    }
    return false;
}


class FindCXXDeclVisitor
  : public RecursiveASTVisitor<FindCXXDeclVisitor> {
public:
  explicit FindCXXDeclVisitor(ASTContext *Context)
    : Context(Context) 
    {
    }

    FTypeDescriptor* ParseToDescriptor(CXXRecordDecl* CXXDecl)
    {
        std::vector<std::string> ReflectAnnotation;
        FTypeDescriptor* TypeDescriptor = FTypeDescriptorTable::Get().GetDescriptor(CXXDecl->getQualifiedNameAsString().c_str());
        if (TypeDescriptor) return TypeDescriptor;
        if (!CXXDecl->hasAttrs()) return nullptr;
        if(!FindReflectAnnotation(CXXDecl, "Object", ReflectAnnotation)) return nullptr;
        //CXXDecl->dump();
        if(CXXDecl->isClass()){
            CCodeGenerator::Get().Descriptors.emplace_back(std::make_unique<FClassDescriptor>(CXXDecl->getQualifiedNameAsString().c_str()));
        } else if(CXXDecl->isStruct()){
            CCodeGenerator::Get().Descriptors.emplace_back(std::make_unique<FStructDescriptor>(CXXDecl->getQualifiedNameAsString().c_str()));
        }else if(CXXDecl->isEnum()){
            CCodeGenerator::Get().Descriptors.emplace_back(std::make_unique<FEnumDescriptor>(CXXDecl->getQualifiedNameAsString().c_str()));
        } else if(CXXDecl->isUnion()){
            SourceRange Loc = CXXDecl->getSourceRange();
            PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
            llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported this type <Union>", PLoc.getFilename(), PLoc.getLine(), CXXDecl->getQualifiedNameAsString());
            return nullptr;
        }else{
            SourceRange Loc = CXXDecl->getSourceRange();
            PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
            llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported this  unknown type<???>", PLoc.getFilename(), PLoc.getLine(), CXXDecl->getQualifiedNameAsString());
            return nullptr;
        }

        TypeDescriptor = CCodeGenerator::Get().Descriptors.back().get();

        // Function parse
        CXXConstructorDecl* UserDeclaredDefaultConstructor = nullptr;
        for (auto MethodIterator = CXXDecl->method_begin(); MethodIterator != CXXDecl->method_end(); MethodIterator++)
        {
            CXXMethodDecl* Method = *MethodIterator;
            CXXConstructorDecl* Constructor = dyn_cast<CXXConstructorDecl>(*MethodIterator);
            if (Constructor && Constructor->param_empty())
            {
                UserDeclaredDefaultConstructor = Constructor;
            }
            if (!FindReflectAnnotation(Method, "Function", ReflectAnnotation)) continue;
            TypeDescriptor->Functions.push_back(FFunction());
            FFunction& Function = TypeDescriptor->Functions.back();
            Function.FunctionName = Method->getNameAsString();
            (void)Function.Ptr;
            Function.OwnerDescriptor = TypeDescriptor;
            Function.FunctionFlag |= kMemberFlagBit;
            if(Method->isStatic())
            {
                Function.FunctionFlag |= kStaticFlagBit;
            }
            QualType ReturnType = Method->getReturnType();
            if(ReturnType->isVoidType()){
                Function.Ret.TypeDescriptor = FTypeDescriptorTable::Get().GetDescriptor(0);
                Function.Ret.QualifierFlag = kQualifierNoFlag;
            }else{
                Function.Ret.TypeDescriptor;
                Function.Ret.QualifierFlag;
            }
            for (auto ParamIterator = Method->param_begin(); ParamIterator != Method->param_end(); ParamIterator++)
            {
                Function.Args.push_back(FParameter());
                FParameter& Parameter = Function.Args.back();
                Parameter.ParameterName = (*ParamIterator)->getNameAsString();
                Parameter.TypeDescriptor;
                Parameter.QualifierFlag;
            }

        }

        // check constructor and destructor 
        if(UserDeclaredDefaultConstructor){
            if (!UserDeclaredDefaultConstructor->isDeleted())
                TypeDescriptor->TypeFlag |= kHasDefaultConstructorFlagBit;
        }else{
            if (CXXDecl->hasDefaultConstructor()) {
                TypeDescriptor->TypeFlag |= kHasDefaultConstructorFlagBit;
            }
        }
        auto Destructor = CXXDecl->getDestructor();
        if(Destructor){
            if(!Destructor->isDeleted()) 
                TypeDescriptor->TypeFlag |= kHasDestructorFlagBit;
        }else{
            if(CXXDecl->hasSimpleDestructor())
                TypeDescriptor->TypeFlag |= kHasDestructorFlagBit;
        }

        // Field parse
        SourceRange Loc = CXXDecl->getSourceRange();
        TypeDescriptor->DeclaredFile = Context->getSourceManager().getFilename(Loc.getBegin());
        FTypeDescriptorTable::Get().RegisterDescriptor(CXXDecl->getQualifiedNameAsString().c_str(), TypeDescriptor);
        const ASTRecordLayout& RecordLayout = Context->getASTRecordLayout(CXXDecl);
        for (auto Field = CXXDecl->field_begin(); Field != CXXDecl->field_end(); Field++)
        {
            if (!FindReflectAnnotation(*Field, "Property", ReflectAnnotation)) continue;
            QualType FieldType = Field->getType();
            QualType FieldUnqualifiedType = Field->getType();
            TypeDescriptor->Fields.push_back(FField());
            TypeDescriptor->Fields.back().FieldName = Field->getName().str();
            TypeDescriptor->Fields.back().FieldOffset = RecordLayout.getFieldOffset(Field->getFieldIndex());
            // if is array
            if(FieldType->isArrayType()){
                TypeInfo FieldTypeTypeInfo = Context->getTypeInfo(FieldType.getTypePtr());
                QualType CheckedType = FieldType;
                QualType ArrayElementType;
                while (CheckedType->isArrayType())
                {
                    const clang::ArrayType* ArrayFieldType = CheckedType->getAsArrayTypeUnsafe();
                    ArrayElementType = ArrayFieldType->getElementType();
                    CheckedType = ArrayElementType;
                }
                TypeInfo ArrayElementTypeInfo = Context->getTypeInfo(ArrayElementType);
                TypeDescriptor->Fields.back().Number = FieldTypeTypeInfo.Width / ArrayElementTypeInfo.Width;
                FieldUnqualifiedType = ArrayElementType;
            }
            else
            {
                FieldUnqualifiedType = FieldType;
            }
            // if is pointer type
            if (FieldUnqualifiedType->isPointerType())
            {
                TypeDescriptor->Fields.back().QualifierFlag |= kPointerFlagBit;
                if (FieldUnqualifiedType.isConstant(*Context))
                    TypeDescriptor->Fields.back().QualifierFlag |= kConstPointerFlagBit;
                QualType PointeeType = FieldUnqualifiedType->getPointeeType();
                FieldUnqualifiedType = PointeeType;
            }
            // if is reference type
            if(FieldUnqualifiedType->isReferenceType())
            {
                TypeDescriptor->Fields.back().QualifierFlag |= kReferenceFlagBit;
                QualType PointeeType = FieldUnqualifiedType->getPointeeType();
                FieldUnqualifiedType = PointeeType;
            }
            // unsupported Multi-level Pointer
            if(FieldUnqualifiedType->isPointerType() || FieldUnqualifiedType->isReferenceType()){
                SourceRange Loc = Field->getSourceRange();
                PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported Complex Type <Multi-level Pointer>\n", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
            }
            // unsupported The Pointer Pointer To Array
            if (FieldUnqualifiedType->isArrayType()) {
                SourceRange Loc = Field->getSourceRange();
                PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported Complex Type <The Pointer Pointer To Array>\n", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
            }
            if (FieldUnqualifiedType.isConstant(*Context)){
                TypeDescriptor->Fields.back().QualifierFlag |= kConstValueFlagBit;
                FieldUnqualifiedType = FieldUnqualifiedType.getUnqualifiedType();
            }
            // if is ReserveObject
            auto it = std::find_if(ReflectAnnotation.begin(), ReflectAnnotation.end(), [](std::string& str) { return 0 == strncmp(str.c_str(), "ReserveObjectId", 15); });
            if (std::end(ReflectAnnotation) != it) {
                size_t pos = it->find_last_of("=");
                int32_t ReserveObjectId = std::atoi(it->substr(16).c_str());
                TypeDescriptor->Fields.back().TypeDescriptor = FTypeDescriptorTable::Get().GetDescriptor(ReserveObjectId);
            }
            else if (FieldUnqualifiedType->isBuiltinType())
            {
                TypeDescriptor->Fields.back().TypeDescriptor = FTypeDescriptorTable::Get().GetDescriptor(FieldUnqualifiedType.getCanonicalType().getAsString().c_str());
            }
            else if(FieldUnqualifiedType->isStructureOrClassType()|| FieldUnqualifiedType->isEnumeralType())
            {
                FTypeDescriptor* ParsedDescriptor = ParseToDescriptor(FieldUnqualifiedType->getAsCXXRecordDecl());
                if(ParsedDescriptor){
                    TypeDescriptor->Fields.back().TypeDescriptor = ParsedDescriptor;
                }else{
                    SourceRange Loc = Field->getSourceRange();
                    PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                    llvm::errs() << std::format("<{:s}:{:d}> property<{:s}> not is object\n", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
                }
            }
            else
            {
                SourceRange Loc = Field->getSourceRange();
                PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                llvm::errs() << std::format("<{:s}:{:d}> <{:s}> unsupported type <Union>\n", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
            }
            assert(TypeDescriptor->Fields.back().TypeDescriptor != nullptr);
        }
        //llvm::outs() << TypeDescriptor->Dump();
        return TypeDescriptor;
    }
    bool VisitCXXRecordDecl(CXXRecordDecl *CXXDecl) {
        ParseToDescriptor(CXXDecl);
        return true;
    }

private:
    ASTContext *Context;
};

class FindCXXDeclConsumer : public clang::ASTConsumer {
public:
    explicit FindCXXDeclConsumer(ASTContext *Context)
        : Visitor(Context) 
    {}

    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }
private:
  FindCXXDeclVisitor Visitor;
};

class FindCXXDeclAction : public clang::ASTFrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
        return std::make_unique<FindCXXDeclConsumer>(&Compiler.getASTContext());
    }
};

