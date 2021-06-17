#include "Type.h"

Type* Type::GetVoidType() 
{
    struct Void : public Type
    {    
        Void() : Type(100, "Void", 0) {}
    };
    static Void V;
    return &V;
}

Type* Type::GetBooleanType() 
{
    struct Boolean : public Type
    {    
        Boolean() : Type(101, "Boolean", 1) {}
    };
    static Boolean Bool;
    return &Bool;
}

Type* Type::GetInt8Type() 
{
    struct Int8 : public Type
    {    
        Int8() : Type(102, "Int8", 1) {}
    };
    static Int8 Int;
    return &Int;
}

Type* Type::GetUint8Type() 
{
    struct Uint8 : public Type
    {    
        Uint8() : Type(103, "Uint8", 1) {}
    };
    static Uint8 Int;
    return &Int;
}

Type* Type::GetInt16Type() 
{
    struct Int16 : public Type
    {    
        Int16() : Type(104, "Int16", 2) {}
    };
    static Int16 Int;
    return &Int;
}

Type* Type::GetUint16Type() 
{
    struct Uint16 : public Type
    {    
        Uint16() : Type(105, "Uint16", 2) {}
    };
    static Uint16 Int;
    return &Int;
}

Type* Type::GetInt32Type() 
{
    struct Int32 : public Type
    {    
        Int32() : Type(106, "Int32", 4) {}
    };
    static Int32 Int;
    return &Int;
}

Type* Type::GetUint32Type() 
{
    struct Uint32 : public Type
    {    
        Uint32() : Type(107, "Uint32", 4) {}
    };
    static Uint32 Int;
    return &Int;
}

Type* Type::GetInt64Type() 
{
    struct Int64 : public Type
    {    
        Int64() : Type(108, "Int64", 8) {}
    };
    static Int64 Int;
    return &Int;
}

Type* Type::GetUint64Type() 
{
    struct Uint64 : public Type
    {    
        Uint64() : Type(109, "Uint64", 8) {}
    };
    static Uint64 Int;
    return &Int;
}

Type* Type::GetFloatType() 
{
    struct Float : public Type
    {    
        Float() : Type(110, "Float", 4) {}
    };
    static Float F;
    return &F;
}

Type* Type::GetDoubleType() 
{
    struct Double : public Type
    {    
        Double() : Type(111, "Double", 8) {}
    };
    static Double D;
    return &D;
}

Type* GetStringType()
{
    struct String : public Type
    {    
        String() : Type(112, "String", sizeof(std::string)) {}
    };
    static String S;
    return &S;
}
