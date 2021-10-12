#include "ParsingClass.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/RecordLayout.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "Tool.h"

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
        Function.Flag |= kMemberFlagBit;
        if (Method->isStatic())
        {
            Function.Flag |= kStaticFlagBit;
        }
        //QualType ReturnType = Method->getReturnType();
        //if (ReturnType->isVoidType()) {
        //    Function.Ret.Class = CodeGenerator.ClassTable.GetClass("void");
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
            InClass->Flag |= kHasDefaultConstructorFlagBit;
    }
    else {
        if (InCXXRecordDecl->hasDefaultConstructor()) {
            InClass->Flag |= kHasDefaultConstructorFlagBit;
        }
    }
    auto Destructor = InCXXRecordDecl->getDestructor();
    if (Destructor) {
        if (!Destructor->isDeleted())
            InClass->Flag |= kHasDestructorFlagBit;
    }
    else {
        if (InCXXRecordDecl->hasSimpleDestructor())
            InClass->Flag |= kHasDestructorFlagBit;
    }
}





CField* ParseReflectCXXRecord(CCodeGenerator& CodeGenerator, clang::ASTContext* const Context, const CXXRecordDecl* ClassCXXRecordDecl)
{
    bool IsClass = false;
    std::vector<std::string> ReflectAnnotation;
    SourceManager& SM = Context->getSourceManager();
    CField* FieldClass = CodeGenerator.ClassTable.GetClass(ClassCXXRecordDecl->getQualifiedNameAsString().c_str());

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
    if (FieldClass)
    {
        if (NeedReflectClass) {
            if (FieldClass->IsReflectionDataCollectionCompleted)
                return FieldClass;
        }
        else
        {
            return FieldClass;
        }
    }
    else
    {
        if (!FindReflectAnnotation(ClassCXXRecordDecl, { "Class", "Struct" }, ReflectAnnotation)) return nullptr;
        if (ReflectAnnotation[0] == "Class" && ClassCXXRecordDecl->isClass()) CodeGenerator.GeneratedReflectClasses.emplace_back(std::make_unique<CClass>(ClassCXXRecordDecl->getQualifiedNameAsString().c_str()));
        else if (ReflectAnnotation[0] == "Struct" && ClassCXXRecordDecl->isStruct()) CodeGenerator.GeneratedReflectClasses.emplace_back(std::make_unique<CClass>(ClassCXXRecordDecl->getQualifiedNameAsString().c_str()));
        else {
            SourceRange Loc = ClassCXXRecordDecl->getSourceRange();
            PresumedLoc PLoc = Context->getSourceManager().getPresumedLoc(Loc.getBegin());
            llvm::errs() << std::format("<{:s}:{:d}> <{:s}> Unsupported this  unknown type<???>", PLoc.getFilename(), PLoc.getLine(), ClassCXXRecordDecl->getQualifiedNameAsString());
            return nullptr;
        }
        FieldClass = CodeGenerator.GeneratedReflectClasses.back().get();
        CodeGenerator.ClassTable.RegisterClassToTable(FieldClass->Name.c_str(), FieldClass);
        if (!NeedReflectClass) {
            return FieldClass;
        }
    }
    FieldClass->DeclaredFile = std::string(DeclHeaderFile.data(), DeclHeaderFile.size());

    CStructClass* StructClass = dynamic_cast<CStructClass*>(FieldClass);
    if (StructClass) {
        // parend class parse
        for (auto BasesIterator = ClassCXXRecordDecl->bases_begin(); BasesIterator != ClassCXXRecordDecl->bases_end(); BasesIterator++)
        {
            CField* ParentField = ParseReflectCXXRecord(CodeGenerator, Context, BasesIterator->getType()->getAsCXXRecordDecl());
            //if (!ParentClass)
            //{
            //    CodeGenerator.OtherClasses.emplace_back(std::make_unique<FNonReflectClass>());
            //    ParentClass = CodeGenerator.GeneratedReflectClasses.back().get();
            //    ParentClass->Name = BasesIterator->getType()->getAsCXXRecordDecl()->getQualifiedNameAsString();
            //    CodeGenerator.ClassTable.RegisterClassToTable(ParentClass->Name.c_str(), ParentClass);
            //}
            if(ParentField) {
                CStructClass* ParentStructClass = dynamic_cast<CStructClass*>(ParentField);
                if (ParentStructClass)
                    StructClass->ParentClasses.push_back(ParentStructClass);
            }
        }

        const ASTRecordLayout& RecordLayout = Context->getASTRecordLayout(ClassCXXRecordDecl);
        for (auto Field = ClassCXXRecordDecl->field_begin(); Field != ClassCXXRecordDecl->field_end(); Field++)
        {
            if (!FindReflectAnnotation(*Field, "Property", ReflectAnnotation)) continue;
            QualType FieldType = Field->getType();
            QualType FieldUnqualifiedType = Field->getType();
            EPropertyFlag PropertyFlag = CPF_NoneFlag;
            //Uint32 PropertyOffset = 0;
            Uint32 PropertyNumber = 1;
            //Class->Properties.push_back(CProperty(0x0, CPF_NoneFlag));
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
                PropertyFlag = EPropertyFlag(PropertyFlag | CPF_ArrayFlag);
            }
            else
            {
                FieldUnqualifiedType = FieldType;
            }
            // if is pointer type
            if (FieldUnqualifiedType->isPointerType())
            {
                PropertyFlag = EPropertyFlag(PropertyFlag | CPF_PointerFlag);
                if (FieldUnqualifiedType.isConstant(*Context))
                    PropertyFlag = EPropertyFlag(PropertyFlag | CPF_ConstPointerceFlag);
                QualType PointeeType = FieldUnqualifiedType->getPointeeType();
                FieldUnqualifiedType = PointeeType;
            }
            // if is reference type
            if (FieldUnqualifiedType->isReferenceType())
            {
                PropertyFlag = EPropertyFlag(PropertyFlag | CPF_ReferenceFlag);
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
                PropertyFlag = EPropertyFlag(PropertyFlag | CPF_ConstValueFlag);
                FieldUnqualifiedType = FieldUnqualifiedType.getUnqualifiedType();
            }
            if (FieldUnqualifiedType->isBuiltinType())
            {
                TypeInfo FieldTypeTypeInfo = Context->getTypeInfo(FieldUnqualifiedType.getTypePtr());
                if (FieldUnqualifiedType->isSignedIntegerType()) {
                    if (FieldTypeTypeInfo.Width == 1) StructClass->Properties.push_back(std::make_unique<CInt8Property>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width == 2) StructClass->Properties.push_back(std::make_unique<CInt16Property>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width == 4) StructClass->Properties.push_back(std::make_unique<CInt32Property>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width == 8) StructClass->Properties.push_back(std::make_unique<CInt64Property>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                }
                else if (FieldUnqualifiedType->isUnsignedIntegerType()) {
                    if (FieldTypeTypeInfo.Width == 1) StructClass->Properties.push_back(std::make_unique<CUint8Property>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width == 2) StructClass->Properties.push_back(std::make_unique<CUint16Property>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width == 4) StructClass->Properties.push_back(std::make_unique<CUint32Property>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width == 8) StructClass->Properties.push_back(std::make_unique<CUint64Property>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                }
                else if (FieldUnqualifiedType->isFloatingType()) {
                    if (FieldTypeTypeInfo.Width == 4) StructClass->Properties.push_back(std::make_unique<CFloatProperty>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                    else if (FieldTypeTypeInfo.Width == 8) StructClass->Properties.push_back(std::make_unique<CDoubleProperty>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                }
                else if (FieldUnqualifiedType->isBooleanType()) {
                    StructClass->Properties.push_back(std::make_unique<CDoubleProperty>(Field->getName().data(), 0, PropertyFlag, PropertyNumber));
                }
                else{
                    assert(!"???");
                }
            }
            else if (FieldUnqualifiedType->isStructureOrClassType())
            {
                CStructClass* ParsedStructClass = (CStructClass*)ParseReflectClass(CodeGenerator, Context, FieldUnqualifiedType->getAsCXXRecordDecl());
                if (ParsedStructClass) {
                    CClass* ParsedClass = dynamic_cast<CClass*>(ParsedStructClass);
                    if (ParsedClass) StructClass->Properties.push_back(std::make_unique<CClassProperty>(Field->getName().data(), ParsedClass, 0, PropertyFlag, PropertyNumber));
                    else StructClass->Properties.push_back(std::make_unique<CStructProperty>(Field->getName().data(), ParsedStructClass, 0, PropertyFlag, PropertyNumber));
                }
                else
                {
                    std::string ForwardDeclaredStructName = FieldUnqualifiedType->getAsCXXRecordDecl()->getNameAsString().c_str();
                    if (!FieldUnqualifiedType->getAsCXXRecordDecl()->isThisDeclarationADefinition() && (PropertyFlag & (CPF_PointerFlag | CPF_ReferenceFlag)))
                    {
                        CStructClass* ForwardDeclaredStructClass = (CStructClass*)CodeGenerator.ClassTable.GetClass(ForwardDeclaredStructName.c_str());
                        if (!ForwardDeclaredStructClass) {
                            if (FieldUnqualifiedType->isStructureType()) CodeGenerator.OtherClasses.emplace_back(std::make_unique<CStructClass>(ForwardDeclaredStructName.c_str()));
                            else if (FieldUnqualifiedType->isClassType()) CodeGenerator.OtherClasses.emplace_back(std::make_unique<CClass>(ForwardDeclaredStructName.c_str()));
                            else assert(!"???");
                            ForwardDeclaredStructClass = (CStructClass*)CodeGenerator.OtherClasses.back().get();
                            ForwardDeclaredStructClass->IsForwardDeclared = true;
                            CodeGenerator.ClassTable.RegisterClassToTable(ForwardDeclaredStructName.c_str(), ForwardDeclaredStructClass);
                        }
                        CClass* ForwardDeclaredClass = dynamic_cast<CClass*>(ForwardDeclaredStructClass);
                        if (ForwardDeclaredClass) StructClass->Properties.push_back(std::make_unique<CClassProperty>(Field->getName().data(), ForwardDeclaredClass, 0, PropertyFlag, PropertyNumber));
                        else StructClass->Properties.push_back(std::make_unique<CStructProperty>(Field->getName().data(), ForwardDeclaredStructClass, 0, PropertyFlag, PropertyNumber));
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
            else if (FieldUnqualifiedType->isEnumeralType())
            {
                CEnumClass* ParsedEnumClass = dynamic_cast<CEnumClass*>(ParseReflectClass(CodeGenerator, Context, FieldUnqualifiedType->getAsCXXRecordDecl()));
                if (ParsedEnumClass) {
                    StructClass->Properties.push_back(std::make_unique<CEnumProperty>(Field->getName().data(), ParsedEnumClass, 0, PropertyFlag, PropertyNumber));
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
            assert(Class->Properties.back().Class != nullptr);
        }
        // Function parse
        CClass* Class = dynamic_cast<CClass*>(StructClass);
        if (Class) 
            ParseMethod(ClassCXXRecordDecl, Class);
    }
    FieldClass->IsReflectionDataCollectionCompleted = true;
    //llvm::outs() << Class->Dump();
    return FieldClass;
}

CField* ParseReflectEnum(CCodeGenerator& CodeGenerator, clang::ASTContext* const Context, const EnumDecl* ClassEnumDecl)
{
    std::vector<std::string> ReflectAnnotation;
    SourceManager& SM = Context->getSourceManager();
    CField* FieldClass = CodeGenerator.ClassTable.GetClass(ClassEnumDecl->getQualifiedNameAsString().c_str());

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
    if (FieldClass)
    {
        if (NeedReflectClass)
            if (FieldClass->IsReflectionDataCollectionCompleted) return FieldClass;
            else return FieldClass;
    }
    else
    {
        if (!FindReflectAnnotation(ClassEnumDecl, "Enum", ReflectAnnotation)) return nullptr;
        assert(ClassEnumDecl->isEnum());
        CodeGenerator.GeneratedReflectClasses.emplace_back(std::make_unique<CEnumClass>(ClassEnumDecl->getQualifiedNameAsString().c_str()));
        FieldClass = CodeGenerator.GeneratedReflectClasses.back().get();
        CodeGenerator.ClassTable.RegisterClassToTable(FieldClass->Name.c_str(), FieldClass);
        if (!NeedReflectClass) {
            return FieldClass;
        }
    }
    FieldClass->DeclaredFile = std::string(DeclHeaderFile.data(), DeclHeaderFile.size());
    CEnumClass* EnumClass = dynamic_cast<CEnumClass*>(FieldClass);
    if (EnumClass) {
        TypeInfo FieldTypeTypeInfo = Context->getTypeInfo(ClassEnumDecl->getTypeForDecl());
        EnumClass->Size = FieldTypeTypeInfo.Width;
        for (auto Iterator = ClassEnumDecl->enumerator_begin(); Iterator != ClassEnumDecl->enumerator_end(); Iterator++)
        {
            EnumClass->Options.push_back(std::make_pair<>(Iterator->getNameAsString(), Iterator->getInitVal().getZExtValue()));
        }
    }
    EnumClass->IsReflectionDataCollectionCompleted = true;
    return FieldClass;
}

CField* ParseReflectClass(CCodeGenerator& CodeGenerator, clang::ASTContext* const Context, const TagDecl* ClassTagDecl)
{
    const CXXRecordDecl* ClassCXXRecordDecl = dyn_cast<CXXRecordDecl>(ClassTagDecl);
    if (ClassCXXRecordDecl) return ParseReflectCXXRecord(CodeGenerator, Context, ClassCXXRecordDecl);
    const EnumDecl* ClassEnumDecl = dyn_cast<EnumDecl>(ClassTagDecl);
    if (ClassEnumDecl) return ParseReflectEnum(CodeGenerator, Context, ClassEnumDecl);
    return nullptr;
}
