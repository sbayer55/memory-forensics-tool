#pragma once

#include "common.hpp"
#include "memory_scanner.hpp"
#include "app_logger.hpp"
#include "dotnet_biginteger_reader.hpp"
#include <vector>
#include <memory>

namespace MemoryForensics {

// SerializableBigInteger.BigIntegerContents structure
struct BigIntegerContents {
    int32_t sign;
    uint32_t* bits_ptr;         // Pointer to uint32 array
    uint32_t bits_length;       // Length of bits array (not stored in original, but derived)
    std::vector<uint32_t> bits_data; // Actual bits data read from memory
};

// SerializableBigInteger structure (uses union layout in C#)
struct SerializableBigInteger {
    // The C# struct uses FieldOffset(0) for both value and raw, meaning they're overlaid
    // We'll read both representations to handle the union properly
    
    // BigInteger representation (standard .NET BigInteger)
    DotNetBigIntegerData bigint_value;
    
    // Raw representation (for encryption/decryption)
    BigIntegerContents raw_contents;
    
    bool is_valid = false;
};

// ObscuredBigInteger structure
struct ObscuredBigIntegerData {
    SerializableBigInteger hidden_value;    // Encrypted value
    SerializableBigInteger fake_value;      // Fake value for cheat detection
    uint32_t current_crypto_key;            // Encryption key
    bool fake_value_active;                 // Whether fake value checking is active
    bool inited;                           // Whether the struct is initialized
    bool is_valid = false;
};

class ObscuredBigIntegerReader {
public:
    explicit ObscuredBigIntegerReader(std::shared_ptr<MemoryScanner> scanner);
    ~ObscuredBigIntegerReader() = default;
    
    // Read an ObscuredBigInteger from the specified memory address
    std::optional<ObscuredBigIntegerData> ReadObscuredBigInteger(MemoryAddress base_address);
    
    // Read with detailed logging and indentation
    std::optional<ObscuredBigIntegerData> ReadObscuredBigIntegerVerbose(MemoryAddress base_address);
    
    // Decrypt the hidden value using the crypto key
    std::optional<DotNetBigIntegerData> DecryptHiddenValue(const ObscuredBigIntegerData& obscured);
    
    // Validate if the data at address looks like a valid ObscuredBigInteger
    bool IsValidObscuredBigInteger(MemoryAddress base_address);
    
    // Convert decrypted value to string
    std::string DecryptedValueToString(const ObscuredBigIntegerData& obscured);
    
    // Convert decrypted value to hex string
    std::string DecryptedValueToHex(const ObscuredBigIntegerData& obscured);

private:
    std::shared_ptr<MemoryScanner> scanner_;
    std::shared_ptr<DotNetBigIntegerReader> bigint_reader_;
    
    // Helper methods for reading structures
    std::optional<SerializableBigInteger> ReadSerializableBigInteger(MemoryAddress address, 
                                                                   const std::string& field_name);
    std::optional<BigIntegerContents> ReadBigIntegerContents(MemoryAddress address, 
                                                           const std::string& field_name);
    
    // Decryption algorithm implementation (SymmetricShuffle)
    SerializableBigInteger DecryptSerializableBigInteger(const SerializableBigInteger& encrypted, 
                                                       uint32_t key);
    BigIntegerContents DecryptBigIntegerContents(const BigIntegerContents& encrypted, 
                                                uint32_t key);
    
    // Logging helpers
    void LogMemoryValue(const std::string& field_name, MemoryAddress address, 
                       const std::string& value_str);
    
    template<typename T>
    void LogTypedValue(const std::string& field_name, MemoryAddress address, T value);
    
    // Validation helpers
    bool IsValidPointer(MemoryAddress address);
    bool IsValidBitsArray(MemoryAddress bits_ptr, uint32_t length);
    
    // Constants
    static constexpr uint32_t MAX_REASONABLE_BITS_LENGTH = 10000;
    static constexpr MemoryAddress MIN_VALID_POINTER = 0x10000;
    static constexpr MemoryAddress MAX_VALID_POINTER = 0x7FFFFFFFFFFF;
};

// Template implementation
template<typename T>
void ObscuredBigIntegerReader::LogTypedValue(const std::string& field_name, MemoryAddress address, T value) {
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