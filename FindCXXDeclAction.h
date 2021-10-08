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
#include "Reflect.h"
#include "CodeGenerator.h"
#include <filesystem>

using namespace clang;
extern std::string BuildPath;
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

    FClass* ParseToClass(CXXRecordDecl* CXXDecl)
    {
        std::vector<std::string> ReflectAnnotation;
        FClass* Class = FClassTable::Get().GetClass(CXXDecl->getQualifiedNameAsString().c_str());
        if (Class) return Class;
        if (!CXXDecl->hasAttrs()) return nullptr;
        if(!FindReflectAnnotation(CXXDecl, "Object", ReflectAnnotation)) return nullptr;
        //CXXDecl->dump();
        if(CXXDecl->isClass()){
        } else if(CXXDecl->isStruct()){
        }else if(CXXDecl->isEnum()){
            //CCodeGenerator::Get().Classes.emplace_back(std::make_unique<FClass>(CXXDecl->getQualifiedNameAsString().c_str()));
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
        CCodeGenerator::Get().Classes.emplace_back(std::make_unique<FClass>());
        CCodeGenerator::Get().Classes.back()->Name = CXXDecl->getQualifiedNameAsString().c_str();
        Class = CCodeGenerator::Get().Classes.back().get();

        for (auto BasesIterator = CXXDecl->bases_begin(); BasesIterator != CXXDecl->bases_end(); BasesIterator++)
        {
            Class->ParentClassName.push_back(BasesIterator->getType()->getAsCXXRecordDecl()->getQualifiedNameAsString());
        }

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
            Class->Functions.push_back(FFunction());
            FFunction& Function = Class->Functions.back();
            Function.Name = Method->getNameAsString();
            (void)Function.Ptr;
            Function.OwnerClass = Class;
            Function.Flag |= kMemberFlagBit;
            if(Method->isStatic())
            {
                Function.Flag |= kStaticFlagBit;
            }
            QualType ReturnType = Method->getReturnType();
            if(ReturnType->isVoidType()){
                Function.Ret.Class = FClassTable::Get().GetClass("void");
                Function.Ret.Flag = kQualifierNoFlag;
            }else{
                Function.Ret.Class;
                Function.Ret.Flag;
            }
            for (auto ParamIterator = Method->param_begin(); ParamIterator != Method->param_end(); ParamIterator++)
            {
                Function.Args.push_back(FParameter());
                FParameter& Parameter = Function.Args.back();
                Parameter.Name = (*ParamIterator)->getNameAsString();
                Parameter.Class;
                Parameter.Flag;
            }

        }

        // check constructor and destructor 
        if(UserDeclaredDefaultConstructor){
            if (!UserDeclaredDefaultConstructor->isDeleted())
                Class->Flag |= kHasDefaultConstructorFlagBit;
        }else{
            if (CXXDecl->hasDefaultConstructor()) {
                Class->Flag |= kHasDefaultConstructorFlagBit;
            }
        }
        auto Destructor = CXXDecl->getDestructor();
        if(Destructor){
            if(!Destructor->isDeleted()) 
                Class->Flag |= kHasDestructorFlagBit;
        }else{
            if(CXXDecl->hasSimpleDestructor())
                Class->Flag |= kHasDestructorFlagBit;
        }
        // Field parse
        SourceRange Loc = CXXDecl->getSourceRange();
        Class->DeclaredFile = std::filesystem::path(CCodeGenerator::Get().BuildPath + "/" + Context->getSourceManager().getFilename(Loc.getBegin()).str()).string();
        FClassTable::Get().RegisterClass(CXXDecl->getQualifiedNameAsString().c_str(), Class);
        const ASTRecordLayout& RecordLayout = Context->getASTRecordLayout(CXXDecl);
        for (auto Field = CXXDecl->field_begin(); Field != CXXDecl->field_end(); Field++)
        {
            if (!FindReflectAnnotation(*Field, "Property", ReflectAnnotation)) continue;
            QualType FieldType = Field->getType();
            QualType FieldUnqualifiedType = Field->getType();
            Class->Fields.push_back(FField());
            Class->Fields.back().Name = Field->getName().str();
            Class->Fields.back().Offset = RecordLayout.getFieldOffset(Field->getFieldIndex());
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
                Class->Fields.back().Number = FieldTypeTypeInfo.Width / ArrayElementTypeInfo.Width;
                FieldUnqualifiedType = ArrayElementType;
            }
            else
            {
                FieldUnqualifiedType = FieldType;
            }
            // if is pointer type
            if (FieldUnqualifiedType->isPointerType())
            {
                Class->Fields.back().Flag |= kPointerFlagBit;
                if (FieldUnqualifiedType.isConstant(*Context))
                    Class->Fields.back().Flag |= kConstPointerFlagBit;
                QualType PointeeType = FieldUnqualifiedType->getPointeeType();
                FieldUnqualifiedType = PointeeType;
            }
            // if is reference type
            if(FieldUnqualifiedType->isReferenceType())
            {
                Class->Fields.back().Flag |= kReferenceFlagBit;
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
                Class->Fields.back().Flag |= kConstValueFlagBit;
                FieldUnqualifiedType = FieldUnqualifiedType.getUnqualifiedType();
            }
            // if is ReserveObject
            auto it = std::find_if(ReflectAnnotation.begin(), ReflectAnnotation.end(), [](std::string& str) { return 0 == strncmp(str.c_str(), "ReserveObjectId", 15); });
            if (std::end(ReflectAnnotation) != it) {
                //size_t pos = it->find_last_of("=");
                int32_t ReserveObjectId = std::atoi(it->substr(16).c_str());
                Class->Fields.back().Class = FClassTable::Get().GetClass(ReserveObjectId);
            }
            else if (FieldUnqualifiedType->isBuiltinType())
            {
                Class->Fields.back().Class = FClassTable::Get().GetClass(FieldUnqualifiedType.getCanonicalType().getAsString().c_str());
            }
            else if(FieldUnqualifiedType->isStructureOrClassType()|| FieldUnqualifiedType->isEnumeralType())
            {
                FClass* ParsedClass = ParseToClass(FieldUnqualifiedType->getAsCXXRecordDecl());
                if(ParsedClass){
                    Class->Fields.back().Class = ParsedClass;
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
            assert(Class->Fields.back().Class != nullptr);
        }
        //llvm::outs() << Class->Dump();
        return Class;
    }
    bool VisitCXXRecordDecl(CXXRecordDecl *CXXDecl) {
        ParseToClass(CXXDecl);
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

