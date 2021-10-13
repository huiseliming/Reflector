#include "CodeGenerator.h"
#include <fstream>
#include <filesystem>
#include "clang/Tooling/Tooling.h"

using namespace llvm;
using namespace clang;

bool CCodeGenerator::Generate()
{
    struct FGeneratedFileContext {
        std::list<CMeta*> FieldClassList;
        std::fstream GeneratedSourceFile;
        std::fstream GeneratedHeaderFile;
    };
    std::vector<std::unique_ptr<CMeta>>& FieldClasses = GeneratedReflectMetas;
    std::unordered_map<std::string, FGeneratedFileContext> GeneratedFileContextMap;
    for (size_t i = 0; i < FieldClasses.size(); i++)
    {
        if (FieldClasses[i]->IsReflectionDataCollectionCompleted && !FieldClasses[i]->DeclaredFile.empty())
        {
            if (std::end(GeneratedFileContextMap) == GeneratedFileContextMap.find(FieldClasses[i]->DeclaredFile)) {
                GeneratedFileContextMap.insert(std::make_pair<>(FieldClasses[i]->DeclaredFile, FGeneratedFileContext()));
                std::string DeclaredFile = FieldClasses[i]->DeclaredFile;
                std::size_t Pos = DeclaredFile.find_last_of("/\\");
                std::string DeclaredPath = DeclaredFile.substr(0, Pos);
                std::string Filename = DeclaredFile.substr(Pos + 1);
                std::size_t DotPos = Filename.rfind(".");
                std::string FilenameNotDotH = Filename.substr(0, DotPos);
                std::string GeneratedHeaderFile = std::filesystem::current_path().string() + "/" + FilenameNotDotH + ".reflect.h";
                std::string GeneratedSourceFile = std::filesystem::current_path().string() + "/" + FilenameNotDotH + ".reflect.cpp";
                GeneratedFileContextMap[FieldClasses[i]->DeclaredFile].GeneratedHeaderFile.open(GeneratedHeaderFile, std::ios::out | std::ios::trunc);
                GeneratedFileContextMap[FieldClasses[i]->DeclaredFile].GeneratedSourceFile.open(GeneratedSourceFile, std::ios::out | std::ios::trunc);
                if (!GeneratedFileContextMap[FieldClasses[i]->DeclaredFile].GeneratedHeaderFile.is_open()) {
                    llvm::errs() << std::format("File<{:s}> open failed\n", GeneratedHeaderFile);
                    return false;
                }
                if (!GeneratedFileContextMap[FieldClasses[i]->DeclaredFile].GeneratedSourceFile.is_open()) {
                    llvm::errs() << std::format("File<{:s}> open failed\n", GeneratedSourceFile);
                    return false;
                }
                llvm::outs() << std::format("File<{:s}> generated\n", GeneratedHeaderFile);
                llvm::outs() << std::format("File<{:s}> generated\n", GeneratedSourceFile);
                std::string HeaderFileBegin = "#pragma once\n";
                std::string SourceFileBegin = "#include \"" + std::filesystem::path(DeclaredFile).lexically_proximate(std::filesystem::current_path()).string() + "\"\n";
                SourceFileBegin += "#include \"" + std::filesystem::path(GeneratedHeaderFile).lexically_proximate(std::filesystem::current_path()).string() + "\"\n\n";
                HeaderFileBegin += SourceFileBegin;
                GeneratedFileContextMap[FieldClasses[i]->DeclaredFile].GeneratedHeaderFile.write(HeaderFileBegin.data(), HeaderFileBegin.size());
                GeneratedFileContextMap[FieldClasses[i]->DeclaredFile].GeneratedSourceFile.write(SourceFileBegin.data(), SourceFileBegin.size());
            }
            GeneratedFileContextMap[FieldClasses[i]->DeclaredFile].FieldClassList.push_back(FieldClasses[i].get());
        }
    }
    for (auto GeneratedFileContextIterator = GeneratedFileContextMap.begin(); GeneratedFileContextIterator != GeneratedFileContextMap.end(); GeneratedFileContextIterator++)
    {
        std::vector<std::string> HeaderDependHeaderFile;
        std::vector<std::string> SourceDependHeaderFile;
        for (auto Iterator = GeneratedFileContextIterator->second.FieldClassList.begin(); Iterator != GeneratedFileContextIterator->second.FieldClassList.end(); Iterator++)
        {
            std::string GeneratedHeaderCode = ToGeneratedHeaderCode(*Iterator, HeaderDependHeaderFile);
            std::string GeneratedSourceCode = ToGeneratedSourceCode(*Iterator, SourceDependHeaderFile);
            GeneratedFileContextIterator->second.GeneratedHeaderFile.write(GeneratedHeaderCode.data(), GeneratedHeaderCode.size());
            GeneratedFileContextIterator->second.GeneratedSourceFile.write(GeneratedSourceCode.data(), GeneratedSourceCode.size());
        }
        
        GeneratedFileContextIterator->second.GeneratedHeaderFile.close();
        GeneratedFileContextIterator->second.GeneratedSourceFile.close();
    }
    return true;
}

