#pragma once
#include "clang/AST/Type.h"
#include "CodeGenerator.h"

CMeta* ParseReflectCXXRecord(CCodeGenerator& CG, clang::ASTContext* const Context, const clang::CXXRecordDecl* ClassCXXRecordDecl);

CMeta* ParseReflectEnum(CCodeGenerator& CG, clang::ASTContext* const Context, const clang::EnumDecl* ClassEnumDecl);

CMeta* ParseReflectClass(CCodeGenerator& CG, clang::ASTContext* const Context, const clang::TagDecl* ClassTagDecl);
