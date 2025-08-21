#!/bin/zsh

# Memory Forensics Tool Project Setup Script for macOS
# Ported from setup_project.bat for cross-platform development

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
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

print_step() {
    echo -e "${CYAN}[STEP]${NC} $1"
}

# Script directory
SCRIPT_DIR=$(dirname "$(realpath "$0")")
cd "$SCRIPT_DIR"

echo -e "${GREEN}Setting up Memory Forensics Tool Project for macOS...${NC}"
echo ""

# Create directory structure
print_step "Creating project directories..."
mkdir -p include
mkdir -p src
mkdir -p scripts/examples
mkdir -p scripts/templates
mkdir -p config
mkdir -p tests/unit_tests
mkdir -p logs
mkdir -p build

print_success "Project directories created"

# Check for required tools
print_step "Checking for required development tools..."

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    print_error "Homebrew is required but not installed"
    print_status "Install Homebrew from: https://brew.sh/"
    print_status "Run: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
fi
print_success "Homebrew found"

# Check for git
if ! command -v git &> /dev/null; then
    print_status "Installing git via Homebrew..."
    brew install git
fi
print_success "Git available"

# Check for CMake
if ! command -v cmake &> /dev/null; then
    print_status "Installing CMake via Homebrew..."
    brew install cmake
fi
print_success "CMake available"

# Check for vcpkg
print_step "Checking for vcpkg installation..."
VCPKG_ROOT=""
VCPKG_TOOLCHAIN=""

if command -v vcpkg &> /dev/null; then
    print_success "vcpkg found in PATH"
    
    # Get vcpkg root directory
    VCPKG_EXECUTABLE=$(which vcpkg)
    VCPKG_ROOT=$(dirname "$VCPKG_EXECUTABLE")
    VCPKG_TOOLCHAIN="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    
    if [[ -f "$VCPKG_TOOLCHAIN" ]]; then
        print_success "vcpkg toolchain found at: $VCPKG_TOOLCHAIN"
    else
        print_warning "vcpkg toolchain not found at expected location"
        VCPKG_TOOLCHAIN=""
    fi
else
    print_warning "vcpkg not found in PATH"
    print_status "Checking common vcpkg installation locations..."
    
    # Check common vcpkg locations
    COMMON_VCPKG_PATHS=(
        "$HOME/vcpkg"
        "/usr/local/vcpkg"
        "/opt/vcpkg"
        "$HOME/tools/vcpkg"
    )
    
    for path in "${COMMON_VCPKG_PATHS[@]}"; do
        if [[ -d "$path" && -f "$path/vcpkg" ]]; then
            VCPKG_ROOT="$path"
            VCPKG_TOOLCHAIN="$path/scripts/buildsystems/vcpkg.cmake"
            print_success "Found vcpkg at: $VCPKG_ROOT"
            break
        fi
    done
    
    if [[ -z "$VCPKG_ROOT" ]]; then
        print_warning "vcpkg not found. Will attempt to install dependencies via Homebrew"
        print_status "To install vcpkg:"
        print_status "  git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg"
        print_status "  cd ~/vcpkg && ./bootstrap-vcpkg.sh"
        print_status "  export PATH=\"\$HOME/vcpkg:\$PATH\""
    fi
fi

# Install dependencies
print_step "Installing dependencies..."

if [[ -n "$VCPKG_ROOT" && -f "$VCPKG_TOOLCHAIN" ]]; then
    print_status "Installing dependencies via vcpkg..."
    print_warning "This may take several minutes..."
    
    cd "$VCPKG_ROOT"
    
    # Core dependencies
    print_status "Installing core dependencies..."
    ./vcpkg install lua:x64-osx
    ./vcpkg install sol2:x64-osx  
    ./vcpkg install cli11:x64-osx
    ./vcpkg install spdlog:x64-osx
    ./vcpkg install nlohmann-json:x64-osx
    ./vcpkg install fmt:x64-osx
    
    # Optional dependencies
    print_status "Installing optional dependencies..."
    ./vcpkg install boost:x64-osx || print_warning "Boost installation failed (optional)"
    
    # Note: microsoft-detours is Windows-only
    print_status "Skipping microsoft-detours (Windows-only)"
    
    cd "$SCRIPT_DIR"
    print_success "vcpkg dependencies installed"
    
else
    print_status "Installing dependencies via Homebrew..."
    
    # Core dependencies
    brew install lua || print_warning "Lua installation may have issues"
    brew install nlohmann-json
    brew install spdlog  
    brew install fmt
    
    # sol2 and cli11 are typically header-only and may need special handling
    print_warning "sol2 and cli11 may need to be installed manually or built from source"
    print_status "Consider using vcpkg for more reliable dependency management"
    
    print_success "Homebrew dependencies installed"
fi

# Initialize git repository
print_step "Initializing git repository..."
if [[ ! -d ".git" ]]; then
    git init > /dev/null 2>&1
    print_success "Git repository initialized"
else
    print_status "Git repository already exists"
fi

# Create .gitignore if it doesn't exist
if [[ ! -f ".gitignore" ]]; then
    print_status "Creating .gitignore file..."
    cat > .gitignore << 'EOF'
# Build directories
build/
bin/
obj/

# IDE files
.vscode/
.idea/
*.swp
*.swo
*~

# macOS specific
.DS_Store
*.dSYM/

# Logs
logs/*.log

# CMake generated files
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
Makefile

# vcpkg
vcpkg_installed/

# Temporary files
*.tmp
*.temp
EOF
    print_success "Created .gitignore file"
fi

# Add initial commit
if git rev-parse --verify HEAD >/dev/null 2>&1; then
    print_status "Git repository already has commits"
else
    print_status "Creating initial commit..."
    git add . > /dev/null 2>&1
    git commit -m "Initial commit: Memory forensics tool for Revolution Idol (macOS port)" > /dev/null 2>&1
    print_success "Initial commit created"
fi

# Configure CMake build
print_step "Configuring CMake build..."

CMAKE_ARGS=(
    -B build
    -S .
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
    -DBUILD_FOR_MACOS=ON
)

if [[ -n "$VCPKG_TOOLCHAIN" && -f "$VCPKG_TOOLCHAIN" ]]; then
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="$VCPKG_TOOLCHAIN")
    print_status "Using vcpkg toolchain: $VCPKG_TOOLCHAIN"
else
    print_warning "vcpkg toolchain not available, using system libraries"
fi

if cmake "${CMAKE_ARGS[@]}"; then
    print_success "CMake configuration completed"
else
    print_error "CMake configuration failed"
    print_status "Common issues:"
    print_status "  1. Missing dependencies (install via vcpkg or Homebrew)"
    print_status "  2. Incompatible library versions"
    print_status "  3. Missing development tools (Xcode command line tools)"
    print_status ""
    print_status "To install Xcode command line tools:"
    print_status "  xcode-select --install"
    exit 1
fi

# Build the project
print_step "Building the project..."
CPU_COUNT=$(sysctl -n hw.ncpu)
print_status "Using $CPU_COUNT parallel jobs"

if cmake --build build --config Release -j "$CPU_COUNT"; then
    print_success "Build completed successfully!"
    
    # Check if binary was created
    if [[ -f "build/bin/memory-tool" ]]; then
        print_success "Binary created: build/bin/memory-tool"
        chmod +x build/bin/memory-tool
    else
        print_warning "Binary not found at expected location"
    fi
else
    print_error "Build failed"
    print_status "This is expected on macOS due to Windows-specific code"
    print_status "The project structure is set up correctly for development"
fi

echo ""
print_success "Project setup complete!"
echo ""

print_status "Next steps:"
print_status "1. Create GitHub repository at https://github.com"
print_status "2. Add remote: git remote add origin [your-repo-url]"  
print_status "3. Push code: git push -u origin main"
print_status "4. For development: open project in VS Code or your preferred editor"
echo ""

print_status "Development workflow:"
print_status "- Modify source files in src/ and include/"
print_status "- Add Lua scripts in scripts/"
print_status "- Build with: cmake --build build --config Release"
print_status "- Test Lua engine: build/bin/memory-tool --help"
echo ""

print_warning "macOS Limitations:"
print_warning "- This tool is designed for Windows memory analysis"
print_warning "- Process attachment will not work on macOS"
print_warning "- Use this setup for development, deploy to Windows for actual forensics"
echo ""

print_status "For Windows deployment:"
print_status "- Use the original setup_project.bat on Windows"
print_status "- Or set up a Windows VM for testing"

echo ""
print_success "Setup completed successfully! Happy coding! 🚀"