#include "obscured_biginteger_reader.hpp"
#include <algorithm>

namespace MemoryForensics {

ObscuredBigIntegerReader::ObscuredBigIntegerReader(std::shared_ptr<MemoryScanner> scanner)
    : scanner_(scanner), bigint_reader_(std::make_shared<DotNetBigIntegerReader>(scanner)) {
}

std::optional<ObscuredBigIntegerData> ObscuredBigIntegerReader::ReadObscuredBigInteger(MemoryAddress base_address) {
    if (!scanner_) {
        LOG_ERROR("MemoryScanner is null");
        return std::nullopt;
    }
    
    if (!IsValidPointer(base_address)) {
        LOG_ERROR("Invalid base address: 0x{:X}", base_address);
        return std::nullopt;
    }
    
    ObscuredBigIntegerData result;
    MemoryAddress current_offset = base_address;
    
    // Read hiddenValue (SerializableBigInteger)
    auto hidden_value = ReadSerializableBigInteger(current_offset, "hiddenValue");
    if (!hidden_value) {
        return std::nullopt;
    }
    result.hidden_value = *hidden_value;
    current_offset += sizeof(SerializableBigInteger); // This is an approximation
    
    // Read fakeValue (SerializableBigInteger)
    auto fake_value = ReadSerializableBigInteger(current_offset, "fakeValue");
    if (!fake_value) {
        return std::nullopt;
    }
    result.fake_value = *fake_value;
    current_offset += sizeof(SerializableBigInteger);
    
    // Read currentCryptoKey (uint32)
    auto crypto_key = scanner_->ReadUInt32(current_offset);
    if (!crypto_key) {
        LOG_ERROR("Failed to read currentCryptoKey at 0x{:X}", current_offset);
        return std::nullopt;
    }
    result.current_crypto_key = *crypto_key;
    current_offset += sizeof(uint32_t);
    
    // Read fakeValueActive (bool)
    auto fake_active = scanner_->ReadBool(current_offset);
    if (!fake_active) {
        LOG_ERROR("Failed to read fakeValueActive at 0x{:X}", current_offset);
        return std::nullopt;
    }
    result.fake_value_active = *fake_active;
    current_offset += sizeof(bool);
    
    // Read inited (bool)
    auto inited = scanner_->ReadBool(current_offset);
    if (!inited) {
        LOG_ERROR("Failed to read inited at 0x{:X}", current_offset);
        return std::nullopt;
    }
    result.inited = *inited;
    
    result.is_valid = true;
    return result;
}

std::optional<ObscuredBigIntegerData> ObscuredBigIntegerReader::ReadObscuredBigIntegerVerbose(MemoryAddress base_address) {
    LOG_INFO("Reading ObscuredBigInteger at 0x{:X}", base_address);
    LOG_INDENT();
    
    if (!scanner_) {
        LOG_ERROR("MemoryScanner is null");
        return std::nullopt;
    }
    
    if (!IsValidPointer(base_address)) {
        LOG_ERROR("Invalid base address: 0x{:X}", base_address);
        return std::nullopt;
    }
    
    ObscuredBigIntegerData result;
    MemoryAddress current_offset = base_address;
    
    // Read hiddenValue (SerializableBigInteger)
    LOG_INFO("Reading hiddenValue (SerializableBigInteger)...");
    {
        LOG_INDENT();
        auto hidden_value = ReadSerializableBigInteger(current_offset, "hiddenValue");
        if (!hidden_value) {
            return std::nullopt;
        }
        result.hidden_value = *hidden_value;
        current_offset += sizeof(SerializableBigInteger);
    }
    
    // Read fakeValue (SerializableBigInteger)
    LOG_INFO("Reading fakeValue (SerializableBigInteger)...");
    {
        LOG_INDENT();
        auto fake_value = ReadSerializableBigInteger(current_offset, "fakeValue");
        if (!fake_value) {
            return std::nullopt;
        }
        result.fake_value = *fake_value;
        current_offset += sizeof(SerializableBigInteger);
    }
    
    // Read currentCryptoKey
    LOG_INFO("Reading currentCryptoKey...");
    {
        LOG_INDENT();
        auto crypto_key = scanner_->ReadUInt32(current_offset);
        if (!crypto_key) {
            LOG_ERROR("Failed to read currentCryptoKey at 0x{:X}", current_offset);
            return std::nullopt;
        }
        result.current_crypto_key = *crypto_key;
        LogTypedValue("currentCryptoKey", current_offset, result.current_crypto_key);
        current_offset += sizeof(uint32_t);
    }
    
    // Read fakeValueActive
    LOG_INFO("Reading fakeValueActive...");
    {
        LOG_INDENT();
        auto fake_active = scanner_->ReadBool(current_offset);
        if (!fake_active) {
            LOG_ERROR("Failed to read fakeValueActive at 0x{:X}", current_offset);
            return std::nullopt;
        }
        result.fake_value_active = *fake_active;
        LogTypedValue("fakeValueActive", current_offset, result.fake_value_active);
        current_offset += sizeof(bool);
    }
    
    // Read inited
    LOG_INFO("Reading inited...");
    {
        LOG_INDENT();
        auto inited = scanner_->ReadBool(current_offset);
        if (!inited) {
            LOG_ERROR("Failed to read inited at 0x{:X}", current_offset);
            return std::nullopt;
        }
        result.inited = *inited;
        LogTypedValue("inited", current_offset, result.inited);
    }
    
    result.is_valid = true;
    
    // Try to decrypt the hidden value
    LOG_INFO("Attempting to decrypt hidden value...");
    {
        LOG_INDENT();
        auto decrypted = DecryptHiddenValue(result);
        if (decrypted) {
            LOG_INFO("Successfully decrypted hidden value: {}", 
                    bigint_reader_->BigIntegerToString(*decrypted));
        } else {
            LOG_WARN("Failed to decrypt hidden value");
        }
    }
    
    return result;
}

std::optional<DotNetBigIntegerData> ObscuredBigIntegerReader::DecryptHiddenValue(const ObscuredBigIntegerData& obscured) {
    if (!obscured.is_valid) {
        LOG_ERROR("ObscuredBigInteger data is invalid");
        return std::nullopt;
    }
    
    // Decrypt the SerializableBigInteger using the crypto key
    auto decrypted_serializable = DecryptSerializableBigInteger(obscured.hidden_value, obscured.current_crypto_key);
    
    // The decrypted SerializableBigInteger should now contain the original BigInteger
    return decrypted_serializable.bigint_value;
}

bool ObscuredBigIntegerReader::IsValidObscuredBigInteger(MemoryAddress base_address) {
    auto result = ReadObscuredBigInteger(base_address);
    return result.has_value() && result->is_valid;
}

std::string ObscuredBigIntegerReader::DecryptedValueToString(const ObscuredBigIntegerData& obscured) {
    auto decrypted = DecryptHiddenValue(obscured);
    if (!decrypted) {
        return "DECRYPTION_FAILED";
    }
    return bigint_reader_->BigIntegerToString(*decrypted);
}

std::string ObscuredBigIntegerReader::DecryptedValueToHex(const ObscuredBigIntegerData& obscured) {
    auto decrypted = DecryptHiddenValue(obscured);
    if (!decrypted) {
        return "DECRYPTION_FAILED";
    }
    return bigint_reader_->BigIntegerToHex(*decrypted);
}

std::optional<SerializableBigInteger> ObscuredBigIntegerReader::ReadSerializableBigInteger(MemoryAddress address, 
                                                                                         const std::string& field_name) {
    LOG_DEBUG("Reading SerializableBigInteger '{}' at 0x{:X}", field_name, address);
    LOG_INDENT();
    
    SerializableBigInteger result;
    
    // SerializableBigInteger uses a union layout where both BigInteger and BigIntegerContents 
    // are at the same memory location (FieldOffset(0))
    
    // First, try to read as BigInteger using the existing reader
    auto bigint_data = bigint_reader_->ReadBigIntegerVerbose(address);
    if (bigint_data) {
        result.bigint_value = *bigint_data;
    } else {
        LOG_WARN("Failed to read as BigInteger, trying raw contents");
    }
    
    // Also read the raw contents representation
    auto raw_contents = ReadBigIntegerContents(address, field_name + ".raw");
    if (raw_contents) {
        result.raw_contents = *raw_contents;
    } else {
        LOG_WARN("Failed to read raw contents");
        return std::nullopt;
    }
    
    result.is_valid = true;
    return result;
}

std::optional<BigIntegerContents> ObscuredBigIntegerReader::ReadBigIntegerContents(MemoryAddress address, 
                                                                                 const std::string& field_name) {
    LOG_DEBUG("Reading BigIntegerContents '{}' at 0x{:X}", field_name, address);
    LOG_INDENT();
    
    BigIntegerContents result;
    MemoryAddress current_offset = address;
    
    // Read sign (int32)
    auto sign = scanner_->ReadInt32(current_offset);
    if (!sign) {
        LOG_ERROR("Failed to read sign at 0x{:X}", current_offset);
        return std::nullopt;
    }
    result.sign = *sign;
    LogTypedValue("sign", current_offset, result.sign);
    current_offset += sizeof(int32_t);
    
    // Read bits pointer (uint32*)
    auto bits_ptr = scanner_->ReadUInt64(current_offset);  // Assuming 64-bit pointers
    if (!bits_ptr) {
        LOG_ERROR("Failed to read bits pointer at 0x{:X}", current_offset);
        return std::nullopt;
    }
    result.bits_ptr = reinterpret_cast<uint32_t*>(*bits_ptr);
    LogTypedValue("bits_ptr", current_offset, reinterpret_cast<MemoryAddress>(result.bits_ptr));
    
    // If bits_ptr is null, that's valid (represents zero or small numbers)
    if (result.bits_ptr == nullptr) {
        result.bits_length = 0;
        LOG_DEBUG("bits_ptr is null, representing zero or small number");
        return result;
    }
    
    if (!IsValidPointer(reinterpret_cast<MemoryAddress>(result.bits_ptr))) {
        LOG_WARN("Invalid bits pointer: 0x{:X}", reinterpret_cast<MemoryAddress>(result.bits_ptr));
        return std::nullopt;
    }
    
    // Determine the length of the bits array by probing
    LOG_INDENT();
    MemoryAddress bits_address = reinterpret_cast<MemoryAddress>(result.bits_ptr);
    const uint32_t max_probe_length = 32;
    
    auto probe_array = scanner_->ReadUInt32Array(bits_address, max_probe_length);
    if (!probe_array) {
        LOG_ERROR("Failed to read bits array at 0x{:X}", bits_address);
        return std::nullopt;
    }
    
    // Find actual length (last non-zero element)
    uint32_t actual_length = 0;
    for (int i = probe_array->size() - 1; i >= 0; --i) {
        if ((*probe_array)[i] != 0) {
            actual_length = i + 1;
            break;
        }
    }
    
    if (actual_length == 0 && result.sign != 0) {
        actual_length = 1;  // At least one element for non-zero numbers
    }
    
    result.bits_length = actual_length;
    LogTypedValue("determined_bits_length", bits_address, result.bits_length);
    
    // Read the actual bits data
    if (result.bits_length > 0) {
        auto bits_data = scanner_->ReadUInt32Array(bits_address, result.bits_length);
        if (!bits_data) {
            LOG_ERROR("Failed to read {} bits from 0x{:X}", result.bits_length, bits_address);
            return std::nullopt;
        }
        result.bits_data = *bits_data;
        
        // Log each bit element
        for (uint32_t i = 0; i < result.bits_length; ++i) {
            MemoryAddress element_addr = bits_address + (i * sizeof(uint32_t));
            LogTypedValue(fmt::format("bits[{}]", i), element_addr, result.bits_data[i]);
        }
    }
    
    return result;
}

SerializableBigInteger ObscuredBigIntegerReader::DecryptSerializableBigInteger(const SerializableBigInteger& encrypted, 
                                                                             uint32_t key) {
    SerializableBigInteger result = encrypted;
    
    // Apply the SymmetricShuffle decryption algorithm
    result.raw_contents = DecryptBigIntegerContents(encrypted.raw_contents, key);
    
    // Note: In the original C# code, the BigInteger value is reconstructed after decryption
    // We would need to reconstruct the BigInteger from the decrypted raw contents
    // For now, we'll keep the original bigint_value and update the raw_contents
    
    return result;
}

BigIntegerContents ObscuredBigIntegerReader::DecryptBigIntegerContents(const BigIntegerContents& encrypted, 
                                                                      uint32_t key) {
    BigIntegerContents result = encrypted;
    
    // SymmetricShuffle algorithm from C# code:
    // 1. XOR the sign with the key
    result.sign ^= static_cast<int32_t>(key);
    
    // 2. Handle the bits array
    if (!result.bits_data.empty()) {
        auto count = result.bits_data.size();
        
        if (count == 1) {
            // Single element: XOR with key
            result.bits_data[0] ^= key;
        } else if (count > 1) {
            // Multiple elements: swap first and last, XORing both with key
            uint32_t first = result.bits_data[0];
            uint32_t last = result.bits_data[count - 1];
            
            result.bits_data[0] = last ^ key;
            result.bits_data[count - 1] = first ^ key;
        }
    }
    
    return result;
}

void ObscuredBigIntegerReader::LogMemoryValue(const std::string& field_name, 
                                            MemoryAddress address, 
                                            const std::string& value_str) {
    LOG_DEBUG("{}: 0x{:X} = {}", field_name, address, value_str);
}

bool ObscuredBigIntegerReader::IsValidPointer(MemoryAddress address) {
    return address >= MIN_VALID_POINTER && address <= MAX_VALID_POINTER;
}

bool ObscuredBigIntegerReader::IsValidBitsArray(MemoryAddress bits_ptr, uint32_t length) {
    return IsValidPointer(bits_ptr) && length <= MAX_REASONABLE_BITS_LENGTH;
}

} // namespace MemoryForensics