#pragma once
#include "Field.h"
#include <vector>

struct Function
{
    Function() = delete;
    Function(std::string FuncName, std::vector<Field>&& FuncArgs = {}, Field&& FuncRet = {})
        : FuncName(FuncName)
        , FuncArgs(std::move(FuncArgs))
        , FuncRet(std::move(FuncRet))
    {

    }
    std::string FuncName;
    std::vector<Field> FuncArgs;
    Field FuncRet;
};