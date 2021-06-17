#pragma once 
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

uint64_t GetNextTypeId(){
  static uint64_t TypeIdCounter = 1024;
  return TypeIdCounter + 1;
}

class FindCXXDeclVisitor
  : public RecursiveASTVisitor<FindCXXDeclVisitor> {
public:
  explicit FindCXXDeclVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXDecl) {
    //llvm::outs() << "\ndump: \n" ;// << Declaration->getTypeForDecl()->getTypeClassName();
    CXXDecl->dump();
    llvm::outs() << CXXDecl << "\n";
    if(CXXDecl->isClass() || CXXDecl->isStruct()){
      
      // llvm::outs() << "QualifiedNameAsString : " << CXXDecl->getQualifiedNameAsString();
      // llvm::outs() << "QualifiedNameAsString : " << CXXDecl->printNestedNameSpecifier();
      llvm::outs() << CXXDecl->getQualifiedNameAsString() << "\n";
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
      for(auto Field = CXXDecl->field_begin(); Field != CXXDecl->field_end(); Field++)
      {
        llvm::outs() << Field->getType().getAsString() << "\n";
        const Type* TheType = Field->getType().getTypePtr();
        if(TheType->isPointerType())
        {
          // TheType->getPointeeType();
          llvm::outs() << "this ptr class type" << TheType->getPointeeCXXRecordDecl() << TheType->getPointeeCXXRecordDecl()->getNameAsString() << "\n";
        }
        if(TheType->isClassType())
        {
          llvm::outs() << "this class type " << TheType->getAsCXXRecordDecl()->getNameAsString() << "\n";
        }
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
      }
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
    }
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

