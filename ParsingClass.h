#pragma once
#include "clang/AST/Type.h"
#include "CodeGenerator.h"

FClass* ParseReflectCXXRecord(clang::ASTContext* const Context, const clang::CXXRecordDecl* ClassCXXRecordDecl);

FClass* ParseReflectEnum(clang::ASTContext* const Context, const clang::EnumDecl* ClassEnumDecl);

FClass* ParseReflectClass(clang::ASTContext* const Context, const clang::TagDecl* ClassTagDecl);
