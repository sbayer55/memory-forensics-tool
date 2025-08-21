#include "dotnet_biginteger_reader.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace MemoryForensics {

DotNetBigIntegerReader::DotNetBigIntegerReader(std::shared_ptr<MemoryScanner> scanner)
    : scanner_(scanner) {
}

std::optional<DotNetBigIntegerData> DotNetBigIntegerReader::ReadBigInteger(MemoryAddress base_address) {
    if (!scanner_) {
        LOG_ERROR("MemoryScanner is null");
        return std::nullopt;
    }
    
    if (!IsValidPointer(base_address)) {
        LOG_ERROR("Invalid base address: 0x{:X}", base_address);
        return std::nullopt;
    }
    
    DotNetBigIntegerData result;
    
    // Read the sign field (typically at offset 0)
    auto sign_opt = scanner_->ReadInt32(base_address);
    if (!sign_opt) {
        LOG_ERROR("Failed to read sign field at 0x{:X}", base_address);
        return std::nullopt;
    }
    result.sign = *sign_opt;
    
    // Read the bits pointer (typically at offset 4 or 8 depending on architecture)
    MemoryAddress bits_ptr_address = base_address + sizeof(int32_t);
    auto bits_ptr_opt = scanner_->ReadUInt64(bits_ptr_address);  // Assuming 64-bit pointers
    if (!bits_ptr_opt) {
        LOG_ERROR("Failed to read bits pointer at 0x{:X}", bits_ptr_address);
        return std::nullopt;
    }
    result.bits_ptr = reinterpret_cast<uint32_t*>(*bits_ptr_opt);
    
    // Validate the bits pointer
    if (!IsValidPointer(reinterpret_cast<MemoryAddress>(result.bits_ptr))) {
        LOG_WARN("Invalid bits pointer: 0x{:X}", reinterpret_cast<MemoryAddress>(result.bits_ptr));
        return std::nullopt;
    }
    
    // Read the bits length (may be stored separately or derived)
    // For now, we'll try to determine it by reading a reasonable amount
    // In a real .NET BigInteger, this information might be stored differently
    
    // Try to read the first few uint32 values to determine the actual length
    const uint32_t max_probe_length = 32;  // Probe up to 32 uint32 values
    MemoryAddress bits_address = reinterpret_cast<MemoryAddress>(result.bits_ptr);
    
    auto probe_array = scanner_->ReadUInt32Array(bits_address, max_probe_length);
    if (!probe_array) {
        LOG_ERROR("Failed to read bits array at 0x{:X}", bits_address);
        return std::nullopt;
    }
    
    // Find the actual length by looking for the last non-zero value
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
    
    if (!IsValidBitsLength(result.bits_length)) {
        LOG_WARN("Invalid bits length: {}", result.bits_length);
        return std::nullopt;
    }
    
    // Read the actual bits data
    if (result.bits_length > 0) {
        auto bits_data_opt = scanner_->ReadUInt32Array(bits_address, result.bits_length);
        if (!bits_data_opt) {
            LOG_ERROR("Failed to read {} bits from 0x{:X}", result.bits_length, bits_address);
            return std::nullopt;
        }
        result.bits_data = *bits_data_opt;
    }
    
    result.is_valid = true;
    return result;
}

