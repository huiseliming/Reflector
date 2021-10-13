#pragma once
#include <thread>
#include "Reflect.h"

class CObjectBase;

std::unordered_map<std::thread::id, CThreadContext> GThreadContexts;

enum EObjectFlag
{
    EOF_NoneFlag
};

struct CThreadContext
{
    std::vector<CObjectBase*> ObjectArray;
};


CThreadContext& GetCurrentThreadContext()
{
    return GThreadContexts[std::this_thread::get_id()];
}



struct CObjectArrayItem
{
    Uint32 ArrayIndex;
    CObjectBase* Object;
    Uint32 Flag;
};




//TObject<>::New

/**
 * 
 * 
**/
class CObjectBase
{
public:
    CObjectBase(CClass* InClass, EObjectFlag InFlag, std::string Name)
        : Class(InClass)
        , Flag(InFlag)
    {}

private:
    CClass* Class;
    Uint32 Flag;
    std::string Name;
};


















