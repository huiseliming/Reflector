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

std::vector<std::string> SplitAnnotation(StringRef AnnotationString)
{
    std::vector<std::string> Annotations;
    size_t PreviousPos = 0;
    for (size_t i = 0; i < AnnotationString.size(); i++)
    {
        if(AnnotationString[i] == ',') {
            Annotations.emplace_back(std::string(AnnotationString.data() + PreviousPos, i - PreviousPos));
            PreviousPos = i + 1;
        }
    }
    Annotations.emplace_back(std::string(AnnotationString.data() + PreviousPos, AnnotationString.size() - PreviousPos));
    return std::move(Annotations);
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
        FTypeDescriptor* TypeDescriptor = FTypeDescriptorTable::Get().GetDescriptor(CXXDecl->getQualifiedNameAsString().c_str());
        if (TypeDescriptor) return TypeDescriptor;
        bool NeedParse = false;
        if (!CXXDecl->hasAttrs()) return nullptr;
        AttrVec Attrs = CXXDecl->getAttrs();
        for (size_t i = 0; i < Attrs.size(); i++)
        {
            if (Attrs[i]->getKind() == attr::Annotate)
            {
                AnnotateAttr* AnnotateAttrPtr = dyn_cast<AnnotateAttr>(Attrs[i]);
                std::vector<std::string> Annotations = SplitAnnotation(AnnotateAttrPtr->getAnnotation());
                for (size_t i = 0; i < Annotations.size(); i++)
                {
                    if (Annotations[i] == "Object") {
                        llvm::outs() << std::format("Found Object<{:s}>\n", CXXDecl->getQualifiedNameAsString());
                        NeedParse = true;
                    }
                }
            }
        }
        if (!NeedParse) return nullptr;
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
        CXXConstructorDecl* UserDeclaredDeDefaultConstructor = nullptr;
        for (auto Method = CXXDecl->method_begin(); Method != CXXDecl->method_end(); Method++)
        {
            CXXConstructorDecl* Constructor = dyn_cast<CXXConstructorDecl>(*Method);
            if (Constructor && Constructor->param_size() == 0)
            {
                UserDeclaredDeDefaultConstructor = Constructor;
            }
        }
        if(UserDeclaredDeDefaultConstructor){
            if (!UserDeclaredDeDefaultConstructor->isDeleted())
                TypeDescriptor->TypeFlag |= kHasDestructorFlagBit;
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

        SourceRange Loc = CXXDecl->getSourceRange();
        TypeDescriptor->DeclaredFile = Context->getSourceManager().getFilename(Loc.getBegin());
        FTypeDescriptorTable::Get().RegisterDescriptor(CXXDecl->getQualifiedNameAsString().c_str(), TypeDescriptor);
        const ASTRecordLayout& RecordLayout = Context->getASTRecordLayout(CXXDecl);
        for (auto Field = CXXDecl->field_begin(); Field != CXXDecl->field_end(); Field++)
        {
            NeedParse = false;
            if(!Field->hasAttrs()) continue;
            AttrVec Attrs = Field->getAttrs();
            for (size_t i = 0; i < Attrs.size(); i++)
            {
                if (Attrs[i]->getKind() == attr::Annotate)
                {
                    AnnotateAttr* AnnotateAttrPtr = dyn_cast<AnnotateAttr>(Attrs[i]);
                    std::vector<std::string> Annotations = SplitAnnotation(AnnotateAttrPtr->getAnnotation());
                    for (size_t i = 0; i < Annotations.size(); i++)
                    {
                        if (Annotations[i] == "Property") {
                            llvm::outs() << std::format("Found Object<{:s}> Property<{:s}>\n", CXXDecl->getQualifiedNameAsString(), Field->getNameAsString());
                            NeedParse = true;
                        }
                    }
                }
            }
            if (!NeedParse) continue;
            QualType FieldType = Field->getType();
            QualType FieldUnqualifiedType = Field->getType();
            TypeDescriptor->Fields.push_back(FField());
            TypeDescriptor->Fields.back().FieldName = Field->getName().str();
            TypeDescriptor->Fields.back().FieldOffset = RecordLayout.getFieldOffset(Field->getFieldIndex());
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
            if (FieldUnqualifiedType->isPointerType())
            {
                TypeDescriptor->Fields.back().QualifierFlag |= kPointerFlagBit;
                if (FieldUnqualifiedType.isConstant(*Context))
                    TypeDescriptor->Fields.back().QualifierFlag |= kConstPointerFlagBit;
                QualType PointeeType = FieldUnqualifiedType->getPointeeType();
                FieldUnqualifiedType = PointeeType;
            }
            if(FieldUnqualifiedType->isReferenceType())
            {
                TypeDescriptor->Fields.back().QualifierFlag |= kReferenceFlagBit;
                QualType PointeeType = FieldUnqualifiedType->getPointeeType();
                FieldUnqualifiedType = PointeeType;
            }
            if(FieldUnqualifiedType->isPointerType() || FieldUnqualifiedType->isReferenceType()){
                SourceRange Loc = Field->getSourceRange();
                PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported Complex Type <Multi-level Pointer>", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
            }
            if (FieldUnqualifiedType->isArrayType()) {
                SourceRange Loc = Field->getSourceRange();
                PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported Complex Type <The Pointer Pointer To Array>", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
            }
            if (FieldUnqualifiedType.isConstant(*Context)){
                TypeDescriptor->Fields.back().QualifierFlag |= kConstValueFlagBit;
                FieldUnqualifiedType = FieldUnqualifiedType.getUnqualifiedType();
            }
            if (FieldUnqualifiedType->isBuiltinType())
            {
                TypeDescriptor->Fields.back().TypeDescriptor = FTypeDescriptorTable::Get().GetDescriptor(FieldUnqualifiedType.getAsString().c_str());
            }
            else if(FieldUnqualifiedType->isStructureOrClassType()|| FieldUnqualifiedType->isEnumeralType())
            {
                FTypeDescriptor* ParsedDescriptor = ParseToDescriptor(FieldUnqualifiedType->getAsCXXRecordDecl());
                if(ParsedDescriptor){
                    TypeDescriptor->Fields.back().TypeDescriptor = ParsedDescriptor;
                }else{
                    SourceRange Loc = Field->getSourceRange();
                    PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                    llvm::errs() << std::format("<{:s}:{:d}> property<{:s}> not is object", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
                }
            }
            else
            {
                SourceRange Loc = Field->getSourceRange();
                PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                llvm::errs() << std::format("<{:s}:{:d}> <{:s}> unsupported type <Union>", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
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

