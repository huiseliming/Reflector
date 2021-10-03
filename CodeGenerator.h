#pragma once
#include "Descriptor.h"

class CCodeGenerator
{
public:
    static CCodeGenerator& Get();
    bool Generate();
    std::string ToGeneratedHeaderCode(FTypeDescriptor* Descriptor);
    std::string ToGeneratedSourceCode(FTypeDescriptor* Descriptor);
    std::vector<std::unique_ptr<FTypeDescriptor>> Descriptors;
};
