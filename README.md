# Enhanced IL2CPP Dumper

An advanced tool for dumping and analyzing IL2CPP metadata from Unity games and applications. This enhanced version provides comprehensive metadata extraction with advanced deobfuscation capabilities.

## Features

- **Accurate Metadata Parsing**: Properly handles IL2CPP metadata files with correct structure definitions matching Perfare's Il2CppDumper approach
- **Multiple Unity Versions Support**: Compatible with various Unity versions including newer formats (Unity 29 tested)
- **Large File Handling**: Safely processes large metadata files without crashing
- **Enhanced Deobfuscation**: Advanced string decryption, control flow restoration, and symbol recovery capabilities
- **Comprehensive Analysis**: Provides detailed deobfuscation reports and analysis
- **Detailed Output**: Generates C# code, JSON mapping, and detailed deobfuscation reports with memory offsets

## Supported Unity Versions

- Unity 2017+
- Unity 2018+
- Unity 2019+
- Unity 2020+
- Unity 2021+ (tested with version 29)
- And more

## Installation

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+
- Git

### Building from Source

```bash
git clone https://github.com/yourusername/enhanced-il2cpp-dumper.git
cd enhanced-il2cpp-dumper
mkdir build && cd build
cmake ..
make -j4
```

## Usage

```bash
./il2cpp-dumper <path_to_global-metadata.dat> [optional_path_to_libil2cpp.so]
```

### Examples

```bash
# Basic usage with metadata file only
./il2cpp-dumper global-metadata.dat

# With both metadata and library files
./il2cpp-dumper global-metadata.dat libil2cpp.so
```

## Output

The tool generates several output files:

- `dump.cs`: C#-style representation of the type information with offsets
- `script.json`: Structured JSON with metadata including offsets
- `deobfuscation_report.txt`: Detailed report of deobfuscation activities

## Advanced Features

### String Decryption
- XOR decryption with automatic key detection
- Caesar cipher with shift detection
- Base64 decoding
- ROT13 decoding
- Custom pattern decryption

### Control Flow Restoration
- Dispatcher pattern identification
- Switch statement obfuscation detection
- Junk code removal
- Linear control flow restoration

### Symbol Recovery
- Class name inference
- Method name enhancement
- Field name recovery
- Property name restoration

## Architecture

The tool follows the structure of Perfare's Il2CppDumper with the following key components:

- `MetadataLoader`: Core metadata parsing and extraction
- `il2cpp_structs.h`: IL2CPP structure definitions
- `main.cpp`: Entry point and command-line interface
- `CMakeLists.txt`: Build configuration

## Compatibility

The tool has been tested with:
- Large Unity games (35MB+ metadata files)
- Unity 29 format (sanity: 0xEAB11BAF)
- Various obfuscation techniques
- Different Unity engine versions

## Troubleshooting

### Common Issues

1. **Segmentation Faults**: Usually caused by malformed metadata files. The tool includes bounds checking to prevent most of these.

2. **Empty String Names**: Some Unity versions use different string table formats. The tool provides index-based fallback names.

3. **Large File Processing**: For very large files, the tool provides limited output modes to prevent memory issues.

### Known Limitations

- Some Unity 2021+ games with heavy obfuscation may require additional analysis
- Extremely large metadata files (>50MB) may require more memory
- Some custom obfuscation techniques may not be fully handled

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Based on Perfare's original Il2CppDumper approach
- Inspired by the reverse engineering community's work on Unity games
- Special thanks to contributors who helped test with various Unity versions
