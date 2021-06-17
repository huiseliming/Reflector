#pragma once
#include <string>
#include "Type.h"
struct Type;

struct Field
{
    Field(Type* FieldType = Type::GetVoidType(), uint32_t FieldSize = 0, uint16_t FieldOffset = 0)
        : FieldType(FieldType)
        , FieldSize(FieldSize)
        , FieldOffset(FieldOffset)
    {
    }
    Type*    FieldType;
    uint32_t FieldSize;
    uint16_t FieldOffset;
};
