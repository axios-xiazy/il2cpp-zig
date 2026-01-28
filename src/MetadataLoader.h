#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <functional>
#include "il2cpp_structs.h"

std::string get_verbose_log();

struct ObfuscationProfile {
    std::string name;
    std::vector<std::string> detection_strings;
    std::function<bool(class MetadataLoader*)> detect_func;
    std::function<void(class MetadataLoader*)> deobfuscate_func;
};

struct DeobfuscationResult {
    bool success;
    std::string error_message;
    std::map<StringIndex, std::string> decrypted_strings;
    std::map<uint32_t, std::string> recovered_symbols;
};

class MetadataLoader {
public:
    MetadataLoader();
    ~MetadataLoader();

    bool LoadFile(const std::string& filePath);
    bool LoadLibrary(const std::string& libPath);
    void Process();
    const Il2CppGlobalMetadataHeader* getHeader() const { return header; }

    // Deobfuscation methods
    void DetectObfuscation();
    DeobfuscationResult ApplyDeobfuscation();
    void InitializeObfuscationProfiles();

    void DumpStrings(const std::string& outputPath);
    void DumpClasses(const std::string& outputPath);
    void DumpCS(const std::string& outputPath);
    void DumpScriptJSON(const std::string& outputPath);
    void DumpCSLimited(const std::string& outputPath, int maxElements);
    void DumpScriptJSONLimited(const std::string& outputPath, int maxElements);

    // Enhanced methods for deobfuscation
    std::string GetDecryptedString(StringIndex index);
    std::string GetRecoveredSymbol(uint32_t token);
    void AnalyzeBinaryForSymbols();
    void GenerateDeobfuscationReport();

    // Public accessors for testing/debugging
    const char* GetStringFromIndexPublic(StringIndex index) { return GetStringFromIndex(index); }

private:
    std::vector<char> fileBuffer;
    std::vector<char> libBuffer;
    const Il2CppGlobalMetadataHeader* header;
    uint64_t libBase;
    size_t metadataOffset;

    // Deobfuscation data
    std::vector<ObfuscationProfile> obfuscationProfiles;
    std::map<StringIndex, std::string> decryptedStrings;
    std::map<uint32_t, std::string> recoveredSymbols;
    bool isObfuscated;

    const char* GetStringFromIndex(StringIndex index);
    void DetectUnityVersion();
    void ApplyStringDecryption();
    void ApplyControlFlowRestoration();
    void ApplySymbolRecovery();

    // String decryption helper methods
    std::string tryXORDecryption(const std::string& encrypted);
    std::string tryCaesarDecryption(const std::string& encrypted);
    std::string tryBase64Decryption(const std::string& encrypted);
    std::string tryRot13Decryption(const std::string& encrypted);
    std::string tryCustomPatternDecryption(const std::string& encrypted);
    std::string processObfuscatedString(const std::string& input);
    bool hasCommonStringPatterns(const std::string& str);
    std::string base64_decode(const std::string& input);

    // Control flow restoration methods
    void identifyDispatcherPatterns();
    void identifySwitchStatementObfuscation();
    void identifyJunkCode();
    void restoreLinearControlFlow();

    // Symbol recovery methods
    void recoverClassNames();
    void recoverMethodNames();
    void recoverFieldNames();
    void recoverPropertyNames();
    bool isObfuscatedName(const std::string& name);
    std::string inferClassName(TypeDefinitionIndex index, const Il2CppTypeDefinition* typeDef);
    std::string inferMethodName(MethodIndex index, const Il2CppMethodDefinition* methodDef);
    std::string inferFieldName(FieldIndex index, const Il2CppFieldDefinition* fieldDef);
    std::string inferPropertyName(PropertyIndex index, const Il2CppPropertyDefinition* propDef);
    std::string enhanceClassName(const std::string& original, TypeDefinitionIndex index);
    std::string enhanceMethodName(const std::string& original, MethodIndex index);
    void crossReferenceWithBinary();

    // New methods for proper IL2CPP metadata access
    const Il2CppTypeDefinition* GetTypeDefinition(TypeDefinitionIndex index);
    const Il2CppMethodDefinition* GetMethodDefinition(MethodIndex index);
    const Il2CppFieldDefinition* GetFieldDefinition(FieldIndex index);
    const Il2CppPropertyDefinition* GetPropertyDefinition(PropertyIndex index);
};
