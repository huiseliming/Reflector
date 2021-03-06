#include "ParsingClass.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/RecordLayout.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "Tool.h"
#include <filesystem>

using namespace llvm;
using namespace clang;
using namespace clang::tooling;




void ParseMethod(const CXXRecordDecl* InCXXRecordDecl, CClass* InClass)
{
    std::vector<std::string> ReflectAnnotation;
    CXXConstructorDecl* UserDeclaredDefaultConstructor = nullptr;
    for (auto MethodIterator = InCXXRecordDecl->method_begin(); MethodIterator != InCXXRecordDecl->method_end(); MethodIterator++)
    {
        CXXMethodDecl* Method = *MethodIterator;
        CXXConstructorDecl* Constructor = dyn_cast<CXXConstructorDecl>(*MethodIterator);
        if (Constructor && Constructor->param_empty())
        {
            UserDeclaredDefaultConstructor = Constructor;
        }
        if (!FindReflectAnnotation(Method, "Function", ReflectAnnotation)) continue;
        InClass->Functions.push_back(FFunction());
        FFunction& Function = InClass->Functions.back();
        Function.Name = Method->getNameAsString();
        (void)Function.Ptr;
        Function.OwnerClass = InClass;
        Function.Flag |= EFF_MemberFlag;
        if (Method->isStatic())
        {
            Function.Flag |= EFF_StaticFlag;
        }
        //QualType ReturnType = Method->getReturnType();
        //if (ReturnType->isVoidType()) {
        //    Function.Ret.Class = CodeGenerator.MetaTable.GetMeta("void");
        //    Function.Ret.Flag = kQualifierNoFlag;
        //}
        //else {
        //    Function.Ret.Class;
        //    Function.Ret.Flag;
        //}
        //for (auto ParamIterator = Method->param_begin(); ParamIterator != Method->param_end(); ParamIterator++)
        //{
        //    Function.Args.push_back(FParameter());
        //    FParameter& Parameter = Function.Args.back();
        //    Parameter.Name = (*ParamIterator)->getNameAsString();
        //    Parameter.Class;
        //    Parameter.Flag;
        //}
    }
    // check constructor and destructor 
    if (UserDeclaredDefaultConstructor) {
        if (!UserDeclaredDefaultConstructor->isDeleted())
            InClass->AddFlag(ECF_DefaultConstructorExist);
    }
    else {
        if (InCXXRecordDecl->hasDefaultConstructor()) {
            InClass->AddFlag(ECF_DefaultConstructorExist);
        }
    }
    auto Destructor = InCXXRecordDecl->getDestructor();
    if (Destructor) {
        if (!Destructor->isDeleted())
            InClass->AddFlag(ECF_DefaultDestructorExist);
    }
    else {
        if (InCXXRecordDecl->hasSimpleDestructor())
            InClass->AddFlag(ECF_DefaultDestructorExist);
    }
}





