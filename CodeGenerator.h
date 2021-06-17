#pragma once
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "MetaRelect/MetaType.h"
#include "clang/AST/DeclCXX.h"

class CodeGenerator
{
public:
    static std::unordered_map<std::string, uint64_t> TypeNameMap;
    static std::unordered_map<uint64_t, std::unique_ptr<Type>> TypeTable;
    static uint64_t TypeIdCounter;

    static CodeGenerator& Get()
    {
        static CodeGenerator Generator;
        return Generator;
    }

    Type* FindType(clang::CXXRecordDecl* CXXDecl)
    {
        if(!CXXDecl) return nullptr;
        auto It = TypeNameMap.find(CXXDecl->getQualifiedNameAsString());
        if(It != TypeNameMap.end())
        {
            return TypeTable.find(It->second)->second.get();
        }
        return nullptr;
    }

    Type* AddTypeIfNotExist(clang::CXXRecordDecl* CXXDecl)
    {
        if(!CXXDecl) return nullptr;
        std::string TypeName = CXXDecl->getQualifiedNameAsString();
        auto It = TypeNameMap.find(TypeName);
        if(It != TypeNameMap.end())
        {
            return TypeTable.find(It->second)->second.get();
        }
        CXXDecl->getASTContext().
        CXXDecl->getASTContext().getTypeInfo(CXXDecl->getTypeForDecl());
        uint64_t TypeId = TypeIdCounter++;
        TypeNameMap.insert(std::make_pair(TypeName, TypeId));
        TypeTable.insert(std::make_pair(TypeId, std::make_unique<Type>(TypeIdCounter, CXXDecl->getQualifiedNameAsString(), CXXDecl->)));
        return TypeTable.find(TypeId)->second.get();
    }

};


