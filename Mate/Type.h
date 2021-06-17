#pragma once
#include <string>

struct Type
{
    Type() = delete;
    Type(uint64_t Id, std::string Name, uint16_t Size)
        : Id(Id)
        , Name(Name)
        , Size(Size)
    {}

    uint64_t Id;
    std::string Name;
    uint16_t Size;
    static Type* GetVoidType();
    static Type* GetBooleanType();
    static Type* GetInt8Type();
    static Type* GetUint8Type();
    static Type* GetInt16Type();
    static Type* GetUint16Type();
    static Type* GetInt32Type();
    static Type* GetUint32Type();
    static Type* GetInt64Type();
    static Type* GetUint64Type();
    static Type* GetFloatType();
    static Type* GetDoubleType();
    static Type* GetStringType();
};

struct Pointer : public Type
{
    Type* PointToType;
};

struct Reference : public Type
{
    Type* ReferenceType;
};


struct Struct : public Type
{

};

struct Enum : public Type
{
    
};



