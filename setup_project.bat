@echo off
echo Setting up Memory Forensics Tool Project...
echo.

REM Create directory structure
echo Creating project directories...
mkdir include 2>nul
mkdir src 2>nul
mkdir scripts\examples 2>nul
mkdir scripts\templates 2>nul
mkdir config 2>nul
mkdir tests\unit_tests 2>nul
mkdir logs 2>nul

REM Check for vcpkg
echo Checking for vcpkg installation...
where vcpkg >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: vcpkg not found in PATH
    echo Please install vcpkg and add it to your PATH
    echo See: https://github.com/Microsoft/vcpkg
    pause
    exit /b 1
)

REM Install dependencies
echo Installing vcpkg dependencies...
echo This may take several minutes...
vcpkg install lua:x64-windows
vcpkg install sol2:x64-windows
vcpkg install cli11:x64-windows
vcpkg install spdlog:x64-windows
vcpkg install nlohmann-json:x64-windows

echo.
echo Installing optional dependencies...
vcpkg install microsoft-detours:x64-windows
vcpkg install boost:x64-windows

REM Get vcpkg root for CMake configuration
for /f "tokens=*" %%i in ('vcpkg integrate install ^| findstr "CMake projects"') do set VCPKG_CMAKE=%%i
echo %VCPKG_CMAKE% | findstr /C:"CMake projects should use" >nul
if %ERRORLEVEL% EQU 0 (
    for /f "tokens=6 delims= " %%a in ("%VCPKG_CMAKE%") do set VCPKG_TOOLCHAIN=%%a
    set VCPKG_TOOLCHAIN=%VCPKG_TOOLCHAIN:"=%
)

REM Initialize git repository
echo Initializing git repository...
git init >nul 2>nul
git add . >nul 2>nul
git commit -m "Initial commit: Memory forensics tool for Revolution Idol" >nul 2>nul

REM Configure CMake
echo Configuring CMake build...
if defined VCPKG_TOOLCHAIN (
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%"
) else (
    echo WARNING: Could not detect vcpkg toolchain file
    echo Please configure CMake manually with:
    echo cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
)

echo.
echo Project setup complete!
echo.
echo Next steps:
echo 1. Create GitHub repository at https://github.com
echo 2. Add remote: git remote add origin [your-repo-url]
echo 3. Push code: git push -u origin main
echo 4. Build project: cmake --build build --config Release
echo.
echo To start development:
echo - Open project in Visual Studio or VS Code
echo - Modify source files in src/ and include/
echo - Add Lua scripts in scripts/
echo - Test with: build\bin\Release\memory-tool.exe --help
echo.
pause