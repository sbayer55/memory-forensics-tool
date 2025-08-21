#!/bin/zsh

# Memory Forensics Tool - macOS Build Script
# This script builds the memory forensics tool on macOS
# Note: This tool is primarily designed for Windows, but can be built on macOS for development

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Script directory
SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build"

print_status "Memory Forensics Tool - macOS Build Script"
print_warning "Note: This tool is designed for Windows memory analysis"
print_warning "Building on macOS for development/testing purposes only"

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    print_error "This script is designed for macOS (Darwin)"
    exit 1
fi

# Check for required tools
print_status "Checking for required tools..."

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    print_error "Homebrew is required but not installed"
    print_status "Install Homebrew from: https://brew.sh/"
    exit 1
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    print_status "Installing CMake via Homebrew..."
    brew install cmake
fi

# Check for vcpkg
VCPKG_ROOT=""
if command -v vcpkg &> /dev/null; then
    VCPKG_ROOT=$(vcpkg integrate install 2>/dev/null | grep -o '/[^"]*vcpkg\.cmake' | head -1 2>/dev/null || echo "")
fi

if [[ -z "$VCPKG_ROOT" ]]; then
    print_warning "vcpkg not found or not integrated"
    print_status "Attempting to use Homebrew for dependencies..."
    
    # Install dependencies via Homebrew
    print_status "Installing dependencies via Homebrew..."
    
    # Core dependencies
    brew install lua
    brew install nlohmann-json
    brew install spdlog
    brew install fmt
    
    # Try to install sol2 (Lua C++ binding)
    if ! brew list sol2 &>/dev/null; then
        print_warning "sol2 not available via Homebrew, will need to be built from source"
    fi
    
    # CLI11 is header-only, might be available
    if ! brew list cli11 &>/dev/null; then
        print_warning "CLI11 not available via Homebrew, will need to be built from source"
    fi
    
else
    print_success "vcpkg found at: $VCPKG_ROOT"
    
    # Install dependencies via vcpkg
    print_status "Installing dependencies via vcpkg..."
    
    vcpkg install lua:x64-osx
    vcpkg install sol2:x64-osx
    vcpkg install cli11:x64-osx
    vcpkg install spdlog:x64-osx
    vcpkg install nlohmann-json:x64-osx
    vcpkg install fmt:x64-osx
fi

# Create build directory
print_status "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake
print_status "Configuring CMake..."

CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
    -DCMAKE_CXX_STANDARD=17
)

# Add vcpkg toolchain if available
if [[ -n "$VCPKG_ROOT" ]]; then
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT")
fi

# Configure with platform-specific adaptations
print_status "Note: Disabling Windows-specific features for macOS build"
CMAKE_ARGS+=(-DBUILD_FOR_MACOS=ON)

cmake "${CMAKE_ARGS[@]}" "$PROJECT_ROOT"

if [[ $? -ne 0 ]]; then
    print_error "CMake configuration failed"
    print_status "Common issues:"
    print_status "  1. Missing dependencies (install via vcpkg or Homebrew)"
    print_status "  2. Windows-specific code needs macOS adaptations"
    print_status "  3. Check CMakeLists.txt for platform-specific logic"
    exit 1
fi

# Build the project
print_status "Building the project..."
CPU_COUNT=$(sysctl -n hw.ncpu)
print_status "Using $CPU_COUNT parallel jobs"

cmake --build . --config Release -j "$CPU_COUNT"

if [[ $? -ne 0 ]]; then
    print_error "Build failed"
    print_status "Common issues on macOS:"
    print_status "  1. Windows API calls need macOS equivalents"
    print_status "  2. Platform-specific headers (Windows.h, etc.)"
    print_status "  3. Memory management APIs differ between platforms"
    exit 1
fi

# Check if binary was created
BINARY_PATH="$BUILD_DIR/bin/memory-tool"
if [[ -f "$BINARY_PATH" ]]; then
    print_success "Build completed successfully!"
    print_status "Binary location: $BINARY_PATH"
    
    # Make binary executable
    chmod +x "$BINARY_PATH"
    
    # Show binary info
    print_status "Binary information:"
    file "$BINARY_PATH"
    ls -lh "$BINARY_PATH"
    
else
    print_error "Binary not found at expected location: $BINARY_PATH"
    print_status "Checking for alternative locations..."
    find "$BUILD_DIR" -name "memory-tool" -type f 2>/dev/null || print_status "No binary found"
fi

# Copy configuration and scripts
print_status "Setting up runtime environment..."

if [[ -d "$PROJECT_ROOT/config" ]]; then
    cp -r "$PROJECT_ROOT/config" "$BUILD_DIR/bin/" 2>/dev/null || true
fi

if [[ -d "$PROJECT_ROOT/scripts" ]]; then
    cp -r "$PROJECT_ROOT/scripts" "$BUILD_DIR/bin/" 2>/dev/null || true
fi

print_success "macOS build process completed!"
print_warning "Important notes for macOS:"
print_warning "  - This tool is designed for Windows memory analysis"
print_warning "  - Process attachment and memory reading will not work on macOS"
print_warning "  - Use this build for development and testing only"
print_warning "  - For actual memory forensics, run on Windows or Windows VM"

# Suggest next steps
print_status "Next steps:"
print_status "  1. Test basic functionality: $BINARY_PATH --help"
print_status "  2. Check Lua integration: $BINARY_PATH --interactive"
print_status "  3. For Windows analysis, deploy to Windows environment"

exit 0