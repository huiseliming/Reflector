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

CCodeGenerator& CCodeGenerator::Get()
{
    static CCodeGenerator CodeGenerator;
    return CodeGenerator;
}

bool CCodeGenerator::Generate()
{
    struct FGeneratedFileContext {
        std::list<FTypeDescriptor*> Descriptors;
        std::fstream GeneratedSourceFile;
        std::fstream GeneratedHeaderFile;
    };
    std::vector<FTypeDescriptor*>& Descriptors = FTypeDescriptorTable::Get().Descriptors;
    std::unordered_map<std::string, FGeneratedFileContext> GeneratedFileContextMap;
    for (size_t i = 0; i < Descriptors.size(); i++)
    {
        if (!Descriptors[i]->DeclaredFile.empty())
        {
            if (std::end(GeneratedFileContextMap) == GeneratedFileContextMap.find(Descriptors[i]->DeclaredFile)) {
                GeneratedFileContextMap.insert(std::make_pair<>(Descriptors[i]->DeclaredFile, FGeneratedFileContext()));
                std::string DeclaredFile = Descriptors[i]->DeclaredFile;
                std::size_t Pos = DeclaredFile.find_last_of("/\\");
                std::string DeclaredPath = DeclaredFile.substr(0, Pos);
                std::string Filename = DeclaredFile.substr(Pos + 1);
                std::size_t DotPos = Filename.rfind(".");
                std::string FilenameNotDotH = Filename.substr(0, DotPos);
                std::string GeneratedHeaderFile = std::filesystem::current_path().string() + "/" + FilenameNotDotH + ".reflect.h";
                std::string GeneratedSourceFile = std::filesystem::current_path().string() + "/" + FilenameNotDotH + ".reflect.cpp";
                GeneratedFileContextMap[Descriptors[i]->DeclaredFile].GeneratedHeaderFile.open(GeneratedHeaderFile, std::ios::out | std::ios::trunc);
                GeneratedFileContextMap[Descriptors[i]->DeclaredFile].GeneratedSourceFile.open(GeneratedSourceFile, std::ios::out | std::ios::trunc);
                if (!GeneratedFileContextMap[Descriptors[i]->DeclaredFile].GeneratedHeaderFile.is_open()) {
                    llvm::errs() << std::format("File<{:s}> open failed\n", GeneratedHeaderFile);
                    return false;
                }
                if (!GeneratedFileContextMap[Descriptors[i]->DeclaredFile].GeneratedSourceFile.is_open()) {
                    llvm::errs() << std::format("File<{:s}> open failed\n", GeneratedSourceFile);
                    return false;
                }
                llvm::outs() << std::format("File<{:s}> generated\n", GeneratedHeaderFile);
                llvm::outs() << std::format("File<{:s}> generated\n", GeneratedSourceFile);
                std::string HeaderFileBegin = "#pragma once\n\n";
                std::string SourceFileBegin = "#include \"" + Filename + "\"\n\n";
                GeneratedFileContextMap[Descriptors[i]->DeclaredFile].GeneratedHeaderFile.write(HeaderFileBegin.data(), HeaderFileBegin.size());
                GeneratedFileContextMap[Descriptors[i]->DeclaredFile].GeneratedSourceFile.write(SourceFileBegin.data(), SourceFileBegin.size());
            }
            GeneratedFileContextMap[Descriptors[i]->DeclaredFile].Descriptors.push_back(Descriptors[i]);
        }
    }
    for (auto GeneratedFileContextIterator = GeneratedFileContextMap.begin(); GeneratedFileContextIterator != GeneratedFileContextMap.end(); GeneratedFileContextIterator++)
    {
        for (auto Iterator = GeneratedFileContextIterator->second.Descriptors.begin(); Iterator != GeneratedFileContextIterator->second.Descriptors.end(); Iterator++)
        {
            std::string GeneratedHeaderCode = ToGeneratedHeaderCode(*Iterator);
            std::string GeneratedSourceCode = ToGeneratedSourceCode(*Iterator);
            GeneratedFileContextIterator->second.GeneratedHeaderFile.write(GeneratedHeaderCode.data(), GeneratedHeaderCode.size());
            GeneratedFileContextIterator->second.GeneratedSourceFile.write(GeneratedSourceCode.data(), GeneratedSourceCode.size());
        }
        GeneratedFileContextIterator->second.GeneratedHeaderFile.close();
        GeneratedFileContextIterator->second.GeneratedSourceFile.close();
    }
    return true;
}

std::string CCodeGenerator::ToGeneratedHeaderCode(FTypeDescriptor* Descriptor)
{
    std::string HeaderCode;
    return HeaderCode;
}

