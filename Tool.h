#pragma once 
#include "clang/AST/Type.h"
#include "clang/Tooling/Tooling.h"

void SplitAnnotation(llvm::StringRef AnnotationString, std::vector<std::string>& Annotations);

bool FindReflectAnnotation(const clang::Decl* CheckedDecl, const char* FoundMarkStr, std::vector<std::string>& ReflectAnnotation);

//std::string GetDeclFileAbsPath(clang::ASTContext* const Context, const clang::Decl* D);

bool IsMatchedCppHeaderAndSource(const char* HeaderFile, uint32_t HeaderFileLength, const char* SourceFile, uint32_t SourceFileLength);

