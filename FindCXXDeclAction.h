#pragma once 
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

class FindCXXDeclVisitor
  : public RecursiveASTVisitor<FindCXXDeclVisitor> {
public:
  explicit FindCXXDeclVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXDecl) {
    //llvm::outs() << "\ndump: \n" ;// << Declaration->getTypeForDecl()->getTypeClassName();
    //CXXDecl->dump();
    if(CXXDecl->isClass()){
      // llvm::outs() << "QualifiedNameAsString : " << CXXDecl->getQualifiedNameAsString();
      // llvm::outs() << "QualifiedNameAsString : " << CXXDecl->printNestedNameSpecifier();
      llvm::outs() << "Fields Dump : \n"; 
      for(auto Field = CXXDecl->field_begin(); Field != CXXDecl->field_end(); Field++)
      {
        Field->dump();
        Field->printName(llvm::outs());
        llvm::outs() << "\n"
      }
      llvm::outs() << "\n"
      ;
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

