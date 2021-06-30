#pragma once
#include "Type.h"
#include "Function.h"
#include <string>

struct Class : public Type
{
    Class(uint64_t Id, std::string Name, uint16_t Size, std::vector<Field>&& Fields = {}, std::vector<Function>&& Functions = {})
        : Type(Id, Name, Size)
        , Fields(std::move(Fields))
        , Functions(std::move(Functions))
    {
    }

    void AddField(Field&& NewField)
    {
        Fields.emplace_back(NewField);
    }

    void AddFunction(Function&& NewFunction)
    {
        Functions.emplace_back(NewFunction);
    }

    std::vector<Field> Fields;
    std::vector<Function> Functions;
};

Class* GetStaticClass()
{
    static std::function<Class*()> GenerateClass = 
    []()
    {
        static Class MetaClass(1001, "Type", sizeof(Type));
        MetaClass.AddField(Field(Type::GetUint64Type(), Type::GetUint64Type()->Size));
        MetaClass.AddField(Field(Type::GetStringType(), Type::GetStringType()->Size));
        MetaClass.AddField(Field(Type::GetUint16Type(), Type::GetUint16Type()->Size));
        MetaClass.AddFunction(Function("XXX",{},{}));
        
        return &MetaClass;
    };
    static Class* MetaClass = GenerateClass();
    return MetaClass;
}

