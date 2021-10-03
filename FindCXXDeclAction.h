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

std::string WarpperString(std::string Str)
{
  return "\"" + Str + "\"";
}

uint64_t GetNextTypeId(){
    static uint64_t TypeIdCounter = 1024;
    return TypeIdCounter + 1;
}

clang::Type* GetType(QualType QT)
{
    if (QT->isPointerType())
    {
        QualType PointeeType = QT->getPointeeType();
        const clang::Type *TypePtr = PointeeType.getTypePtr();
        if (TypePtr->isPointerType()) {
        }
    }
    return nullptr;
}

class FindCXXDeclVisitor
  : public RecursiveASTVisitor<FindCXXDeclVisitor> {
public:
  explicit FindCXXDeclVisitor(ASTContext *Context)
    : Context(Context) 
    {
    }


  bool IsAddressType(QualType InType)
  {
    return InType->isPointerType() || InType->isReferenceType();
  }
  
  void GenerateAndCheckedAddressType(QualType AddressType, std::string& GeneratorCode)
  {
    assert(AddressType->isPointerType() || AddressType->isReferenceType());
    GeneratorCode.clear();
    GeneratorCode += "std::funtion<void> fn = []{";
    LoopingAddressType(AddressType, GeneratorCode);
    GeneratorCode += "}";
  }
  
  QualType LoopingAddressType(QualType AddressType, std::string& GeneratorCode)
  {
    QualType ParsedType;
    std::string AddressTypeName = AddressType->getAsCXXRecordDecl()->getQualifiedNameAsString();
    if (AddressType->isPointerType()) 
    {
      QualType PointeeType = AddressType->getPointeeType();
      if (IsAddressType(PointeeType)) {
        ParsedType = LoopingAddressType(PointeeType, GeneratorCode);
      }else{
        ParsedType = PointeeType;
      }
      GeneratorCode += "if(Type::SearchType(" + WarpperString(AddressTypeName) + ") == nullptr){\n";
      GeneratorCode += "  Type::RegisterPointerType(" + WarpperString(AddressTypeName) + ");\n";
      GeneratorCode += "}\n";
    }
    if (AddressType->isReferenceType()) 
    {
      QualType NonReferenceType = AddressType.getNonReferenceType();
      if (IsAddressType(NonReferenceType)) {
        ParsedType = LoopingAddressType(NonReferenceType, GeneratorCode);
      }else{
        ParsedType = NonReferenceType;
      }
      GeneratorCode += "if(Type::SearchType(" + WarpperString(AddressTypeName) + ") == nullptr){\n";
      GeneratorCode += "  Type::RegisterReferenceType(" + WarpperString(AddressTypeName) + ");\n";
      GeneratorCode += "}\n";
    }
    return ParsedType;
  }

  void PrintCXXRecordDecl(CXXRecordDecl *CXXDecl)
  {
    if(CXXDecl->isClass() || CXXDecl->isStruct()){
      llvm::outs() << "{CXXName}" << CXXDecl->getQualifiedNameAsString() << "\n";
      for(auto Field = CXXDecl->field_begin(); Field != CXXDecl->field_end(); Field++)
      {
        llvm::outs() << "  [Filed] " << "\n";
        QualType FieldType = Field->getType();
        if(FieldType->isStructureOrClassType() || FieldType->isEnumeralType() || FieldType->isUnionType())
        {
          // 对于非内建等类型
          std::string TypeName = Field->getType()->getAsCXXRecordDecl()->getQualifiedNameAsString();
          llvm::outs() << "    (TypeName)      " << TypeName <<"\n";
        }
        else
        {
          // 对于内建类型
          std::string TypeName = Field->getType().getAsString();
          llvm::outs() << "    (TypeName)      " << TypeName << "\n";
        }
        std::string FieldName = Field->getName().str();
        std::string QualifiedName = Field->getQualifiedNameAsString();
        std::string NameAsString = Field->getNameAsString();
        std::string DeclKindName = Field->getDeclKindName();
        llvm::outs() << "    (FieldName)     " << FieldName << "\n";
        llvm::outs() << "    (QualifiedName) " << QualifiedName << "\n";
        llvm::outs() << "    (NameAsString)  " << NameAsString << "\n";
        //llvm::outs() << "    (DeclKindName)  " << DeclKindName << "\n";
      }
    }
  }
    FTypeDescriptor* ParseToDescriptor(CXXRecordDecl* CXXDecl)
    {
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
                    if(Annotations[i] == "Object") {
                        NeedParse = true;
                    }
                }
            }
        }
        if (!NeedParse) return nullptr;
        FTypeDescriptor* TypeDescriptor = FTypeDescriptorTable::Get().GetDescriptor(CXXDecl->getQualifiedNameAsString().c_str());
        if (TypeDescriptor) return TypeDescriptor;
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
            if(FieldUnqualifiedType->isPointerType() || FieldUnqualifiedType->isReferenceType())
            {
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
            if (FieldUnqualifiedType->isBuiltinType())
            {
                TypeDescriptor->Fields.back().TypeDescriptor = FTypeDescriptorTable::Get().GetDescriptor(FieldUnqualifiedType.getAsString().c_str());
            }
            else if(FieldUnqualifiedType->isStructureOrClassType()|| FieldUnqualifiedType->isEnumeralType())
            {
                FTypeDescriptor* ParsedDescriptor = ParseToDescriptor(FieldUnqualifiedType->getAsCXXRecordDecl());
                if(ParsedDescriptor){
                    TypeDescriptor->Fields.back().TypeDescriptor = ParseToDescriptor(FieldUnqualifiedType->getAsCXXRecordDecl());
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
        llvm::outs() << TypeDescriptor->Dump();
        return TypeDescriptor;
    }
  bool VisitCXXRecordDecl(CXXRecordDecl *CXXDecl) {
    //CXXDecl->dump();
    //llvm::outs() << "\ndump: \n" ;// << Declaration->getTypeForDecl()->getTypeClassName();
    //PrintCXXRecordDecl(CXXDecl);
    ParseToDescriptor(CXXDecl);
    // CXXDecl->dump();
    // llvm::outs() << CXXDecl << "\n";
    // if(CXXDecl->isClass() || CXXDecl->isStruct()){
      
      // llvm::outs() << "QualifiedNameAsString : " << CXXDecl->getQualifiedNameAsString();
      // llvm::outs() << "QualifiedNameAsString : " << CXXDecl->printNestedNameSpecifier();
      // llvm::outs() << CXXDecl->getQualifiedNameAsString() << "\n";
      // llvm::outs() << CXXDecl->getQualifier()->getAsNamespace()->getNameAsString() << "\n";
      // CXXDecl->printNestedNameSpecifier(llvm::outs());
      // TypeInfo ThisDeclTypeInfo = CXXDecl->getASTContext().getTypeInfo(CXXDecl->getTypeForDecl());
      // llvm::outs() << "class " << CXXDecl->getNameAsString() << "\n";
      // llvm::outs() << "class " << CXXDecl->getQualifiedNameAsString() << "\n";
      // llvm::outs() << "{\n";
      // llvm::outs() << "  static Class* GetStaticClass()";
      // llvm::outs() << "  {";
      // llvm::outs() << "    static Class StaticClass";
      // llvm::outs() << "    (";
      // llvm::outs() << "        " << GetNextTypeId() << ",";
      // llvm::outs() << "        \"" << CXXDecl->getQualifiedNameAsString() << "\", ";
      // llvm::outs() << "        " << ThisDeclTypeInfo.Width << ", ";
      // llvm::outs() << "        std::vector<Field>{ ";
      // for(auto Field = CXXDecl->field_begin(); Field != CXXDecl->field_end(); Field++)
      // {
        
      //   llvm::outs() << Field->getType().getAsString() << "\n";
      //   const Type* TheType = Field->getType().getTypePtr();
      //   if(TheType->isPointerType())
      //   {
      //     // TheType->getPointeeType();
      //     llvm::outs() << "this ptr class type" << TheType->getPointeeCXXRecordDecl() << TheType->getPointeeCXXRecordDecl()->getNameAsString() << "\n";
      //   }
      //   if(TheType->isClassType())
      //   {
      //     llvm::outs() << "this class type " << TheType->getAsCXXRecordDecl()->getNameAsString() << "\n";
      //   }
        // if (Field->getType().getBaseTypeIdentifier()) 
        // {
        //   llvm::outs() << Field->getType().getBaseTypeIdentifier() << "sdads\n";
        // }
        // llvm::outs() << Field->getType().getUnqualifiedType().getBaseTypeIdentifier()->getPPKeywordID() << "\n";
        // if(Field->getType().getAsString() == "__Bool"){
        //   llvm::outs() << "            Field(Type::GetFloatType()), ";
        // }else if(Field->getType().getAsString() == "float"){
        //   llvm::outs() << "            Field(Type::GetFloatType()), ";
        // }else if(Field->getType().getAsString() == "float"){
        //   llvm::outs() << "            Field(Type::GetFloatType()), ";
        // }else if(Field->getType().getAsString() == "float"){
        //   llvm::outs() << "            Field(Type::GetFloatType()), ";
        // }else if(Field->getType().getAsString() == "float"){
        //   llvm::outs() << "            Field(Type::GetFloatType()), ";
        // }else if(Field->getType().getAsString() == "float"){
        //   llvm::outs() << "            Field(Type::GetFloatType()), ";
        // }else if(Field->getType().getAsString() == "float"){
        //   llvm::outs() << "            Field(Type::GetFloatType()), ";
        // }
        // llvm::outs() << "            Field(" << "Type::GetInt8Type() << "), ";
        
        // llvm::outs() << "  Field " << Field->getType().getAsString() << " " << Field->getNameAsString() << "\n";
        // if(Field->getType()->isBuiltinType()){
          
        // }else{

        // }
      // }
      // llvm::outs() << "            Field(Type::GetInt8Type(), 0), ";
      // llvm::outs() << "            Field(Type::GetUint8Type(), 0, 4),";

      // llvm::outs() << "        }, ";
      // llvm::outs() << "        std::vector<Function>{ ";
      // llvm::outs() << "            Function(";
      // llvm::outs() << "                \"FunctionName\",";
      // llvm::outs() << "                std::vector<Field>{ ";
      // llvm::outs() << "                    Field(Type::GetInt8Type(), 0), ";
      // llvm::outs() << "                    Field(Type::GetUint8Type(), 0, 4),";
      // llvm::outs() << "                },";
      // llvm::outs() << "                Field(Type::GetVoidType(),0)";
      // llvm::outs() << "            ),";
      // llvm::outs() << "        }";
      // llvm::outs() << "    );";
      // llvm::outs() << "    return &StaticClass;";
      // llvm::outs() << "  }";
      

      // for(auto Field = CXXDecl->field_begin(); Field != CXXDecl->field_end(); Field++)
      // {
      //   // Field->dump();
      //   llvm::outs() << "  Field " << Field->getType().getAsString() << " " << Field->getNameAsString() << "\n";
      //   if(Field->getType()->isBuiltinType()){
      //   }else{
      //   }
      // }
      // llvm::outs() << "} " << "\n";
    // }
    return true;

  }

private:
  ASTContext *Context;
};

class FindCXXDeclConsumer : public clang::ASTConsumer {
public:
  explicit FindCXXDeclConsumer(ASTContext *Context)
    : Visitor(Context) {}

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

