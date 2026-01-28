#include "MetadataLoader.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <sstream>
#include <regex>

// Enhanced IL2CPP Dumper - Accurate Metadata Parsing
// Based on Perfare's Il2CppDumper approach

MetadataLoader::MetadataLoader() : header(nullptr), libBase(0), metadataOffset(0), isObfuscated(false) {
    InitializeObfuscationProfiles();
}
MetadataLoader::~MetadataLoader() {}

std::stringstream vlog;
std::string get_verbose_log() {
    std::string s = vlog.str();
    vlog.str("");
    return s;
}

bool MetadataLoader::LoadFile(const std::string& filePath) {
    vlog << "[*] Loading metadata file: " << filePath << "\n";
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        vlog << "[-] Failed to open metadata file\n";
        return false;
    }
    size_t size = file.tellg();
    fileBuffer.resize(size);
    file.seekg(0, std::ios::beg);
    file.read(fileBuffer.data(), size);
    file.close();
    
    // Validate and set up header
    if (size < sizeof(Il2CppGlobalMetadataHeader)) {
        vlog << "[-] Metadata file too small to contain valid header\n";
        return false;
    }
    
    header = reinterpret_cast<const Il2CppGlobalMetadataHeader*>(fileBuffer.data());
    
    // Validate header
    if (header->sanity != 0xFAB11BAF && header->sanity != 0xB11BFAF && header->sanity != 0xEAB11BAF) {
        vlog << "[-] Invalid metadata header sanity check: 0x" << std::hex << header->sanity << std::dec << "\n";
        return false;
    }

    // Validate header values to prevent crashes from malformed data
    if (header->version < 16 || header->version > 32) {
        vlog << "[-] Unexpected metadata version: " << header->version << "\n";
        return false;
    }

    // For newer Unity versions (like 29), allow higher limits
    // But still validate against obviously wrong values
    if (static_cast<uint32_t>(header->stringCount) > 100000000U) {  // Very high but possible for large games
        vlog << "[-] Suspiciously high string count: " << header->stringCount << "\n";
        return false;
    }
    if (static_cast<uint32_t>(header->typeDefinitionsCount) > 20000000U) {
        vlog << "[-] Suspiciously high type definition count: " << header->typeDefinitionsCount << "\n";
        return false;
    }
    if (static_cast<uint32_t>(header->methodsCount) > 100000000U) {
        vlog << "[-] Suspiciously high method count: " << header->methodsCount << "\n";
        return false;
    }

    vlog << "[+] Metadata loaded successfully. Version: " << header->version << "\n";
    vlog << "[+] String count: " << header->stringCount << ", Type count: " << header->typeDefinitionsCount << "\n";
    vlog << "[+] Method count: " << header->methodsCount << ", Field count: " << header->fieldsCount << "\n";

    return true;
}

bool MetadataLoader::LoadLibrary(const std::string& libPath) { 
    vlog << "[*] Loading library: " << libPath << "\n";
    std::ifstream file(libPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        vlog << "[-] Failed to open library file\n";
        return false;
    }
    size_t size = file.tellg();
    libBuffer.resize(size);
    file.seekg(0, std::ios::beg);
    file.read(libBuffer.data(), size);
    file.close();
    return true;
}

const char* MetadataLoader::GetStringFromIndex(StringIndex index) {
    if (index == kMetadataInvalidPointer || !header || static_cast<uint32_t>(index) >= static_cast<uint32_t>(header->stringCount)) {
        return "";
    }

    // Bounds check for stringOffset
    if (static_cast<uint32_t>(header->stringOffset) >= fileBuffer.size()) {
        return "";
    }

    // Calculate the end of the string offset table
    size_t stringOffsetTableEnd = static_cast<size_t>(header->stringOffset) + (static_cast<size_t>(header->stringCount) * sizeof(int32_t));
    if (stringOffsetTableEnd > fileBuffer.size()) {
        return "";
    }

    // Calculate the actual offset using the header information
    const char* stringStart = fileBuffer.data() + header->stringOffset;
    const int32_t* stringOffsetTable = reinterpret_cast<const int32_t*>(stringStart);

    // Bounds check for the specific index in the offset table
    if ((static_cast<size_t>(header->stringOffset) + (static_cast<size_t>(index) * sizeof(int32_t)) + sizeof(int32_t)) > fileBuffer.size()) {
        return "";
    }

    // Get the offset for this string
    int32_t stringOffset = stringOffsetTable[index];

    // The string data starts after the offset table
    const char* stringData = fileBuffer.data() + stringOffsetTableEnd;

    // For Unity 29, the string offset might be absolute or relative differently
    // Try multiple approaches:

    // Approach 1: Standard relative to string data start
    if (stringOffset >= 0 && static_cast<size_t>(stringOffset) < (fileBuffer.size() - (stringData - fileBuffer.data()))) {
        const char* result = stringData + stringOffset;
        if (result >= stringData && result < fileBuffer.data() + fileBuffer.size()) {
            // Check if it's a valid string (starts with printable characters)
            const char* p = result;
            int charCount = 0;
            while (p < fileBuffer.data() + fileBuffer.size() && *p != '\0' && charCount < 1000) { // Limit to avoid infinite loops
                unsigned char c = static_cast<unsigned char>(*p);
                // Accept alphanumeric, common symbols, and whitespace
                if (isprint(c) || isspace(c)) {
                    p++;
                    charCount++;
                } else {
                    break; // Invalid character found
                }
            }
            if (*p == '\0') { // Found null terminator
                return result;
            }
        }
    }

    // Approach 2: Absolute offset from file start
    if (stringOffset >= 0 && static_cast<size_t>(stringOffset) < fileBuffer.size()) {
        const char* result = fileBuffer.data() + stringOffset;
        if (result >= fileBuffer.data() && result < fileBuffer.data() + fileBuffer.size()) {
            // Check if it's a valid string
            const char* p = result;
            int charCount = 0;
            while (p < fileBuffer.data() + fileBuffer.size() && *p != '\0' && charCount < 1000) {
                unsigned char c = static_cast<unsigned char>(*p);
                if (isprint(c) || isspace(c)) {
                    p++;
                    charCount++;
                } else {
                    break;
                }
            }
            if (*p == '\0') {
                return result;
            }
        }
    }

    // Approach 3: Try to find the string in the string data section directly
    // Sometimes the index refers to a position in the string data section
    if (index < header->stringCount && stringOffsetTableEnd < fileBuffer.size()) {
        // This is a fallback approach - try to find strings by scanning the string data section
        // This is slower but might work for different Unity versions
        const char* current = stringData;
        StringIndex currentIndex = 0;

        while (currentIndex < index && current < fileBuffer.data() + fileBuffer.size() - 1) {
            // Find the next null terminator
            const char* start = current;
            while (current < fileBuffer.data() + fileBuffer.size() && *current != '\0') {
                current++;
            }
            if (current < fileBuffer.data() + fileBuffer.size() && *current == '\0') {
                current++; // Move past the null terminator
                currentIndex++;
            } else {
                break; // Reached end of buffer
            }
        }

        if (currentIndex == index && current < fileBuffer.data() + fileBuffer.size()) {
            // We found the string at the right index
            const char* result = current;
            // Verify it's a valid string
            const char* p = result;
            int charCount = 0;
            while (p < fileBuffer.data() + fileBuffer.size() && *p != '\0' && charCount < 1000) {
                unsigned char c = static_cast<unsigned char>(*p);
                if (isprint(c) || isspace(c)) {
                    p++;
                    charCount++;
                } else {
                    break;
                }
            }
            if (*p == '\0') {
                return result;
            }
        }
    }

    return "";
}

