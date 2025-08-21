# Revolution Idol Memory Forensics Tool

A specialized command-line memory forensics tool designed to analyze and extract encrypted data structures from the Unity-based game "Revolution Idol". This tool provides Lua scripting capabilities for flexible memory analysis and automated decryption of obfuscated BigInteger objects.

## Overview

This tool functions similarly to Cheat Engine, providing programmatic access to process memory with built-in decryption capabilities. It specifically targets .NET applications running in Unity, with a focus on extracting and decrypting BigInteger objects that are encrypted using in-memory keys.

## Features

- **Process Memory Access**: Direct memory reading from running Revolution Idol processes
- **Lua Scripting Engine**: Execute custom Lua scripts for flexible memory analysis
- **Encrypted Data Extraction**: Automatically locate and decrypt BigInteger objects
- **Unity .NET Integration**: Parse managed heap structures and follow object references
- **Signature-Based Scanning**: Identify container structs holding encrypted data and keys
- **Automated Decryption**: Built-in decryption algorithm based on reverse-engineered source code

## Technology Stack

- **Language**: C++17/C++20
- **Build System**: CMake 3.20+
- **Dependency Management**: vcpkg
- **Target Platform**: Windows 10/11
- **Target Framework**: Unity .NET applications

## Dependencies

### Core Libraries
- **Lua 5.4**: Embedded scripting engine for custom analysis scripts
- **Sol2**: Modern C++/Lua binding library for seamless integration
- **CLI11**: Command-line argument parsing and interface
- **spdlog**: High-performance logging framework
- **nlohmann/json**: JSON configuration and data serialization

### Windows-Specific Libraries
- **Windows SDK**: Core Windows API functions (OpenProcess, ReadProcessMemory, etc.)
- **Microsoft Detours**: Process manipulation and function hooking
- **PolyHook**: Advanced hooking capabilities for memory interception

### Optional Dependencies
- **Boost**: Additional utilities and data structures
- **fmt**: String formatting (if not using C++20 std::format)

## Architecture Components

### Memory Scanner
- Process enumeration and attachment
- Managed heap traversal for Unity applications
- Signature-based struct identification
- Pointer dereferencing and validation

### Lua Integration
- Script execution environment
- Memory access APIs exposed to Lua
- Custom functions for .NET object analysis
- Scripted automation for repetitive tasks

### Decryption Engine
- Container struct pattern recognition
- BigInteger and encryption key extraction
- Reimplemented decryption algorithm
- Batch processing capabilities

### .NET Metadata Parser
- Method table analysis
- Object header interpretation
- Class type identification
- Garbage collector reference following

## Build Requirements

- **Compiler**: MSVC 2019+ or GCC 9+ with C++17 support
- **CMake**: Version 3.20 or higher
- **vcpkg**: Latest version for dependency management
- **Windows SDK**: Version 10.0.19041.0 or higher

## Installation

1. Clone the repository
2. Install vcpkg and integrate with Visual Studio
3. Install dependencies via vcpkg
4. Configure with CMake
5. Build using your preferred IDE or command line

```bash
# Install dependencies
vcpkg install lua sol2 cli11 spdlog nlohmann-json detours

# Configure build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# Build project
cmake --build build --config Release
```

## Usage

```bash
# Basic memory scan
memory-tool.exe --process "Revolution Idol" --script analyze.lua

# Extract encrypted BigIntegers
memory-tool.exe --target-pid 1234 --decrypt --output results.json

# Interactive Lua shell
memory-tool.exe --interactive --attach "Revolution Idol"
```

## Project Structure

```
memory-forensics-tool/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── memory_scanner.hpp
│   ├── lua_engine.hpp
│   ├── process_manager.hpp
│   ├── decryption_engine.hpp
│   └── dotnet_parser.hpp
├── src/
│   ├── main.cpp
│   ├── memory_scanner.cpp
│   ├── lua_engine.cpp
│   ├── process_manager.cpp
│   ├── decryption_engine.cpp
│   └── dotnet_parser.cpp
├── scripts/
│   ├── examples/
│   └── templates/
├── config/
│   └── default_config.json
└── tests/
    └── unit_tests/
```

## Security Considerations

This tool requires elevated privileges to access process memory. It should only be used on systems you own or have explicit permission to analyze. The tool is designed for legitimate security research and reverse engineering purposes.

## Contributing

This is a specialized tool for a specific application. Contributions should focus on improving memory analysis accuracy, Lua integration features, and decryption reliability.

## License

[Specify your preferred license here]