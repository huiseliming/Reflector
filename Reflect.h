#pragma once
#include <list>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <cassert>
#include <functional>
#include <unordered_map>
#include "Traits.h"
#ifdef CORE_MODULE
#include "CoreApi.h"
#else
#define CORE_API
#endif

#ifdef __REFLECTOR__
#define CLASS(...)     __attribute__((annotate("Class"    __VA_OPT__(",") #__VA_ARGS__)))
#define ENUM(...)      __attribute__((annotate("Enum"     __VA_OPT__(",") #__VA_ARGS__)))
#define PROPERTY(...)  __attribute__((annotate("Property" __VA_OPT__(",") #__VA_ARGS__)))
#define FUNCTION(...)  __attribute__((annotate("Function" __VA_OPT__(",") #__VA_ARGS__)))
#else
#define CLASS(...)
#define ENUM(...)
#define PROPERTY(...)
#define FUNCTION(...)
#endif

#define REFLECT_GENERATED_BODY() \
public: \
static const CClass* GetClass();\
static Uint32 ClassId;


#ifdef COMPILE_REFLECTOR
#define STRING_TYPE std::string
#else
#define STRING_TYPE const char *
#endif // COMPILE_REFLECTOR

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

struct CClass;
struct CProperty;

enum EClassFlag :Uint32 {
	kClassNoFlag = 0,
	kHasDefaultConstructorFlagBit = 1 << 0,
	kHasDestructorFlagBit = 1 << 1,
	kUserWritedReflectClassFlagBit = 1 << 2,
};

enum EFunctionFlag :Uint32 {
	kFunctionNoFlag = 0,
	kMemberFlagBit,
	kStaticFlagBit,
};

enum EQualifierFlag :Uint32 {
	kQualifierNoFlag = 0,
};
enum EPropertyFlag : Uint32
{
	CPF_NoneFlag             = 0x00000000,
								 
	CPF_BoolFlag             = 0x00000001,
	CPF_Int8Flag             = 0x00000002,
	CPF_Int16Flag            = 0x00000004,
	CPF_Int32Flag            = 0x00000008,
	CPF_Int64Flag            = 0x00000010,
	CPF_Uint8Flag            = 0x00000020,
	CPF_Uint16Flag           = 0x00000040,
	CPF_Uint32Flag           = 0x00000080,
	CPF_Uint64Flag           = 0x00000100,
	CPF_FloatNullFlag        = 0x00000200,
	CPF_DoubleNullFlag       = 0x00000400,
	CPF_StringFlag           = 0x00000800,
	CPF_StructFlag           = 0x00001000,
	CPF_ClassFlag            = 0x00002000,
								 
	CPF_ArrayFlag            = 0x00004000,
	CPF_MapFlag              = 0x00008000,
								 
	CPF_PointerFlag          = 0x10000000,
	CPF_ReferenceFlag        = 0x20000000,
	CPF_ConstValueFlag       = 0x40000000,
	CPF_ConstPointerceFlag   = 0x80000000,
								 
	CPF_TypeMaskBitFlag      = 0x0000FFFF,
	CPF_QualifierMaskBitFlag = 0xF0000000,

};

#ifdef CORE_MODULE
// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)
#endif

struct CORE_API FFunction {
	STRING_TYPE Name{ "" };
	void* Ptr{ nullptr };
	const CClass* OwnerClass{ nullptr }; //  Owner {Class | Struct} TypeDescriptor
	Uint32 Flag{ kFunctionNoFlag }; //EFunctionFlag
	bool IsStaticMemberFunction() { return Flag & (kMemberFlagBit | kStaticFlagBit); }
	bool IsMemberFunction() { return Flag & kMemberFlagBit; }
	bool IsStaticFunction() { return Flag & kStaticFlagBit; }
};

struct CORE_API FField
{
	STRING_TYPE Name{ "" };
	const CClass* Class{ nullptr };
	Uint32 Flag{ kQualifierNoFlag }; // EQualifierFlag
	size_t Offset{ 0 };
	size_t Number{ 1 };

	//#ifdef COMPILE_REFLECTOR
	//	STRING_TYPE ClassName {""};
	//#endif


	bool IsNoQualifierType() { return Flag == kQualifierNoFlag; }
	bool IsPointerType() { return Flag & CPF_PointerFlag; }
	bool IsReferenceType() { return Flag & CPF_ReferenceFlag; }
	bool IsConstValueType() { return Flag & CPF_ConstValueFlag; }
	bool IsConstPointerType() { return Flag & CPF_ConstPointerceFlag; }

