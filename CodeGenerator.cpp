#include "CodeGenerator.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#endif



std::string GetExePath()
{
#ifdef _WIN32
    char result[MAX_PATH];
    return std::string(result, GetModuleFileNameA(NULL, result, MAX_PATH));
#elif __APPLE__
    char result[PATH_MAX];
    uint32_t size = 0;
    assert(0 != _NSGetExecutablePath(nullptr, &size));
    char* buffer = new char[size + 1];
    assert(0 == _NSGetExecutablePath(buffer, &size));
    char buf[PATH_MAX]; /* PATH_MAX incudes the \0 so +1 is not required */
    realpath(buffer, result);
    if (!realpath(buffer, result)) {
        delete[] buffer;
        return "";
    }
    delete[] buffer;
    return result;
#else
    char result[PATH_MAX];
    size_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
#endif
}


std::string GetExeDir()
{
    std::string executeFileDir;
    std::string executeFilePath = GetExePath();
    if (!executeFilePath.empty()) {
        std::size_t pos = executeFilePath.find_last_of("/\\");
        if (pos != std::string::npos) {
            executeFileDir = executeFilePath.substr(0ul, pos);
        }
    }
    return executeFileDir;
}

//CCodeGenerator& CCodeGenerator::Get()
//{
//    static CCodeGenerator CodeGenerator;
//    return CodeGenerator;
//}

