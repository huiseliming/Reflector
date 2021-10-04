#pragma once
#include <list>
#include <vector>
#include <memory>
#include <atomic>
#include <cassert>
#include <functional>
#include <unordered_map>


#ifdef __REFLECTOR__
#define REFLECT_OBJECT(...) __attribute__((annotate("Object"   __VA_OPT__(",") #__VA_ARGS__)))
#define PROPERTY(...)       __attribute__((annotate("Property" __VA_OPT__(",") #__VA_ARGS__)))
#define FUNCTION(...)       __attribute__((annotate("Function" __VA_OPT__(",") #__VA_ARGS__)))
#else
#define REFLECT_OBJECT(...)
#define PROPERTY(...)
#define FUNCTION(...)
#endif

#define REFLECT_GENERATED_BODY() \
public: \
static FTypeDescriptor* GetTypeDescriptor();\
static Uint32 GetTypeId();

typedef void               Void;
typedef bool               Bool;
typedef char               Int8;
typedef unsigned char      Uint8;
typedef short              Int16;
typedef unsigned short     Uint16;
typedef int                Int32;
typedef unsigned int       Uint32;
typedef long long          Int64;
typedef unsigned long long Uint64;
typedef float              Float;
typedef double             Double;

#ifdef REFLECT_CODE_GENERATOR
#define STRING_TYPE std::string
#else
#define STRING_TYPE const char *
#endif // REFLECT_CODE_GENERATOR

struct FTypeDescriptor;

enum EQualifierFlag :Uint32
{
	kNoQualifierFlag = 0,
	kPointerFlagBit = 1 << 0,
	kReferenceFlagBit = 1 << 1,
	kConstValueFlagBit = 1 << 2,
	kConstPointerFlagBit = 1 << 3,
};

enum ETypeFlag :Uint32
{
	kNoFlag = 0,
	kHasDefaultConstructorFlagBit = 1 << 0,
	kHasDestructorFlagBit = 1 << 1,
};

struct FField {
	STRING_TYPE FieldName{""};
	size_t FieldOffset{0};
	size_t Number{ 1 };
	FTypeDescriptor *TypeDescriptor{nullptr};
	Uint32 QualifierFlag{kNoQualifierFlag}; // EQualifierFlag
	bool IsNoQualifierType() { return QualifierFlag == kNoQualifierFlag; }
	bool IsPointerType() { return QualifierFlag & kPointerFlagBit; }
	bool IsReferenceType() { return QualifierFlag & kReferenceFlagBit; }
	bool IsConstValueType() { return QualifierFlag & kConstValueFlagBit; }
	bool IsConstPointerType() { return QualifierFlag & kConstPointerFlagBit; }
};

struct FFunction {
	STRING_TYPE FieldName{nullptr};
	void *Ptr{nullptr};
	FField Ret;
	std::vector<FField> Args;
};

struct FTypeDescriptor 
{
	FTypeDescriptor(const char* InTypeName, size_t InTypeSize)
		: TypeName{InTypeName}
		, TypeSize{InTypeSize}
	{
	}
	virtual ~FTypeDescriptor() {}
	static std::unordered_map<std::string, FTypeDescriptor> TypeDescriptorTable;
	virtual bool IsBuiltInType() { return false; }
	virtual bool IsClass() { return false; }
	virtual bool IsStruct() { return false; }
	virtual bool IsEnum() { return false; }
	virtual const char* GetTypeKind() = 0;

	bool HasDefaultConstructor() { return TypeFlag & kHasDefaultConstructorFlagBit; }
	//bool HasCopyConstructor() { return false; }
	//bool HasMoveConstructor() { return false; }
	bool HasDestructor() { return TypeFlag & kHasDestructorFlagBit; }

	virtual void Constructor(void* ConstructedObject) { }
	virtual void CopyConstructor(void* ConstructedObject, void* CopyedObject) { }
	virtual void MoveConstructor(void* ConstructedObject, void* MoveedObject) { }
	virtual void Destructor(void* DestructedObject) { }
	virtual void CopyAssignmentOperator(void* LObject, void* RObject) {}
	virtual void MoveAssignmentOperator(void* LObject, void* RObject) {}

	const char * GetTypeName() 
	{ 
#ifdef REFLECT_CODE_GENERATOR
		return TypeName[0].c_str();
#else
		return TypeName[0];
#endif // REFLECT_CODE_GENERATOR
	}

