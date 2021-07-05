#include "CodeGenerator.h"
#include "MetaRelect/MetaType.h"
#include "clang/AST/DeclCXX.h"
#include <functional>
#include <string>

std::unordered_map<std::string, uint64_t> CodeGenerator::TypeNameMap;
std::unordered_map<uint64_t, std::unique_ptr<Type>> CodeGenerator::TypeTable;
uint64_t CodeGenerator::TypeIdCounter;

class ClassGenerator
{
public:
    // static Class StaticClass
    // (
    //     1, 
    //     "ClassName", 
    //     14, 
    //     std::vector<Field>{ 
    //         Field(Type::GetInt8Type(), 0), 
    //         Field(Type::GetUint8Type(), 0, 4),
    //     }, 
    //     std::vector<Function>{ 
    //         Function(
    //             "FunctionName",
    //             std::vector<Field>{ 
    //                 Field(Type::GetInt8Type(), 0), 
    //                 Field(Type::GetUint8Type(), 0, 4),
    //             },
    //             Field(Type::GetVoidType(),0)
    //         ),
    //     }
    // );

    ClassGenerator(clang::CXXRecordDecl* CXXDecl)
    {
        if (CXXDecl) {
            Type* ThisType = CodeGenerator::Get().AddTypeIfNotExist(CXXDecl);
            ClassCode += "static Class ThisClass\n";
            ClassCode += "{\n";
            ClassCode += std::to_string(ThisType->TypeId) + ",\n";
            ClassCode +=  "\"" + ThisType->TypeName + "\",\n";
            ClassCode += std::to_string(ThisType->TypeSize) + ",\n";
            
            ClassCode += "std::vector<Field>{\n";
            for (auto Field = CXXDecl->field_begin(); Field != CXXDecl->field_end(); Field++) {
                ClassCode += "Field(" "),";
            }
            ClassCode += "},\n";
            
            ClassCode += "}\n";
            ClassCode += CXXDecl->getQualifiedNameAsString();
            
        }
    }

    std::string ClassCode;
};