	template<typename T>
	T& Ref(void* OwnerBaseAddress) { return *reinterpret_cast<T*>(((Uint8*)OwnerBaseAddress) + Offset); }
	template<typename T>
	T* Ptr(void* OwnerBaseAddress) { return reinterpret_cast<T*>(((Uint8*)OwnerBaseAddress) + Offset); }
	template<typename T>
	const T& CRef(void* OwnerBaseAddress) { return GetRef<T>(OwnerBaseAddress); }
	template<typename T>
	const T* CPtr(void* OwnerBaseAddress) { return GetPtr<T>(OwnerBaseAddress); }
};

struct CField
{
	CField(const char * InName)
		: Name(InName)
	{}
	virtual ~CField() {}
	STRING_TYPE Name;
	std::vector<STRING_TYPE> Alias;

	Uint32 Id{0};
	std::unordered_map<std::string, const char*> MetaData;

#ifdef COMPILE_REFLECTOR
	std::string DeclaredFile;
	bool IsReflectionDataCollectionCompleted{ false };
	bool IsForwardDeclared{ false };
#endif // COMPILE_REFLECTOR
};

struct CStructClass : public CField
{
	CStructClass(const char* InName)
		: CField(InName)
	{}
	
	size_t Size{ 0 };
	Uint32 Flag{ kClassNoFlag }; // EClassFlag
	std::vector<std::unique_ptr<CProperty>> Properties;
	std::vector<const CStructClass*> ParentClasses;
#ifdef COMPILE_REFLECTOR
	virtual bool IsReflectClass() { return true; }
	bool IsReflectGeneratedClass() { return !(Flag & kUserWritedReflectClassFlagBit); }
#endif // COMPILE_REFLECTOR
};

struct CORE_API CClass : public CStructClass
{
	CClass(const char* InName)
		: CStructClass(InName)
	{}

	std::vector<FFunction> Functions;

	virtual bool IsBuiltInType() { return false; }
	virtual bool IsEnumClass() { return false; }


	bool HasDefaultConstructor() { return Flag & kHasDefaultConstructorFlagBit; }
	bool HasDestructor() { return Flag & kHasDestructorFlagBit; }


	virtual void* New() { return nullptr; }
	virtual void Delete(void* Object) { }
	virtual void Constructor(void* ConstructedObject) { }
	virtual void Destructor(void* DestructedObject) { }

	virtual void CopyConstructor(void* ConstructedObject, void* CopyedObject) { }
	virtual void MoveConstructor(void* ConstructedObject, void* MoveedObject) { }
	virtual void CopyAssignmentOperator(void* LObject, void* RObject) {}
	virtual void MoveAssignmentOperator(void* LObject, void* RObject) {}
};

struct CEnumClass : public CField
{
	CEnumClass(const char* InName)
		: CField(InName)
	{}

	Uint32 Size;
	std::vector<std::pair<STRING_TYPE, uint64_t>> Options;
};

struct CORE_API CClassTable {
public:
	CClassTable();
	std::unordered_map<std::string, int32_t> NameToId;
	std::vector<CField*> Classes;
	std::atomic<int32_t> IdCounter{ 1 };
	std::list<std::function<bool()>> DeferredRegisterList;

	static CClassTable& Get();

	CField* GetClass(const char* ClassName);
	CField* GetClass(Uint32 ClassId);
	uint32_t RegisterClassToTable(const char* TypeName, CField* Class);

	/**
	 * must be called after global initialization is complete and before use,
	 * this function will defer registration
	 *
	 * exmaple:
	 * int main(int argc, const char* argv[])
	 * {
	 *     GClassTable->Initialize();
	 *     //Do something ...
	 *     return 0;
	 * }
	**/
	void Initialize();
};

/**
 * can be used after global initialization is complete
**/
#ifndef COMPILE_REFLECTOR
extern CORE_API CClassTable* GClassTable;
#endif

#ifdef CORE_MODULE
#pragma warning(pop)
#endif

template<typename T>
struct TEnum {

};

template<typename T>
struct TClassAutoRegister {
	TClassAutoRegister()
	{
		const CClass* Class = T::GetClass();
		T::ClassId = CClassTable::Get().RegisterClassToTable(Class->Name, const_cast<CClass*>(Class));
	}
};

//template<typename TA, typename TB>
//bool IsA(TB* B)
//{
//	return B->IsA<TA>();
//}

