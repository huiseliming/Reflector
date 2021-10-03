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
                    llvm::errs() << std::format("File<{:s}> open failed", GeneratedHeaderFile);
                    return false;
                }
                if (!GeneratedFileContextMap[Descriptors[i]->DeclaredFile].GeneratedSourceFile.is_open()) {
                    llvm::errs() << std::format("File<{:s}> open failed", GeneratedSourceFile);
                    return false;
                }
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
    SourceCode += std::format("Uint32 {:s}::GetTypeId() {{ return {:d}; }}\n\n", Descriptor->TypeName[0], Descriptor->TypeId);
    SourceCode += std::format("FTypeDescriptor* {:s}::GetTypeDescriptor()\n", Descriptor->TypeName[0]);
    SourceCode += std::format("{{\n");
    SourceCode += std::format("    static std::function<FTypeDescriptor* ()> DescriptorInitializer = []() -> FTypeDescriptor* {{       \n");
    SourceCode += std::format("        static F{0:s}Descriptor Descriptor(\"{1:s}\", sizeof({1:s}));                                      \n", TypeKind, Descriptor->TypeName[0]);
    SourceCode += std::format("        Descriptor.Fields.resize({:d});                                                               \n", Descriptor->Fields.size());
    for (size_t i = 0; i < Descriptor->Fields.size(); i++)
    {
        SourceCode += std::format("        Descriptor.Fields[{:d}].FieldName = \"{:s}\";                                                \n", i, Descriptor->Fields[i].FieldName);
        SourceCode += std::format("        Descriptor.Fields[{:d}].FieldOffset = {:d};                                                   \n", i, Descriptor->Fields[i].FieldOffset);
        SourceCode += std::format("        Descriptor.Fields[{:d}].Number = {:d};                                                        \n", i, Descriptor->Fields[i].Number);
        if (Descriptor->Fields[i].TypeDescriptor->IsBuiltInType())
            SourceCode += std::format("        Descriptor.Fields[{:d}].TypeDescriptor = FTypeDescriptorTable::Get().GetDescriptor({:d});//{:s}                           \n", i, Descriptor->Fields[i].TypeDescriptor->TypeId, Descriptor->Fields[i].TypeDescriptor->TypeName[0]);
        else
            SourceCode += std::format("        Descriptor.Fields[{:d}].TypeDescriptor = {:s}::GetTypeDescriptor();                           \n", i, Descriptor->Fields[i].TypeDescriptor->GetTypeName());
    }
    for (size_t i = 0; i < Descriptor->TypeName.size(); i++)
    {
        SourceCode += std::format("        Descriptor.TypeName.push_back(\"{:s}\");                          \n", Descriptor->TypeName[i]);
    }
    SourceCode += std::format("        return &Descriptor;                                                                            \n");
    SourceCode += std::format("    }};                                                                                                 \n");
    SourceCode += std::format("    static FTypeDescriptor* DescriptorPtr = DescriptorInitializer();                                      \n", TypeKind, Descriptor->TypeName[0]);
    SourceCode += std::format("    return DescriptorPtr;                                      \n", TypeKind, Descriptor->TypeName[0]);
    SourceCode += std::format("}}\n\n");
    return SourceCode;
}
