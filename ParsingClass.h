#pragma once
#include "clang/AST/Type.h"
#include "CodeGenerator.h"

FClass* ParseReflectCXXRecord(CCodeGenerator& CG, clang::ASTContext* const Context, const clang::CXXRecordDecl* ClassCXXRecordDecl);

FClass* ParseReflectEnum(CCodeGenerator& CG, clang::ASTContext* const Context, const clang::EnumDecl* ClassEnumDecl);

FClass* ParseReflectClass(CCodeGenerator& CG, clang::ASTContext* const Context, const clang::TagDecl* ClassTagDecl);
