#pragma once
#include "Reflect.h"

class CCodeGenerator
{
public:
    static CCodeGenerator& Get();
    bool Generate();
    std::string ToGeneratedHeaderCode(FClass* Descriptor);
    std::string ToGeneratedSourceCode(FClass* Descriptor);
    std::vector<std::unique_ptr<FClass>> Classes;
    std::string BuildPath;
};
