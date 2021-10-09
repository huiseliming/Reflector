#include "Reflect.h"


FClassTable& FClassTable::Get()
{
    static std::function<FClassTable* ()> ClassTableInitializer = []() -> FClassTable* {
    	static FClassTable ClassTable;
        ClassTable.Classes.push_back(nullptr);
#ifdef REFLECT_CODE_GENERATOR
        const_cast<FClass*>(FVoid::GetClass())->Id = ClassTable.IdCounter++;
        ClassTable.Classes.push_back(const_cast<FClass*>(FVoid::GetClass()));
        ClassTable.NameToId.insert(std::make_pair("FVoid", const_cast<FClass*>(FVoid::GetClass())->Id));
        ClassTable.RegisterClassToTable("FBool", const_cast<FClass*>(FBool::GetClass()));
        ClassTable.RegisterClassToTable("FInt8", const_cast<FClass*>(FInt8::GetClass()));
        ClassTable.RegisterClassToTable("FUint8", const_cast<FClass*>(FUint8::GetClass()));
        ClassTable.RegisterClassToTable("FInt16", const_cast<FClass*>(FInt16::GetClass()));
        ClassTable.RegisterClassToTable("FUint16", const_cast<FClass*>(FUint16::GetClass()));
        ClassTable.RegisterClassToTable("FInt32", const_cast<FClass*>(FInt32::GetClass()));
        ClassTable.RegisterClassToTable("FUint32", const_cast<FClass*>(FUint32::GetClass()));
        ClassTable.RegisterClassToTable("FInt64", const_cast<FClass*>(FInt64::GetClass()));
        ClassTable.RegisterClassToTable("FUint64", const_cast<FClass*>(FUint64::GetClass()));
        ClassTable.RegisterClassToTable("FFloat", const_cast<FClass*>(FFloat::GetClass()));
        ClassTable.RegisterClassToTable("FDouble", const_cast<FClass*>(FDouble::GetClass()));

        ClassTable.RegisterClassToTable("Void"  , const_cast<FClass*>(FVoid::GetClass()));
        ClassTable.RegisterClassToTable("Bool"  , const_cast<FClass*>(FBool::GetClass()));
        ClassTable.RegisterClassToTable("Int8"  , const_cast<FClass*>(FInt8::GetClass()));
        ClassTable.RegisterClassToTable("Uint8" , const_cast<FClass*>(FUint8::GetClass()));
        ClassTable.RegisterClassToTable("Int16" , const_cast<FClass*>(FInt16::GetClass()));
        ClassTable.RegisterClassToTable("Uint16", const_cast<FClass*>(FUint16::GetClass()));
        ClassTable.RegisterClassToTable("Int32" , const_cast<FClass*>(FInt32::GetClass()));
        ClassTable.RegisterClassToTable("Uint32", const_cast<FClass*>(FUint32::GetClass()));
        ClassTable.RegisterClassToTable("Int64" , const_cast<FClass*>(FInt64::GetClass()));
        ClassTable.RegisterClassToTable("Uint64", const_cast<FClass*>(FUint64::GetClass()));
        ClassTable.RegisterClassToTable("Float" , const_cast<FClass*>(FFloat::GetClass()));
        ClassTable.RegisterClassToTable("Double", const_cast<FClass*>(FDouble::GetClass()));

        ClassTable.RegisterClassToTable("void"              , const_cast<FClass*>(FVoid::GetClass()));
        ClassTable.RegisterClassToTable("bool"              , const_cast<FClass*>(FBool::GetClass()));
        ClassTable.RegisterClassToTable("char"              , const_cast<FClass*>(FInt8::GetClass()));
        ClassTable.RegisterClassToTable("unsigned char"     , const_cast<FClass*>(FUint8::GetClass()));
        ClassTable.RegisterClassToTable("short"             , const_cast<FClass*>(FInt16::GetClass()));
        ClassTable.RegisterClassToTable("unsigned short"    , const_cast<FClass*>(FUint16::GetClass()));
        ClassTable.RegisterClassToTable("int"               , const_cast<FClass*>(FInt32::GetClass()));
        ClassTable.RegisterClassToTable("unsigned int"      , const_cast<FClass*>(FUint32::GetClass()));
        ClassTable.RegisterClassToTable("long long"         , const_cast<FClass*>(FInt64::GetClass()));
        ClassTable.RegisterClassToTable("unsigned long long", const_cast<FClass*>(FUint64::GetClass()));
        ClassTable.RegisterClassToTable("float"             , const_cast<FClass*>(FFloat::GetClass()));
        ClassTable.RegisterClassToTable("double"            , const_cast<FClass*>(FDouble::GetClass()));

        ClassTable.RegisterClassToTable("_Bool",const_cast<FClass*>(FVoid::GetClass()));
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

uint32_t FClassTable::RegisterClassToTable(const char* ClassName, FClass* Class)
{
    assert(Class != nullptr);
    assert(std::end(NameToId) == NameToId.find(ClassName));
    if (Class->Id == 0) {
#ifdef REFLECT_CODE_GENERATOR
        assert(strcmp(Class->Name.c_str(), ClassName) == 0);
#else
        assert(strcmp(Class->Name, ClassName) == 0);
#endif
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
    return Class->Id;
}

const Uint32 FVoid::ClassId = 0;
#ifdef REFLECT_CODE_GENERATOR
#define DEFINE_BUILT_IN_CLASS_MEMBER(VarName) const Uint32 F##VarName##::ClassId = 0;
#else
#define DEFINE_BUILT_IN_CLASS_MEMBER(VarName) \
const Uint32 F##VarName##::ClassId = 0;\
static FClassAutoRegister<F##VarName> F##VarName##ClassAutoRegister;
#endif

DEFINE_BUILT_IN_CLASS_MEMBER(Bool);
DEFINE_BUILT_IN_CLASS_MEMBER(Int8);
DEFINE_BUILT_IN_CLASS_MEMBER(Uint8);
DEFINE_BUILT_IN_CLASS_MEMBER(Int16);
DEFINE_BUILT_IN_CLASS_MEMBER(Uint16);
DEFINE_BUILT_IN_CLASS_MEMBER(Int32);
DEFINE_BUILT_IN_CLASS_MEMBER(Uint32);
DEFINE_BUILT_IN_CLASS_MEMBER(Int64);
DEFINE_BUILT_IN_CLASS_MEMBER(Uint64);
DEFINE_BUILT_IN_CLASS_MEMBER(Float);
DEFINE_BUILT_IN_CLASS_MEMBER(Double);

#undef DEFINE_BUILT_IN_CLASS_MEMBER
