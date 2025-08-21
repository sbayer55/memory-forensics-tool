cmake_minimum_required(VERSION 3.20)

project(MemoryForensicsTool
    VERSION 1.0.0
    DESCRIPTION "Memory forensics tool for Revolution Idol Unity application"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(_WIN32_WINNT=0x0A00)  # Windows 10
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required packages
find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Optional packages
find_package(Boost QUIET)
find_package(Microsoft.Detours CONFIG QUIET)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/memory_scanner.cpp
    src/lua_engine.cpp
    src/process_manager.cpp
    src/decryption_engine.cpp
    src/dotnet_parser.cpp
)

# Header files
set(HEADERS
    include/memory_scanner.hpp
    include/lua_engine.hpp
    include/process_manager.hpp
    include/decryption_engine.hpp
    include/dotnet_parser.hpp
    include/common.hpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LUA_LIBRARIES}
    sol2::sol2
    CLI11::CLI11
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        psapi
        tlhelp32
        advapi32
        kernel32
        user32
    )
    
    # Add Detours if available
    if(Microsoft.Detours_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft.Detours::Microsoft.Detours)
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_DETOURS)
    endif()
endif()

# Add Boost if available
if(Boost_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_BOOST)
endif()

# Include Lua headers
target_include_directories(${PROJECT_NAME} PRIVATE ${LUA_INCLUDE_DIR})

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "memory-tool"
)

# Copy configuration files to build directory
configure_file(${CMAKE_SOURCE_DIR}/config/default_config.json 
               ${CMAKE_BINARY_DIR}/bin/config/default_config.json 
               COPYONLY)

# Copy example scripts
file(GLOB LUA_SCRIPTS "scripts/*.lua")
foreach(script ${LUA_SCRIPTS})
    configure_file(${script} ${CMAKE_BINARY_DIR}/bin/scripts/ COPYONLY)
endforeach()

# Enable testing
enable_testing()

# Add subdirectory for tests if available
if(EXISTS ${CMAKE_SOURCE_DIR}/tests)
    add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY scripts/
    DESTINATION bin/scripts
    FILES_MATCHING PATTERN "*.lua"
)

install(FILES config/default_config.json
    DESTINATION bin/config
)

# vcpkg integration hint
message(STATUS "To install dependencies with vcpkg:")
message(STATUS "  vcpkg install lua sol2 cli11 spdlog nlohmann-json")
message(STATUS "  vcpkg install microsoft-detours --optional")
message(STATUS "Configure with: cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake")