# Enhanced IL2CPP Dumper Architecture

## Overview
This document outlines the architecture for an enhanced il2cppdump project with integrated deobfuscation capabilities. The system will provide comprehensive extraction and deobfuscation of IL2CPP metadata and binaries.

## System Architecture

### Core Components

#### 1. Metadata Loader
- Enhanced version of current MetadataLoader
- Support for multiple Unity versions and metadata formats
- Automatic header validation and correction
- Memory-mapped file access for large files

#### 2. Deobfuscation Engine
- Modular deobfuscation pipeline
- Multiple deobfuscation strategies
- Configurable obfuscation detection
- Pluggable deobfuscation modules

#### 3. Binary Analysis Module
- Advanced disassembly capabilities
- Cross-reference analysis
- Pattern recognition for obfuscated code
- Symbol recovery from binary

#### 4. Output Generator
- Multiple output formats (C#, JSON, IDA scripts, Ghidra scripts)
- Clean, readable output with recovered symbols
- Integration with reverse engineering tools

### Deobfuscation Capabilities

#### A. String Decryption
- Detection of common string encryption algorithms
- Automated decryption of encrypted strings
- Support for custom string obfuscation schemes
- Runtime string resolution

#### B. Control Flow Restoration
- Identification of obfuscated control structures
- Restoration of linear control flow
- Recognition of switch statement obfuscation
- Unflattening of flattened control flow

#### C. Symbol Recovery
- Class and method name recovery
- Type information reconstruction
- Field and property identification
- Namespace reconstruction

#### D. Type Information Recovery
- Reconstruction of type hierarchies
- Generic type parameter recovery
- Interface implementation restoration
- Virtual method table reconstruction

### Technical Implementation

#### Enhanced Data Structures
```cpp
struct ObfuscationProfile {
    std::string name;
    std::vector<std::string> detection_strings;
    std::function<bool(MetadataLoader*)> detect_func;
    std::function<void(MetadataLoader*)> deobfuscate_func;
};

struct DeobfuscationResult {
    bool success;
    std::string error_message;
    std::map<uint32_t, std::string> decrypted_strings;
    std::map<uint32_t, std::string> recovered_symbols;
};
```

#### Plugin Architecture
- Dynamic loading of deobfuscation modules
- Configuration-based obfuscation detection
- Extensible framework for new techniques
- Version-specific optimizations

### Integration Pipeline

1. **Input Processing**
   - Load metadata and binary files
   - Validate file integrity
   - Detect Unity version and obfuscation

2. **Analysis Phase**
   - Extract raw metadata
   - Identify obfuscation patterns
   - Prepare for deobfuscation

3. **Deobfuscation Phase**
   - Apply string decryption
   - Restore control flow
   - Recover symbols
   - Reconstruct type information

4. **Output Generation**
   - Generate clean C# code
   - Create JSON mapping files
   - Export to reverse engineering tools
   - Produce analysis reports

### Supported Unity Versions
- Unity 2017.x - 2023.x
- Various IL2CPP runtime versions
- Platform-specific variations (Android, iOS, Windows, etc.)

### Output Formats
- C# source code with recovered names
- JSON mapping for tool integration
- IDA/Ghidra/Frida scripts
- Documentation files

## Implementation Strategy

### Phase 1: Core Enhancement
- Upgrade current MetadataLoader
- Add support for newer Unity versions
- Improve error handling and validation

### Phase 2: Deobfuscation Engine
- Implement string decryption modules
- Add control flow restoration
- Create symbol recovery system

### Phase 3: Integration
- Connect all components
- Add plugin architecture
- Implement automated detection

### Phase 4: Testing and Optimization
- Test with various obfuscated binaries
- Optimize performance
- Add comprehensive error reporting

## Security and Ethics
- Educational/research purposes only
- Compliance with applicable laws
- Responsible disclosure practices