bool CCodeGenerator::Generate()
{
    struct FGeneratedFileContext {
        std::list<FClass*> Classes;
        std::fstream GeneratedSourceFile;
        std::fstream GeneratedHeaderFile;
    };
    std::vector<std::unique_ptr<FClass>>& Classes = GeneratedReflectClasses;
    std::unordered_map<std::string, FGeneratedFileContext> GeneratedFileContextMap;
    for (size_t i = 0; i < Classes.size(); i++)
    {
        if (Classes[i]->IsReflectionDataCollectionCompleted && !Classes[i]->DeclaredFile.empty())
        {
            if (std::end(GeneratedFileContextMap) == GeneratedFileContextMap.find(Classes[i]->DeclaredFile)) {
                GeneratedFileContextMap.insert(std::make_pair<>(Classes[i]->DeclaredFile, FGeneratedFileContext()));
                std::string DeclaredFile = Classes[i]->DeclaredFile;
                std::size_t Pos = DeclaredFile.find_last_of("/\\");
                std::string DeclaredPath = DeclaredFile.substr(0, Pos);
                std::string Filename = DeclaredFile.substr(Pos + 1);
                std::size_t DotPos = Filename.rfind(".");
                std::string FilenameNotDotH = Filename.substr(0, DotPos);
                std::string GeneratedHeaderFile = std::filesystem::current_path().string() + "/" + FilenameNotDotH + ".reflect.h";
                std::string GeneratedSourceFile = std::filesystem::current_path().string() + "/" + FilenameNotDotH + ".reflect.cpp";
                GeneratedFileContextMap[Classes[i]->DeclaredFile].GeneratedHeaderFile.open(GeneratedHeaderFile, std::ios::out | std::ios::trunc);
                GeneratedFileContextMap[Classes[i]->DeclaredFile].GeneratedSourceFile.open(GeneratedSourceFile, std::ios::out | std::ios::trunc);
                if (!GeneratedFileContextMap[Classes[i]->DeclaredFile].GeneratedHeaderFile.is_open()) {
                    llvm::errs() << std::format("File<{:s}> open failed\n", GeneratedHeaderFile);
                    return false;
                }
                if (!GeneratedFileContextMap[Classes[i]->DeclaredFile].GeneratedSourceFile.is_open()) {
                    llvm::errs() << std::format("File<{:s}> open failed\n", GeneratedSourceFile);
                    return false;
                }
                llvm::outs() << std::format("File<{:s}> generated\n", GeneratedHeaderFile);
                llvm::outs() << std::format("File<{:s}> generated\n", GeneratedSourceFile);
                std::string HeaderFileBegin = "#pragma once\n\n";
                std::string SourceFileBegin = "#include \"" + std::filesystem::path(DeclaredFile).lexically_proximate(std::filesystem::current_path()) .string() + "\"\n\n";
                GeneratedFileContextMap[Classes[i]->DeclaredFile].GeneratedHeaderFile.write(HeaderFileBegin.data(), HeaderFileBegin.size());
                GeneratedFileContextMap[Classes[i]->DeclaredFile].GeneratedSourceFile.write(SourceFileBegin.data(), SourceFileBegin.size());
            }
            GeneratedFileContextMap[Classes[i]->DeclaredFile].Classes.push_back(Classes[i].get());
        }
    }
    for (auto GeneratedFileContextIterator = GeneratedFileContextMap.begin(); GeneratedFileContextIterator != GeneratedFileContextMap.end(); GeneratedFileContextIterator++)
    {
        std::vector<std::string> HeaderDependHeaderFile;
        std::vector<std::string> SourceDependHeaderFile;
        for (auto Iterator = GeneratedFileContextIterator->second.Classes.begin(); Iterator != GeneratedFileContextIterator->second.Classes.end(); Iterator++)
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

std::string CCodeGenerator::ToGeneratedHeaderCode(FClass* Class, std::vector<std::string>& DependHeaderFile)
{
    std::string HeaderCode;
    return HeaderCode;
}

std::string CCodeGenerator::ToGeneratedSourceCode(FClass* Class, std::vector<std::string>& DependHeaderFile)
{
    std::string SourceCode;
    SourceCode.reserve(4*1024);
    if(Class->IsEnumClass()){
        FEnumClass* EnumClass = (FEnumClass*)Class;
        SourceCode += std::format(
        "const char* ToString(const {0:s} In)\n"
        "{{\n"
        "    switch (In)\n"
        "    {{\n",
        EnumClass->Name);
        for (size_t i = 0; i < EnumClass->OptVal.size(); i++)
        {
            SourceCode += std::format(
        "    case {0:d}:\n"
        "        return \"{1:s}\";\n",
            EnumClass->OptVal[i],
            EnumClass->OptName[i]);
        }
        SourceCode += std::format(
        "        default:\n"
        "            return \"Undefine\";\n"
        "    }}\n"
        "}}\n\n");
        return SourceCode;
    }
    SourceCode += std::format(
        "const FClass* {0:s}::GetClass()\n"
        "{{\n"
        "    static std::function<FClass* ()> ClassInitializer = []() -> FClass* {{\n"
        "        static struct {0:s}Class : public FClass {{\n",
        Class->Name
    );
    if(Class->HasDefaultConstructor()){
        SourceCode += std::format(
        "            void* New()                    override {{ return new {0:s}(); }}\n"
        "            void Constructor(void* Object) override {{ new (Object) {0:s}(); }}\n", Class->Name);
    }
    if (Class->HasDefaultConstructor()) {
        SourceCode += std::format(
        "            void Delete(void* Object)      override {{ delete ({0:s}*)Object; }}\n"
        "            void Destructor(void* Object)  override {{ reinterpret_cast<{0:s}*>(Object)->~{0:s}(); }}\n", Class->Name);
    }
    SourceCode += std::format(
        "        }} Class{{}};\n"
        "        Class.Name = \"{0:s}\";\n"
        "        Class.Size = sizeof({0:s});\n"
        "        Class.Flag = {1:#010x};\n",
        Class->Name,
        Class->Flag
    );
    SourceCode += std::format(
        "        Class.Fields.resize({:d});\n", Class->Fields.size());
    for (size_t i = 0; i < Class->Fields.size(); i++)
    {
        FClass* FieldsClass = const_cast<FClass*>(Class->Fields[i].Class);
        if(FieldsClass->IsForwardDeclaredClass()){
            SourceCode += std::format(
        "        FClassTable::Get().DeferredRegisterList.push_back([&] {{\n"
        "            FClass* FieldClass = FClassTable::Get().GetClass(\"{1:s}\");\n"
        "            if(FieldClass != nullptr) {{\n"
        "                Class.Fields[{0:d}].Class = FieldClass;\n"
        "                return true;\n"
        "            }}\n"
        "            assert(false && \"CLASS {1:s} NO EXIST\");\n"
        "            return false;\n"
        "        }});\n", 
        i, 
        Class->Fields[i].Class->Name);
        }
        else
        {
            SourceCode += std::format(
        "        Class.Fields[{0:d}].Class = {1:s}::GetClass();\n", i, Class->Fields[i].Class->Name);
        }
        SourceCode += std::format(
        "        Class.Fields[{0:d}].Name = \"{2:s}\";\n"
        "        Class.Fields[{0:d}].Flag = {3:#010x};\n"
        "        Class.Fields[{0:d}].Offset = offsetof({5:s},{2:s});\n"
        "        Class.Fields[{0:d}].Number = {4:d};\n",
        i,
        Class->Fields[i].Class->Name,
        Class->Fields[i].Name,
        Class->Fields[i].Flag,
        Class->Fields[i].Number,
        Class->Name);
    }
    if (Class->Alias.size() > 0) {
        SourceCode += std::format(
        "        Class.Alias.resize({:d});\n", Class->Alias.size());
        for (size_t i = 0; i < Class->Alias.size(); i++)
        {
            SourceCode += std::format(
        "        Class.Alias[{0:d}] = {1:s};\n", i, Class->Alias[i]);
        }

    }

    if (Class->ParentClasses.size() > 0) {
        SourceCode += std::format(
        "        Class.ParentClasses.resize({:d});\n", Class->ParentClasses.size());
        for (size_t i = 0; i < Class->ParentClasses.size(); i++)
        {
            if (const_cast<FClass*>(Class->ParentClasses[i])->IsReflectClass())
            {
                SourceCode += std::format(
        "        Class.ParentClasses[{0:d}] = {1:s}::GetClass();\n", i, Class->ParentClasses[i]->Name);
            }
        }
    }

    SourceCode += std::format(
        "        return &Class;\n"
        "    }};\n"
        "    static FClass* {0:s}Class = ClassInitializer();\n"
        "    return {0:s}Class;\n"
        "}}\n\n",
        Class->Name);

    // ClassId init must before ClassAutoRegister
    SourceCode += std::format(
        "Uint32 {0:s}::ClassId = 0;\n\n",
        Class->Name);
    SourceCode += std::format(
        "static FClassAutoRegister<{0:s}> {0:s}ClassAutoRegister;\n\n",
        Class->Name);
    return SourceCode;
}


