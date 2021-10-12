#include "Reflect.h"

CClassTable::CClassTable(){
    Classes.push_back(nullptr);
}

CClassTable* GClassTable = &CClassTable::Get();

CClassTable& CClassTable::Get()
{
    static std::function<CClassTable* ()> ClassTableInitializer = []() -> CClassTable* {
    	static CClassTable ClassTable;
    	return &ClassTable;
    };
    static CClassTable* ClassTablePtr = ClassTableInitializer();
    return *ClassTablePtr;
}

CField* CClassTable::GetClass(const char* ClassName)
{
    auto NameToIdIterator = NameToId.find(ClassName);
    if (NameToIdIterator != NameToId.end())
    	return Classes[NameToIdIterator->second];
    return nullptr;
}

CField* CClassTable::GetClass(Uint32 ClassId)
{
    if (ClassId < Classes.size())
    	return Classes[ClassId];
    return nullptr;
}

uint32_t CClassTable::RegisterClassToTable(const char* ClassName, CField* Class)
{
    assert(Class != nullptr);
    assert(std::end(NameToId) == NameToId.find(ClassName));

    if (Class->Id == 0) {
#ifdef COMPILE_REFLECTOR
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
#ifdef COMPILE_REFLECTOR
        Class->Alias.push_back(ClassName);
#endif
    	NameToId.insert(std::make_pair(ClassName, Class->Id));
    }
    return Class->Id;
}

void CClassTable::Initialize()
{
    while (!DeferredRegisterList.empty())
    {
        DeferredRegisterList.front()();
        DeferredRegisterList.pop_front();
    }
}
