# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Enhanced IL2CPP metadata parsing capabilities
- Support for Unity 29 format (sanity: 0xEAB11BAF)
- Advanced deobfuscation features (string decryption, control flow restoration, symbol recovery)
- Memory offset information in output files
- Comprehensive error handling and bounds checking
- Support for large metadata files (35MB+)

### Changed
- Improved string access logic for different Unity versions
- Enhanced output formatting with detailed type and method information
- Better handling of malformed metadata files
- More robust memory access with validation

### Fixed
- Segmentation fault issues with large datasets
- String access problems in Unity 29 format
- Memory access violations in metadata parsing
- Build warnings and compilation issues

## [1.0.0] - 2026-01-28

### Added
- Initial release of Enhanced IL2CPP Dumper
- Core metadata parsing functionality
- Support for multiple Unity versions
- Basic deobfuscation capabilities
- C# and JSON output generation
