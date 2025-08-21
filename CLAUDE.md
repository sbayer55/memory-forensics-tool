# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a specialized C++ memory forensics tool designed for analyzing the Unity-based game "Revolution Idol". The tool provides programmatic access to process memory with built-in decryption capabilities for extracting and decrypting BigInteger objects encrypted with in-memory keys.

**Target Platform**: Windows 10/11  
**Language**: C++17  
**Build System**: CMake 3.20+  
**Package Manager**: vcpkg

## Essential Commands

### Initial Setup
```bash
# Install dependencies via vcpkg (Windows)
vcpkg install lua:x64-windows sol2:x64-windows cli11:x64-windows spdlog:x64-windows nlohmann-json:x64-windows
vcpkg install microsoft-detours:x64-windows boost:x64-windows

# Configure build with vcpkg integration
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# Build project
cmake --build build --config Release
```

### Development Commands
```bash
# Build project
cmake --build build --config Release

# Build debug version
cmake --build build --config Debug

# Run the tool
./build/bin/memory-tool.exe --help
./build/bin/memory-tool.exe --process "Revolution Idol" --script scripts/examples/basic_scan.lua
```

### Testing
```bash
# Enable testing is built into CMakeLists.txt but no test framework is currently configured
# Tests directory exists at tests/unit_tests/ but contains minimal content
```

## Core Architecture

### Component Structure
The codebase follows a modular architecture with these core components:

1. **ProcessManager** (`src/process_manager.cpp`, `include/process_manager.hpp`) - Handles process enumeration, attachment, and low-level memory operations
2. **MemoryScanner** (`src/memory_scanner.cpp`, `include/memory_scanner.hpp`) - Pattern-based memory scanning, signature detection, and pointer following
3. **DecryptionEngine** (`src/decryption_engine.cpp`, `include/decryption_engine.hpp`) - Container struct recognition and BigInteger decryption
4. **DotNetParser** (`src/dotnet_parser.cpp`, `include/dotnet_parser.hpp`) - .NET metadata parsing, method table analysis, and object header interpretation
5. **LuaEngine** (`src/lua_engine.cpp`, `include/lua_engine.hpp`) - Lua scripting integration with exposed memory access APIs

### Key Data Structures
- `EncryptedBigInteger` - Represents encrypted BigInteger objects with container addresses and decryption keys
- `MemoryRegion` - Defines scannable memory areas with protection flags
- `MemoryAddress` (alias for `uintptr_t`) - Standardized memory address type

### Configuration System
- **Main Config**: `config/default_config.json` - Central configuration for process targeting, memory scanning parameters, decryption methods, and performance settings
- **Signatures**: Memory patterns for container struct and BigInteger header detection
- **Lua Scripts**: Located in `scripts/` directory with examples and templates

## Memory Forensics Workflow

1. **Process Attachment**: Target "Revolution Idol.exe" process and obtain necessary privileges
2. **Signature Scanning**: Use predefined patterns to locate container structs holding encrypted data
3. **Pointer Following**: Navigate object references to extract BigInteger and encryption key pointers
4. **Decryption**: Apply reverse-engineered decryption algorithm using extracted in-memory keys
5. **Lua Scripting**: Execute custom analysis scripts with exposed memory APIs

## Important Security Context

This tool requires elevated privileges for memory access and is designed for legitimate security research and reverse engineering. The codebase targets a specific Unity application for educational/research purposes in memory forensics techniques.

## Development Notes

- All memory operations use Windows API (`OpenProcess`, `ReadProcessMemory`, etc.)
- Lua integration uses Sol2 for C++/Lua bindings
- Error handling uses custom exception types: `MemoryException`, `ProcessException`, `DecryptionException`
- Memory scanning optimized with 4-byte alignment and configurable chunk sizes
- Progress callbacks available for long-running operations