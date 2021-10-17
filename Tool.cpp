#include "Tool.h"
#include "clang/AST/Type.h"
#include "clang/AST/Attr.h"
#include <filesystem>
#include "CodeGenerator.h"

using namespace llvm;
using namespace clang;

void SplitAnnotation(StringRef AnnotationString, std::vector<std::string>& Annotations)
{
    uint32_t BracketCounter = 0;
    Annotations.clear();
    size_t PreviousPos = 0;
    for (size_t i = 0; i < AnnotationString.size(); i++)
    {
        if (BracketCounter == 0 && AnnotationString[i] == ',') {
            std::string Str(AnnotationString.data() + PreviousPos, i - PreviousPos);
            char MatchCharacter = '\0';
            Str.erase(std::remove_if(Str.begin(), Str.end(), [&] (char& C) {
                    if (MatchCharacter == '\0' && (C == '\'' || C == '\"')) MatchCharacter = C;
                    else if (MatchCharacter == C)  MatchCharacter = '\0';
                    else if (std::isspace(C) && MatchCharacter == '\0') return true;
                    return false;
                }
            ), Str.end());
            Annotations.emplace_back(Str);
            PreviousPos = i + 1;
        }
        if (AnnotationString[i] == '(')
        {
            BracketCounter++;
        }
        if (AnnotationString[i] == ')')
        {
            BracketCounter--;
        }
    }
    std::string Str(AnnotationString.data() + PreviousPos, AnnotationString.size() - PreviousPos);
    char MatchCharacter = '\0';
    Str.erase(std::remove_if(Str.begin(), Str.end(), [&](char& C) {
        if (MatchCharacter == '\0' && (C == '\'' || C == '\"')) MatchCharacter = C;
        else if (MatchCharacter == C)  MatchCharacter = '\0';
        else if (std::isspace(C) && MatchCharacter == '\0') return true;
        return false;
        }
    ), Str.end());
    Annotations.emplace_back(Str);

}

bool FindReflectAnnotation(const clang::Decl* CheckedDecl, const char* FoundMarkStr, std::vector<std::string>& ReflectAnnotation) {
    for (auto AttrIterator = CheckedDecl->attr_begin(); AttrIterator < CheckedDecl->attr_end(); AttrIterator++) {
        if ((*AttrIterator)->getKind() == clang::attr::Annotate)
        {
            AnnotateAttr* AnnotateAttrPtr = dyn_cast<AnnotateAttr>(*AttrIterator);
            SplitAnnotation(AnnotateAttrPtr->getAnnotation(), ReflectAnnotation);
            if (ReflectAnnotation.size() > 0 && ReflectAnnotation[0] == FoundMarkStr) {
                return true;
            }
        }
    }
    return false;
}

bool FindReflectAnnotation(const clang::Decl* CheckedDecl, std::vector<const char*> FoundMarkStrs, std::vector<std::string>& ReflectAnnotation) {
    for (auto AttrIterator = CheckedDecl->attr_begin(); AttrIterator < CheckedDecl->attr_end(); AttrIterator++) {
        if ((*AttrIterator)->getKind() == clang::attr::Annotate)
        {
            AnnotateAttr* AnnotateAttrPtr = dyn_cast<AnnotateAttr>(*AttrIterator);
            SplitAnnotation(AnnotateAttrPtr->getAnnotation(), ReflectAnnotation);
            for (size_t i = 0; i < FoundMarkStrs.size(); i++)
            {
                if (ReflectAnnotation.size() > 0 && ReflectAnnotation[0] == FoundMarkStrs[i]) {
                    return true;
                }
            }
        }
    }
    return false;
}


void ParsingMetaString(CMeta* Meta, std::vector<std::string>& ReflectAnnotation)
{
    for (size_t i = 0; i < ReflectAnnotation.size(); i++)
    {
        if (std::strncmp(ReflectAnnotation[i].c_str(), "Meta=(", sizeof("Meta=(") - 1) == 0 && ')' == ReflectAnnotation[i].back())
        {
            std::string MetaString = ReflectAnnotation[i].substr(6, ReflectAnnotation[i].size() - 6 - 1);
            size_t PreviousPos = 0;
            for (size_t j = 0; j < MetaString.size(); j++)
            {
                if (MetaString[j] == ',') {
                    std::string Str(MetaString.data() + PreviousPos, j - PreviousPos);
                    size_t Pos = Str.find_last_of('=');
                    std::string Key = Str.substr(0, Pos);
                    std::string Value = Str.substr(Pos + 1);
                    Meta->Data.insert_or_assign(Key, Value);
                    PreviousPos = j + 1;
                }
            }
            std::string Str(MetaString.data() + PreviousPos, MetaString.size() - PreviousPos);
            size_t Pos = Str.find_first_of('=');
            std::string Key;
            std::string Value;
            if (Pos == std::string::npos) {
                Key = Str;
            }
            else
            {
                Key = Str.substr(0, Pos);
                Value = Str.substr(Pos + 1);
            }
            Meta->Data.insert_or_assign(Key, Value);
        }
    }
}


//std::string GetDeclFileAbsPath(clang::ASTContext* const Context, const Decl* D)
//{
//    SourceRange Loc = D->getSourceRange();
//    std::filesystem::path File(Context->getSourceManager().getFilename(Loc.getBegin()).str());
//    if (File.is_absolute())
//        return File.string();
//    else
//        return std::filesystem::canonical(std::filesystem::path(CCodeGenerator::Get().BuildPath + "/" + File.string())).string();
//}

bool IsMatchedCppHeaderAndSource(std::string& HeaderFile, std::string& SourceFile)
{
    std::string HeaderFileName = std::filesystem::path(HeaderFile).filename().string();
    std::string SourceFileName = std::filesystem::path(SourceFile).filename().string();
    size_t HeaderPos = HeaderFileName.find_last_of(".");
    size_t SourcePos = SourceFileName.find_last_of(".");
    if (HeaderFileName.substr(0, HeaderPos) == SourceFileName.substr(0, SourcePos))
    {
        return true;
    }
    return false;
}