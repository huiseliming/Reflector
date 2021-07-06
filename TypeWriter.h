#pragma once
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "clang/AST/DeclCXX.h"

using namespace clang;

struct TransitionInfo {
  uint64_t Id;
  const Type* type;
};

class TypeManager
{
public:
    static std::unordered_map<std::string, TransitionInfo> StaticType;

    static uint64_t TypeIdCounter;

    static TypeManager &Get()
    {
        static TypeManager Manager;
        return Manager;
    }

    const Type *FindType(clang::CXXRecordDecl *CXXDecl)
    {
        if(!CXXDecl) return nullptr;
        auto It = StaticType.find(CXXDecl->getQualifiedNameAsString());
        if (It != StaticType.end())
        {
          return It->second.type;
        }
        return nullptr;
    }

    const Type *AddTypeIfNotExist(clang::CXXRecordDecl *CXXDecl)
    {
        if(!CXXDecl) return nullptr;
        std::string TypeName = CXXDecl->getQualifiedNameAsString();
        auto It = StaticType.find(CXXDecl->getQualifiedNameAsString());
        if (It != StaticType.end()) {
          return It->second.type;
        }
        //CXXDecl->getASTContext().getTypeInfo(CXXDecl->getTypeForDecl());
        uint64_t TypeId = TypeIdCounter++;
        StaticType.insert(std::make_pair(TypeName, TransitionInfo{TypeId, CXXDecl->getTypeForDecl()}));
        return StaticType.find(TypeName)->second.type;
    }

};


