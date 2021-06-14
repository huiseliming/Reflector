#pragma once
#include <cstdint>
#include <functional>
#include <type_traits>
#include <vector>
#include <unordered_map>

struct Type
{
    Type() = delete;
    Type(uint64_t Id, const char* Name, uint16_t Size)
        : TypeId(Id)
        , TypeName(Name)
        , TypeSize(Size)
    {
    }
    uint64_t    TypeId;
    const char* TypeName;
    uint16_t    TypeSize;

    static Type* GetVoidType(){
        static Type TheType(0, "Void", 0);
        return &TheType;
    }
    static Type* GetInt8Type(){
        static Type TheType(1, "Int8", 1);
        return &TheType;
    }
    static Type* GetUint8Type(){
        static Type TheType(2, "Uint8", 1);
        return &TheType;
    }
    static Type* GetInt16Type(){
        static Type TheType(3, "Int16", 2);
        return &TheType;
    }
    static Type* GetUint16Type(){
        static Type TheType(4, "Uint16", 2);
        return &TheType;
    }
    static Type* GetInt32Type(){
        static Type TheType(5, "Int32", 4);
        return &TheType;
    }
    static Type* GetUint32Type(){
        static Type TheType(6, "Uint32", 4);
        return &TheType;
    }
    static Type* GetInt64Type(){
        static Type TheType(7, "Int64", 8);
        return &TheType;
    }
    static Type* GetUint64Type(){
        static Type TheType(8, "Uint64", 8);
        return &TheType;
    }
    static Type* GetFloatType(){
        static Type TheType(9, "Float", 4);
        return &TheType;
    }
    static Type* GetDoubleType(){
        static Type TheType(10, "Double", 8);
        return &TheType;
    }
    static Type* GetBooleanType(){
        static Type TheType(11, "Bool", 1);
        return &TheType;
    }
    
};


struct Field
{
    Field() = delete;
    Field(Type* FieldType, uint16_t FieldOffset, uint32_t FieldSize = 0)
        : FieldType(FieldType)
        , FieldOffset(FieldOffset)
        , FieldSize(FieldSize)
    {
        if(FieldSize == 0)
        {
            FieldSize = FieldType->TypeSize;
        }
    }
    Type*    FieldType;
    uint16_t FieldOffset;
    uint32_t FieldSize;
};

struct Function
{
    Function() = delete;
    Function(const char* FuncName, std::vector<Field>&& FuncArgs, Field&& FuncRet)
        : FuncName(FuncName)
        , FuncArgs(std::move(FuncArgs))
        , FuncRet(std::move(FuncRet))
    {

    }
    const char* FuncName;
    std::vector<Field> FuncArgs;
    Field FuncRet;
};

struct Class : public Type 
{
    Class(uint64_t Id, const char* Name, uint16_t Size, std::vector<Field>&& Fields, std::vector<Function>&& Functions)
        : Type(Id, Name, Size)
        , Fields(std::move(Fields))
        , Functions(std::move(Functions))
    {
        ClassesMap.insert(std::make_pair<>(Id, this));
    }
    std::vector<Field> Fields;
    std::vector<Function> Functions;

    static std::unordered_map<uint64_t, Class*> ClassesMap;
};



Class* ClassGenerator()
{
    static Class StaticClass
    (
        1, 
        "ClassName", 
        14, 
        std::vector<Field>{ 
            Field(Type::GetInt8Type(), 0), 
            Field(Type::GetUint8Type(), 0, 4),
        }, 
        std::vector<Function>{ 
            Function(
                "FunctionName",
                std::vector<Field>{ 
                    Field(Type::GetInt8Type(), 0), 
                    Field(Type::GetUint8Type(), 0, 4),
                },
                Field(Type::GetVoidType(),0)
            ),
        }
    );
    return &StaticClass;
}