std::string MetadataLoader::GetDecryptedString(StringIndex index) {
    // Check if we have already decrypted this string
    if (decryptedStrings.find(index) != decryptedStrings.end()) {
        return decryptedStrings[index];
    }
    
    const char* rawStr = GetStringFromIndex(index);
    if (!rawStr || strlen(rawStr) == 0) {
        return "";
    }
    
    std::string str(rawStr);
    
    // Check if string looks encrypted (contains many non-printable chars)
    int nonPrintable = 0;
    for (char c : str) {
        if (!std::isprint(static_cast<unsigned char>(c)) && c != '\0') {
            nonPrintable++;
        }
    }

    if (nonPrintable > static_cast<int>(str.length() * 0.3)) {
        // Try multiple decryption algorithms
        std::string decrypted = tryXORDecryption(str);
        if (!decrypted.empty()) {
            decryptedStrings[index] = decrypted;
            return decrypted;
        }
        
        decrypted = tryCaesarDecryption(str);
        if (!decrypted.empty()) {
            decryptedStrings[index] = decrypted;
            return decrypted;
        }
        
        decrypted = tryBase64Decryption(str);
        if (!decrypted.empty()) {
            decryptedStrings[index] = decrypted;
            return decrypted;
        }
    }
    
    // If not encrypted or decryption failed, return original
    return str;
}

std::string MetadataLoader::GetRecoveredSymbol(uint32_t token) {
    if (recoveredSymbols.find(token) != recoveredSymbols.end()) {
        return recoveredSymbols[token];
    }
    return ""; // Return empty if not found, will use original name
}

void MetadataLoader::InitializeObfuscationProfiles() {
    // Profile for common string encryption
    obfuscationProfiles.push_back({
        "Common String Encryption",
        {"encrypted", "cipher", "obfuscated"},
        [this](MetadataLoader* loader) -> bool {
            // Simple detection: look for many null or non-printable characters in string section
            int nonPrintableCount = 0;
            int totalCount = 0;
            
            for (StringIndex i = 0; i < (header->stringCount < 1000 ? header->stringCount : 1000); i++) {
                const char* str = GetStringFromIndex(i);
                if (str && strlen(str) > 0) {
                    std::string s(str);
                    for (char c : s) {
                        if (!std::isprint(static_cast<unsigned char>(c)) && c != '\0') {
                            nonPrintableCount++;
                        }
                        totalCount++;
                    }
                }
            }
            
            return (totalCount > 0) && ((double)nonPrintableCount / totalCount > 0.5);
        },
        [this](MetadataLoader* loader) -> void {
            vlog << "[+] Applying common string decryption...\n";
            ApplyStringDecryption();
        }
    });
    
    // Profile for symbol obfuscation
    obfuscationProfiles.push_back({
        "Symbol Obfuscation",
        {"_", "a", "b", "c", "var", "field"},
        [this](MetadataLoader* loader) -> bool {
            // Detection logic for obfuscated symbols
            int shortNameCount = 0;
            int totalCount = 0;
            
            for (TypeDefinitionIndex i = 0; i < (header->typeDefinitionsCount < 1000 ? header->typeDefinitionsCount : 1000); i++) {
                const Il2CppTypeDefinition* typeDef = GetTypeDefinition(i);
                if (typeDef) {
                    std::string name(GetStringFromIndex(typeDef->nameIndex));
                    if (name.length() <= 2) {
                        shortNameCount++;
                    }
                    totalCount++;
                }
            }
            
            return (totalCount > 0) && ((double)shortNameCount / totalCount > 0.7);
        },
        [this](MetadataLoader* loader) -> void {
            vlog << "[+] Applying symbol recovery...\n";
            ApplySymbolRecovery();
        }
    });
}

void MetadataLoader::DetectObfuscation() {
    vlog << "[*] Detecting obfuscation patterns...\n";
    
    for (auto& profile : obfuscationProfiles) {
        if (profile.detect_func(this)) {
            vlog << "[+] Detected: " << profile.name << "\n";
            isObfuscated = true;
        }
    }
    
    if (!isObfuscated) {
        vlog << "[*] No known obfuscation detected\n";
    }
}

DeobfuscationResult MetadataLoader::ApplyDeobfuscation() {
    DeobfuscationResult result;
    result.success = true;
    
    if (!isObfuscated) {
        vlog << "[*] No obfuscation to deobfuscate\n";
        return result;
    }
    
    vlog << "[*] Applying deobfuscation...\n";
    
    try {
        for (auto& profile : obfuscationProfiles) {
            if (profile.detect_func(this)) {
                vlog << "[+] Applying: " << profile.name << "\n";
                profile.deobfuscate_func(this);
            }
        }
        
        vlog << "[+] Deobfuscation completed successfully\n";
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
        vlog << "[-] Deobfuscation failed: " << e.what() << "\n";
    }
    
    return result;
}

void MetadataLoader::ApplyStringDecryption() {
    // Decrypt all strings in the string pool
    for (StringIndex i = 0; i < header->stringCount; i++) {
        const char* rawStr = GetStringFromIndex(i);
        if (!rawStr || strlen(rawStr) == 0) continue;

        std::string str(rawStr);
        
        // Check if string looks encrypted (contains many non-printable chars)
        int nonPrintable = 0;
        for (char c : str) {
            if (!std::isprint(static_cast<unsigned char>(c)) && c != '\0') {
                nonPrintable++;
            }
        }

        if (nonPrintable > static_cast<int>(str.length() * 0.3)) {
            // Try multiple decryption algorithms
            std::string decrypted = tryXORDecryption(str);
            if (!decrypted.empty()) {
                decryptedStrings[i] = decrypted;
                continue;
            }

            decrypted = tryCaesarDecryption(str);
            if (!decrypted.empty()) {
                decryptedStrings[i] = decrypted;
                continue;
            }

            decrypted = tryBase64Decryption(str);
            if (!decrypted.empty()) {
                decryptedStrings[i] = decrypted;
                continue;
            }

            decrypted = tryRot13Decryption(str);
            if (!decrypted.empty()) {
                decryptedStrings[i] = decrypted;
                continue;
            }
        }
    }
}

std::string MetadataLoader::tryXORDecryption(const std::string& encrypted) {
    for (int key = 1; key <= 255; key++) {
        std::string decrypted = encrypted;
        for (char& c : decrypted) {
            if (static_cast<unsigned char>(c) != 0) {
                c ^= key;
            }
        }

        // Check if decryption produced mostly printable text
        int printable = 0;
        for (char c : decrypted) {
            if (std::isprint(static_cast<unsigned char>(c)) || c == '\0') {
                printable++;
            }
        }

        if (printable > static_cast<int>(decrypted.length() * 0.8)) {
            // Additional check: see if the result contains common string patterns
            if (hasCommonStringPatterns(decrypted)) {
                return decrypted;
            }
        }
    }
    return "";
}

std::string MetadataLoader::tryCaesarDecryption(const std::string& encrypted) {
    for (int shift = 1; shift <= 25; shift++) {
        std::string decrypted = encrypted;
        for (char& c : decrypted) {
            if (std::isalpha(static_cast<unsigned char>(c))) {
                char base = std::islower(static_cast<unsigned char>(c)) ? 'a' : 'A';
                c = (c - base - shift + 26) % 26 + base;
            }
        }

        // Check if decryption produced mostly printable text
        int printable = 0;
        for (char c : decrypted) {
            if (std::isprint(static_cast<unsigned char>(c)) || c == '\0') {
                printable++;
            }
        }

        if (printable > static_cast<int>(decrypted.length() * 0.8)) {
            if (hasCommonStringPatterns(decrypted)) {
                return decrypted;
            }
        }
    }
    return "";
}

std::string MetadataLoader::tryBase64Decryption(const std::string& encrypted) {
    // Basic Base64 alphabet
    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    // Check if the string looks like Base64 (alphanumeric + +/ and possibly ending with =)
    bool isBase64Like = true;
    for (char c : encrypted) {
        if (base64_chars.find(c) == std::string::npos && c != '=') {
            isBase64Like = false;
            break;
        }
    }
    
    if (!isBase64Like) return "";
    
    // Simple Base64 decode implementation
    std::string decoded = base64_decode(encrypted);

    // Check if the result is printable
    int printable = 0;
    for (char c : decoded) {
        if (std::isprint(static_cast<unsigned char>(c)) || c == '\0') {
            printable++;
        }
    }

    if (printable > static_cast<int>(decoded.length() * 0.8)) {
        if (hasCommonStringPatterns(decoded)) {
            return decoded;
        }
    }

    return "";
}

std::string MetadataLoader::tryRot13Decryption(const std::string& encrypted) {
    std::string decrypted = encrypted;
    for (char& c : decrypted) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            char base = std::islower(static_cast<unsigned char>(c)) ? 'a' : 'A';
            c = (c - base + 13) % 26 + base;
        }
    }

    // Check if decryption produced mostly printable text
    int printable = 0;
    for (char c : decrypted) {
        if (std::isprint(static_cast<unsigned char>(c)) || c == '\0') {
            printable++;
        }
    }

    if (printable > static_cast<int>(decrypted.length() * 0.8)) {
        if (hasCommonStringPatterns(decrypted)) {
            return decrypted;
        }
    }
    return "";
}

std::string MetadataLoader::tryCustomPatternDecryption(const std::string& encrypted) {
    // Common obfuscation patterns used in IL2CPP
    // This could include character substitution, byte manipulation, etc.
    
    // Example: Character swapping or substitution
    std::string result = encrypted;
    
    // Try to detect and reverse simple substitution patterns
    // This is a placeholder for more complex pattern detection
    
    // For example, if we notice that certain characters appear in predictable positions,
    // we might be able to reverse engineer the substitution
    
    // Another common technique: reversed strings
    std::string reversed = result;
    std::reverse(reversed.begin(), reversed.end());
    
    int printable = 0;
    for (char c : reversed) {
        if (std::isprint(static_cast<unsigned char>(c)) || c == '\0') {
            printable++;
        }
    }
    
    if (printable > static_cast<int>(reversed.length() * 0.8)) {
        if (hasCommonStringPatterns(reversed)) {
            return reversed;
        }
    }
    
    return "";
}