std::string CCodeGenerator::ToGeneratedSourceCode(FTypeDescriptor* Descriptor)
{
    std::string SourceCode;
    std::string TypeKind = Descriptor->GetTypeKind();
    TypeKind[0] = std::toupper(TypeKind[0]);
    SourceCode += std::format("Uint32 {0:s}::GetTypeId() {{ return {1:d}; }}\n\n"
                              "FTypeDescriptor* {0:s}::GetTypeDescriptor()\n"
                              "{{\n"
                              "    static std::function<FTypeDescriptor* ()> DescriptorInitializer = []() -> FTypeDescriptor* {{\n"
                              "        struct F{0:s}Descriptor : public F{3:s}Descriptor {{\n"
                              "            F{0:s}Descriptor(const char* InTypeName, size_t InTypeSize = 0)\n"
                              "                : F{3:s}Descriptor(InTypeName, InTypeSize)\n"
                              "            {{}}\n"
                              "            virtual void* New()                               override {{ {4:s} }}\n"
                              "            virtual void Delete(void* Object)                 override {{ {5:s} }}\n"
                              "            virtual void Constructor(void* ConstructedObject) override {{ {6:s} }}\n"
                              "            virtual void Destructor(void* DestructedObject)   override {{ {7:s} }}\n"
                              "        }};\n"
                              "        static F{0:s}Descriptor Descriptor(\"{0:s}\", sizeof({0:s}));\n"
                              "        Descriptor.Fields.resize({2:d});\n", Descriptor->TypeName[0], Descriptor->TypeId, Descriptor->Fields.size(), TypeKind,
                                Descriptor->HasDefaultConstructor() ? std::format("return new {0:s}();", Descriptor->TypeName[0]) : "return nullptr;",
                                Descriptor->HasDestructor() ? std::format("delete ({0:s}*)Object;", Descriptor->TypeName[0]) : "",
                                Descriptor->HasDefaultConstructor() ? std::format("new (ConstructedObject) {0:s}();", Descriptor->TypeName[0]) : "",
                                Descriptor->HasDestructor() ? std::format("reinterpret_cast<{0:s}*>(DestructedObject)->~{0:s}();", Descriptor->TypeName[0]) : "");
    for (size_t i = 0; i < Descriptor->Fields.size(); i++)
    {
    SourceCode += std::format("        Descriptor.Fields[{0:d}].FieldName = \"{1:s}\";\n"
                              "        Descriptor.Fields[{0:d}].FieldOffset = offsetof({2:s},{1:s});\n"
                              "        Descriptor.Fields[{0:d}].Number = {3:d};\n"
                              "        Descriptor.Fields[{0:d}].TypeDescriptor = {4:s}\n"
                              "        Descriptor.Fields[{0:d}].QualifierFlag = {5:#010x};\n", i, Descriptor->Fields[i].FieldName, Descriptor->TypeName[0], Descriptor->Fields[i].Number, 
                                (Descriptor->Fields[i].TypeDescriptor->IsBuiltInType() || (Descriptor->Fields[i].TypeDescriptor->TypeId < ReserveObjectIdEnd)) ?
                                std::format("FTypeDescriptorTable::Get().GetDescriptor({:d}); //{:s}", Descriptor->Fields[i].TypeDescriptor->TypeId, Descriptor->Fields[i].TypeDescriptor->TypeName[0]) :
                                std::format("{:s}::GetTypeDescriptor()", Descriptor->Fields[i].TypeDescriptor->GetTypeName()),
                                Descriptor->Fields[i].QualifierFlag);
    }
    for (size_t i = 0; i < Descriptor->TypeName.size(); i++)
    {
    SourceCode += std::format("        Descriptor.TypeName.push_back(\"{:s}\");\n", Descriptor->TypeName[i]);
    }

    SourceCode += std::format("        Descriptor.TypeFlag = {0:#010x};\n"
                              "        return &Descriptor;\n"
                              "    }};\n"
                              "    static FTypeDescriptor* DescriptorPtr = DescriptorInitializer();\n"
                              "    return DescriptorPtr;\n"
                              "}}\n\n", Descriptor->TypeFlag);
    SourceCode += std::format("struct F{0:s}DescriptorAutoRegister {{\n"
                              "    F{0:s}DescriptorAutoRegister(){{\n"
                              "        FTypeDescriptor* Descriptor = {0:s}::GetTypeDescriptor();\n"
                              "        int32_t DescriptorId = {0:s}::GetTypeId();\n"
                              "        FTypeDescriptorTable::Get().NameToId.insert(std::make_pair(\"{0:s}\", DescriptorId));\n", Descriptor->TypeName[0]);
                                       for (size_t i = 1; i < Descriptor->TypeName.size(); i++)
                                       {
    SourceCode += std::format("            FTypeDescriptorTable::Get().NameToId.insert(std::make_pair(\"{0:s}\", DescriptorId));\n", Descriptor->TypeName[i]);
                                       }
    SourceCode += std::format("        if(FTypeDescriptorTable::Get().Descriptors.size() <= DescriptorId){{\n"
                              "            FTypeDescriptorTable::Get().Descriptors.resize(DescriptorId + 1);\n"
                              "        }}\n"
                              "        FTypeDescriptorTable::Get().Descriptors[DescriptorId] = Descriptor;\n"
                              "    }}\n"
                              "}};\n\n"
                              "static F{0:s}DescriptorAutoRegister {0:s}DescriptorAutoRegister;\n\n", Descriptor->TypeName[0]);
    return SourceCode;
}


