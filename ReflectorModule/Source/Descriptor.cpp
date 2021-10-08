//#include "Descriptor.h"
//// #ifdef __REFLECTOR__ // reflector using clang, c++23 std::format not impl
//// namespace std {
//// 	template <class... _Types>
//// 	string format(const string_view _Fmt, const _Types&... _Args) {
//// 		return {};
//// 	}
//// }
//// #else
//#include <format>
//// #endif
//
//#define MAKE_BUILT_IN_TYPE_DESCRIPTOR(BuiltInTypeName, BuiltInType, IsUnsigned) \
//struct F##BuiltInTypeName##TypeDescriptor : public FBuiltInTypeDescriptor \
//{\
//	F##BuiltInTypeName##TypeDescriptor() \
//		: FBuiltInTypeDescriptor(#BuiltInType, sizeof(BuiltInType)) \
//	{}\
//	virtual bool IsUnsignedType() { return IsUnsigned; }\
//	virtual void* New() { return new BuiltInType; }\
//	virtual void Delete(void* Object) { delete (BuiltInType*)Object; }\
//}\
//
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Bool, bool, false);
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Int8, char, false);
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Uint8, unsigned char, true);
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Int16, short, false);
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Uint16, unsigned short, true);
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Int32, int, false);
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Uint32, unsigned int, true);
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Int64, long long, false);
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Uint64, unsigned long long, true);
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Float, float, false);
//MAKE_BUILT_IN_TYPE_DESCRIPTOR(Double, double, false);
//
////#define MAKE_BUILT_IN_TYPE_DESCRIPTOR(BuiltInTypeName, BuiltInType) FBuiltInTypeDescriptor(#BuiltInType, sizeof(BuiltInType))
//
//std::unique_ptr<FTypeDescriptor> GVoidDescriptor  ;
//std::unique_ptr<FTypeDescriptor> GBoolDescriptor  ;
//std::unique_ptr<FTypeDescriptor> GInt8Descriptor  ;
//std::unique_ptr<FTypeDescriptor> GUint8Descriptor ;
//std::unique_ptr<FTypeDescriptor> GInt16Descriptor ;
//std::unique_ptr<FTypeDescriptor> GUint16Descriptor;
//std::unique_ptr<FTypeDescriptor> GInt32Descriptor ;
//std::unique_ptr<FTypeDescriptor> GUint32Descriptor;
//std::unique_ptr<FTypeDescriptor> GInt64Descriptor ;
//std::unique_ptr<FTypeDescriptor> GUint64Descriptor;
//std::unique_ptr<FTypeDescriptor> GFloatDescriptor ;
//std::unique_ptr<FTypeDescriptor> GDoubleDescriptor;
//
//int32_t ReserveObjectIdStart = 32;
//int32_t ReserveObjectIdEnd = 128;
//
//std::string FTypeDescriptor::Dump()
//{
//	std::string DumpString = std::format("{:s} {:s}\n", GetTypeKind(), TypeName[0]);
//	DumpString.append("{\n");
//	for (size_t i = 0; i < Fields.size(); i++)
//	{
//		size_t FieldOffset = Fields[i].FieldOffset;
//		size_t Number = Fields[i].Number;
//		std::string FieldDecl = std::format("  {:s}{:s}{:s} {:s}{:s}{:s}; <Offset:{:d}>\n",
//			Fields[i].IsConstValueType() ? "const " : "",
//			Fields[i].TypeDescriptor->GetTypeName(),
//			Fields[i].IsPointerType() ? "*" : (Fields[i].IsReferenceType() ? "&" : ""),
//			Fields[i].IsConstPointerType() ? "const " : "",
//			Fields[i].FieldName,
//			Number == 1 ? "" : std::format("[{:d}]", Number),
//			FieldOffset);
//		DumpString.append(FieldDecl);
//	}
//	DumpString.append("};\n");
//	return DumpString;
//}
//
//
//
//
//FTypeDescriptorTable& FTypeDescriptorTable::Get()
//{
//	static std::function<FTypeDescriptorTable* ()> TypeDescriptorTableInitializer = []() -> FTypeDescriptorTable* {
//		static FTypeDescriptorTable TypeDescriptorTable;
//		GBoolDescriptor   = std::move(std::unique_ptr<FTypeDescriptor>(new FBoolTypeDescriptor  ()));
//		GInt8Descriptor   = std::move(std::unique_ptr<FTypeDescriptor>(new FInt8TypeDescriptor  ()));
//		GUint8Descriptor  = std::move(std::unique_ptr<FTypeDescriptor>(new FUint8TypeDescriptor ()));
//		GInt16Descriptor  = std::move(std::unique_ptr<FTypeDescriptor>(new FInt16TypeDescriptor ()));
//		GUint16Descriptor = std::move(std::unique_ptr<FTypeDescriptor>(new FUint16TypeDescriptor()));
//		GInt32Descriptor  = std::move(std::unique_ptr<FTypeDescriptor>(new FInt32TypeDescriptor ()));
//		GUint32Descriptor = std::move(std::unique_ptr<FTypeDescriptor>(new FUint32TypeDescriptor()));
//		GInt64Descriptor  = std::move(std::unique_ptr<FTypeDescriptor>(new FInt64TypeDescriptor ()));
//		GUint64Descriptor = std::move(std::unique_ptr<FTypeDescriptor>(new FUint64TypeDescriptor()));
//		GFloatDescriptor  = std::move(std::unique_ptr<FTypeDescriptor>(new FFloatTypeDescriptor ()));
//		GDoubleDescriptor = std::move(std::unique_ptr<FTypeDescriptor>(new FDoubleTypeDescriptor()));
//		// Register built-in type
//		GVoidDescriptor->TypeId = TypeDescriptorTable.IdCounter++;
//		TypeDescriptorTable.Descriptors.push_back(GVoidDescriptor.get());
//		TypeDescriptorTable.NameToId.insert(std::make_pair("void", GVoidDescriptor->TypeId));
//
//		TypeDescriptorTable.RegisterDescriptor("bool", GBoolDescriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("char", GInt8Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("unsigned char", GUint8Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("short", GInt16Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("unsigned short", GUint16Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("int", GInt32Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("unsigned int", GUint32Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("long long", GInt64Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("unsigned long long", GUint64Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("float", GFloatDescriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("double", GDoubleDescriptor.get());
//
//		TypeDescriptorTable.RegisterDescriptor("Boolean", GBoolDescriptor.get());
//
//		TypeDescriptorTable.RegisterDescriptor("Void", GVoidDescriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Bool", GBoolDescriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Int8", GInt8Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Uint8", GUint8Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Int16", GInt16Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Uint16", GUint16Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Int32", GInt32Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Uint32", GUint32Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Int64", GInt64Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Uint64", GUint64Descriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Float", GFloatDescriptor.get());
//		TypeDescriptorTable.RegisterDescriptor("Double", GDoubleDescriptor.get());
//#ifdef REFLECT_CODE_GENERATOR
//		TypeDescriptorTable.RegisterDescriptor("_Bool", GBoolDescriptor.get());
//#endif
//		static std::vector<std::unique_ptr<FTypeDescriptor>> ReserveObjectDescriptor; //("ReserveObject", )
//		int32_t CurrentId = TypeDescriptorTable.IdCounter;
//		for (int32_t i = CurrentId; i < ReserveObjectIdStart; i++)
//		{
//			std::string ReserveObjectIndexName = std::format("ReserveObject{:d}", i);
//			ReserveObjectDescriptor.push_back(std::make_unique<FClassDescriptor>(ReserveObjectIndexName.c_str(), i));
//			TypeDescriptorTable.RegisterDescriptor(ReserveObjectIndexName.c_str(), ReserveObjectDescriptor.back().get());
//		}
//
//		struct FStdStringDescriptor : public FClassDescriptor {
//			FStdStringDescriptor(const char* InTypeName, size_t InTypeSize = 0)
//				: FClassDescriptor(InTypeName, InTypeSize)
//			{}
//			virtual void* New() override { return new std::string(); }
//			virtual void Delete(void* Object) override { delete (std::string*)Object; }
//			virtual void Constructor(void* ConstructedObject) override { new (ConstructedObject) std::basic_string<char, std::char_traits<char>, std::allocator<char>>(); }
//			virtual void Destructor(void* DestructedObject) override { reinterpret_cast<std::string*>(DestructedObject)->~basic_string<char, std::char_traits<char>, std::allocator<char>>(); }
//		};
//		static FStdStringDescriptor StdStringDescriptor("std::string", sizeof(std::string));
//		StdStringDescriptor.TypeFlag = 0x00000003;
//		TypeDescriptorTable.RegisterDescriptor("std::string", &StdStringDescriptor);
//
//		CurrentId = TypeDescriptorTable.IdCounter;
//		for (int32_t i = CurrentId; i < ReserveObjectIdEnd; i++)
//		{
//			std::string ReserveObjectIndexName = std::format("ReserveObject{:d}", i);
//			ReserveObjectDescriptor.push_back(std::make_unique<FClassDescriptor>(ReserveObjectIndexName.c_str(), i));
//			TypeDescriptorTable.RegisterDescriptor(ReserveObjectIndexName.c_str(), ReserveObjectDescriptor.back().get());
//		}
//		return &TypeDescriptorTable;
//	};
//	static FTypeDescriptorTable* TypeDescriptorTable = TypeDescriptorTableInitializer();
//	return *TypeDescriptorTable;
//}
//
//FTypeDescriptor* FTypeDescriptorTable::GetDescriptor(const char* DescriptorName)
//{
//	auto NameToIdIterator = NameToId.find(DescriptorName);
//	if (NameToIdIterator != NameToId.end())
//		return Descriptors[abs(NameToIdIterator->second)];
//	return nullptr;
//}
//
//FTypeDescriptor* FTypeDescriptorTable::GetDescriptor(int32_t DescriptorId)
//{
//	int32_t AbsDescriptorId = abs(DescriptorId);
//	if (AbsDescriptorId < Descriptors.size())
//		return Descriptors[AbsDescriptorId];
//	return nullptr;
//}
//
//bool FTypeDescriptorTable::RegisterDescriptor(const char* TypeName, FTypeDescriptor* Descriptor) {
//	assert(Descriptor);
//	assert(std::end(NameToId) == NameToId.find(TypeName) || (std::end(NameToId) != NameToId.find(TypeName) && !Descriptor->HasTypeName(TypeName)));
//	if (Descriptor->TypeId == 0) {
//		Descriptor->TypeId = IdCounter++;
//		Descriptors.push_back(Descriptor);
//		NameToId.insert(std::make_pair(TypeName, Descriptor->TypeId));
//	}
//	else
//	{
//		Descriptor->Typedef(TypeName);
//		NameToId.insert(std::make_pair(TypeName, -Descriptor->TypeId));
//	}
//	return true;
//}