struct CProperty : public CField
{
	CProperty(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: CField(InName), Offset(InOffset), Flag(InFlag), Number(InNumber)
	{}

	Uint32        Offset{ 0 };
	EPropertyFlag Flag{ CPF_NoneFlag };
	Uint32        Number{ 1 };


	void AddPropertyFlag(EPropertyFlag InFlag)
	{
		Flag = EPropertyFlag(Flag | InFlag);
	}

	void RemovePropertyFlag(EPropertyFlag InFlag)
	{
		Flag = EPropertyFlag(Flag & !InFlag);
	}

	bool IsSimpleValueType() { return Flag == kQualifierNoFlag; }
	bool IsPointerType() { return Flag & CPF_PointerFlag; }
	bool IsReferenceType() { return Flag & CPF_ReferenceFlag; }
	bool IsConstValueType() { return Flag & CPF_ConstValueFlag; }
	bool IsConstPointerType() { return Flag & CPF_ConstPointerceFlag; }

	virtual bool IsFloatingPoint() const { return false; }
	virtual bool IsInteger() const { return false; }

	virtual void SetBoolPropertyValue(void* Data, bool Value) const                     {}
	virtual void SetIntPropertyValue(void* Data, Uint64 Value) const                    {}
	virtual void SetIntPropertyValue(void* Data, Int64 Value) const                     {}
	virtual void SetFloatingPointPropertyValue(void* Data, double Value) const          {}
	virtual void SetStringPropertyValue(void* Data, std::string& Value) const           {}
	virtual void SetStringPropertyValue(void* Data, const char* Value) const            {}

	virtual Bool GetBoolPropertyValue(void const* Data) const                           { return false; }
	virtual Int64 GetSignedIntPropertyValue(void const* Data) const                     { return 0; }
	virtual Uint64 GetUnsignedIntPropertyValue(void const* Data) const                  { return 0; }
	virtual double GetFloatingPointPropertyValue(void const* Data) const                { return 0.f; }
	virtual std::string GetStringPropertyValue(void const* Data) const                  { return ""; }

	virtual void SetNumericPropertyValueFromString(void* Data, char const* Value) const {}
	virtual std::string GetBoolPropertyValueToString(void const* Data) const            { return ""; }
	virtual std::string GetNumericPropertyValueToString(void const* Data) const         { return ""; }
};

#define OFFSET_VOID_PTR(PVoid,Offset) (void*)(((char*)(PVoid)) + Offset)

template<typename CppType>
struct TPropertyValue
{
	enum
	{
		CPPSize = sizeof(CppType),
		CPPAlignment = alignof(CppType)
	};

	/** Convert the address of a value of the property to the proper type */
	static CppType const* GetPropertyValuePtr(void const* A)
	{
		return (CppType const*)(A);
	}
	/** Convert the address of a value of the property to the proper type */
	static CppType* GetPropertyValuePtr(void* A)
	{
		return (CppType*)(A);
	}
	/** Get the value of the property from an address */
	static CppType const& GetPropertyValue(void const* A)
	{
		return *GetPropertyValuePtr(A);
	}
	/** Get the default value of the cpp type, just the default constructor, which works even for things like in32 */
	static CppType GetDefaultPropertyValue()
	{
		return CppType();
	}
	/** Get the value of the property from an address, unless it is NULL, then return the default value */
	static CppType GetOptionalPropertyValue(void const* B)
	{
		return B ? GetPropertyValue(B) : GetDefaultPropertyValue();
	}
	/** Set the value of a property at an address */
	static void SetPropertyValue(void* A, CppType const& Value)
	{
		*GetPropertyValuePtr(A) = Value;
	}
	/** Initialize the value of a property at an address, this assumes over uninitialized memory */
	static CppType* InitializePropertyValue(void* A)
	{
		return new (A) CppType();
	}
	/** Destroy the value of a property at an address */
	static void DestroyPropertyValue(void* A)
	{
		GetPropertyValuePtr(A)->~CppType();
	}
};

template <typename CppType>
struct TNumericProperty : public CProperty
{
	using TPropertyValue = TPropertyValue<CppType>;

	TNumericProperty(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: CProperty(InName, InOffset, InFlag, InNumber)
	{
	}

	virtual bool IsFloatingPoint() const override
	{
		return TIsFloatingPoint<CppType>::Value;
	}
	virtual bool IsInteger() const override
	{
		return TIsInteger<CppType>::Value;
	}
	virtual void SetIntPropertyValue(void* Data, Uint64 Value) const override
	{
		assert(TIsInteger<CppType>::Value);
		TPropertyValue::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), (CppType)Value);
	}
	virtual void SetIntPropertyValue(void* Data, Int64 Value) const override
	{
		assert(TIsInteger<CppType>::Value);
		TPropertyValue::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), (CppType)Value);
	}
	virtual void SetFloatingPointPropertyValue(void* Data, double Value) const override
	{
		assert(TIsFloatingPoint<CppType>::Value);
		TPropertyValue::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), (CppType)Value);
	}
	virtual void SetNumericPropertyValueFromString(void* Data, char const* Value) const override
	{
		*TPropertyValue::GetPropertyValuePtr(OFFSET_VOID_PTR(Data, CProperty::Offset)) =  atoll(Value);
	}
	virtual std::string GetNumericPropertyValueToString(void const* Data) const override
	{
		return std::to_string(TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset)));
	}
	virtual Int64 GetSignedIntPropertyValue(void const* Data) const override
	{
		assert(TIsInteger<CppType>::Value);
		return (Int64)TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset));
	}
	virtual Uint64 GetUnsignedIntPropertyValue(void const* Data) const override
	{
		assert(TIsInteger<CppType>::Value);
		return (Uint64)TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset));
	}
	virtual double GetFloatingPointPropertyValue(void const* Data) const override
	{
		assert(TIsFloatingPoint<CppType>::Value);
		return (double)TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset));
	}

};


