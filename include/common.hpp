#pragma once

#include "platform_compat.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <optional>
#include <cstdint>

// Logging
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

// JSON configuration
#include <nlohmann/json.hpp>

// Additional formatting support
#include <fmt/format.h>

namespace MemoryForensics {
    // Type aliases for clarity
    using ProcessID = DWORD;
    using MemoryAddress = uintptr_t;
    using ByteVector = std::vector<uint8_t>;
    
    // Constants
    constexpr const char* TARGET_PROCESS_NAME = "Revolution Idol.exe";
    constexpr size_t MAX_READ_SIZE = 0x1000000; // 16MB max read
    constexpr size_t SCAN_CHUNK_SIZE = 0x10000;  // 64KB chunks
    
    // Common structures
    struct MemoryRegion {
        MemoryAddress base_address;
        size_t size;
        DWORD protection;
        std::string name;
    };
    
    struct EncryptedBigInteger {
        MemoryAddress container_address;
        MemoryAddress bigint_ptr;
        MemoryAddress key_ptr;
        ByteVector encrypted_data;
        ByteVector decryption_key;
        bool is_decrypted = false;
    };
    
    // Exception types
    class MemoryException : public std::runtime_error {
    public:
        explicit MemoryException(const std::string& msg) : std::runtime_error(msg) {}
    };
    
    class ProcessException : public std::runtime_error {
    public:
        explicit ProcessException(const std::string& msg) : std::runtime_error(msg) {}
    };
    
    class DecryptionException : public std::runtime_error {
    public:
        explicit DecryptionException(const std::string& msg) : std::runtime_error(msg) {}
    };
    
    // Utility functions
    std::string GetLastErrorString();
    bool IsValidPointer(MemoryAddress address);
    std::vector<uint8_t> HexStringToBytes(const std::string& hex);
    std::string BytesToHexString(const std::vector<uint8_t>& bytes);
}