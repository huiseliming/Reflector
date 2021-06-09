#pragma once 
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

class FindTypeVisitor
  : public RecursiveASTVisitor<FindTypeVisitor> {
public:
  explicit FindTypeVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
    return true;
  }

private:
  ASTContext *Context;
};

class FindTypeConsumer : public clang::ASTConsumer {
public:
  explicit FindTypeConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  FindTypeVisitor Visitor;
};

class FindTypeAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::make_unique<FindTypeConsumer>(&Compiler.getASTContext());
  }
};


