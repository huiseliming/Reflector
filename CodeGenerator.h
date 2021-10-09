#pragma once
#include "Reflect.h"

class CCodeGenerator
{
public:
    static CCodeGenerator& Get();
    bool Generate();
    std::string ToGeneratedHeaderCode(FClass* Descriptor, std::vector<std::string>& DependHeaderFile);
    std::string ToGeneratedSourceCode(FClass* Descriptor, std::vector<std::string>& DependHeaderFile);
    std::vector<std::unique_ptr<FClass>> GeneratedReflectClasses;
    std::vector<std::unique_ptr<FClass>> OtherClasses;
    std::string BuildPath;
};
