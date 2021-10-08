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
static const FClass* GetClass();\



#ifdef REFLECT_CODE_GENERATOR
#define STRING_TYPE std::string
#else
#define STRING_TYPE const char *
#endif // REFLECT_CODE_GENERATOR

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

struct FClass;


enum EClassFlag :Uint32 {
	kClassNoFlag = 0,
	kHasDefaultConstructorFlagBit = 1 << 0,
	kHasDestructorFlagBit = 1 << 1,
};

enum EFunctionFlag :Uint32 {
	kFunctionNoFlag = 0,
	kMemberFlagBit,
	kStaticFlagBit,
};

enum EQualifierFlag :Uint32 {
	kQualifierNoFlag = 0,
	kPointerFlagBit = 1 << 0,
	kReferenceFlagBit = 1 << 1,
	kConstValueFlagBit = 1 << 2,
	kConstPointerFlagBit = 1 << 3,
};

struct FParameter 
{
	FClass* Class{ nullptr };
	STRING_TYPE Name{ "" };
	Uint32 Flag{ kQualifierNoFlag }; // EQualifierFlag
};

struct FRetVal
{
	FClass* Class{ nullptr };
	Uint32 Flag{ kQualifierNoFlag }; // EQualifierFlag
};

struct FFunction {
	STRING_TYPE Name{""};
	void *Ptr{nullptr};
	FClass* OwnerClass{nullptr}; //  Owner {Class | Struct} TypeDescriptor
	FRetVal Ret;
	std::vector<FParameter> Args;
	Uint32 Flag{ kFunctionNoFlag }; //EFunctionFlag
	bool IsStaticMemberFunction() { return Flag & (kMemberFlagBit | kStaticFlagBit); }
	bool IsMemberFunction() { return Flag & kMemberFlagBit; }
	bool IsStaticFunction() { return Flag & kStaticFlagBit; }
};

struct FField
{
	STRING_TYPE Name{ "" };
	FClass* Class{ nullptr };
	STRING_TYPE ClassName {""};
	Uint32 Flag{ kQualifierNoFlag }; // EQualifierFlag
	size_t Offset{ 0 };
	size_t Number{ 1 };

	bool IsNoQualifierType()  { return Flag == kQualifierNoFlag; }
	bool IsPointerType()      { return Flag & kPointerFlagBit; }
	bool IsReferenceType()    { return Flag & kReferenceFlagBit; }
	bool IsConstValueType()   { return Flag & kConstValueFlagBit; }
	bool IsConstPointerType() { return Flag & kConstPointerFlagBit; }

	template<typename T>
	T& Ref(void* OwnerBaseAddress) { return *reinterpret_cast<T*>(((Uint8*)OwnerBaseAddress) + Offset); }
	template<typename T>
	T* Ptr(void* OwnerBaseAddress) { return reinterpret_cast<T*>(((Uint8*)OwnerBaseAddress) + Offset); }
	template<typename T>
	const T& CRef(void* OwnerBaseAddress) { return GetRef<T>(OwnerBaseAddress); }
	template<typename T>
	const T* CPtr(void* OwnerBaseAddress) { return GetPtr<T>(OwnerBaseAddress); }
};

// 功能描述对象

struct FClass 
{
	STRING_TYPE Name {""};
	size_t Size{ 0 };
	Uint32 Flag{ kClassNoFlag }; // EClassFlag
	std::vector<FField> Fields;
	std::vector<FFunction> Functions;
	std::vector<STRING_TYPE> Alias;
	std::vector<STRING_TYPE> ParentClassName;
	Uint32 Id{ 0 };
#ifdef REFLECT_CODE_GENERATOR
	std::string DeclaredFile;
#endif // REFLECT_CODE_GENERATOR
	bool HasDefaultConstructor() { return Flag & kHasDefaultConstructorFlagBit; }
	bool HasDestructor() { return Flag & kHasDestructorFlagBit; }

	virtual void* New() { return nullptr; }
	virtual void Delete(void* Object) { }
	virtual void Constructor(void* ConstructedObject) { }
	virtual void CopyConstructor(void* ConstructedObject, void* CopyedObject) { }
	virtual void MoveConstructor(void* ConstructedObject, void* MoveedObject) { }
	virtual void Destructor(void* DestructedObject) { }
	virtual void CopyAssignmentOperator(void* LObject, void* RObject) {}
	virtual void MoveAssignmentOperator(void* LObject, void* RObject) {}
};

struct FEnum
{

};


struct FClassTable {
protected:
	FClassTable() = default;
public:
	std::unordered_map<std::string, int32_t> NameToId;
	std::vector<FClass*> Classes;
	std::atomic<int32_t> IdCounter{ 1 };
	std::list<std::function<void()>> ReflectorInitializer;

	static FClassTable& Get();
	FClass* GetClass(const char* ClassName);
	FClass* GetClass(Uint32 ClassId);
	bool RegisterClass(const char* TypeName, FClass* Class);
};


struct FVoid
{
	static const FClass* GetClass()
	{
		static std::function<FClass* ()> ClassInitializer = []() -> FClass* {
			static FClass Class;
			Class.Name = "void";
			Class.Size = 0;
			Class.Flag = kClassNoFlag;
			return &Class;
		};
		static FClass* VoidClass = ClassInitializer();
		return VoidClass;
	}
};

#define DEFINE_BUILT_IN_CLASS(VarName, BuiltInType) \
struct F##VarName\
{\
	static const FClass* GetClass()\
	{\
		static std::function<FClass* ()> ClassInitializer = []() -> FClass* {\
			static FClass Class{};\
			Class.Name = #BuiltInType;\
			Class.Size = sizeof(BuiltInType);\
			Class.Flag = kClassNoFlag;\
			return &Class;\
		};\
		static FClass* VarName##Class = ClassInitializer();\
		return VarName##Class;\
	}\
};


DEFINE_BUILT_IN_CLASS(Bool  , bool);
DEFINE_BUILT_IN_CLASS(Int8  , char);
DEFINE_BUILT_IN_CLASS(Uint8 , unsigned char);
DEFINE_BUILT_IN_CLASS(Int16 , short);
DEFINE_BUILT_IN_CLASS(Uint16, unsigned short);
DEFINE_BUILT_IN_CLASS(Int32 , int);
DEFINE_BUILT_IN_CLASS(Uint32, unsigned int);
DEFINE_BUILT_IN_CLASS(Int64 , long long);
DEFINE_BUILT_IN_CLASS(Uint64, unsigned long long);
DEFINE_BUILT_IN_CLASS(Float , float);
DEFINE_BUILT_IN_CLASS(Double, double);
