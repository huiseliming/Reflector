#include "Descriptor.h"
#ifdef __REFLECTOR__ // reflector using clang, c++23 std::format not impl
namespace std {
	template <class... _Types>
	string format(const string_view _Fmt, const _Types&... _Args) {
		return {};
	}
}
#else
#include <format>
#endif

#define MAKE_BUILT_IN_TYPE_DESCRIPTOR(BuiltInType) FBuiltInTypeDescriptor(#BuiltInType, sizeof(BuiltInType))
std::unique_ptr<FTypeDescriptor> GVoidDescriptor  (new FBuiltInTypeDescriptor("void"));
std::unique_ptr<FTypeDescriptor> GBoolDescriptor  (new MAKE_BUILT_IN_TYPE_DESCRIPTOR(bool));
std::unique_ptr<FTypeDescriptor> GInt8Descriptor  (new MAKE_BUILT_IN_TYPE_DESCRIPTOR(char));
std::unique_ptr<FTypeDescriptor> GUint8Descriptor (new MAKE_BUILT_IN_TYPE_DESCRIPTOR(unsigned char));
std::unique_ptr<FTypeDescriptor> GInt16Descriptor (new MAKE_BUILT_IN_TYPE_DESCRIPTOR(short));
std::unique_ptr<FTypeDescriptor> GUint16Descriptor(new MAKE_BUILT_IN_TYPE_DESCRIPTOR(unsigned short));
std::unique_ptr<FTypeDescriptor> GInt32Descriptor (new MAKE_BUILT_IN_TYPE_DESCRIPTOR(int));
std::unique_ptr<FTypeDescriptor> GUint32Descriptor(new MAKE_BUILT_IN_TYPE_DESCRIPTOR(unsigned int));
std::unique_ptr<FTypeDescriptor> GInt64Descriptor (new MAKE_BUILT_IN_TYPE_DESCRIPTOR(long long));
std::unique_ptr<FTypeDescriptor> GUint64Descriptor(new MAKE_BUILT_IN_TYPE_DESCRIPTOR(unsigned long long));
std::unique_ptr<FTypeDescriptor> GFloatDescriptor (new MAKE_BUILT_IN_TYPE_DESCRIPTOR(float));
std::unique_ptr<FTypeDescriptor> GDoubleDescriptor(new MAKE_BUILT_IN_TYPE_DESCRIPTOR(double));


std::string FTypeDescriptor::Dump()
{
	std::string DumpString = std::format("{:s} {:s}\n", GetTypeKind(), TypeName[0]);
	DumpString.append("{\n");
	for (size_t i = 0; i < Fields.size(); i++)
	{
		std::string FieldDecl;
		std::string FieldName = Fields[i].FieldName;
		std::string TypeName = Fields[i].TypeDescriptor->GetTypeName();
		size_t FieldOffset = Fields[i].FieldOffset;
		size_t Number = Fields[i].Number;
		if (Number != 1) FieldDecl = std::format("  {:s} {:s}[{:d}]; <Offset:{:d}>\n", TypeName, FieldName, Number, FieldOffset);
		else            FieldDecl = std::format("  {:s} {:s}; <Offset:{:d}>\n", TypeName, FieldName, FieldOffset);
		DumpString.append(FieldDecl);
	}
	DumpString.append("};\n");
	return DumpString;
}




FTypeDescriptorTable& FTypeDescriptorTable::Get()
{
	static std::function<FTypeDescriptorTable* ()> TypeDescriptorTableInitializer = []() -> FTypeDescriptorTable* {
		static FTypeDescriptorTable TypeDescriptorTable;
		// Register built-in type
		GVoidDescriptor->TypeId = TypeDescriptorTable.IdCounter++;
		TypeDescriptorTable.Descriptors.push_back(GVoidDescriptor.get());
		TypeDescriptorTable.NameToId.insert(std::make_pair("void", GVoidDescriptor->TypeId));

		TypeDescriptorTable.RegisterDescriptor("bool", GBoolDescriptor.get());
		TypeDescriptorTable.RegisterDescriptor("char", GInt8Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("unsigned char", GUint8Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("short", GInt16Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("unsigned short", GUint16Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("int", GInt32Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("unsigned int", GUint32Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("long long", GInt64Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("unsigned long long", GUint64Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("float", GFloatDescriptor.get());
		TypeDescriptorTable.RegisterDescriptor("double", GDoubleDescriptor.get());

		TypeDescriptorTable.RegisterDescriptor("Boolean", GBoolDescriptor.get());

		TypeDescriptorTable.RegisterDescriptor("Void", GVoidDescriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Bool", GBoolDescriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Int8", GInt8Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Uint8", GUint8Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Int16", GInt16Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Uint16", GUint16Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Int32", GInt32Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Uint32", GUint32Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Int64", GInt64Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Uint64", GUint64Descriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Float", GFloatDescriptor.get());
		TypeDescriptorTable.RegisterDescriptor("Double", GDoubleDescriptor.get());
#ifdef REFLECT_CODE_GENERATOR
		TypeDescriptorTable.RegisterDescriptor("_Bool", GBoolDescriptor.get());
#endif
		return &TypeDescriptorTable;
	};
	static FTypeDescriptorTable* TypeDescriptorTable = TypeDescriptorTableInitializer();
	return *TypeDescriptorTable;
}

FTypeDescriptor* FTypeDescriptorTable::GetDescriptor(const char* DescriptorName)
{
	auto NameToIdIterator = NameToId.find(DescriptorName);
	if (NameToIdIterator != NameToId.end())
		return Descriptors[abs(NameToIdIterator->second)];
	return nullptr;
}

FTypeDescriptor* FTypeDescriptorTable::GetDescriptor(int32_t DescriptorId)
{
	int32_t AbsDescriptorId = abs(DescriptorId);
	if (AbsDescriptorId < Descriptors.size())
		return Descriptors[AbsDescriptorId];
	return nullptr;
}

bool FTypeDescriptorTable::RegisterDescriptor(const char* TypeName, FTypeDescriptor* Descriptor) {
	assert(Descriptor);
	assert(std::end(NameToId) == NameToId.find(TypeName) || (std::end(NameToId) != NameToId.find(TypeName) && !Descriptor->HasTypeName(TypeName)));
	if (Descriptor->TypeId == 0) {
		Descriptor->TypeId = IdCounter++;
		Descriptors.push_back(Descriptor);
		NameToId.insert(std::make_pair(TypeName, Descriptor->TypeId));
	}
	else
	{
		Descriptor->Typedef(TypeName);
		NameToId.insert(std::make_pair(TypeName, -Descriptor->TypeId));
	}
	return true;
}