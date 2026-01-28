#pragma once
#include <cstdint>

typedef uint32_t TypeIndex;
typedef uint32_t TypeDefinitionIndex;
typedef uint32_t FieldIndex;
typedef uint32_t DefaultValueIndex;
typedef uint32_t DefaultValueDataIndex;
typedef uint32_t CustomAttributeIndex;
typedef uint32_t ParameterIndex;
typedef uint32_t MethodIndex;
typedef uint32_t GenericParameterIndex;
typedef uint32_t GenericContainerIndex;
typedef uint32_t PropertyIndex;
typedef uint32_t EventIndex;
typedef uint32_t GenericParameterConstraintIndex;
typedef uint32_t NestedTypeIndex;
typedef uint32_t InterfacesIndex;
typedef uint32_t VTableIndex;
typedef uint32_t InterfaceOffsetIndex;
typedef uint32_t RGCTXIndex;
typedef uint32_t StringIndex;
typedef uint32_t StringLiteralIndex;
typedef uint32_t GenericInstIndex;
typedef uint32_t MethodSpecIndex;
typedef uint32_t AssemblyIndex;
typedef uint32_t ImageIndex;

const int32_t kMetadataInvalidPointer = 0xFFFFFFFF;

struct Il2CppGlobalMetadataHeader {
    int32_t sanity;
    int32_t version;
    int32_t stringLiteralOffset;
    int32_t stringLiteralCount;
    int32_t stringLiteralDataOffset;
    int32_t stringLiteralDataCount;
    int32_t stringOffset;
    int32_t stringCount;
    int32_t eventsOffset;
    int32_t eventsCount;
    int32_t propertiesOffset;
    int32_t propertiesCount;
    int32_t methodsOffset;
    int32_t methodsCount;
    int32_t parameterDefaultValuesOffset;
    int32_t parameterDefaultValuesCount;
    int32_t fieldDefaultValuesOffset;
    int32_t fieldDefaultValuesCount;
    int32_t fieldAndParameterDefaultValueDataOffset;
    int32_t fieldAndParameterDefaultValueDataCount;
    int32_t fieldMarshaledSizesOffset;
    int32_t fieldMarshaledSizesCount;
    int32_t parametersOffset;
    int32_t parametersCount;
    int32_t fieldsOffset;
    int32_t fieldsCount;
    int32_t genericParametersOffset;
    int32_t genericParametersCount;
    int32_t genericParameterConstraintsOffset;
    int32_t genericParameterConstraintsCount;
    int32_t genericContainersOffset;
    int32_t genericContainersCount;
    int32_t nestedTypesOffset;
    int32_t nestedTypesCount;
    int32_t interfacesOffset;
    int32_t interfacesCount;
    int32_t vtableMethodsOffset;
    int32_t vtableMethodsCount;
    int32_t interfaceOffsetsOffset;
    int32_t interfaceOffsetsCount;
    int32_t typeDefinitionsOffset;
    int32_t typeDefinitionsCount;
    int32_t imagesOffset;
    int32_t imagesCount;
    int32_t assembliesOffset;
    int32_t assembliesCount;
    int32_t metadataUsageListsOffset;
    int32_t metadataUsageListsCount;
    int32_t metadataUsagePairsOffset;
    int32_t metadataUsagePairsCount;
    int32_t fieldRefsOffset;
    int32_t fieldRefsCount;
    int32_t referencedAssembliesOffset;
    int32_t referencedAssembliesCount;
    int32_t attributesInfoOffset;
    int32_t attributesInfoCount;
    int32_t attributeTypesOffset;
    int32_t attributeTypesCount;
    int32_t unresolvedVirtualCallParameterTypesOffset;
    int32_t unresolvedVirtualCallParameterTypesCount;
    int32_t unresolvedVirtualCallParameterRangesOffset;
    int32_t unresolvedVirtualCallParameterRangesCount;
    int32_t windowsRuntimeTypeNamesOffset;
    int32_t windowsRuntimeTypeNamesSize;
    int32_t exportedTypeDefinitionsOffset;
    int32_t exportedTypeDefinitionsCount;
};

struct Il2CppImageDefinition {
    StringIndex nameIndex;
    AssemblyIndex assemblyIndex;
    TypeDefinitionIndex typeStart;
    uint32_t typeCount;
    TypeDefinitionIndex exportedTypeStart;
    uint32_t exportedTypeCount;
    MethodIndex entryPointIndex;
    uint32_t token;
    int32_t customAttributeStart;
    int32_t customAttributeCount;
};

struct Il2CppTypeDefinition {
    StringIndex nameIndex;
    StringIndex namespaceIndex;
    TypeIndex byvalTypeIndex;
    TypeIndex byrefTypeIndex;
    TypeIndex declaringTypeIndex;
    TypeIndex parentIndex;
    TypeIndex elementTypeIndex;
    GenericContainerIndex genericContainerIndex;
    uint32_t flags;
    FieldIndex fieldStart;
    MethodIndex methodStart;
    EventIndex eventStart;
    PropertyIndex propertyStart;
    NestedTypeIndex nestedTypesStart;
    InterfacesIndex interfacesStart;
    VTableIndex vtableStart;
    InterfacesIndex interfaceOffsetsStart;
    uint16_t method_count;
    uint16_t property_count;
    uint16_t field_count;
    uint16_t event_count;
    uint16_t nested_type_count;
    uint16_t vtable_count;
    uint16_t interfaces_count;
    uint16_t interface_offsets_count;
    uint32_t bitfield;
    uint32_t token;
};

struct Il2CppMethodDefinition {
    StringIndex nameIndex;
    TypeDefinitionIndex declaringType;
    TypeIndex returnType;
    ParameterIndex parameterStart;
    GenericContainerIndex genericContainerIndex;
    uint32_t token;
    uint16_t flags;
    uint16_t iflags;
    uint16_t slot;
    uint16_t parameterCount;
};

struct Il2CppParameterDefinition {
    StringIndex nameIndex;
    uint32_t token;
    TypeIndex typeIndex;
};

struct Il2CppFieldDefinition {
    StringIndex nameIndex;
    TypeDefinitionIndex declaringType;
    TypeIndex typeIndex;
    uint32_t token;
};

struct Il2CppPropertyDefinition {
    StringIndex nameIndex;
    MethodIndex get;
    MethodIndex set;
    uint32_t attrs;
    uint32_t token;
};

struct Il2CppEventDefinition {
    StringIndex nameIndex;
    TypeDefinitionIndex declaringType;
    MethodIndex add;
    MethodIndex remove;
    MethodIndex raise;
    uint32_t token;
};

struct Il2CppAssemblyNameDefinition {
    StringIndex nameIndex;
    StringIndex cultureIndex;
    StringIndex publicKeyTokenIndex;
    StringIndex hashValueIndex;
    uint32_t hash_alg;
    int32_t hash_len;
    uint32_t flags;
    int32_t major;
    int32_t minor;
    int32_t build;
    int32_t revision;
    uint8_t public_key[8]; // Actually a pointer, but we'll handle it differently
    uint32_t token;
    uint8_t publicKeyIndex; // Index into the public key blob
};

struct Il2CppAssemblyDefinition {
    ImageIndex imageIndex;
    uint32_t token;
    int32_t referencedAssemblyStart;
    int32_t referencedAssemblyCount;
    Il2CppAssemblyNameDefinition aname;
};