const char* S0Str  = "";
const char* S4Str  = "    ";
const char* S8Str  = "        ";
const char* S12Str = "            ";
const char* S16Str = "                ";

std::string CCodeGenerator::ToGeneratedHeaderCode(CMeta* Meta, std::vector<std::string>& DependHeaderFile)
{
    std::string HeaderCode;
    HeaderCode.reserve(4 * 1024);
    CEnumClass* EnumClass = dyn_cast<CEnumClass>(Meta);
    if(EnumClass){
        const char* EnumTemplateDecl =
            "{0:s}template<>\n"
            "{0:s}struct TEnumClass<{1:s}>{{\n"
            "{0:s}    static const CMeta * StaticMeta();\n"
            "{0:s}    static Uint32 MetaId;\n"
            "{0:s}}}; \n\n";
        HeaderCode += std::format(EnumTemplateDecl, S0Str, EnumClass->Name);
        return HeaderCode;
    }
    return HeaderCode;
}

const char* DeferredRegisterCode =
"{0:s}CMetaTable::Get().DeferredRegisterList.push_back([&] {{\n"
"{0:s}    CMeta* Meta = CMetaTable::Get().GetMeta(\"{1:s}\");\n"
"{0:s}    if(Meta != nullptr) {{\n"
"{0:s}        CMetaProperty* MetaProperty = (CMetaProperty*)Struct.Properties[1].get();\n"
"{0:s}        MetaProperty->Meta = Meta;\n"
"{0:s}        return true;\n"
"{0:s}    }}\n"
"{0:s}    assert(false && \"CLASS {1:s} NO EXIST\");\n"
"{0:s}    return false;\n"
"{0:s}}});\n";

