//#pragma once
//#include <list>
//#include <vector>
//#include <memory>
//#include <atomic>
//#include <cassert>
//#include <functional>
//#include <unordered_map>
//
//
//#ifdef __REFLECTOR__
//#define REFLECT_OBJECT(...) __attribute__((annotate("Object"   __VA_OPT__(",") #__VA_ARGS__)))
//#define PROPERTY(...)       __attribute__((annotate("Property" __VA_OPT__(",") #__VA_ARGS__)))
//#define FUNCTION(...)       __attribute__((annotate("Function" __VA_OPT__(",") #__VA_ARGS__)))
//#else
//#define REFLECT_OBJECT(...)
//#define PROPERTY(...)
//#define FUNCTION(...)
//#endif
//
//#define REFLECT_GENERATED_BODY() \
//public: \
//static FTypeDescriptor* GetTypeDescriptor();\
//static Uint32 GetTypeId();
//
//typedef void               Void;
//typedef bool               Bool;
//typedef char               Int8;
//typedef unsigned char      Uint8;
//typedef short              Int16;
//typedef unsigned short     Uint16;
//typedef int                Int32;
//typedef unsigned int       Uint32;
//typedef long long          Int64;
//typedef unsigned long long Uint64;
//typedef float              Float;
//typedef double             Double;
//
//#ifdef REFLECT_CODE_GENERATOR
//#define STRING_TYPE std::string
//#else
//#define STRING_TYPE const char *
//#endif // REFLECT_CODE_GENERATOR
//
//struct FTypeDescriptor;
//
//enum EQualifierFlag :Uint32
//{
//	kQualifierNoFlag = 0,
//	kPointerFlagBit = 1 << 0,
//	kReferenceFlagBit = 1 << 1,
//	kConstValueFlagBit = 1 << 2,
//	kConstPointerFlagBit = 1 << 3,
//};
//
//enum ETypeFlag :Uint32
//{
//	kTypeNoFlag = 0,
//	kHasDefaultConstructorFlagBit = 1 << 0,
//	kHasDestructorFlagBit = 1 << 1,
//};
//
//enum EFunctionFlag :Uint32
//{
//	kFunctionNoFlag = 0,
//	kMemberFlagBit,
//	kStaticFlagBit,
//};
//
//struct FDecl 
//{
//	FTypeDescriptor* TypeDescriptor{ nullptr };
//	Uint32 QualifierFlag{ kQualifierNoFlag }; // EQualifierFlag
//	bool IsNoQualifierType() { return QualifierFlag == kQualifierNoFlag; }
//	bool IsPointerType() { return QualifierFlag & kPointerFlagBit; }
//	bool IsReferenceType() { return QualifierFlag & kReferenceFlagBit; }
//	bool IsConstValueType() { return QualifierFlag & kConstValueFlagBit; }
//	bool IsConstPointerType() { return QualifierFlag & kConstPointerFlagBit; }
//};
//
//struct FField : public FDecl
//{
//	STRING_TYPE FieldName{""};
//	size_t FieldOffset{0};
//	size_t Number{ 1 };
//
//	template<typename T>
//	T& GetRef(void* OwnerBaseAddress) { return *reinterpret_cast<T*>(((Uint8*)OwnerBaseAddress) + FieldOffset); }
//	template<typename T>
//	T* GetPtr(void* OwnerBaseAddress) { return reinterpret_cast<T*>(((Uint8*)OwnerBaseAddress) + FieldOffset); }
//	template<typename T>
//	const T& GetCRef(void* OwnerBaseAddress) { return GetRef<T>(OwnerBaseAddress); }
//	template<typename T>
//	const T* GetCPtr(void* OwnerBaseAddress) { return GetPtr<T>(OwnerBaseAddress); }
//};
//
//struct FParameter : public FDecl {
//	STRING_TYPE ParameterName{ "" };
//};
//
//struct FReturnValue : public FDecl {
//
//};
//
//struct FFunction {
//	STRING_TYPE FunctionName{""};
//	void *Ptr{nullptr};
//	FTypeDescriptor* OwnerDescriptor{nullptr}; //  Owner {Class | Struct} TypeDescriptor
//	FReturnValue Ret;
//	std::vector<FParameter> Args;
//	Uint32 FunctionFlag{ kFunctionNoFlag }; //EFunctionFlag
//	bool IsStaticMemberFunction() { return FunctionFlag & (kMemberFlagBit | kStaticFlagBit); }
//	bool IsMemberFunction() { return FunctionFlag & kMemberFlagBit; }
//	bool IsStaticFunction() { return FunctionFlag & kStaticFlagBit; }
//};
//
//struct FTypeDescriptor 
//{
//	FTypeDescriptor(const char* InTypeName, size_t InTypeSize)
//		: TypeName{InTypeName}
//		, TypeSize{InTypeSize}
//	{
//	}
//	virtual ~FTypeDescriptor() {}
//
//	// BuiltInTypeBegin
//	bool IsVoidType() { return TypeId == 0; }
//	bool IsBoolType() { return TypeId == 1; }
//	bool IsInt8Type() { return TypeId == 2; }
//	bool IsUint8Type() { return TypeId == 3; }
//	bool IsInt16Type() { return TypeId == 4; }
//	bool IsUint16Type() { return TypeId == 5; }
//	bool IsInt32Type() { return TypeId == 6; }
//	bool IsUint32Type() { return TypeId == 7; }
//	bool IsInt64Type() { return TypeId == 8; }
//	bool IsUint64Type() { return TypeId == 9; }
//	bool IsFloatType() { return TypeId == 10; }
//	bool IsDoubleType() { return TypeId == 11; }
//	// BuiltInTypeBegin
//
//	virtual bool IsUnsignedType() { return false; }
//	virtual bool IsBuiltInType() { return false; }
//	virtual bool IsClass() { return false; }
//	virtual bool IsStruct() { return false; }
//	virtual bool IsEnum() { return false; }
//	virtual const char* GetTypeKind() = 0;
//
//	bool HasDefaultConstructor() { return TypeFlag & kHasDefaultConstructorFlagBit; }
//	//bool HasCopyConstructor() { return false; }
//	//bool HasMoveConstructor() { return false; }
//	bool HasDestructor() { return TypeFlag & kHasDestructorFlagBit; }
//
//	virtual void* New() { return nullptr; }
//	virtual void Delete(void* Object) { }
//	virtual void Constructor(void* ConstructedObject) { }
//	virtual void CopyConstructor(void* ConstructedObject, void* CopyedObject) { }
//	virtual void MoveConstructor(void* ConstructedObject, void* MoveedObject) { }
//	virtual void Destructor(void* DestructedObject) { }
//	virtual void CopyAssignmentOperator(void* LObject, void* RObject) {}
//	virtual void MoveAssignmentOperator(void* LObject, void* RObject) {}
//
//	const char * GetTypeName() 
//	{ 
//#ifdef REFLECT_CODE_GENERATOR
//		return TypeName[0].c_str();
//#else
//		return TypeName[0];
//#endif // REFLECT_CODE_GENERATOR
//	}
//
//	bool HasTypeName(const char* InTypeName)
//	{
//#ifdef REFLECT_CODE_GENERATOR
//		return TypeName.end() != std::find_if(TypeName.begin(), TypeName.end(), [&](STRING_TYPE& TheTypeName) { return TheTypeName == InTypeName; });
//#else
//		return TypeName.end() != std::find_if(TypeName.begin(), TypeName.end(), [&](STRING_TYPE& TheTypeName) { return strcmp(InTypeName, TheTypeName) == 0; });
//#endif // REFLECT_CODE_GENERATOR
//	}
//
//	std::string Dump();
//
//	std::vector<STRING_TYPE> TypeName;
//	int32_t TypeId{ 0 };
//	size_t TypeSize{ 0 };
//	std::vector<FField> Fields;
//	std::vector<FFunction> Functions;
//	Uint32 TypeFlag{ kTypeNoFlag }; // ETypeFlag
//
//#ifdef REFLECT_CODE_GENERATOR
//	std::string DeclaredFile;
//#endif // REFLECT_CODE_GENERATOR
//protected: 
//	friend class CCodeGenerator;
//	friend struct FTypeDescriptorTable;
//	void Typedef(const char* InTypeName){
//		assert(!HasTypeName(InTypeName));
//		TypeName.push_back(InTypeName);
//	}
//};
//
//struct FTypeDescriptorTable {
//protected:
//	FTypeDescriptorTable() = default;
//public:
//	std::unordered_map<std::string, int32_t> NameToId;
//	std::vector<FTypeDescriptor*> Descriptors;
//	std::atomic<int32_t> IdCounter{0};
//	std::list<std::function<void()>> ReflectorInitializer;
//
//	static FTypeDescriptorTable& Get();
//	FTypeDescriptor *GetDescriptor(const char * DescriptorName);
//	FTypeDescriptor *GetDescriptor(int32_t DescriptorId);
//	bool RegisterDescriptor(const char * TypeName, FTypeDescriptor *Descriptor);
//};
//
//struct FBuiltInTypeDescriptor : public FTypeDescriptor {
//	FBuiltInTypeDescriptor(const char* InTypeName, size_t InTypeSize = 0)
//		: FTypeDescriptor(InTypeName, InTypeSize)
//	{}
//	virtual bool IsBuiltInType() override { return true; }
//	virtual const char* GetTypeKind() override { return "builtIn"; }
//};
//
//struct FClassDescriptor : public FTypeDescriptor
//{
//	FClassDescriptor(const char* InTypeName, size_t InTypeSize = 0)
//		: FTypeDescriptor(InTypeName, InTypeSize)
//	{}
//	virtual bool IsClass() override { return true; }
//	virtual const char* GetTypeKind() override { return "class"; }
//};
//
//struct FStructDescriptor : public FTypeDescriptor
//{
//	FStructDescriptor(const char* InTypeName, size_t InTypeSize = 0)
//		: FTypeDescriptor(InTypeName, InTypeSize)
//	{}
//	virtual bool IsStruct() override { return true; }
//	virtual const char* GetTypeKind() override { return "struct"; }
//};
//
//struct FEnumDescriptor : public FTypeDescriptor
//{
//	FEnumDescriptor(const char* InTypeName, size_t InTypeSize = 0)
//		: FTypeDescriptor(InTypeName, InTypeSize)
//	{}
//	virtual bool IsEnum() override { return true; }
//	virtual const char* GetTypeKind() override { return "enum"; }
//};
//
//extern std::unique_ptr<FTypeDescriptor> GVoidDescriptor;
//extern std::unique_ptr<FTypeDescriptor> GBoolDescriptor;
//extern std::unique_ptr<FTypeDescriptor> GInt8Descriptor;
//extern std::unique_ptr<FTypeDescriptor> GUint8Descriptor;
//extern std::unique_ptr<FTypeDescriptor> GInt16Descriptor;
//extern std::unique_ptr<FTypeDescriptor> GUint16Descriptor;
//extern std::unique_ptr<FTypeDescriptor> GInt32Descriptor;
//extern std::unique_ptr<FTypeDescriptor> GUint32Descriptor;
//extern std::unique_ptr<FTypeDescriptor> GInt64Descriptor;
//extern std::unique_ptr<FTypeDescriptor> GUint64Descriptor;
//extern std::unique_ptr<FTypeDescriptor> GFloatDescriptor;
//extern std::unique_ptr<FTypeDescriptor> GDoubleDescriptor;
//extern std::unique_ptr<FTypeDescriptor> GStdStringDescriptor;
//
//extern int32_t ReserveObjectIdStart;
//extern int32_t ReserveObjectIdEnd;