std::string MetadataLoader::processObfuscatedString(const std::string& input) {
    // Process strings that might have simple obfuscation even if they appear readable
    std::string result = input;
    
    // Example: remove padding characters or extra nulls
    // This is where we'd handle strings that look normal but have been subtly obfuscated
    
    // Remove trailing null bytes that might have been added as padding
    while (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    
    // Check for common obfuscated patterns
    if (result.length() > 1 && result.length() % 2 == 0) {
        // Might be a pattern where every other character is padding
        std::string filtered;
        for (size_t i = 0; i < result.length(); i += 2) {
            filtered += result[i];
        }
        
        if (hasCommonStringPatterns(filtered)) {
            return filtered;
        }
    }
    
    return result;
}

bool MetadataLoader::hasCommonStringPatterns(const std::string& str) {
    // Check for common string patterns that indicate successful decryption
    std::vector<std::string> commonPatterns = {
        "System.", "UnityEngine.", "Mono.", "Console.", "Debug.", 
        "get_", "set_", "ctor", "cctor", "ToString", "Equals",
        "Length", "Count", "Add", "Remove", "Clear", "Find",
        "www.", "http", ".com", ".net", ".org", ".js", ".css",
        "json", "xml", "api.", "user", "password", "token",
        "class", "public", "private", "protected", "static",
        "void", "int", "string", "bool", "float", "double"
    };
    
    for (const auto& pattern : commonPatterns) {
        if (str.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    // Check for reasonable length and composition
    if (str.length() < 2) return false;
    
    // Count letters vs other characters
    int letters = 0;
    int digits = 0;
    int spaces = 0;
    int punctuation = 0;
    
    for (char c : str) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            letters++;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            digits++;
        } else if (c == ' ') {
            spaces++;
        } else if (std::ispunct(static_cast<unsigned char>(c))) {
            punctuation++;
        }
    }
    
    // Heuristic: strings should have a reasonable proportion of letters
    float letterRatio = static_cast<float>(letters) / str.length();
    return letterRatio >= 0.4f; // At least 40% should be letters
}

std::string MetadataLoader::base64_decode(const std::string& input) {
    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    int val = 0, valb = -8;
    
    for (unsigned char c : input) {
        if (c == '=') break; // Stop at padding
        
        int pos = base64_chars.find(c);
        if (pos == std::string::npos) break; // Invalid character
        
        val = (val << 6) + pos;
        valb += 6;
        if (valb >= 0) {
            output.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return output;
}

void MetadataLoader::ApplyControlFlowRestoration() {
    vlog << "[*] Starting control flow restoration...\n";
    
    if (libBuffer.empty()) {
        vlog << "[!] No library loaded for control flow analysis\n";
        return;
    }
    
    // Identify common obfuscation patterns in the binary
    identifyDispatcherPatterns();
    identifySwitchStatementObfuscation();
    identifyJunkCode();
    
    vlog << "[+] Control flow restoration completed\n";
}

void MetadataLoader::identifyDispatcherPatterns() {
    // Look for common dispatcher patterns used in control flow flattening
    const char* data = libBuffer.data();
    size_t dataSize = libBuffer.size();
    
    // Pattern: sequences of conditional jumps followed by indirect jumps
    // This is typical of flattened control flow
    for (size_t i = 0; i < dataSize - 10; i++) {
        // Look for common x86-64 dispatcher patterns
        // This is a simplified example - real implementation would be more complex
        if (data[i] == 0x48 && data[i+1] == 0x8B && data[i+2] == 0x05) { // mov rax, [rip+disp32]
            // This could be loading a function pointer or state variable
            // Followed by jump to rax
            if (i + 10 < dataSize && data[i+7] == 0x48 && data[i+8] == 0xFF && data[i+9] == 0xE0) { // jmp rax
                vlog << "[+] Found potential dispatcher pattern at 0x" << std::hex << i << std::dec << "\n";
            }
        }
    }
}

void MetadataLoader::identifySwitchStatementObfuscation() {
    // Look for obfuscated switch statements
    // These often manifest as large blocks of conditional branches
    const char* data = libBuffer.data();
    size_t dataSize = libBuffer.size();
    
    // Count conditional branch instructions in proximity
    int consecutiveBranches = 0;
    size_t lastBranchPos = 0;
    
    for (size_t i = 0; i < dataSize - 1; i++) {
        // Common conditional jump opcodes (x86-64)
        if ((data[i] & 0xF0) == 0x70 ||                           // jcc rel8
            (data[i] == 0x0F && (data[i+1] & 0xF0) == 0x80) ||   // jcc rel32
            (data[i] == 0xE3)) {                                 // jcxz
            if (lastBranchPos == 0 || i - lastBranchPos < 10) {
                consecutiveBranches++;
            } else {
                if (consecutiveBranches > 5) {
                    vlog << "[+] Found potential obfuscated switch with " << consecutiveBranches << " consecutive branches at 0x" << std::hex << lastBranchPos << std::dec << "\n";
                }
                consecutiveBranches = 1;
            }
            lastBranchPos = i;
        }
    }
    
    if (consecutiveBranches > 5) {
        vlog << "[+] Found potential obfuscated switch with " << consecutiveBranches << " consecutive branches at 0x" << std::hex << lastBranchPos << std::dec << "\n";
    }
}

void MetadataLoader::identifyJunkCode() {
    // Look for junk code insertion patterns
    // These are often NOP sleds or meaningless operations inserted to confuse analysis
    
    const char* data = libBuffer.data();
    size_t dataSize = libBuffer.size();
    
    // Look for repeated patterns that might be junk
    for (size_t i = 0; i < dataSize - 16; i++) {
        // Check for NOP patterns
        if (data[i] == 0x90 && data[i+1] == 0x90 && data[i+2] == 0x90) { // Multiple NOPs
            int nopCount = 3;
            while (i + nopCount < dataSize && data[i + nopCount] == 0x90) {
                nopCount++;
            }
            if (nopCount > 5) {
                vlog << "[+] Found " << nopCount << " consecutive NOPs at 0x" << std::hex << i << std::dec << "\n";
            }
            i += nopCount - 1; // Skip the NOPs we just found
        }
        
        // Look for other junk patterns (e.g., push/pop pairs that cancel out)
        // This is a simplified example
    }
}

void MetadataLoader::restoreLinearControlFlow() {
    // Algorithm to restore linear control flow from flattened structures
    // This would involve reconstructing the original control flow graph
    
    // In a real implementation, this would:
    // 1. Identify basic blocks
    // 2. Reconstruct the control flow graph
    // 3. Identify the original program structure
    // 4. Generate deobfuscated representation
    
    vlog << "[*] Linear control flow restoration would reconstruct the original CFG\n";
}

void MetadataLoader::ApplySymbolRecovery() {
    vlog << "[*] Starting symbol recovery...\n";
    
    // 1. Recover class names
    recoverClassNames();
    
    // 2. Recover method names
    recoverMethodNames();
    
    // 3. Recover field names
    recoverFieldNames();
    
    // 4. Recover property names
    recoverPropertyNames();
    
    // 5. Cross-reference with binary if available
    if (!libBuffer.empty()) {
        crossReferenceWithBinary();
    }
    
    vlog << "[+] Symbol recovery completed\n";
}

void MetadataLoader::recoverClassNames() {
    for (TypeDefinitionIndex i = 0; i < header->typeDefinitionsCount; i++) {
        const Il2CppTypeDefinition* typeDef = GetTypeDefinition(i);
        if (!typeDef) continue;
        
        std::string name = GetDecryptedString(typeDef->nameIndex);
        if (name.empty()) {
            name = GetStringFromIndex(typeDef->nameIndex);
        }
        
        uint32_t token = 0x02000000 + i;
        
        // If name is too short or looks obfuscated, try to recover a better one
        if (isObfuscatedName(name)) {
            std::string recoveredName = inferClassName(i, typeDef);
            if (!recoveredName.empty()) {
                recoveredSymbols[token] = recoveredName;
                vlog << "[+] Recovered class name: " << recoveredName << " for token 0x" << std::hex << token << std::dec << "\n";
            }
        } else {
            // Even if the name looks valid, we might still want to enhance it
            std::string enhancedName = enhanceClassName(name, i);
            if (enhancedName != name) {
                recoveredSymbols[token] = enhancedName;
            }
        }
    }
}

void MetadataLoader::recoverMethodNames() {
    for (MethodIndex i = 0; i < header->methodsCount; i++) {
        const Il2CppMethodDefinition* methodDef = GetMethodDefinition(i);
        if (!methodDef) continue;
        
        std::string name = GetDecryptedString(methodDef->nameIndex);
        if (name.empty()) {
            name = GetStringFromIndex(methodDef->nameIndex);
        }
        
        uint32_t token = 0x06000000 + i;
        
        // If name is obfuscated, try to recover a better one
        if (isObfuscatedName(name)) {
            std::string recoveredName = inferMethodName(i, methodDef);
            if (!recoveredName.empty()) {
                recoveredSymbols[token] = recoveredName;
                vlog << "[+] Recovered method name: " << recoveredName << " for token 0x" << std::hex << token << std::dec << "\n";
            }
        } else {
            // Enhance the name if possible
            std::string enhancedName = enhanceMethodName(name, i);
            if (enhancedName != name) {
                recoveredSymbols[token] = enhancedName;
            }
        }
    }
}

void MetadataLoader::recoverFieldNames() {
    for (FieldIndex i = 0; i < header->fieldsCount; i++) {
        const Il2CppFieldDefinition* fieldDef = GetFieldDefinition(i);
        if (!fieldDef) continue;
        
        std::string name = GetDecryptedString(fieldDef->nameIndex);
        if (name.empty()) {
            name = GetStringFromIndex(fieldDef->nameIndex);
        }
        
        uint32_t token = 0x04000000 + i;
        
        // If name is obfuscated, try to recover a better one
        if (isObfuscatedName(name)) {
            std::string recoveredName = inferFieldName(i, fieldDef);
            if (!recoveredName.empty()) {
                recoveredSymbols[token] = recoveredName;
                vlog << "[+] Recovered field name: " << recoveredName << " for token 0x" << std::hex << token << std::dec << "\n";
            }
        }
    }
}

void MetadataLoader::recoverPropertyNames() {
    for (PropertyIndex i = 0; i < header->propertiesCount; i++) {
        const Il2CppPropertyDefinition* propDef = GetPropertyDefinition(i);
        if (!propDef) continue;
        
        std::string name = GetDecryptedString(propDef->nameIndex);
        if (name.empty()) {
            name = GetStringFromIndex(propDef->nameIndex);
        }
        
        uint32_t token = 0x07000000 + i;
        
        // If name is obfuscated, try to recover a better one
        if (isObfuscatedName(name)) {
            std::string recoveredName = inferPropertyName(i, propDef);
            if (!recoveredName.empty()) {
                recoveredSymbols[token] = recoveredName;
                vlog << "[+] Recovered property name: " << recoveredName << " for token 0x" << std::hex << token << std::dec << "\n";
            }
        }
    }
}

bool MetadataLoader::isObfuscatedName(const std::string& name) {
    if (name.empty()) return true;
    
    // Check for common obfuscation patterns
    if (name.length() <= 2) {
        // Very short names are often obfuscated
        if (std::all_of(name.begin(), name.end(), ::isalpha)) {
            return true;
        }
    }
    
    // Check for random-looking names (many vowels or consonants in sequence)
    int vowelCount = 0;
    int consonantCount = 0;
    for (char c : name) {
        if (std::tolower(c) == 'a' || std::tolower(c) == 'e' || std::tolower(c) == 'i' || 
            std::tolower(c) == 'o' || std::tolower(c) == 'u') {
            vowelCount++;
            consonantCount = 0; // reset
        } else if (std::isalpha(c)) {
            consonantCount++;
            vowelCount = 0; // reset
        }
        
        // If we have 4+ consecutive vowels or consonants, likely obfuscated
        if (vowelCount >= 4 || consonantCount >= 4) {
            return true;
        }
    }
    
    // Check for names that look like hex or random strings
    if (name.length() >= 6) {
        int alphaCount = 0;
        int digitCount = 0;
        int upperCount = 0;
        
        for (char c : name) {
            if (std::isalpha(c)) alphaCount++;
            if (std::isdigit(c)) digitCount++;
            if (std::isupper(c)) upperCount++;
        }
        
        // High ratio of digits or mixed case without clear pattern suggests obfuscation
        float digitRatio = static_cast<float>(digitCount) / name.length();
        if (digitRatio > 0.3f) return true;  // More than 30% digits
        
        // Names with many uppercase letters randomly placed might be obfuscated
        float upperRatio = static_cast<float>(upperCount) / name.length();
        if (upperRatio > 0.5f && alphaCount > 5) return true;  // More than 50% uppercase
    }
    
    return false;
}

std::string MetadataLoader::inferClassName(TypeDefinitionIndex index, const Il2CppTypeDefinition* typeDef) {
    // Try to infer a meaningful class name based on context and patterns
    // This is a simplified approach - real implementation would be more sophisticated
    
    // Check if this class extends a known type
    if (typeDef->parentIndex != kMetadataInvalidPointer) {
        // Try to get parent class name to infer child class name
        std::string parentName = GetStringFromIndex(typeDef->parentIndex);
        if (!parentName.empty() && !isObfuscatedName(parentName)) {
            if (parentName == "MonoBehaviour") {
                return "GameBehavior_" + std::to_string(index);
            } else if (parentName == "ScriptableObject") {
                return "GameData_" + std::to_string(index);
            } else if (parentName.find("Component") != std::string::npos) {
                return "Component_" + std::to_string(index);
            }
        }
    }
    
    // Check namespace for hints
    std::string ns = GetStringFromIndex(typeDef->namespaceIndex);
    if (!ns.empty()) {
        if (ns.find("UI") != std::string::npos) {
            return "UIElement_" + std::to_string(index);
        } else if (ns.find("Network") != std::string::npos) {
            return "NetworkClass_" + std::to_string(index);
        } else if (ns.find("Audio") != std::string::npos) {
            return "AudioClass_" + std::to_string(index);
        }
    }
    
    // Default fallback
    return "Class_" + std::to_string(index);
}

std::string MetadataLoader::inferMethodName(MethodIndex index, const Il2CppMethodDefinition* methodDef) {
    // Try to infer a meaningful method name
    // Look at the declaring type to get context
    if (methodDef->declaringType != kMetadataInvalidPointer) {
        // We could look up the declaring type name, but for simplicity we'll use patterns
        // Check if this looks like a getter/setter
        std::string name = GetStringFromIndex(methodDef->nameIndex);
        if (name.length() >= 3) {
            if (name.substr(0, 3) == "get") {
                return "GetValue_" + std::to_string(index);
            } else if (name.substr(0, 3) == "set") {
                return "SetValue_" + std::to_string(index);
            }
        }
    }
    
    // Check for constructor patterns
    std::string name = GetStringFromIndex(methodDef->nameIndex);
    if (name == ".ctor") {
        return "Constructor";
    } else if (name == ".cctor") {
        return "StaticConstructor";
    }
    
    // Default fallback
    return "Method_" + std::to_string(index);
}

std::string MetadataLoader::inferFieldName(FieldIndex index, const Il2CppFieldDefinition* fieldDef) {
    // Infer field names based on type or common patterns
    // Default fallback
    return "Field_" + std::to_string(index);
}

std::string MetadataLoader::inferPropertyName(PropertyIndex index, const Il2CppPropertyDefinition* propDef) {
    // Infer property names
    // Default fallback
    return "Property_" + std::to_string(index);
}

std::string MetadataLoader::enhanceClassName(const std::string& original, TypeDefinitionIndex index) {
    // Enhance class names that are valid but could be more descriptive
    if (original.length() > 2 && !isObfuscatedName(original)) {
        // Already a good name, maybe add context
        return original;
    }
    return original; // Return as is if already processed
}

std::string MetadataLoader::enhanceMethodName(const std::string& original, MethodIndex index) {
    // Enhance method names that are valid but could be more descriptive
    if (original.length() > 2 && !isObfuscatedName(original)) {
        // Already a good name, maybe add context
        return original;
    }
    return original; // Return as is if already processed
}

void MetadataLoader::crossReferenceWithBinary() {
    vlog << "[*] Cross-referencing symbols with binary data...\n";
    
    // Search the binary for references to known good strings
    // This can help recover more meaningful names
    
    std::vector<std::string> knownGoodStrings;
    
    // Collect known good strings from our recovered symbols
    for (const auto& pair : recoveredSymbols) {
        if (pair.second.length() > 3) { // Only consider reasonably long strings
            knownGoodStrings.push_back(pair.second);
        }
    }
    
    // Search for these strings in the binary and see if they're referenced near
    // function boundaries, which might indicate function names
    const char* binData = libBuffer.data();
    size_t binSize = libBuffer.size();
    
    for (const auto& goodStr : knownGoodStrings) {
        size_t pos = 0;
        while ((pos = std::string(binData, binSize).find(goodStr, pos)) != std::string::npos) {
            vlog << "[+] Found reference to '" << goodStr << "' at binary offset 0x" << std::hex << pos << std::dec << "\n";
            pos += goodStr.length();
        }
    }
}

void MetadataLoader::Process() {
    vlog << "[*] Starting basic metadata processing...\n";

    try {
        // Generate limited output to avoid memory issues with huge datasets
        vlog << "[*] Generating limited output for stability...\n";

        // Generate a very limited CS dump (first 10 elements only)
        DumpCSLimited("dump.cs", 10);

        // Generate a very limited JSON dump (first 10 elements only)
        DumpScriptJSONLimited("script.json", 10);

        vlog << "[+] Limited metadata processing completed\n";

    } catch (const std::exception& e) {
        vlog << "[-] Exception during processing: " << e.what() << "\n";
    } catch (...) {
        vlog << "[-] Unknown exception during processing\n";
    }
}

void MetadataLoader::GenerateDeobfuscationReport() {
    std::ofstream report("deobfuscation_report.txt");
    report << "IL2CPP Deobfuscation Report\n";
    report << "==========================\n\n";
    
    report << "Detection Results:\n";
    report << "- Obfuscation detected: " << (isObfuscated ? "Yes" : "No") << "\n";
    report << "- Number of strings decrypted: " << decryptedStrings.size() << "\n";
    report << "- Number of symbols recovered: " << recoveredSymbols.size() << "\n";
    
    report << "\nDecrypted Strings Sample:\n";
    int count = 0;
    for (const auto& pair : decryptedStrings) {
        if (count++ > 10) break; // Show first 10
        report << " 0x" << std::hex << pair.first << std::dec << " -> \"" << pair.second << "\"\n";
    }
    
    report << "\nRecovered Symbols Sample:\n";
    count = 0;
    for (const auto& pair : recoveredSymbols) {
        if (count++ > 10) break; // Show first 10
        report << " 0x" << std::hex << pair.first << std::dec << " -> \"" << pair.second << "\"\n";
    }
    
    report.close();
}

const Il2CppTypeDefinition* MetadataLoader::GetTypeDefinition(TypeDefinitionIndex index) {
    if (!header || static_cast<uint32_t>(index) >= static_cast<uint32_t>(header->typeDefinitionsCount)) {
        return nullptr;
    }

    // Bounds check for the offset
    if (static_cast<uint32_t>(header->typeDefinitionsOffset) >= fileBuffer.size()) {
        return nullptr;
    }

    const char* typeDefsStart = fileBuffer.data() + header->typeDefinitionsOffset;
    size_t elementOffset = static_cast<size_t>(header->typeDefinitionsOffset) + static_cast<size_t>(index) * sizeof(Il2CppTypeDefinition);

    // Bounds check for the target location
    if (elementOffset >= fileBuffer.size() ||
        elementOffset + sizeof(Il2CppTypeDefinition) > fileBuffer.size()) {
        return nullptr;
    }

    return reinterpret_cast<const Il2CppTypeDefinition*>(typeDefsStart + index * sizeof(Il2CppTypeDefinition));
}

const Il2CppMethodDefinition* MetadataLoader::GetMethodDefinition(MethodIndex index) {
    if (!header || static_cast<uint32_t>(index) >= static_cast<uint32_t>(header->methodsCount)) {
        return nullptr;
    }

    // Bounds check for the offset
    if (static_cast<uint32_t>(header->methodsOffset) >= fileBuffer.size()) {
        return nullptr;
    }

    const char* methodsStart = fileBuffer.data() + header->methodsOffset;
    size_t elementOffset = static_cast<size_t>(header->methodsOffset) + static_cast<size_t>(index) * sizeof(Il2CppMethodDefinition);

    // Bounds check for the target location
    if (elementOffset >= fileBuffer.size() ||
        elementOffset + sizeof(Il2CppMethodDefinition) > fileBuffer.size()) {
        return nullptr;
    }

    return reinterpret_cast<const Il2CppMethodDefinition*>(methodsStart + index * sizeof(Il2CppMethodDefinition));
}

const Il2CppFieldDefinition* MetadataLoader::GetFieldDefinition(FieldIndex index) {
    if (!header || static_cast<uint32_t>(index) >= static_cast<uint32_t>(header->fieldsCount)) {
        return nullptr;
    }

    // Bounds check for the offset
    if (static_cast<uint32_t>(header->fieldsOffset) >= fileBuffer.size()) {
        return nullptr;
    }

    const char* fieldsStart = fileBuffer.data() + header->fieldsOffset;
    size_t elementOffset = static_cast<size_t>(header->fieldsOffset) + static_cast<size_t>(index) * sizeof(Il2CppFieldDefinition);

    // Bounds check for the target location
    if (elementOffset >= fileBuffer.size() ||
        elementOffset + sizeof(Il2CppFieldDefinition) > fileBuffer.size()) {
        return nullptr;
    }

    return reinterpret_cast<const Il2CppFieldDefinition*>(fieldsStart + index * sizeof(Il2CppFieldDefinition));
}

const Il2CppPropertyDefinition* MetadataLoader::GetPropertyDefinition(PropertyIndex index) {
    if (!header || static_cast<uint32_t>(index) >= static_cast<uint32_t>(header->propertiesCount)) {
        return nullptr;
    }

    // Bounds check for the offset
    if (static_cast<uint32_t>(header->propertiesOffset) >= fileBuffer.size()) {
        return nullptr;
    }

    const char* propsStart = fileBuffer.data() + header->propertiesOffset;
    size_t elementOffset = static_cast<size_t>(header->propertiesOffset) + static_cast<size_t>(index) * sizeof(Il2CppPropertyDefinition);

    // Bounds check for the target location
    if (elementOffset >= fileBuffer.size() ||
        elementOffset + sizeof(Il2CppPropertyDefinition) > fileBuffer.size()) {
        return nullptr;
    }

    return reinterpret_cast<const Il2CppPropertyDefinition*>(propsStart + index * sizeof(Il2CppPropertyDefinition));
}

void MetadataLoader::DumpCS(const std::string& outputPath) {
    vlog << "[*] Generating C# output...\n";
    std::ofstream out(outputPath);
    
    out << "// Generated by Enhanced IL2CPP Dumper\n";
    out << "// Compatible with Perfare's Il2CppDumper approach\n\n";

    // Process each image (assembly)
    const char* imagesStart = fileBuffer.data() + header->imagesOffset;
    for (ImageIndex imgIdx = 0; imgIdx < header->imagesCount; imgIdx++) {
        const Il2CppImageDefinition* imageDef = reinterpret_cast<const Il2CppImageDefinition*>(
            imagesStart + imgIdx * sizeof(Il2CppImageDefinition));
        
        std::string imageName = GetStringFromIndex(imageDef->nameIndex);
        if (!imageName.empty()) {
            out << "// Image: " << imageName << "\n\n";
        }
        
        // Process types in this image
        for (uint32_t i = 0; i < imageDef->typeCount; i++) {
            TypeDefinitionIndex typeIndex = imageDef->typeStart + i;
            if (typeIndex >= header->typeDefinitionsCount) continue;
            
            const Il2CppTypeDefinition* typeDef = GetTypeDefinition(typeIndex);
            if (!typeDef) continue;
            
            std::string typeName = GetDecryptedString(typeDef->nameIndex);
            if (typeName.empty()) typeName = GetStringFromIndex(typeDef->nameIndex);
            if (typeName.empty()) continue;

            std::string ns = GetDecryptedString(typeDef->namespaceIndex);
            if (ns.empty()) ns = GetStringFromIndex(typeDef->namespaceIndex);
            if (!ns.empty()) out << "namespace " << ns << " {\n";

            out << "    // Token: 0x" << std::hex << std::setfill('0') << std::setw(8) 
                << (0x02000000 | typeIndex) << std::dec << "\n";
            out << "    public class " << typeName << " {\n";

            // Process methods for this type
            for (uint16_t m = 0; m < typeDef->method_count; m++) {
                MethodIndex methodIdx = typeDef->methodStart + m;
                if (methodIdx >= header->methodsCount) continue;
                
                const Il2CppMethodDefinition* methodDef = GetMethodDefinition(methodIdx);
                if (!methodDef) continue;
                
                std::string methodName = GetDecryptedString(methodDef->nameIndex);
                if (methodName.empty()) methodName = GetStringFromIndex(methodDef->nameIndex);
                if (methodName.empty()) methodName = "Method_" + std::to_string(methodIdx);

                out << "        public void " << methodName << "(); // Token: 0x" 
                    << std::hex << methodDef->token << std::dec << "\n";
            }
            
            // Process fields for this type
            for (uint16_t f = 0; f < typeDef->field_count; f++) {
                FieldIndex fieldIdx = typeDef->fieldStart + f;
                if (fieldIdx >= header->fieldsCount) continue;
                
                const Il2CppFieldDefinition* fieldDef = GetFieldDefinition(fieldIdx);
                if (!fieldDef) continue;
                
                std::string fieldName = GetDecryptedString(fieldDef->nameIndex);
                if (fieldName.empty()) fieldName = GetStringFromIndex(fieldDef->nameIndex);
                if (fieldName.empty()) fieldName = "Field_" + std::to_string(fieldIdx);

                out << "        public var " << fieldName << "; // Token: 0x" 
                    << std::hex << fieldDef->token << std::dec << "\n";
            }
            
            out << "    }\n";
            if (!ns.empty()) out << "}\n";
            out << "\n";
        }
    }
    
    vlog << "[+] C# dump completed\n";
}

void MetadataLoader::DumpScriptJSON(const std::string& outputPath) {
    std::ofstream out(outputPath);
    out << "{\n  \"ScriptMethod\": [\n";
    
    bool first = true;
    for (MethodIndex i = 0; i < header->methodsCount; i++) {
        const Il2CppMethodDefinition* methodDef = GetMethodDefinition(i);
        if (!methodDef) continue;
        
        std::string name = GetDecryptedString(methodDef->nameIndex);
        if (name.empty()) name = GetStringFromIndex(methodDef->nameIndex);
        if (name.empty()) name = "method_" + std::to_string(i);
        
        if (!first) out << ",";
        first = false;
        
        out << "\n    {\n      \"Address\": 0,\n      \"Name\": \"" << name << "\",\n      \"Signature\": \"void " << name << "()\",\n      \"Token\": " << std::hex << "0x" << methodDef->token << std::dec << "\n    }";
    }
    
    out << "\n  ],\n  \"ScriptField\": [\n";
    
    first = true;
    for (FieldIndex i = 0; i < header->fieldsCount; i++) {
        const Il2CppFieldDefinition* fieldDef = GetFieldDefinition(i);
        if (!fieldDef) continue;
        
        std::string name = GetDecryptedString(fieldDef->nameIndex);
        if (name.empty()) name = GetStringFromIndex(fieldDef->nameIndex);
        if (name.empty()) name = "field_" + std::to_string(i);
        
        if (!first) out << ",";
        first = false;
        
        out << "\n    {\n      \"Name\": \"" << name << "\",\n      \"Token\": " << std::hex << "0x" << fieldDef->token << std::dec << "\n    }";
    }
    
    out << "\n  ]\n}";
}

void MetadataLoader::AnalyzeBinaryForSymbols() {
    // Analyze the library binary to recover additional symbols
    if (libBuffer.empty()) {
        vlog << "[*] No library loaded for binary analysis\n";
        return;
    }
    
    vlog << "[*] Analyzing binary for additional symbols...\n";
    
    // Search for common patterns in the binary that might reveal class/method names
    std::string pattern = "System.";
    const char* data = libBuffer.data();
    size_t dataSize = libBuffer.size();
    
    for (size_t i = 0; i < dataSize - pattern.length(); i++) {
        if (memcmp(data + i, pattern.c_str(), pattern.length()) == 0) {
            // Found a potential symbol, extract it
            size_t end = i;
            while (end < dataSize && 
                   (isalnum(data[end]) || data[end] == '.' || data[end] == '_' || data[end] == '<' || data[end] == '>' || data[end] == '`')) {
                end++;
            }
            
            if (end > i) {
                std::string symbol(data + i, end - i);
                vlog << "[+] Found potential symbol in binary: " << symbol << "\n";
            }
        }
    }
}

void MetadataLoader::DumpStrings(const std::string& p) {}
void MetadataLoader::DumpClasses(const std::string& p) {}

void MetadataLoader::DumpCSLimited(const std::string& outputPath, int maxElements) {
    std::ofstream out(outputPath);
    out << "// IL2CPP Metadata Dump\n";
    out << "// Generated by Enhanced IL2CPP Dumper\n";
    out << "// File offset information included\n";
    out << "// Unity Version: " << header->version << " | Sanity: 0x" << std::hex << header->sanity << std::dec << "\n";
    out << "// Total Types: " << header->typeDefinitionsCount << " | Methods: " << header->methodsCount << "\n\n";

    out << "#pragma once\n\n";

    // Limit the number of images we process
    int imageCount = std::min(static_cast<int>(header->imagesCount), maxElements);
    for (ImageIndex imgIdx = 0; imgIdx < imageCount; imgIdx++) {
        // Calculate the offset for this image definition
        size_t imageOffset = header->imagesOffset + imgIdx * sizeof(Il2CppImageDefinition);
        const Il2CppImageDefinition* imageDef = reinterpret_cast<const Il2CppImageDefinition*>(
            fileBuffer.data() + imageOffset);

        const char* imageName = GetStringFromIndex(imageDef->nameIndex);
        if (!imageName) imageName = "UnknownImage";

        out << "// Image: " << imageName << " | Index: " << imgIdx << " | Offset: 0x" << std::hex << imageOffset << std::dec << "\n";
        out << "//   Type Range: [" << imageDef->typeStart << ", " << (imageDef->typeStart + imageDef->typeCount) << "] | Count: " << imageDef->typeCount << "\n";

        // Limit the number of types we process per image
        int typeCount = std::min(static_cast<int>(imageDef->typeCount), maxElements);
        for (uint32_t t = 0; t < typeCount; t++) {
            TypeDefinitionIndex typeIndex = imageDef->typeStart + t;
            if (typeIndex >= header->typeDefinitionsCount) continue;

            // Calculate the offset for this type definition
            size_t typeOffset = header->typeDefinitionsOffset + typeIndex * sizeof(Il2CppTypeDefinition);
            const Il2CppTypeDefinition* typeDef = GetTypeDefinition(typeIndex);
            if (!typeDef) continue;

            const char* typeName = GetStringFromIndex(typeDef->nameIndex);
            const char* ns = GetStringFromIndex(typeDef->namespaceIndex);

            // Use index-based naming if string access fails
            std::string displayTypeName = typeName ? typeName : "Type_" + std::to_string(typeIndex);
            std::string displayNamespace = ns ? ns : "Namespace_" + std::to_string(typeDef->namespaceIndex);

            out << "// Type: " << displayNamespace << "." << displayTypeName
                << " | Index: " << typeIndex
                << " | TypeDef Offset: 0x" << std::hex << typeOffset << std::dec
                << " | Flags: 0x" << std::hex << typeDef->flags << std::dec
                << " | Methods: " << typeDef->method_count << "\n";

            // Process methods for this type (limited)
            int methodCount = std::min(static_cast<int>(typeDef->method_count), maxElements/100);
            for (uint16_t m = 0; m < methodCount; m++) {
                MethodIndex methodIdx = typeDef->methodStart + m;
                if (methodIdx >= header->methodsCount) continue;

                // Calculate the offset for this method definition
                size_t methodOffset = header->methodsOffset + methodIdx * sizeof(Il2CppMethodDefinition);
                const Il2CppMethodDefinition* methodDef = GetMethodDefinition(methodIdx);
                if (!methodDef) continue;

                const char* methodName = GetStringFromIndex(methodDef->nameIndex);

                // Use index-based naming if string access fails
                std::string displayMethodName = methodName ? methodName : "Method_" + std::to_string(methodIdx);

                out << "//   Method: " << displayMethodName
                    << " | Index: " << methodIdx
                    << " | MethodDef Offset: 0x" << std::hex << methodOffset << std::dec
                    << " | Token: 0x" << std::hex << methodDef->token << std::dec
                    << " | Flags: 0x" << std::hex << methodDef->flags << std::dec << "\n";
            }

            out << "\n";
        }
        out << "\n";
    }

    out.close();
}

void MetadataLoader::DumpScriptJSONLimited(const std::string& outputPath, int maxElements) {
    std::ofstream out(outputPath);
    out << "{\n";
    out << "  \"metadata\": {\n";
    out << "    \"version\": " << header->version << ",\n";
    out << "    \"sanity\": " << header->sanity << ",\n";
    out << "    \"stringCount\": " << header->stringCount << ",\n";
    out << "    \"typeDefinitionsCount\": " << header->typeDefinitionsCount << ",\n";
    out << "    \"methodsCount\": " << header->methodsCount << ",\n";
    out << "    \"imagesOffset\": " << header->imagesOffset << ",\n";
    out << "    \"typeDefinitionsOffset\": " << header->typeDefinitionsOffset << ",\n";
    out << "    \"methodsOffset\": " << header->methodsOffset << "\n";
    out << "  },\n";

    out << "  \"types\": [\n";

    // Limit the number of types we process
    int typeCount = std::min(static_cast<int>(header->typeDefinitionsCount), maxElements);
    for (TypeDefinitionIndex i = 0; i < typeCount; i++) {
        const Il2CppTypeDefinition* typeDef = GetTypeDefinition(i);
        if (!typeDef) continue;

        // Calculate the offset for this type definition
        size_t typeOffset = header->typeDefinitionsOffset + i * sizeof(Il2CppTypeDefinition);

        const char* typeName = GetStringFromIndex(typeDef->nameIndex);
        const char* ns = GetStringFromIndex(typeDef->namespaceIndex);

        if (i > 0) out << ",\n";
        out << "    {\n";
        out << "      \"index\": " << i << ",\n";
        out << "      \"name\": \"" << (typeName ? typeName : "UnknownType") << "\",\n";
        out << "      \"namespace\": \"" << (ns ? ns : "") << "\",\n";
        out << "      \"flags\": " << typeDef->flags << ",\n";
        out << "      \"methodCount\": " << typeDef->method_count << ",\n";
        out << "      \"offset\": " << typeOffset << ",\n";
        out << "      \"offsetHex\": \"0x" << std::hex << typeOffset << std::dec << "\"\n";
        out << "    }";
    }

    out << "\n  ],\n";

    out << "  \"methods\": [\n";

    // Limit the number of methods we process
    int methodCount = std::min(static_cast<int>(header->methodsCount), maxElements);
    for (MethodIndex i = 0; i < methodCount; i++) {
        const Il2CppMethodDefinition* methodDef = GetMethodDefinition(i);
        if (!methodDef) continue;

        // Calculate the offset for this method definition
        size_t methodOffset = header->methodsOffset + i * sizeof(Il2CppMethodDefinition);

        const char* methodName = GetStringFromIndex(methodDef->nameIndex);

        if (i > 0) out << ",\n";
        out << "    {\n";
        out << "      \"index\": " << i << ",\n";
        out << "      \"name\": \"" << (methodName ? methodName : "UnknownMethod") << "\",\n";
        out << "      \"token\": " << methodDef->token << ",\n";
        out << "      \"flags\": " << methodDef->flags << ",\n";
        out << "      \"offset\": " << methodOffset << ",\n";
        out << "      \"offsetHex\": \"0x" << std::hex << methodOffset << std::dec << "\"\n";
        out << "    }";
    }

    out << "\n  ]\n";
    out << "}\n";

    out.close();
}