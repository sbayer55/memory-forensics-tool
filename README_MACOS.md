# macOS Build Instructions

This memory forensics tool is primarily designed for Windows memory analysis, but can be compiled on macOS for development and testing purposes.

## Prerequisites

1. **Homebrew** (https://brew.sh/)
2. **Xcode Command Line Tools**:
   ```bash
   xcode-select --install
   ```

## Quick Build

Run the provided build script:
```bash
./build_macos.zsh
```

## Manual Build

### Install Dependencies

#### Option 1: Using vcpkg (Recommended)
```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install

# Install dependencies
./vcpkg install lua:x64-osx sol2:x64-osx cli11:x64-osx spdlog:x64-osx nlohmann-json:x64-osx fmt:x64-osx
```

#### Option 2: Using Homebrew
```bash
brew install lua nlohmann-json spdlog fmt
# Note: sol2 and cli11 may need to be built from source
```

### Build Process
```bash
mkdir build
cd build

# With vcpkg
cmake -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake -DBUILD_FOR_MACOS=ON ..

# Without vcpkg  
cmake -DBUILD_FOR_MACOS=ON ..

# Build
cmake --build . --config Release -j$(sysctl -n hw.ncpu)
```

## Important Limitations on macOS

⚠️ **This tool will compile but have limited functionality on macOS:**

- **No process attachment**: Windows process APIs are stubbed out
- **No memory reading**: Memory access APIs are non-functional
- **Development only**: Suitable for code development and testing logic
- **Windows deployment required**: For actual memory forensics, deploy to Windows

## What Works on macOS

✅ **These components function normally:**
- Lua scripting engine and API registration
- Configuration loading and validation
- Logging system
- Data structure definitions
- Algorithm implementations (decryption logic)
- Unit tests (if implemented)

## Recommended Workflow

1. **Develop on macOS**: Write and test Lua scripts, algorithms, and logic
2. **Test on Windows**: Deploy to Windows for actual memory analysis
3. **Use VM**: Set up Windows VM for development if needed

## Cross-Platform Development Tips

- Test compilation regularly on both platforms
- Keep Windows-specific code isolated in platform compatibility layers
- Use the Lua scripting interface for testing algorithm logic
- Write unit tests that can run on macOS without process access

## Troubleshooting

### Build Errors
- Ensure all dependencies are installed via vcpkg or Homebrew
- Check that Xcode Command Line Tools are installed
- Verify CMake version is 3.20 or higher

### Missing Libraries
```bash
# Find missing libraries
brew search [library-name]
vcpkg search [library-name]
```

### Dependency Conflicts
- Use vcpkg for consistent library versions
- Clear build directory if switching between vcpkg/Homebrew
- Check library paths in CMake output

For Windows-specific functionality, deploy to a Windows environment or VM.