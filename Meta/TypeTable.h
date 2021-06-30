#pragma once
#include "Type.h"
#include "clang/AST/DeclCXX.h"
#include <memory>
#include <unordered_map>
#include <atomic>
#include <utility>
struct Type;


class TypeTable
{
    TypeTable()
    {
        TypeIdCounter = 1024;
    };
public:
    TypeTable& Get()
    {
        static TypeTable TT;
        return TT;
    }

    Type* AddTypeIfNotExist(clang::CXXRecordDecl* CXXDecl)
    {
        assert(CXXDecl != nullptr);
        auto Iterator = Table.find(CXXDecl->getQualifiedNameAsString());
        if (Iterator != Table.end()) {
            return Iterator->second.get();
        }
        uint32_t NewTypeId = TypeIdCounter++;
        Table.insert(std::make_pair(CXXDecl->getQualifiedNameAsString(), ));
        return ;
    }

    std::unique_ptr<Type> CreateNewType()
    {
        Type* NewType = new Type();
        return std::unique_ptr<Type>();
    }

    std::atomic<uint32_t> TypeIdCounter;
    std::unordered_map<std::string, std::unique_ptr<Type>> Table;
};



