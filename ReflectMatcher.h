#pragma once
#include "clang/AST/Type.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "ParsingClass.h"

using namespace clang;
using namespace clang::ast_matchers;

class ReflectClassMatcher : public MatchFinder::MatchCallback
{
public:
    virtual void run(const MatchFinder::MatchResult& Result)
    {
        std::vector<std::string> ReflectAnnotation;
        if (const CXXRecordDecl* CXXRecord = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("Class"))
        {
            ParseReflectCXXRecord(*CodeGenerator, Result.Context, CXXRecord);
        }
    }
    CCodeGenerator* CodeGenerator;
};

class ReflectEnumMatcher : public MatchFinder::MatchCallback
{
public:
    virtual void run(const MatchFinder::MatchResult& Result)
    {
        if (const EnumDecl* EnumNode = Result.Nodes.getNodeAs<clang::EnumDecl>("Enum"))
        {
            ParseReflectEnum(*CodeGenerator, Result.Context, EnumNode);
        }
    }
    CCodeGenerator* CodeGenerator;
};