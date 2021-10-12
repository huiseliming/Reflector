#pragma once
#include "Reflect.h"

class CCodeGenerator
{
public:
    bool Generate() { return false; }
    std::string ToGeneratedHeaderCode(CField* Descriptor, std::vector<std::string>& DependHeaderFile);
    std::string ToGeneratedSourceCode(CField* Descriptor, std::vector<std::string>& DependHeaderFile);
    std::vector<std::unique_ptr<CField>> GeneratedReflectClasses;
    std::vector<std::unique_ptr<CField>> OtherClasses;
    std::string BuildPath;
    CClassTable ClassTable;
};
