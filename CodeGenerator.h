#pragma once
#include "Reflect.h"

class CCodeGenerator
{
public:
    bool Generate();
    std::string ToGeneratedHeaderCode(CMeta* Meta, std::vector<std::string>& DependHeaderFile);
    std::string ToGeneratedSourceCode(CMeta* Meta, std::vector<std::string>& DependSourceFile);
    std::vector<std::unique_ptr<CMeta>> GeneratedReflectClasses;
    std::vector<std::unique_ptr<CMeta>> OtherClasses;
    std::string BuildPath;
    CClassTable ClassTable;
};