#include "ParsingClass.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/RecordLayout.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "Tool.h"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;

FClass* ParseReflectCXXRecord(CCodeGenerator& CodeGenerator, clang::ASTContext* const Context, const CXXRecordDecl* ClassCXXRecordDecl)
{
    std::vector<std::string> ReflectAnnotation;
    SourceManager& SM = Context->getSourceManager();
    FClass* Class = CodeGenerator.ClassTable.GetClass(ClassCXXRecordDecl->getQualifiedNameAsString().c_str());
    SourceLocation CppFileSourceLocation = ClassCXXRecordDecl->getLocation();
    SourceLocation TempSourceLocation = SM.getIncludeLoc(SM.getFileID(CppFileSourceLocation));
    while (TempSourceLocation.isValid())
    {
        CppFileSourceLocation = TempSourceLocation;
        TempSourceLocation = SM.getIncludeLoc(SM.getFileID(CppFileSourceLocation));
    }
    StringRef DeclHeaderFile = SM.getFileEntryForID(SM.getFileID(ClassCXXRecordDecl->getLocation()))->getName();
    StringRef CurrentSourceFile = SM.getFileEntryForID(SM.getFileID(CppFileSourceLocation))->getName();
    bool NeedReflectClass = IsMatchedCppHeaderAndSource(DeclHeaderFile.data(), DeclHeaderFile.size(), CurrentSourceFile.data(), CurrentSourceFile.size());
    if (Class)
    {
        if (NeedReflectClass) {
            if (Class->IsReflectionDataCollectionCompleted) 
                return Class;
        }
        else
        {
            return Class;
        }
    }
    else
    {
        if (!FindReflectAnnotation(ClassCXXRecordDecl, "Class", ReflectAnnotation)) return nullptr;
        if (ClassCXXRecordDecl->isClass() || ClassCXXRecordDecl->isStruct()) {
            CodeGenerator.GeneratedReflectClasses.emplace_back(std::make_unique<FClass>());
            CodeGenerator.GeneratedReflectClasses.back()->Name = ClassCXXRecordDecl->getQualifiedNameAsString();
            Class = CodeGenerator.GeneratedReflectClasses.back().get();
            CodeGenerator.ClassTable.RegisterClassToTable(Class->Name.c_str(), Class);
        }
        else if (ClassCXXRecordDecl->isUnion()) {
            SourceRange Loc = ClassCXXRecordDecl->getSourceRange();
            PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
            llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported this type <Union>", PLoc.getFilename(), PLoc.getLine(), ClassCXXRecordDecl->getQualifiedNameAsString());
            return nullptr;
        }
        else {
            SourceRange Loc = ClassCXXRecordDecl->getSourceRange();
            PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
            llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported this  unknown type<???>", PLoc.getFilename(), PLoc.getLine(), ClassCXXRecordDecl->getQualifiedNameAsString());
            return nullptr;
        }
        if (!NeedReflectClass) {
            return Class;
        }
    }
    Class->DeclaredFile = std::string(CurrentSourceFile.data(), CurrentSourceFile.size());
    // parend class parse
    for (auto BasesIterator = ClassCXXRecordDecl->bases_begin(); BasesIterator != ClassCXXRecordDecl->bases_end(); BasesIterator++)
    {
        FClass* ParentClass = ParseReflectCXXRecord(CodeGenerator, Context, BasesIterator->getType()->getAsCXXRecordDecl());
        if (!ParentClass)
        {
            CodeGenerator.OtherClasses.emplace_back(std::make_unique<FNonReflectClass>());
            ParentClass = CodeGenerator.GeneratedReflectClasses.back().get();
            ParentClass->Name = BasesIterator->getType()->getAsCXXRecordDecl()->getQualifiedNameAsString();
            CodeGenerator.ClassTable.RegisterClassToTable(ParentClass->Name.c_str(), ParentClass);
        }
        Class->ParentClasses.push_back(ParentClass);
    }

    // Function parse
    CXXConstructorDecl* UserDeclaredDefaultConstructor = nullptr;
    for (auto MethodIterator = ClassCXXRecordDecl->method_begin(); MethodIterator != ClassCXXRecordDecl->method_end(); MethodIterator++)
    {
        CXXMethodDecl* Method = *MethodIterator;
        CXXConstructorDecl* Constructor = dyn_cast<CXXConstructorDecl>(*MethodIterator);
        if (Constructor && Constructor->param_empty())
        {
            UserDeclaredDefaultConstructor = Constructor;
        }
        if (!FindReflectAnnotation(Method, "Function", ReflectAnnotation)) continue;
        Class->Functions.push_back(FFunction());
        FFunction& Function = Class->Functions.back();
        Function.Name = Method->getNameAsString();
        (void)Function.Ptr;
        Function.OwnerClass = Class;
        Function.Flag |= kMemberFlagBit;
        if (Method->isStatic())
        {
            Function.Flag |= kStaticFlagBit;
        }
        QualType ReturnType = Method->getReturnType();
        if (ReturnType->isVoidType()) {
            Function.Ret.Class = CodeGenerator.ClassTable.GetClass("void");
            Function.Ret.Flag = kQualifierNoFlag;
        }
        else {
            Function.Ret.Class;
            Function.Ret.Flag;
        }
        for (auto ParamIterator = Method->param_begin(); ParamIterator != Method->param_end(); ParamIterator++)
        {
            Function.Args.push_back(FParameter());
            FParameter& Parameter = Function.Args.back();
            Parameter.Name = (*ParamIterator)->getNameAsString();
            Parameter.Class;
            Parameter.Flag;
        }

    }

    // check constructor and destructor 
    if (UserDeclaredDefaultConstructor) {
        if (!UserDeclaredDefaultConstructor->isDeleted())
            Class->Flag |= kHasDefaultConstructorFlagBit;
    }
    else {
        if (ClassCXXRecordDecl->hasDefaultConstructor()) {
            Class->Flag |= kHasDefaultConstructorFlagBit;
        }
    }
    auto Destructor = ClassCXXRecordDecl->getDestructor();
    if (Destructor) {
        if (!Destructor->isDeleted())
            Class->Flag |= kHasDestructorFlagBit;
    }
    else {
        if (ClassCXXRecordDecl->hasSimpleDestructor())
            Class->Flag |= kHasDestructorFlagBit;
    }

    const ASTRecordLayout& RecordLayout = Context->getASTRecordLayout(ClassCXXRecordDecl);
    for (auto Field = ClassCXXRecordDecl->field_begin(); Field != ClassCXXRecordDecl->field_end(); Field++)
    {
        if (!FindReflectAnnotation(*Field, "Property", ReflectAnnotation)) continue;
        QualType FieldType = Field->getType();
        QualType FieldUnqualifiedType = Field->getType();
        Class->Fields.push_back(FField());
        Class->Fields.back().Name = Field->getName().str();
        Class->Fields.back().Offset = RecordLayout.getFieldOffset(Field->getFieldIndex());
        // if is array
        if (FieldType->isArrayType()) {
            TypeInfo FieldTypeTypeInfo = Context->getTypeInfo(FieldType.getTypePtr());
            QualType CheckedType = FieldType;
            QualType ArrayElementType;
            while (CheckedType->isArrayType())
            {
                const clang::ArrayType* ArrayFieldType = CheckedType->getAsArrayTypeUnsafe();
                ArrayElementType = ArrayFieldType->getElementType();
                CheckedType = ArrayElementType;
            }
            TypeInfo ArrayElementTypeInfo = Context->getTypeInfo(ArrayElementType);
            Class->Fields.back().Number = FieldTypeTypeInfo.Width / ArrayElementTypeInfo.Width;
            FieldUnqualifiedType = ArrayElementType;
        }
        else
        {
            FieldUnqualifiedType = FieldType;
        }
        // if is pointer type
        if (FieldUnqualifiedType->isPointerType())
        {
            Class->Fields.back().Flag |= kPointerFlagBit;
            if (FieldUnqualifiedType.isConstant(*Context))
                Class->Fields.back().Flag |= kConstPointerFlagBit;
            QualType PointeeType = FieldUnqualifiedType->getPointeeType();
            FieldUnqualifiedType = PointeeType;
        }
        // if is reference type
        if (FieldUnqualifiedType->isReferenceType())
        {
            Class->Fields.back().Flag |= kReferenceFlagBit;
            QualType PointeeType = FieldUnqualifiedType->getPointeeType();
            FieldUnqualifiedType = PointeeType;
        }
        // unsupported Multi-level Pointer
        if (FieldUnqualifiedType->isPointerType() || FieldUnqualifiedType->isReferenceType()) {
            SourceRange Loc = Field->getSourceRange();
            PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
            llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported Complex Type <Multi-level Pointer>\n", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
            return nullptr;
        }
        // unsupported The Pointer Pointer To Array
        if (FieldUnqualifiedType->isArrayType()) {
            SourceRange Loc = Field->getSourceRange();
            PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
            llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported Complex Type <The Pointer Pointer To Array>\n", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
            return nullptr;
        }
        if (FieldUnqualifiedType.isConstant(*Context)) {
            Class->Fields.back().Flag |= kConstValueFlagBit;
            FieldUnqualifiedType = FieldUnqualifiedType.getUnqualifiedType();
        }
        if (FieldUnqualifiedType->isBuiltinType())
        {
            Class->Fields.back().Class = CodeGenerator.ClassTable.GetClass(FieldUnqualifiedType.getCanonicalType().getAsString().c_str());
        }
        else if (FieldUnqualifiedType->isStructureOrClassType() || FieldUnqualifiedType->isEnumeralType())
        {
            FClass* ParsedClass = ParseReflectClass(CodeGenerator, Context, FieldUnqualifiedType->getAsCXXRecordDecl());
            if (ParsedClass && ParsedClass->IsReflectClass() && ParsedClass->IsReflectGeneratedClass()) {
                Class->Fields.back().Class = ParsedClass;
            }
            else {
                // if is ReserveObject
                auto it = std::find_if(ReflectAnnotation.begin(), ReflectAnnotation.end(), [](std::string& str) { return 0 == strncmp(str.c_str(), "ReflectClass=", sizeof("ReflectClass=") - 1); });
                if (std::end(ReflectAnnotation) != it) {
                    std::string ReflectObjectName = it->substr(sizeof("ReflectClass=") - 1);
                    FClass* UserWritedReflectClass = CodeGenerator.ClassTable.GetClass(ReflectObjectName.c_str());
                    if (nullptr == UserWritedReflectClass) {
                        CodeGenerator.OtherClasses.emplace_back(std::make_unique<FClass>());
                        UserWritedReflectClass = CodeGenerator.OtherClasses.back().get();
                        UserWritedReflectClass->Name = ReflectObjectName;
                        UserWritedReflectClass->Flag |= kUserWritedReflectClassFlagBit;
                        CodeGenerator.ClassTable.RegisterClassToTable(ReflectObjectName.c_str(), UserWritedReflectClass);
                    }
                    Class->Fields.back().Class = UserWritedReflectClass;
                }
                else
                {
                    std::string ForwardDeclaredClassName = FieldUnqualifiedType->getAsCXXRecordDecl()->getNameAsString().c_str();
                    if (!FieldUnqualifiedType->getAsCXXRecordDecl()->isThisDeclarationADefinition() && (Class->Fields.back().IsPointerType() || Class->Fields.back().IsReferenceType()))
                    {
                        FClass* ForwardDeclaredClass = CodeGenerator.ClassTable.GetClass(ForwardDeclaredClassName.c_str());
                        if (nullptr == ForwardDeclaredClass) {
                            CodeGenerator.OtherClasses.emplace_back(std::make_unique<FForwardDeclaredClass>());
                            ForwardDeclaredClass = CodeGenerator.OtherClasses.back().get();
                            ForwardDeclaredClass->Name = ForwardDeclaredClassName;
                            CodeGenerator.ClassTable.RegisterClassToTable(ForwardDeclaredClassName.c_str(), ForwardDeclaredClass);
                        }
                        Class->Fields.back().Class = ForwardDeclaredClass;
                    }
                    else
                    {
                        SourceRange Loc = Field->getSourceRange();
                        PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                        llvm::errs() << std::format("<{:s}:{:d}> property<{:s}> not is class\n", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
                        return nullptr;
                    }
                }
            }
        }
        else
        {
            SourceRange Loc = Field->getSourceRange();
            PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
            llvm::errs() << std::format("<{:s}:{:d}> <{:s}> unsupported type <Union>\n", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
            return nullptr;
        }
        assert(Class->Fields.back().Class != nullptr);
    }
    Class->IsReflectionDataCollectionCompleted = true;
    //llvm::outs() << Class->Dump();
    return Class;
}

FClass* ParseReflectEnum(CCodeGenerator& CodeGenerator, clang::ASTContext* const Context, const EnumDecl* ClassEnumDecl)
{
    std::vector<std::string> ReflectAnnotation;
    SourceManager& SM = Context->getSourceManager();
    FEnumClass* Class = (FEnumClass*)CodeGenerator.ClassTable.GetClass(ClassEnumDecl->getQualifiedNameAsString().c_str());

    SourceLocation CppFileSourceLocation = ClassEnumDecl->getLocation();
    SourceLocation TempSourceLocation = SM.getIncludeLoc(SM.getFileID(CppFileSourceLocation));
    while (TempSourceLocation.isValid())
    {
        CppFileSourceLocation = TempSourceLocation;
        TempSourceLocation = SM.getIncludeLoc(SM.getFileID(CppFileSourceLocation));
    }
    StringRef DeclHeaderFile = SM.getFileEntryForID(SM.getFileID(ClassEnumDecl->getLocation()))->getName();
    StringRef CurrentSourceFile = SM.getFileEntryForID(SM.getFileID(CppFileSourceLocation))->getName();
    bool NeedReflectClass = IsMatchedCppHeaderAndSource(DeclHeaderFile.data(), DeclHeaderFile.size(), CurrentSourceFile.data(), CurrentSourceFile.size());
    if (Class)
    {
        if (NeedReflectClass)
            if (Class->IsReflectionDataCollectionCompleted) return Class;
            else return Class;
    }
    else
    {
        if (!FindReflectAnnotation(ClassEnumDecl, "Enum", ReflectAnnotation)) return nullptr;
        assert(ClassEnumDecl->isEnum());
        CodeGenerator.GeneratedReflectClasses.emplace_back(std::make_unique<FEnumClass>());
        CodeGenerator.GeneratedReflectClasses.back()->Name = ClassEnumDecl->getQualifiedNameAsString();
        Class = (FEnumClass*)CodeGenerator.GeneratedReflectClasses.back().get();
        CodeGenerator.ClassTable.RegisterClassToTable(Class->Name.c_str(), Class);
        if (!NeedReflectClass) {
            return Class;
        }
    }
    Class->DeclaredFile = std::string(CurrentSourceFile.data(), CurrentSourceFile.size());
    TypeInfo FieldTypeTypeInfo = Context->getTypeInfo(ClassEnumDecl->getTypeForDecl());
    Class->Size = FieldTypeTypeInfo.Width;
    for (auto Iterator = ClassEnumDecl->enumerator_begin(); Iterator != ClassEnumDecl->enumerator_end(); Iterator++)
    {
        Class->OptName.push_back(Iterator->getNameAsString());
        Class->OptVal.push_back(Iterator->getInitVal().getZExtValue());
    }
    Class->IsReflectionDataCollectionCompleted = true;
    return Class;
}

FClass* ParseReflectClass(CCodeGenerator& CodeGenerator, clang::ASTContext* const Context, const TagDecl* ClassTagDecl)
{
    const CXXRecordDecl* ClassCXXRecordDecl = dyn_cast<CXXRecordDecl>(ClassTagDecl);
    if (ClassCXXRecordDecl) return ParseReflectCXXRecord(CodeGenerator, Context, ClassCXXRecordDecl);
    const EnumDecl* ClassEnumDecl = dyn_cast<EnumDecl>(ClassTagDecl);
    if (ClassEnumDecl) return ParseReflectEnum(CodeGenerator, Context, ClassEnumDecl);
    return nullptr;
}