struct CBoolProperty : public CProperty
{
	CBoolProperty(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: CProperty(InName, InOffset, InFlag, InNumber)
	{}
	virtual void SetBoolPropertyValue(void* Data, bool Value) const
	{
		TPropertyValue<Bool>::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), (Bool)Value);
	}
	virtual Bool GetBoolPropertyValue(void const* Data) const { return TPropertyValue<Bool>::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset)); }
	virtual std::string GetBoolPropertyValueToString(void const* Data) const { return TPropertyValue<Bool>::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset)) ? "True" : "false"; }
};

struct CInt8Property : TNumericProperty<Int8>
{
	CInt8Property(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: TNumericProperty<Int8>(InName, InOffset, InFlag, InNumber)
	{
	}
};

struct CInt16Property : TNumericProperty<Int16>
{
	CInt16Property(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: TNumericProperty<Int16>(InName, InOffset, InFlag, InNumber)
	{
	}
};

struct CInt32Property : TNumericProperty<Uint32>
{
	CInt32Property(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: TNumericProperty<Uint32>(InName, InOffset, InFlag, InNumber)
	{
	}
};

struct CInt64Property : TNumericProperty<Uint64>
{
	CInt64Property(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: TNumericProperty<Uint64>(InName, InOffset, InFlag, InNumber)
	{
	}
};

struct CUint8Property : TNumericProperty<Uint8>
{
	CUint8Property(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: TNumericProperty<Uint8>(InName, InOffset, InFlag, InNumber)
	{
	}
};

struct CUint16Property : TNumericProperty<Uint16>
{
	CUint16Property(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: TNumericProperty<Uint16>(InName, InOffset, InFlag, InNumber)
	{
	}
};

struct CUint32Property : TNumericProperty<Uint32>
{
	CUint32Property(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: TNumericProperty<Uint32>(InName, InOffset, InFlag, InNumber)
	{
	}
};

struct CUint64Property : TNumericProperty<Uint64>
{
	CUint64Property(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: TNumericProperty<Uint64>(InName, InOffset, InFlag, InNumber)
	{
	}
};

struct CFloatProperty : TNumericProperty<Float>
{
	CFloatProperty(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: TNumericProperty<Float>(InName, InOffset, InFlag, InNumber)
	{
	}
};

struct CDoubleProperty : TNumericProperty<Double>
{
	CDoubleProperty(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: TNumericProperty<Double>(InName, InOffset, InFlag, InNumber)
	{
	}
};

struct CStringProperty : public CProperty
{
	CStringProperty(const char * InName, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: CProperty(InName, InOffset, InFlag, InNumber)
	{}

	virtual std::string GetStringPropertyValue(void const* Data) const 
	{ 
		return TPropertyValue<std::string>::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset));
	}

	virtual std::string GetStringPropertyPtr(void const* Data) const 
	{
		return TPropertyValue<std::string>::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset)).c_str(); 
	}

	virtual void SetStringPropertyValue(void* Data, std::string& Value) const 
	{
		TPropertyValue<std::string>::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), Value);
	}

	virtual void SetStringPropertyValue(void* Data, const char* Value) const
	{
		*TPropertyValue<std::string>::GetPropertyValuePtr(OFFSET_VOID_PTR(Data, CProperty::Offset)) = Value;
	}
};

struct CStructProperty : public CProperty
{
	CStructProperty(const char* InName, CStructClass* InStructClass, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: CProperty(InName, InOffset, InFlag, InNumber)
		, StructClass(InStructClass)
	{}

	CStructClass* StructClass;
};

struct CClassProperty : public CProperty
{
	CClassProperty(const char * InName, CClass* InClass, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: CProperty(InName, InOffset, InFlag, InNumber)
		, Class(InClass)
	{}

	CClass* Class;
};


struct CEnumProperty : public CProperty
{
	CEnumProperty(const char* InName, CEnumClass* InEnum, Uint32 InOffset, EPropertyFlag InFlag, Uint32 InNumber = 1)
		: CProperty(InName, InOffset, InFlag, InNumber)
		, Enum(InEnum)
	{}

	CEnumClass* Enum;
};