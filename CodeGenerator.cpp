#include "CodeGenerator.h"
#include <fstream>
#include <filesystem>
#include "clang/Tooling/Tooling.h"

using namespace llvm;
using namespace clang;

std::string ParsingDoubleQuotes(std::string str) {
    size_t Start = 0;
    size_t End = str.size();
    if (((str[0] == '\"' && str[str.size() - 1] == '\"')||
        (str[0] == '\'' && str[str.size() - 1] == '\'')) && str.size() > 2)
    {
        Start = 1;
        End = str.size() - 1;
    }
    
    std::string ret;
    ret.reserve((End - Start) * 2);
    for (size_t i = Start; i < End; i++)
    {
        if (str[i] == '\"' && (str[i - 1] != '\\' || i == 0 )) {

            ret.push_back('\\');
            ret.push_back(str[i]);
        }
        else
        {
            ret.push_back(str[i]);
        }
    }
    return ret;
}

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
        "{0:s}        EnumClass.Options = {{\n",//0
        "{0:s}            {{\"{1:s}\", {2:d}}},\n",//1
        "{0:s}        }};\n"
        "{0:s}        EnumClass.Data = {{\n",//2
        "{0:s}            {{\"{1:s}\", \"{2:s}\"}},\n",//3
        "{0:s}        }};\n"
        "{0:s}        return &EnumClass;\n"
        "{0:s}    }};\n"
        "{0:s}    static CEnumClass* EnumClassPtr = ClassInitializer();\n"
        "{0:s}    return EnumClassPtr;\n"
        "{0:s}}}\n\n"
        "Uint32 TEnumClass<{1:s}>::MetaId = 0;\n\n"
        "static TMetaAutoRegister<TEnumClass<{1:s}>> {1:s}MetaAutoRegister;\n\n"//4
        };
        SourceCode += std::format(EnumTemplateDef[0], S0Str, EnumClass->Name);
        for (size_t i = 0; i < EnumClass->Options.size(); i++)
        {
            SourceCode += std::format(EnumTemplateDef[1], S0Str, EnumClass->Options[i].first, EnumClass->Options[i].second);
        }
        SourceCode += std::format(EnumTemplateDef[2], S0Str);
        for (auto Iterator = EnumClass->Data.begin(); Iterator != EnumClass->Data.end(); Iterator++)
        {
            SourceCode += std::format(EnumTemplateDef[3], S0Str, Iterator->first, ParsingDoubleQuotes(Iterator->second));
        }
        SourceCode += std::format(EnumTemplateDef[4], S0Str, EnumClass->Name);
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
            "{0:s}        Struct.Size = sizeof({1:s});\n"
            "{0:s}        Struct.Data = {{\n",//0
            "{0:s}            {{\"{1:s}\", \"{2:s}\"}},\n",//1
            "{0:s}        }};\n",//2
            "{0:s}        Struct.Properties.push_back(std::make_unique<C{1:s}Property>({2:s}));\n",//3
            "{0:s}        Struct.Properties.back()->Data = {{\n",//4
            "{0:s}            {{ \"{1:s}\", \"{2:s}\" }},\n",//5
            "{0:s}        }};\n",//6
            "{0:s}        {1:s}\n",//7
            "{0:s}        return &Struct;\n"
            "{0:s}    }};\n"
            "{0:s}    static CStruct* StructPtr = ClassInitializer();\n"
            "{0:s}    return StructPtr;\n"
            "{0:s}}}\n\n"
            "Uint32 {1:s}::MetaId = 0;\n\n"
            "static TMetaAutoRegister<{1:s}> {1:s}MetaAutoRegister;\n\n"//8
        };

        SourceCode += std::format(StructDef[0], S0Str, Struct->Name);
        for (auto Iterator = Struct->Data.begin(); Iterator != Struct->Data.end(); Iterator++)
        {
            std::string srt = ParsingDoubleQuotes(Iterator->second);
            SourceCode += std::format(StructDef[1], S0Str, Iterator->first, srt);
        }
        SourceCode += std::format(StructDef[2], S0Str);
        for (size_t i = 0; i < Struct->Properties.size(); i++)
        {
            CProperty* Property = Struct->Properties[i].get();
            switch (Struct->Properties[i]->Flag & EPF_TypeMaskBitFlag)
            {
            case EPF_BoolFlag       :
                SourceCode += std::format(StructDef[3], S0Str, "Bool", 
                    std::format("\"{0:s}\", EPropertyFlag({2:#010x}), offsetof({1:s},{0:s}), {3:d}", 
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case EPF_Int8Flag       :
	        case EPF_Int16Flag      :
	        case EPF_Int32Flag      :
	        case EPF_Int64Flag      :
                SourceCode += std::format(StructDef[3], S0Str, std::format("Int{:d}", 8 * (Struct->Properties[i]->Flag / EPF_Int8Flag)),
                    std::format("\"{0:s}\", EPropertyFlag({2:#010x}), offsetof({1:s},{0:s}), {3:d}",
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case EPF_Uint8Flag      :
	        case EPF_Uint16Flag     :
	        case EPF_Uint32Flag     :
	        case EPF_Uint64Flag     :
                SourceCode += std::format(StructDef[3], S0Str, std::format("Uint{:d}", 8 * (Struct->Properties[i]->Flag / EPF_Uint8Flag)),
                    std::format("\"{0:s}\", EPropertyFlag({2:#010x}), offsetof({1:s},{0:s}), {3:d}",
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case EPF_FloatFlag  :
                SourceCode += std::format(StructDef[3], S0Str, "Float",
                    std::format("\"{0:s}\", EPropertyFlag({2:#010x}), offsetof({1:s},{0:s}), {3:d}",
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case EPF_DoubleFlag :
                SourceCode += std::format(StructDef[3], S0Str, "Double",
                    std::format("\"{0:s}\", EPropertyFlag({2:#010x}), offsetof({1:s},{0:s}), {3:d}",
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case EPF_StringFlag     :
                SourceCode += std::format(StructDef[3], S0Str, "String",
                    std::format("\"{0:s}\", EPropertyFlag({2:#010x}), offsetof({1:s},{0:s}), {3:d}",
                        Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                break;
	        case EPF_StructFlag     :
            {
                CStructProperty* StructProperty = (CStructProperty*)Property;
                if (StructProperty->Meta->IsForwardDeclared)
                {
                    SourceCode += std::format(StructDef[3], S0Str, "Class",
                        std::format("\"{0:s}\", nullptr, EPropertyFlag({2:#010x}), offsetof({1:s},{0:s}), {3:d}",
                            Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                    SourceCode += std::format(DeferredRegisterCode, S8Str, StructProperty->Meta->Name, i);
                }
                else
                {
                    SourceCode += std::format(StructDef[3], S0Str, "Class",
                        std::format("\"{0:s}\", {4:s}::GetStruct(), EPropertyFlag({2:#010x}), offsetof({1:s},{0:s}), {3:d}",
                            Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number, StructProperty->Meta->Name));
                }
                break;
            }
	        case EPF_ClassFlag      :
            {
                CClassProperty* ClassProperty = (CClassProperty*)Property;
                if(ClassProperty->Meta->IsForwardDeclared)
                {
                    SourceCode += std::format(StructDef[3], S0Str, "Class",
                        std::format("\"{0:s}\", nullptr, EPropertyFlag({2:#010x}), offsetof({1:s},{0:s}), {3:d}",
                            Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number));
                    SourceCode += std::format(DeferredRegisterCode, S8Str, ClassProperty->Meta->Name, i);
                }
                else
                {
                    SourceCode += std::format(StructDef[3], S0Str, "Class",
                        std::format("\"{0:s}\", {4:s}::GetMeta(), EPropertyFlag({2:#010x}), offsetof({1:s},{0:s}), {3:d}",
                            Property->Name, Struct->Name, (Uint32)Property->Flag, Property->Number, ClassProperty->Meta->Name));
                }
            }
                break;
            default:
                break;
            }
            SourceCode += std::format(StructDef[4], S0Str);
            for (auto Iterator = Struct->Properties[i]->Data.begin(); Iterator != Struct->Properties[i]->Data.end(); Iterator++)
            {
                SourceCode += std::format(StructDef[5], S0Str, Iterator->first, ParsingDoubleQuotes(Iterator->second));
            }
            SourceCode += std::format(StructDef[6], S0Str);
        }
        if (Struct->Parent) {
            if (Struct->IsForwardDeclared) {
              SourceCode += std::format(StructDef[7], S0Str, std::format(
                  "GMetaTable->DeferredRegisterList.push_back([Struct] {{ Struct.Parent = (CStruct*)GMetaTable->GetMeta(\"{0:s}\"); }});",  
                  Struct->Parent->Name));
            } else {
              SourceCode += std::format(StructDef[7], S0Str, std::format("Struct.Parent = (CStruct*){0:s}::StaticMeta();", Struct->Parent->Name));
            }
        }
        SourceCode += std::format(StructDef[8], S0Str, Struct->Name);
    }
    return SourceCode;
}
