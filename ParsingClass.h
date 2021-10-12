#pragma once
#include "clang/AST/Type.h"
#include "CodeGenerator.h"

CField* ParseReflectCXXRecord(CCodeGenerator& CG, clang::ASTContext* const Context, const clang::CXXRecordDecl* ClassCXXRecordDecl);

CField* ParseReflectEnum(CCodeGenerator& CG, clang::ASTContext* const Context, const clang::EnumDecl* ClassEnumDecl);

CField* ParseReflectClass(CCodeGenerator& CG, clang::ASTContext* const Context, const clang::TagDecl* ClassTagDecl);
