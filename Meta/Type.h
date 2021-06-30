#pragma once
#include "llvm/CodeGen/Register.h"
#include <memory>
#include <string>
#include <unordered_map>

struct Type
{
    Type() = delete;
protected:
    Type(uint64_t Id, std::string Name, uint16_t Size)
        : Id(Id)
        , Name(Name)
        , Size(Size)
    {
    }
public:
    virtual ~Type(){}

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


    Type* SearchType(std::string TypeName);
    Type* SearchType(uint64_t TypeId);

    // Low 32 Bit As Class TypeIdCounter
    static uint32_t GetNewTypeIdLow32BitCounter();
    // Up 32 Bit As Module TypeIdCounter
    static uint32_t GetNewTypeIdUp32BitCounter();
    // 64 Bit Full TypeIdCounter
    static uint32_t GetNewTypeIdCounter();

    const Type* RegisterType(std::string TypeName, std::unique_ptr<Type>&& RegisteredType);
    const Type* RegisterPointerType(std::string PointToType);
    const Type* RegisterReferenceType(std::string ReferenceType);

    static std::unordered_map<std::string, std::unique_ptr<Type>> NameTable;
    static std::unordered_map<uint64_t, Type*> IdTable;

};

struct AddressType : public Type
{
protected:
    AddressType(uint64_t Id, std::string Name, uint16_t Size)
        : Type(Id, Name, Size)
    {
    }
public:
    Type* InnerType;
};


struct PointerType : public AddressType
{
    PointerType(Type* InType, uint64_t Id = 0)
        : AddressType(Id, InType->Name + "*", sizeof(void*))
    {
        assert(InType);
        InnerType = InType;
    }
};

struct ReferenceType : public AddressType
{
    ReferenceType(Type* InType, uint64_t Id = 0)
        : AddressType(Id, InType->Name + "&", sizeof(void*))
    {
        assert(InType);
        InnerType = InType;
    }
};


struct Struct : public Type
{

};

struct Enum : public Type
{
    
};



