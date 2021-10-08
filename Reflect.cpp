#include "Reflect.h"


FClassTable& FClassTable::Get()
{
    static std::function<FClassTable* ()> ClassTableInitializer = []() -> FClassTable* {
    	static FClassTable ClassTable;
        const_cast<FClass*>(FVoid::GetClass())->Id = ClassTable.IdCounter++;
        ClassTable.Classes.push_back(const_cast<FClass*>(FVoid::GetClass()));
        ClassTable.NameToId.insert(std::make_pair("void", const_cast<FClass*>(FVoid::GetClass())->Id));
        //ClassTable.RegisterClass("void"              , const_cast<FClass*>(FVoid::GetClass()));
        ClassTable.RegisterClass("bool"              , const_cast<FClass*>(FBool::GetClass()));
        ClassTable.RegisterClass("char"              , const_cast<FClass*>(FInt8::GetClass()));
        ClassTable.RegisterClass("unsigned char"     , const_cast<FClass*>(FUint8::GetClass()));
        ClassTable.RegisterClass("short"             , const_cast<FClass*>(FInt16::GetClass()));
        ClassTable.RegisterClass("unsigned short"    , const_cast<FClass*>(FUint16::GetClass()));
        ClassTable.RegisterClass("int"               , const_cast<FClass*>(FInt32::GetClass()));
        ClassTable.RegisterClass("unsigned int"      , const_cast<FClass*>(FUint32::GetClass()));
        ClassTable.RegisterClass("long long"         , const_cast<FClass*>(FInt64::GetClass()));
        ClassTable.RegisterClass("unsigned long long", const_cast<FClass*>(FUint64::GetClass()));
        ClassTable.RegisterClass("float"             , const_cast<FClass*>(FFloat::GetClass()));
        ClassTable.RegisterClass("double"            , const_cast<FClass*>(FDouble::GetClass()));

        ClassTable.RegisterClass("Void"  , const_cast<FClass*>(FVoid::GetClass()));
        ClassTable.RegisterClass("Bool"  , const_cast<FClass*>(FBool::GetClass()));
        ClassTable.RegisterClass("Int8"  , const_cast<FClass*>(FInt8::GetClass()));
        ClassTable.RegisterClass("Uint8" , const_cast<FClass*>(FUint8::GetClass()));
        ClassTable.RegisterClass("Int16" , const_cast<FClass*>(FInt16::GetClass()));
        ClassTable.RegisterClass("Uint16", const_cast<FClass*>(FUint16::GetClass()));
        ClassTable.RegisterClass("Int32" , const_cast<FClass*>(FInt32::GetClass()));
        ClassTable.RegisterClass("Uint32", const_cast<FClass*>(FUint32::GetClass()));
        ClassTable.RegisterClass("Int64" , const_cast<FClass*>(FInt64::GetClass()));
        ClassTable.RegisterClass("Uint64", const_cast<FClass*>(FUint64::GetClass()));
        ClassTable.RegisterClass("Float" , const_cast<FClass*>(FFloat::GetClass()));
        ClassTable.RegisterClass("Double", const_cast<FClass*>(FDouble::GetClass()));
#ifdef REFLECT_CODE_GENERATOR
        ClassTable.RegisterClass("_Bool",const_cast<FClass*>(FVoid::GetClass()));
#endif
    	return &ClassTable;
    };
    static FClassTable* ClassTablePtr = ClassTableInitializer();
    return *ClassTablePtr;
}

FClass* FClassTable::GetClass(const char* ClassName)
{
    auto NameToIdIterator = NameToId.find(ClassName);
    if (NameToIdIterator != NameToId.end())
    	return Classes[NameToIdIterator->second];
    return nullptr;
}

FClass* FClassTable::GetClass(Uint32 ClassId)
{
    if (ClassId < Classes.size())
    	return Classes[ClassId];
    return nullptr;
}

bool FClassTable::RegisterClass(const char* ClassName, FClass* Class)
{
    assert(Class != nullptr);
    assert(std::end(NameToId) == NameToId.find(ClassName));
    if (Class->Id == 0) {
        Class->Id = IdCounter++;
        Classes.push_back(Class);
    	NameToId.insert(std::make_pair(ClassName, Class->Id));
    }
    else
    {
#ifdef REFLECT_CODE_GENERATOR
        Class->Alias.push_back(ClassName);
#endif
    	NameToId.insert(std::make_pair(ClassName, Class->Id));
    }
    return true;
}
