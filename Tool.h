#pragma once 
#include "clang/AST/Type.h"
#include "clang/Tooling/Tooling.h"

struct CMeta;

void SplitAnnotation(llvm::StringRef AnnotationString, std::vector<std::string>& Annotations);

bool FindReflectAnnotation(const clang::Decl* CheckedDecl, const char* FoundMarkStr, std::vector<std::string>& ReflectAnnotation);

bool FindReflectAnnotation(const clang::Decl* CheckedDecl, std::vector<const char*> FoundMarkStrs, std::vector<std::string>& ReflectAnnotation);

void ParsingMetaString(CMeta* Meta, std::vector<std::string>& ReflectAnnotation);

//std::string GetDeclFileAbsPath(clang::ASTContext* const Context, const clang::Decl* D);

bool IsMatchedCppHeaderAndSource(const char* HeaderFile, uint32_t HeaderFileLength, const char* SourceFile, uint32_t SourceFileLength);

