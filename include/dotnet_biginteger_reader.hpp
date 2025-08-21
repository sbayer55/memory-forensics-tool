#pragma once

#include "common.hpp"
#include "memory_scanner.hpp"
#include "app_logger.hpp"
#include <vector>
#include <memory>

namespace MemoryForensics {

// .NET BigInteger internal structure representation
struct DotNetBigIntegerData {
    int32_t sign;           // Sign of the number (-1, 0, 1)
    uint32_t* bits_ptr;     // Pointer to array of uint32 values
    uint32_t bits_length;   // Length of the bits array
    std::vector<uint32_t> bits_data; // Actual bits data read from memory
    bool is_valid = false;
};

class DotNetBigIntegerReader {
public:
    explicit DotNetBigIntegerReader(std::shared_ptr<MemoryScanner> scanner);
    ~DotNetBigIntegerReader() = default;
    
    // Read a .NET BigInteger from the specified memory address
    std::optional<DotNetBigIntegerData> ReadBigInteger(MemoryAddress base_address);
    
    // Read and parse BigInteger with detailed logging
    std::optional<DotNetBigIntegerData> ReadBigIntegerVerbose(MemoryAddress base_address);
    
    // Validate if the data at address looks like a valid BigInteger
    bool IsValidBigInteger(MemoryAddress base_address);
    
    // Convert BigInteger data to string representation
    std::string BigIntegerToString(const DotNetBigIntegerData& bigint);
    
    // Convert BigInteger data to hexadecimal string
    std::string BigIntegerToHex(const DotNetBigIntegerData& bigint);

private:
    std::shared_ptr<MemoryScanner> scanner_;
    
    // Helper methods
    void LogMemoryValue(const std::string& field_name, MemoryAddress address, 
                       const std::string& value_str);
    
    template<typename T>
    void LogTypedValue(const std::string& field_name, MemoryAddress address, T value);
    
    bool IsValidPointer(MemoryAddress address);
    bool IsValidBitsLength(uint32_t length);
    
    // Constants for .NET BigInteger validation
    static constexpr uint32_t MAX_REASONABLE_BITS_LENGTH = 10000;  // Reasonable upper limit
    static constexpr MemoryAddress MIN_VALID_POINTER = 0x10000;
    static constexpr MemoryAddress MAX_VALID_POINTER = 0x7FFFFFFFFFFF;
};

// Template implementation
template<typename T>
void DotNetBigIntegerReader::LogTypedValue(const std::string& field_name, MemoryAddress address, T value) {
    if constexpr (std::is_integral_v<T>) {
        if constexpr (std::is_same_v<T, bool>) {
            LogMemoryValue(field_name, address, value ? "true" : "false");
        } else if constexpr (std::is_pointer_v<T> || std::is_same_v<T, MemoryAddress>) {
            LogMemoryValue(field_name, address, fmt::format("0x{:X}", static_cast<uintptr_t>(value)));
        } else {
            LogMemoryValue(field_name, address, std::to_string(value));
        }
    } else {
        LogMemoryValue(field_name, address, "unknown_type");
    }
}

} // namespace MemoryForensics