std::string CCodeGenerator::ToGeneratedSourceCode(CMeta* Meta, std::vector<std::string>& DependSourceFile)
{
    std::string SourceCode;
    SourceCode.reserve(4 * 1024);
    CEnumClass* EnumClass = dyn_cast<CEnumClass>(Meta);
    if (EnumClass) {
        const char* EnumTemplateDef[] = {
        "{0:s}const CMeta* TEnumClass<{1:s}>::StaticMeta()\n"
        "{0:s}{{\n"
        "{0:s}    static std::function<CEnumClass* ()> ClassInitializer = []() -> CEnumClass* {{\n"
        "{0:s}        static CEnumClass EnumClass(\"{1:s}\");\n"
        "{0:s}        EnumClass.Size = sizeof({1:s});\n"
        "{0:s}        EnumClass.Options = {{\n",
        "{0:s}            {{\"{1:s}\", {2:d}}},\n",
        "{0:s}        }};\n"
        "{0:s}        return &EnumClass;\n"
        "{0:s}    }};\n"
        "{0:s}    static CEnumClass* EnumClassPtr = ClassInitializer();\n"
        "{0:s}    return EnumClassPtr;\n"
        "{0:s}}}\n\n"
        "Uint32 TEnumClass<{1:s}>::MetaId = 0;\n\n"
        "static TMetaAutoRegister<TEnumClass<{1:s}>> {1:s}MetaAutoRegister;\n\n"
        };
        SourceCode += std::format(EnumTemplateDef[0], S0Str, EnumClass->Name);
        for (size_t i = 0; i < EnumClass->Options.size(); i++)
        {
            SourceCode += std::format(EnumTemplateDef[1], S0Str, EnumClass->Options[i].first, EnumClass->Options[i].second);
        }
        SourceCode += std::format(EnumTemplateDef[2], S0Str, EnumClass->Name);
        return SourceCode;
    }

    CStruct* Struct = dyn_cast<CStruct>(Meta);
    if(Struct)
    {
        const char* StructDef[] = {
            "{0:s}const CMeta* {1:s}::StaticMeta()\n"
            "{0:s}{{\n"
            "{0:s}    static std::function<CStruct* ()> ClassInitializer = []() -> CStruct* {{\n"
            "{0:s}        static CStruct Struct(\"{1:s}\");\n"
            "{0:s}        Struct.New         = TLifeCycle<{1:s}>::New;\n"
            "{0:s}        Struct.Delete      = TLifeCycle<{1:s}>::Delete;\n"
            "{0:s}        Struct.Constructor = TLifeCycle<{1:s}>::Constructor;\n"
            "{0:s}        Struct.Destructor  = TLifeCycle<{1:s}>::Destructor;\n"
            "{0:s}        Struct.Size = sizeof({1:s});\n",
            "{0:s}        Struct.Properties.push_back(std::make_unique<C{1:s}Property>({2:s}));\n",
            "{0:s}        return &Struct;\n"
            "{0:s}    }};\n"
            "{0:s}    static CStruct* StructPtr = ClassInitializer();\n"
            "{0:s}    return StructPtr;\n"
            "{0:s}}}\n\n"
            "Uint32 {1:s}::MetaId = 0;\n\n"
            "static TMetaAutoRegister<{1:s}> {1:s}MetaAutoRegister;\n\n"
        };

        SourceCode += std::format(StructDef[0], S0Str, Struct->Name);
        for (size_t i = 0; i < Struct->Properties.size(); i++)
        {
            CProperty* Property = Struct->Properties[i].get();
            switch (Struct->Properties[i]->Flag & CPF_TypeMaskBitFlag)
            {
            case CPF_BoolFlag       :
                SourceCode += std::format(StructDef[1], S0Str, "Bool", 
                    std::format("\"{0:s}\", offsetof({1:s},{0:s}), EPropertyFlag({2:#010x}), {3:d}", 
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case CPF_Int8Flag       :
	        case CPF_Int16Flag      :
	        case CPF_Int32Flag      :
	        case CPF_Int64Flag      :
                SourceCode += std::format(StructDef[1], S0Str, std::format("Int{:d}", 8 * (Struct->Properties[i]->Flag / CPF_Int8Flag)),
                    std::format("\"{0:s}\", offsetof({1:s},{0:s}), EPropertyFlag({2:#010x}), {3:d}",
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case CPF_Uint8Flag      :
	        case CPF_Uint16Flag     :
	        case CPF_Uint32Flag     :
	        case CPF_Uint64Flag     :
                SourceCode += std::format(StructDef[1], S0Str, std::format("Uint{:d}", 8 * (Struct->Properties[i]->Flag / CPF_Uint8Flag)),
                    std::format("\"{0:s}\", offsetof({1:s},{0:s}), EPropertyFlag({2:#010x}), {3:d}",
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case CPF_FloatFlag  :
                SourceCode += std::format(StructDef[1], S0Str, "Float",
                    std::format("\"{0:s}\", offsetof({1:s},{0:s}), EPropertyFlag({2:#010x}), {3:d}",
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case CPF_DoubleFlag :
                SourceCode += std::format(StructDef[1], S0Str, "Double",
                    std::format("\"{0:s}\", offsetof({1:s},{0:s}), EPropertyFlag({2:#010x}), {3:d}",
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case CPF_StringFlag     :
                SourceCode += std::format(StructDef[1], S0Str, "String",
                    std::format("\"{0:s}\", offsetof({1:s},{0:s}), EPropertyFlag({2:#010x}), {3:d}",
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case CPF_StructFlag     :
            {
                CStructProperty* StructProperty = (CStructProperty*)Property;
                if (StructProperty->Meta->IsForwardDeclared)
                {
                    SourceCode += std::format(StructDef[1], S0Str, "Class",
                        std::format("\"{0:s}\", nullptr, offsetof({1:s},{0:s}), EPropertyFlag({2:#010x}), {3:d}",
                            Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                    SourceCode += std::format(DeferredRegisterCode, S8Str, StructProperty->Meta->Name, i);
                }
                else
                {
                    SourceCode += std::format(StructDef[1], S0Str, "Class",
                        std::format("\"{0:s}\", {4:s}::GetStruct(), offsetof({1:s},{0:s}), EPropertyFlag({2:#010x}), {3:d}",
                            Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number, StructProperty->Meta->Name));
                }
                break;
            }
	        case CPF_ClassFlag      :
            {
                CClassProperty* ClassProperty = (CClassProperty*)Property;
                if(ClassProperty->Meta->IsForwardDeclared)
                {
                    SourceCode += std::format(StructDef[1], S0Str, "Class",
                        std::format("\"{0:s}\", nullptr, offsetof({1:s},{0:s}), EPropertyFlag({2:#010x}), {3:d}",
                            Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                    SourceCode += std::format(DeferredRegisterCode, S8Str, ClassProperty->Meta->Name, i);
                }
                else
                {
                    SourceCode += std::format(StructDef[1], S0Str, "Class",
                        std::format("\"{0:s}\", {4:s}::GetMeta(), offsetof({1:s},{0:s}), EPropertyFlag({2:#010x}), {3:d}",
                            Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number, ClassProperty->Meta->Name));
                }
            }
                break;
            default:
                break;
            }
        }
        SourceCode += std::format(StructDef[2], S0Str, Struct->Name);
    }
    return SourceCode;
}

// 
//std::string CCodeGenerator::ToGeneratedSourceCode(CClass* Class, std::vector<std::string>& DependHeaderFile)
//{
//    std::string SourceCode;
//    SourceCode.reserve(4*1024);
//    if (Class->IsEnumClass()) {
//        FEnumClass* EnumClass = (FEnumClass*)Class;
//        SourceCode += std::format(
//        "const CClass* TEnum<{0:s}>::GetMeta()\n"
//        "{{\n"
//        "    static std::function<CClass* ()> ClassInitializer = []() -> CClass* {{\n"
//        "        static struct {0:s}Class : public FEnumClass {{\n", EnumClass->Name);
//        if (Class->HasDefaultConstructor()) {
//            SourceCode += std::format(
//        "            void* New()                    override {{ return new {0:s}(); }}\n"
//            , EnumClass->Name);
//        }
//        if (Class->HasDefaultConstructor()) {
//            SourceCode += std::format(
//        "            void Delete(void* Object)      override {{ delete ({0:s}*)Object; }}\n"
//            , EnumClass->Name);
//        }
//        SourceCode += std::format(
//        "            const char* ToString(Uint64 In) override\n"
//        "            {{\n"
//        "                switch (In)\n"
//        "                {{\n",
//            EnumClass->Name);
//        for (size_t i = 0; i < EnumClass->Options.size(); i++)
//        {
//            SourceCode += std::format(
//        "                case {0:d}:\n"
//        "                    return \"{1:s}\";\n",
//                EnumClass->Options[i].second,
//                EnumClass->Options[i].first);
//        }
//        SourceCode += std::format(
//        "                default:\n"
//        "                    return \"?*?*?\";\n"
//        "                }}\n"
//        "            }}\n\n");
//        SourceCode += std::format(
//        "        }} Class{{}};\n"
//        "        Class.Name = \"{0:s}\";\n"
//        "        Class.Size = sizeof({0:s});\n"
//        "        Class.Flag = {1:#010x};\n",
//            Class->Name,
//            Class->Flag);
//    }else{
//        SourceCode += std::format(
//        "const CClass* {0:s}::GetMeta()\n"
//        "{{\n"
//        "    static std::function<CClass* ()> ClassInitializer = []() -> CClass* {{\n"
//        "        static struct {0:s}Class : public CClass {{\n",
//            Class->Name
//        );
//        if(Class->HasDefaultConstructor()){
//            SourceCode += std::format(
//        "            void* New()                    override {{ return new {0:s}(); }}\n"
//        "            void Constructor(void* Object) override {{ new (Object) {0:s}(); }}\n", Class->Name);
//        }
//        if (Class->HasDefaultConstructor()) {
//            SourceCode += std::format(
//        "            void Delete(void* Object)      override {{ delete ({0:s}*)Object; }}\n"
//        "            void Destructor(void* Object)  override {{ reinterpret_cast<{0:s}*>(Object)->~{0:s}(); }}\n", Class->Name);
//        }
//        SourceCode += std::format(
//        "        }} Class{{}};\n"
//        "        Class.Name = \"{0:s}\";\n"
//        "        Class.Size = sizeof({0:s});\n"
//        "        Class.Flag = {1:#010x};\n",
//            Class->Name,
//            Class->Flag
//        );
//    }
//    SourceCode += std::format(
//        "        Class.Properties.resize({:d});\n", Class->Properties.size());
//    for (size_t i = 0; i < Class->Properties.size(); i++)
//    {
//        CClass* FieldsClass = const_cast<CClass*>(Class->Properties[i].Class);
//        if(FieldsClass->IsForwardDeclaredClass()){
//            SourceCode += std::format(
//        "        FMetaTable::Get().DeferredRegisterList.push_back([&] {{\n"
//        "            CClass* FieldClass = FMetaTable::Get().GetMeta(\"{1:s}\");\n"
//        "            if(FieldClass != nullptr) {{\n"
//        "                Class.Properties[{0:d}].Class = FieldClass;\n"
//        "                return true;\n"
//        "            }}\n"
//        "            assert(false && \"CLASS {1:s} NO EXIST\");\n"
//        "            return false;\n"
//        "        }});\n", 
//        i, 
//        Class->Properties[i].Class->Name);
//        }
//        else
//        {
//            SourceCode += std::format(
//        "        Class.Properties[{0:d}].Class = {1:s}::GetMeta();\n", i, Class->Properties[i].Class->Name);
//        }
//        SourceCode += std::format(
//        "        Class.Properties[{0:d}].Name = \"{2:s}\";\n"
//        "        Class.Properties[{0:d}].Flag = {3:#010x};\n"
//        "        Class.Properties[{0:d}].Offset = offsetof({5:s},{2:s});\n"
//        "        Class.Properties[{0:d}].Number = {4:d};\n",
//        i,
//        Class->Properties[i].Class->Name,
//        Class->Properties[i].Name,
//        Class->Properties[i].Flag,
//        Class->Properties[i].Number,
//        Class->Name);
//    }
//    if (Class->Alias.size() > 0) {
//        SourceCode += std::format(
//        "        Class.Alias.resize({:d});\n", Class->Alias.size());
//        for (size_t i = 0; i < Class->Alias.size(); i++)
//        {
//            SourceCode += std::format(
//        "        Class.Alias[{0:d}] = {1:s};\n", i, Class->Alias[i]);
//        }
//
//    }
//
//    if (Class->ParentClasses.size() > 0) {
//        SourceCode += std::format(
//        "        Class.ParentClasses.resize({:d});\n", Class->ParentClasses.size());
//        for (size_t i = 0; i < Class->ParentClasses.size(); i++)
//        {
//            if (const_cast<CClass*>(Class->ParentClasses[i])->IsReflectClass())
//            {
//                SourceCode += std::format(
//        "        Class.ParentClasses[{0:d}] = {1:s}::GetMeta();\n", i, Class->ParentClasses[i]->Name);
//            }
//        }
//    }
//
//    SourceCode += std::format(
//        "        return &Class;\n"
//        "    }};\n"
//        "    static CClass* {0:s}Class = ClassInitializer();\n"
//        "    return {0:s}Class;\n"
//        "}}\n\n",
//        Class->Name);
//
//    // ClassId init must before ClassAutoRegister
//    if (!Class->IsEnumClass()) {
//    SourceCode += std::format(
//        "Uint32 {0:s}::ClassId = 0;\n\n",
//        Class->Name);
//    SourceCode += std::format(
//        "static TClassAutoRegister<{0:s}> {0:s}ClassAutoRegister;\n\n",
//        Class->Name);
//    }else{
//    SourceCode += std::format(
//        "Uint32 TEnum<{0:s}>::ClassId = 0;\n\n",
//        Class->Name);
//    SourceCode += std::format(
//        "static TClassAutoRegister<TEnum<{0:s}>> {0:s}ClassAutoRegister;\n\n",
//        Class->Name);
//    }
//    return SourceCode;
//}
//
//