	bool HasTypeName(const char* InTypeName)
	{
#ifdef REFLECT_CODE_GENERATOR
		return TypeName.end() != std::find_if(TypeName.begin(), TypeName.end(), [&](STRING_TYPE& TheTypeName) { return TheTypeName == InTypeName; });
#else
		return TypeName.end() != std::find_if(TypeName.begin(), TypeName.end(), [&](STRING_TYPE& TheTypeName) { return strcmp(InTypeName, TheTypeName) == 0; });
#endif // REFLECT_CODE_GENERATOR
	}

	std::string Dump();

	std::vector<STRING_TYPE> TypeName;
	size_t TypeSize{ 0 };
	std::vector<FField> Fields;
	std::unordered_map<std::string, FFunction> Functions;
	Uint32 TypeFlag{ kNoFlag }; // ETypeFlag

#ifdef REFLECT_CODE_GENERATOR
	std::string DeclaredFile;
#endif // REFLECT_CODE_GENERATOR
protected: 
	friend class CCodeGenerator;
	friend struct FTypeDescriptorTable;
	void Typedef(const char* InTypeName){
		assert(!HasTypeName(InTypeName));
		TypeName.push_back(InTypeName);
	}
	int32_t TypeId{0};
};

struct FTypeDescriptorTable {
protected:
	FTypeDescriptorTable() = default;
public:
	std::unordered_map<std::string, int32_t> NameToId;
	std::vector<FTypeDescriptor*> Descriptors;
	std::atomic<int32_t> IdCounter{0};
	std::list<std::function<void()>> ReflectorInitializer;

	static FTypeDescriptorTable& Get();
	FTypeDescriptor *GetDescriptor(const char * DescriptorName);
	FTypeDescriptor *GetDescriptor(int32_t DescriptorId);
	bool RegisterDescriptor(const char * TypeName, FTypeDescriptor *Descriptor);
};

struct FBuiltInTypeDescriptor : public FTypeDescriptor {
	FBuiltInTypeDescriptor(const char* InTypeName, size_t InTypeSize = 0)
		: FTypeDescriptor(InTypeName, InTypeSize)
	{}
	virtual bool IsBuiltInType() { return true; }
	virtual const char* GetTypeKind() { return "builtIn"; }
};

struct FClassDescriptor : public FTypeDescriptor
{
	FClassDescriptor(const char* InTypeName, size_t InTypeSize = 0)
		: FTypeDescriptor(InTypeName, InTypeSize)
	{}
	virtual bool IsClass() { return true; }
	virtual const char* GetTypeKind() { return "class"; }
};

struct FStructDescriptor : public FTypeDescriptor
{
	FStructDescriptor(const char* InTypeName, size_t InTypeSize = 0)
		: FTypeDescriptor(InTypeName, InTypeSize)
	{}
	virtual bool IsStruct() { return true; }
	virtual const char* GetTypeKind() { return "struct"; }
};

struct FEnumDescriptor : public FTypeDescriptor
{
	FEnumDescriptor(const char* InTypeName, size_t InTypeSize = 0)
		: FTypeDescriptor(InTypeName, InTypeSize)
	{}
	virtual bool IsEnum() { return true; }
	virtual const char* GetTypeKind() { return "enum"; }
};

extern std::unique_ptr<FTypeDescriptor> GVoidDescriptor;
extern std::unique_ptr<FTypeDescriptor> GBoolDescriptor;
extern std::unique_ptr<FTypeDescriptor> GInt8Descriptor;
extern std::unique_ptr<FTypeDescriptor> GUint8Descriptor;
extern std::unique_ptr<FTypeDescriptor> GInt16Descriptor;
extern std::unique_ptr<FTypeDescriptor> GUint16Descriptor;
extern std::unique_ptr<FTypeDescriptor> GInt32Descriptor;
extern std::unique_ptr<FTypeDescriptor> GUint32Descriptor;
extern std::unique_ptr<FTypeDescriptor> GInt64Descriptor;
extern std::unique_ptr<FTypeDescriptor> GUint64Descriptor;
extern std::unique_ptr<FTypeDescriptor> GFloatDescriptor;
extern std::unique_ptr<FTypeDescriptor> GDoubleDescriptor;
extern std::unique_ptr<FTypeDescriptor> GStdStringDescriptor;