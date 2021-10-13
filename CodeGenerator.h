#pragma once
#include "Reflect.h"

class CCodeGenerator
{
public:
    bool Generate();
    std::string ToGeneratedHeaderCode(CMeta* Meta, std::vector<std::string>& DependHeaderFile);
    std::string ToGeneratedSourceCode(CMeta* Meta, std::vector<std::string>& DependSourceFile);
    std::vector<std::unique_ptr<CMeta>> GeneratedReflectMetas;
    std::vector<std::unique_ptr<CMeta>> OtherMetas;
    std::string BuildPath;
    CMetaTable MetaTable;
};