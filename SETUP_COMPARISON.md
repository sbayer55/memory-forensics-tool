# Setup Script Comparison: Windows vs macOS

This document outlines the differences between the original Windows setup script (`setup_project.bat`) and the macOS port (`setup_project.zsh`).

## Core Functionality Mapping

| Windows (setup_project.bat) | macOS (setup_project.zsh) | Notes |
|------------------------------|---------------------------|-------|
| `@echo off` | `#!/bin/zsh` | Shell specification |
| `mkdir dir 2>nul` | `mkdir -p dir` | Recursive directory creation |
| `where vcpkg` | `command -v vcpkg` | Command existence check |
| `pause` | Interactive prompts | No direct equivalent needed |
| Batch error handling | `set -e` + functions | More robust error handling |

## Key Adaptations

### 1. Dependency Management

**Windows:**
```batch
vcpkg install lua:x64-windows
vcpkg install sol2:x64-windows
vcpkg install cli11:x64-windows
vcpkg install spdlog:x64-windows
vcpkg install nlohmann-json:x64-windows
vcpkg install microsoft-detours:x64-windows
vcpkg install boost:x64-windows
```

**macOS:**
```zsh
# Primary: vcpkg with macOS targets
./vcpkg install lua:x64-osx
./vcpkg install sol2:x64-osx
./vcpkg install cli11:x64-osx
./vcpkg install spdlog:x64-osx
./vcpkg install nlohmann-json:x64-osx
./vcpkg install fmt:x64-osx
./vcpkg install boost:x64-osx

# Fallback: Homebrew
brew install lua nlohmann-json spdlog fmt
# Note: microsoft-detours is Windows-only, skipped
```

### 2. vcpkg Detection

**Windows:**
```batch
where vcpkg >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: vcpkg not found in PATH
    exit /b 1
)
```

**macOS:**
```zsh
if command -v vcpkg &> /dev/null; then
    # vcpkg in PATH
else
    # Check common installation locations
    COMMON_VCPKG_PATHS=(
        "$HOME/vcpkg"
        "/usr/local/vcpkg"
        "/opt/vcpkg"
        "$HOME/tools/vcpkg"
    )
    # Search and fallback to Homebrew
fi
```

### 3. CMake Configuration

**Windows:**
```batch
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%"
```

**macOS:**
```zsh
CMAKE_ARGS=(
    -B build
    -S .
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
    -DBUILD_FOR_MACOS=ON
)
if [[ -n "$VCPKG_TOOLCHAIN" ]]; then
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="$VCPKG_TOOLCHAIN")
fi
cmake "${CMAKE_ARGS[@]}"
```

### 4. Platform-Specific Exclusions

| Component | Windows | macOS | Reason |
|-----------|---------|-------|--------|
| microsoft-detours | ✅ Installed | ❌ Skipped | Windows-only library |
| Process APIs | ✅ Functional | ⚠️ Stubbed | Windows memory model |
| Build target | `x64-windows` | `x64-osx` | Platform architecture |

## Enhanced Features in macOS Version

### 1. Improved Error Handling
- Colored output for better visibility
- Comprehensive error messages
- Graceful fallbacks (vcpkg → Homebrew)
- Pre-flight checks for required tools

### 2. Better User Experience
- Progress indicators with colored output
- Detailed status messages
- Multiple dependency installation methods
- Common issue troubleshooting

### 3. Enhanced Git Integration
- Automatic .gitignore creation
- Platform-specific ignore patterns
- Initial commit with descriptive message
- Git repository status checking

### 4. Development Workflow Support
- Parallel compilation detection
- Binary validation and permissions
- Runtime environment setup
- Clear next steps and limitations

## Installation Methods Comparison

### vcpkg (Preferred for both platforms)

**Advantages:**
- Consistent library versions
- Better dependency resolution
- Cross-platform package management
- Integration with CMake

**Windows:**
```batch
vcpkg install [package]:x64-windows
```

**macOS:**
```zsh
./vcpkg install [package]:x64-osx
```

### Platform Package Managers

**Windows (Chocolatey/winget):**
- Less commonly used for C++ development
- Not implemented in original script

**macOS (Homebrew):**
```zsh
brew install [package]
```
- Fallback option when vcpkg unavailable
- Some packages may have compatibility issues

## Limitations and Workarounds

### Windows-Only Dependencies

| Dependency | Windows | macOS Solution |
|------------|---------|---------------|
| microsoft-detours | Required | Skipped (Windows-only) |
| Windows SDK | Required | Platform compatibility layer |
| psapi.lib | Linked | Stubbed in platform_compat.hpp |

### Memory Analysis Functionality

| Feature | Windows | macOS |
|---------|---------|-------|
| Process attachment | ✅ Functional | ❌ Stubbed |
| Memory reading | ✅ Full access | ❌ Compatibility layer |
| Windows APIs | ✅ Native | ⚠️ Stubbed for compilation |

## Usage Recommendations

### For Development:
1. **Use macOS setup** for code development and testing
2. **Validate compilation** regularly on both platforms
3. **Test logic** using Lua scripting interface
4. **Deploy to Windows** for actual memory forensics

### For Production:
1. **Use Windows setup** for actual memory analysis
2. **Deploy to Windows environment** or VM
3. **Use original batch script** on Windows systems

## Next Steps After Setup

### Both Platforms:
1. Verify compilation: `cmake --build build --config Release`
2. Test basic functionality: `./build/bin/memory-tool --help`
3. Set up IDE/editor for development
4. Create GitHub repository and push code

### macOS Specific:
1. Set up Windows VM for testing (optional)
2. Configure cross-compilation if needed
3. Use Lua interface for algorithm development
4. Test Windows deployment pipeline

### Windows Specific:
1. Test process attachment capabilities
2. Validate memory reading functionality
3. Set up target analysis environment
4. Configure security permissions for memory access