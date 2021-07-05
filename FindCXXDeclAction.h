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
#include <unordered_map>

using namespace clang;

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
    : Context(Context) {}

  std::string ParseField(FieldDecl* Field)
  {
    QualType FieldType = Field->getType();
    // QualType RemovePointersReferencesType = RemovePointersAndReferences(FieldType);
    TypeInfo FieldTypeInfo = Context->getTypeInfo(Field->getType().getTypePtr());
    if (FieldType->isBuiltinType())
    {
      if(FieldType->isBooleanType()) //TODO: test
      {
        return "Type::GetBooleanType()";
      }
      if(FieldType->isIntegerType())//TODO: test
      {
        if(FieldType->isUnsignedIntegerType())
        {
          if (FieldTypeInfo.Width == 1) return "Type::GetUint8Type()";
          if (FieldTypeInfo.Width == 2) return "Type::GetUint16Type()";
          if (FieldTypeInfo.Width == 4) return "Type::GetUint32Type()";
          if (FieldTypeInfo.Width == 8) return "Type::GetUint64Type()";
          return "tan90";
        }
        if (FieldTypeInfo.Width == 1) return "Type::GetInt8Type()";
        if (FieldTypeInfo.Width == 2) return "Type::GetInt16Type()";
        if (FieldTypeInfo.Width == 4) return "Type::GetInt32Type()";
        if (FieldTypeInfo.Width == 8) return "Type::GetInt64Type()";
        return "tan90";
      }
      if(FieldType->isFloatingType()) //TODO: test
      {
        if (FieldTypeInfo.Width == 4) return "Type::GetFloatType()";
        if (FieldTypeInfo.Width == 8) return "Type::GetDoubleType()";
      }
    }
    return "Type::GetVoidType()";
  }
  
  void GenerateRegisterType(QualType )
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

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXDecl) {
    //llvm::outs() << "\ndump: \n" ;// << Declaration->getTypeForDecl()->getTypeClassName();
    PrintCXXRecordDecl(CXXDecl);
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