std::optional<DotNetBigIntegerData> DotNetBigIntegerReader::ReadBigIntegerVerbose(MemoryAddress base_address) {
    LOG_INFO("Reading .NET BigInteger at 0x{:X}", base_address);
    LOG_INDENT();
    
    if (!scanner_) {
        LOG_ERROR("MemoryScanner is null");
        return std::nullopt;
    }
    
    if (!IsValidPointer(base_address)) {
        LOG_ERROR("Invalid base address: 0x{:X}", base_address);
        return std::nullopt;
    }
    
    DotNetBigIntegerData result;
    
    // Read and log the sign field
    LOG_INFO("Reading sign field...");
    {
        LOG_INDENT();
        auto sign_opt = scanner_->ReadInt32(base_address);
        if (!sign_opt) {
            LOG_ERROR("Failed to read sign field at 0x{:X}", base_address);
            return std::nullopt;
        }
        result.sign = *sign_opt;
        LogTypedValue("sign", base_address, result.sign);
    }
    
    // Read and log the bits pointer
    LOG_INFO("Reading bits pointer...");
    {
        LOG_INDENT();
        MemoryAddress bits_ptr_address = base_address + sizeof(int32_t);
        auto bits_ptr_opt = scanner_->ReadUInt64(bits_ptr_address);
        if (!bits_ptr_opt) {
            LOG_ERROR("Failed to read bits pointer at 0x{:X}", bits_ptr_address);
            return std::nullopt;
        }
        result.bits_ptr = reinterpret_cast<uint32_t*>(*bits_ptr_opt);
        LogTypedValue("bits_ptr", bits_ptr_address, reinterpret_cast<MemoryAddress>(result.bits_ptr));
        
        if (!IsValidPointer(reinterpret_cast<MemoryAddress>(result.bits_ptr))) {
            LOG_WARN("Invalid bits pointer: 0x{:X}", reinterpret_cast<MemoryAddress>(result.bits_ptr));
            return std::nullopt;
        }
    }
    
    // Probe and determine bits length
    LOG_INFO("Determining bits array length...");
    {
        LOG_INDENT();
        const uint32_t max_probe_length = 32;
        MemoryAddress bits_address = reinterpret_cast<MemoryAddress>(result.bits_ptr);
        
        auto probe_array = scanner_->ReadUInt32Array(bits_address, max_probe_length);
        if (!probe_array) {
            LOG_ERROR("Failed to read bits array at 0x{:X}", bits_address);
            return std::nullopt;
        }
        
        // Find actual length
        uint32_t actual_length = 0;
        for (int i = probe_array->size() - 1; i >= 0; --i) {
            if ((*probe_array)[i] != 0) {
                actual_length = i + 1;
                break;
            }
        }
        
        if (actual_length == 0 && result.sign != 0) {
            actual_length = 1;
        }
        
        result.bits_length = actual_length;
        LogTypedValue("determined_bits_length", bits_address, result.bits_length);
        
        if (!IsValidBitsLength(result.bits_length)) {
            LOG_WARN("Invalid bits length: {}", result.bits_length);
            return std::nullopt;
        }
    }
    
    // Read and log the bits data
    if (result.bits_length > 0) {
        LOG_INFO("Reading bits array ({} elements)...", result.bits_length);
        LOG_INDENT();
        
        MemoryAddress bits_address = reinterpret_cast<MemoryAddress>(result.bits_ptr);
        auto bits_data_opt = scanner_->ReadUInt32Array(bits_address, result.bits_length);
        if (!bits_data_opt) {
            LOG_ERROR("Failed to read {} bits from 0x{:X}", result.bits_length, bits_address);
            return std::nullopt;
        }
        result.bits_data = *bits_data_opt;
        
        // Log each element in the bits array
        for (uint32_t i = 0; i < result.bits_length; ++i) {
            MemoryAddress element_addr = bits_address + (i * sizeof(uint32_t));
            LogTypedValue(fmt::format("bits[{}]", i), element_addr, result.bits_data[i]);
        }
    } else {
        LOG_INFO("BigInteger has zero length (represents zero)");
    }
    
    result.is_valid = true;
    
    // Log final result summary
    LOG_INFO("BigInteger parsing complete - Sign: {}, Length: {}, Value: {}", 
             result.sign, result.bits_length, BigIntegerToString(result));
    
    return result;
}

bool DotNetBigIntegerReader::IsValidBigInteger(MemoryAddress base_address) {
    auto result = ReadBigInteger(base_address);
    return result.has_value() && result->is_valid;
}

std::string DotNetBigIntegerReader::BigIntegerToString(const DotNetBigIntegerData& bigint) {
    if (!bigint.is_valid) {
        return "INVALID";
    }
    
    if (bigint.sign == 0 || bigint.bits_length == 0) {
        return "0";
    }
    
    // For simple cases, convert to string representation
    if (bigint.bits_length == 1) {
        int64_t value = static_cast<int64_t>(bigint.bits_data[0]);
        if (bigint.sign < 0) {
            value = -value;
        }
        return std::to_string(value);
    }
    
    // For larger numbers, return a hex representation for now
    return BigIntegerToHex(bigint);
}

std::string DotNetBigIntegerReader::BigIntegerToHex(const DotNetBigIntegerData& bigint) {
    if (!bigint.is_valid || bigint.bits_length == 0) {
        return "0x0";
    }
    
    std::stringstream ss;
    ss << "0x";
    if (bigint.sign < 0) {
        ss << "-";
    }
    
    // Print from most significant to least significant
    for (int i = bigint.bits_length - 1; i >= 0; --i) {
        if (i == static_cast<int>(bigint.bits_length) - 1) {
            ss << std::hex << bigint.bits_data[i];  // No leading zeros for first element
        } else {
            ss << std::setfill('0') << std::setw(8) << std::hex << bigint.bits_data[i];
        }
    }
    
    return ss.str();
}

void DotNetBigIntegerReader::LogMemoryValue(const std::string& field_name, 
                                          MemoryAddress address, 
                                          const std::string& value_str) {
    LOG_DEBUG("{}: 0x{:X} = {}", field_name, address, value_str);
}

bool DotNetBigIntegerReader::IsValidPointer(MemoryAddress address) {
    return address >= MIN_VALID_POINTER && address <= MAX_VALID_POINTER;
}

bool DotNetBigIntegerReader::IsValidBitsLength(uint32_t length) {
    return length <= MAX_REASONABLE_BITS_LENGTH;
}

} // namespace MemoryForensics