CMeta* ParseReflectCXXRecord(CCodeGenerator& CodeGenerator, clang::ASTContext* const Context, const CXXRecordDecl* InCXXRecordDecl)
{
    std::vector<std::string> ReflectAnnotation;
    SourceManager& SM = Context->getSourceManager();
    CMeta* Meta = CodeGenerator.MetaTable.GetMeta(InCXXRecordDecl->getQualifiedNameAsString().c_str());

    SourceLocation CppFileSourceLocation = InCXXRecordDecl->getLocation();
    SourceLocation TempSourceLocation = SM.getIncludeLoc(SM.getFileID(CppFileSourceLocation));
    while (TempSourceLocation.isValid())
    {
        CppFileSourceLocation = TempSourceLocation;
        TempSourceLocation = SM.getIncludeLoc(SM.getFileID(CppFileSourceLocation));
    }
    std::string DeclHeaderFile = std::filesystem::canonical(std::filesystem::path(SM.getFileEntryForID(SM.getFileID(InCXXRecordDecl->getLocation()))->getName().str())).string();
    std::string CurrentSourceFile = std::filesystem::canonical(std::filesystem::path(SM.getFileEntryForID(SM.getFileID(CppFileSourceLocation))->getName().str())).string();
    bool NeedReflectMeta = IsMatchedCppHeaderAndSource(DeclHeaderFile ,CurrentSourceFile);
    if (Meta)
    {
        if (NeedReflectMeta) {
            if (Meta->IsReflectionDataCollectionCompleted)
                return Meta;
        }
        else
        {
            return Meta;
        }
    }
    else
    {
        if (!FindReflectAnnotation(InCXXRecordDecl, { "Class", "Struct" }, ReflectAnnotation)) return nullptr;
        if (ReflectAnnotation[0] == "Class" && InCXXRecordDecl->isClass()) CodeGenerator.GeneratedReflectMetas.emplace_back(std::make_unique<CClass>(InCXXRecordDecl->getQualifiedNameAsString().c_str()));
        else if (ReflectAnnotation[0] == "Struct" && InCXXRecordDecl->isStruct()) CodeGenerator.GeneratedReflectMetas.emplace_back(std::make_unique<CClass>(InCXXRecordDecl->getQualifiedNameAsString().c_str()));
        else {
            SourceRange Loc = InCXXRecordDecl->getSourceRange();
            PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
            llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported this  unknown type<???>\n", PLoc.getFilename(), PLoc.getLine(), InCXXRecordDecl->getQualifiedNameAsString());
            return nullptr;
        }
        Meta = CodeGenerator.GeneratedReflectMetas.back().get();
        CodeGenerator.MetaTable.RegisterMetaToTable(Meta);
        if (!NeedReflectMeta) {
            return Meta;
        }
    }
    Meta->DeclaredFile = std::string(DeclHeaderFile.data(), DeclHeaderFile.size());
    ParsingMetaString(Meta, ReflectAnnotation);
    CStruct* Struct = dyn_cast<CStruct>(Meta);
    if (Struct) {
        // parend class parse
        for (auto BasesIterator = InCXXRecordDecl->bases_begin(); BasesIterator != InCXXRecordDecl->bases_end(); BasesIterator++)
        {
            CMeta* ParentMeta = ParseReflectCXXRecord(CodeGenerator, Context, BasesIterator->getType()->getAsCXXRecordDecl());
            if (BasesIterator == InCXXRecordDecl->bases_begin()) 
            {
                if (ParentMeta) {
                    CStruct* ParentStruct = dyn_cast<CStruct>(ParentMeta);
                    if (ParentStruct)
                        Struct->Parent = ParentStruct;
                }
            }
            else
            {
                // must be interface 
                if (BasesIterator->getType()->getAsCXXRecordDecl()->hasDirectFields())
                {
                    SourceRange Loc = InCXXRecordDecl->getSourceRange();
                    PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                    llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Inherited interface<> has field\n", PLoc.getFilename(), PLoc.getLine(), BasesIterator->getType()->getAsCXXRecordDecl()->getQualifiedNameAsString());
                    return nullptr;
                }
                if (ParentMeta) {
                    CInterface* ParentStruct = dyn_cast<CInterface>(ParentMeta);
                    if (ParentStruct)
                        Struct->Interfaces.push_back(ParentStruct);
                }
            }
            //if (!ParentClass)
            //{
            //    CodeGenerator.OtherMetas.emplace_back(std::make_unique<FNonReflectClass>());
            //    ParentClass = CodeGenerator.GeneratedReflectMetas.back().get();
            //    ParentClass->Name = BasesIterator->getType()->getAsCXXRecordDecl()->getQualifiedNameAsString();
            //    CodeGenerator.MetaTable.RegisterMetaToTable(ParentClass->Name.c_str(), ParentClass);
            //}
        }

        //const ASTRecordLayout& RecordLayout = Context->getASTRecordLayout(ClassCXXRecordDecl);
        for (auto Field = InCXXRecordDecl->field_begin(); Field != InCXXRecordDecl->field_end(); Field++)
        {
            if (!FindReflectAnnotation(*Field, "Property", ReflectAnnotation)) continue;
            QualType FieldType = Field->getType();
            QualType FieldUnqualifiedType = Field->getType();
            EPropertyFlag PropertyFlag = EPF_NoneFlag;
            //Uint32 PropertyOffset = 0;
            Uint32 PropertyNumber = 1;
            //Class->Properties.push_back(CProperty(0x0, EPF_NoneFlag));
            //Class->Properties.back().Name = Field->getName().str();
            //PropertyOffset = RecordLayout.getFieldOffset(Field->getFieldIndex());
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
                PropertyNumber = FieldTypeTypeInfo.Width / ArrayElementTypeInfo.Width;
                FieldUnqualifiedType = ArrayElementType;
                PropertyFlag = EPropertyFlag(PropertyFlag | EPF_ArrayFlag);
            }
            else
            {
                FieldUnqualifiedType = FieldType;
            }
            // if is pointer type
            if (FieldUnqualifiedType->isPointerType())
            {
                PropertyFlag = EPropertyFlag(PropertyFlag | EPF_PointerFlag);
                if (FieldUnqualifiedType.isConstant(*Context))
                    PropertyFlag = EPropertyFlag(PropertyFlag | EPF_ConstPointerceFlag);
                QualType PointeeType = FieldUnqualifiedType->getPointeeType();
                FieldUnqualifiedType = PointeeType;
            }
            // if is reference type
            if (FieldUnqualifiedType->isReferenceType())
            {
                PropertyFlag = EPropertyFlag(PropertyFlag | EPF_ReferenceFlag);
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
                PropertyFlag = EPropertyFlag(PropertyFlag | EPF_ConstValueFlag);
                FieldUnqualifiedType = FieldUnqualifiedType.getUnqualifiedType();
            }
            if (FieldUnqualifiedType->isBuiltinType())
            {
                TypeInfo FieldTypeTypeInfo = Context->getTypeInfo(FieldUnqualifiedType.getTypePtr());
                if (FieldUnqualifiedType->isSignedIntegerType()) {
                    if      (FieldTypeTypeInfo.Width / 8 == 1) Struct->Properties.push_back(std::make_unique<CInt8Property> (Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width / 8 == 2) Struct->Properties.push_back(std::make_unique<CInt16Property>(Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width / 8 == 4) Struct->Properties.push_back(std::make_unique<CInt32Property>(Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width / 8 == 8) Struct->Properties.push_back(std::make_unique<CInt64Property>(Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                }
                else if (FieldUnqualifiedType->isUnsignedIntegerType()) {
                    if      (FieldTypeTypeInfo.Width / 8 == 1) Struct->Properties.push_back(std::make_unique<CUint8Property> (Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width / 8 == 2) Struct->Properties.push_back(std::make_unique<CUint16Property>(Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width / 8 == 4) Struct->Properties.push_back(std::make_unique<CUint32Property>(Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width / 8 == 8) Struct->Properties.push_back(std::make_unique<CUint64Property>(Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                }
                else if (FieldUnqualifiedType->isFloatingType()) {
                    if      (FieldTypeTypeInfo.Width / 8 == 4) Struct->Properties.push_back(std::make_unique<CFloatProperty>(Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width / 8 == 8) Struct->Properties.push_back(std::make_unique<CDoubleProperty>(Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                }
                else if (FieldUnqualifiedType->isBooleanType()) {
                    Struct->Properties.push_back(std::make_unique<CDoubleProperty>(Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                }
                else{
                    assert(!"???");
                }
            }
            else if (FieldUnqualifiedType->isStructureOrClassType())
            {
                CStruct* ParsedStruct = (CStruct*)ParseReflectClass(CodeGenerator, Context, FieldUnqualifiedType->getAsCXXRecordDecl());
                if (ParsedStruct) {
                    CClass* ParsedClass = dyn_cast<CClass>(ParsedStruct);
                    if (ParsedClass) Struct->Properties.push_back(std::make_unique<CClassProperty>(Field->getName().data(), ParsedClass, PropertyFlag, 0, PropertyNumber));
                    else Struct->Properties.push_back(std::make_unique<CStructProperty>(Field->getName().data(), ParsedStruct, PropertyFlag, 0, PropertyNumber));
                }
                else
                {
                    if (FieldUnqualifiedType.getAsString() == "std::string") {
                        Struct->Properties.push_back(std::make_unique<CStringProperty>(Field->getName().data(), PropertyFlag, 0, PropertyNumber));
                    }
                    else
                    {
                        std::string ForwardDeclaredStructName = FieldUnqualifiedType->getAsCXXRecordDecl()->getNameAsString().c_str();
                        if (!FieldUnqualifiedType->getAsCXXRecordDecl()->isThisDeclarationADefinition() && (PropertyFlag & (EPF_PointerFlag | EPF_ReferenceFlag)))
                        {
                            CStruct* ForwardDeclaredStruct = (CStruct*)CodeGenerator.MetaTable.GetMeta(ForwardDeclaredStructName.c_str());
                            if (!ForwardDeclaredStruct) {
                                if (FieldUnqualifiedType->isStructureType()) CodeGenerator.OtherMetas.emplace_back(std::make_unique<CStruct>(ForwardDeclaredStructName.c_str()));
                                else if (FieldUnqualifiedType->isClassType()) CodeGenerator.OtherMetas.emplace_back(std::make_unique<CClass>(ForwardDeclaredStructName.c_str()));
                                else assert(!"???");
                                ForwardDeclaredStruct = (CStruct*)CodeGenerator.OtherMetas.back().get();
                                ForwardDeclaredStruct->IsForwardDeclared = true;
                                CodeGenerator.MetaTable.RegisterMetaToTable(ForwardDeclaredStruct);
                            }
                            CClass* ForwardDeclaredClass = dyn_cast<CClass>(ForwardDeclaredStruct);
                            if (ForwardDeclaredClass) Struct->Properties.push_back(std::make_unique<CClassProperty>(Field->getName().data(), ForwardDeclaredClass, PropertyFlag, 0, PropertyNumber));
                            else Struct->Properties.push_back(std::make_unique<CStructProperty>(Field->getName().data(), ForwardDeclaredStruct, PropertyFlag, 0, PropertyNumber));
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
            else if (FieldUnqualifiedType->isEnumeralType())
            {
                CEnumClass* ParsedEnumClass = dyn_cast<CEnumClass>(ParseReflectClass(CodeGenerator, Context, FieldUnqualifiedType->getAsCXXRecordDecl()));
                if (ParsedEnumClass) {
                    Struct->Properties.push_back(std::make_unique<CEnumProperty>(Field->getName().data(), ParsedEnumClass, PropertyFlag, 0, PropertyNumber));
                }
                else
                {
                    SourceRange Loc = Field->getSourceRange();
                    PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                    llvm::errs() << std::format("<{:s}:{:d}> property<{:s}> not is enum\n", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
                }
            }
            else
            {
                SourceRange Loc = Field->getSourceRange();
                PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
                llvm::errs() << std::format("<{:s}:{:d}> <{:s}> unsupported type <???>\n", PLoc.getFilename(), PLoc.getLine(), Field->getType().getAsString());
                return nullptr;
            }
            ParsingMetaString(Struct->Properties.back().get(), ReflectAnnotation);
        }
        // Function parse
        CClass* Class = dyn_cast<CClass>(Struct);
        if (Class) 
            ParseMethod(InCXXRecordDecl, Class);
    }
    Meta->IsReflectionDataCollectionCompleted = true;
    //llvm::outs() << Class->Dump();
    return Meta;
}

CMeta* ParseReflectEnum(CCodeGenerator& CodeGenerator, clang::ASTContext* const Context, const EnumDecl* InEnumDecl)
{
    std::vector<std::string> ReflectAnnotation;
    SourceManager& SM = Context->getSourceManager();
    CMeta* Meta = CodeGenerator.MetaTable.GetMeta(InEnumDecl->getQualifiedNameAsString().c_str());

    SourceLocation CppFileSourceLocation = InEnumDecl->getLocation();
    SourceLocation TempSourceLocation = SM.getIncludeLoc(SM.getFileID(CppFileSourceLocation));
    while (TempSourceLocation.isValid())
    {
        CppFileSourceLocation = TempSourceLocation;
        TempSourceLocation = SM.getIncludeLoc(SM.getFileID(CppFileSourceLocation));
    }
    std::string DeclHeaderFile = std::filesystem::canonical(std::filesystem::path(SM.getFileEntryForID(SM.getFileID(InEnumDecl->getLocation()))->getName().str())).string();
    std::string CurrentSourceFile = std::filesystem::canonical(std::filesystem::path(SM.getFileEntryForID(SM.getFileID(CppFileSourceLocation))->getName().str())).string();
    bool NeedReflectMeta = IsMatchedCppHeaderAndSource(DeclHeaderFile, CurrentSourceFile);
    if (Meta)
    {
        if (NeedReflectMeta)
            if (Meta->IsReflectionDataCollectionCompleted) return Meta;
            else return Meta;
    }
    else
    {
        if (!FindReflectAnnotation(InEnumDecl, "Enum", ReflectAnnotation)) return nullptr;
        assert(InEnumDecl->isEnum());
        CodeGenerator.GeneratedReflectMetas.emplace_back(std::make_unique<CEnumClass>(InEnumDecl->getQualifiedNameAsString().c_str()));
        Meta = CodeGenerator.GeneratedReflectMetas.back().get();
        CodeGenerator.MetaTable.RegisterMetaToTable(Meta);
        if (!NeedReflectMeta) {
            return Meta;
        }
    }
    Meta->DeclaredFile = std::string(DeclHeaderFile.data(), DeclHeaderFile.size());
    ParsingMetaString(Meta, ReflectAnnotation);
    CEnumClass* EnumClass = dyn_cast<CEnumClass>(Meta);
    if (EnumClass) {
        TypeInfo EnumTypeInfo = Context->getTypeInfo(InEnumDecl->getTypeForDecl());
        EnumClass->Size = EnumTypeInfo.Width;
        for (auto Iterator = InEnumDecl->enumerator_begin(); Iterator != InEnumDecl->enumerator_end(); Iterator++)
        {
            EnumClass->Options.push_back(std::make_pair<>(Iterator->getNameAsString(), Iterator->getInitVal().getZExtValue()));
        }
    }
    EnumClass->IsReflectionDataCollectionCompleted = true;
    return Meta;
}

CMeta* ParseReflectClass(CCodeGenerator& CodeGenerator, clang::ASTContext* const Context, const TagDecl* InTagDecl)
{
    const CXXRecordDecl* CastCXXRecordDecl = dyn_cast<CXXRecordDecl>(InTagDecl);
    if (CastCXXRecordDecl) return ParseReflectCXXRecord(CodeGenerator, Context, CastCXXRecordDecl);
    const EnumDecl* CastEnumDecl = dyn_cast<EnumDecl>(InTagDecl);
    if (CastEnumDecl) return ParseReflectEnum(CodeGenerator, Context, CastEnumDecl);
    return nullptr;
}
