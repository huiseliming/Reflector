#include "Tool.h"
#include "clang/AST/Type.h"
#include "clang/AST/Attr.h"
#include <filesystem>
#include "CodeGenerator.h"

using namespace llvm;
using namespace clang;

void SplitAnnotation(StringRef AnnotationString, std::vector<std::string>& Annotations)
{
    Annotations.clear();
    size_t PreviousPos = 0;
    for (size_t i = 0; i < AnnotationString.size(); i++)
    {
        if (AnnotationString[i] == ',') {
            std::string Str(AnnotationString.data() + PreviousPos, i - PreviousPos);
            Str.erase(std::remove_if(Str.begin(), Str.end(), isspace), Str.end());
            Annotations.emplace_back(Str);
            PreviousPos = i + 1;
        }
    }
    std::string Str(AnnotationString.data() + PreviousPos, AnnotationString.size() - PreviousPos);
    Str.erase(std::remove_if(Str.begin(), Str.end(), isspace), Str.end());
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

std::string GetDeclFileAbsPath(clang::ASTContext* const Context, const Decl* D)
{
    SourceRange Loc = D->getSourceRange();
    std::filesystem::path File(Context->getSourceManager().getFilename(Loc.getBegin()).str());
    if (File.is_absolute())
        return File.string();
    else
        std::filesystem::canonical(std::filesystem::path(CCodeGenerator::Get().BuildPath + "/" + File.string())).string();
}

bool IsMatchedCppHeaderAndSource(const char* HeaderFile, uint32_t HeaderFileLength, const char* SourceFile, uint32_t SourceFileLength)
{
    uint32_t Len = std::min(HeaderFileLength, SourceFileLength);
    uint32_t CmpPos = UINT32_MAX;
    for (uint32_t i = 0; i < Len; i++)
    {
        if (HeaderFile[i] != SourceFile[i] &&
            !((HeaderFile[i] == '/' || HeaderFile[i] == '\\') && (SourceFile[i] == '/' || SourceFile[i] == '\\')))
        {
            CmpPos = i;
            break;
        }
    }
    bool IsCppHeaderSuffix = false;
    bool IsCppSourceSuffix = false;
    if ((HeaderFileLength - CmpPos == 1 && 0 == strncmp(&HeaderFile[CmpPos], "h", 1)) ||
        (HeaderFileLength - CmpPos == 3 && 0 == strncmp(&HeaderFile[CmpPos], "hpp", 3))) {
        IsCppHeaderSuffix = true;
    }
    if ((SourceFileLength - CmpPos == 1 && 0 == strncmp(&SourceFile[CmpPos], "c", 1)) ||
        (SourceFileLength - CmpPos == 3 && 0 == strncmp(&SourceFile[CmpPos], "cpp", 3))) {
        IsCppSourceSuffix = true;
    }
    return IsCppHeaderSuffix && IsCppSourceSuffix;
}