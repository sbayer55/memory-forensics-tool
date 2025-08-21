#include "common.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace MemoryForensics {

std::string GetLastErrorString() {
    DWORD error = GetLastError();
    if (error == 0) {
        return "No error";
    }
    
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&messageBuffer),
        0,
        nullptr
    );
    
    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    
    // Remove trailing newlines
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }
    
    return message;
}

bool IsValidPointer(MemoryAddress address) {
    // Basic pointer validation
    if (address == 0) {
        return false;
    }
    
    // Check if it's in reasonable user space range
    if (address < 0x10000 || address > 0x7FFFFFFFFFFF) {
        return false;
    }
    
    return true;
}

std::vector<uint8_t> HexStringToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    
    // Remove spaces and convert to uppercase
    std::string clean_hex;
    for (char c : hex) {
        if (std::isxdigit(c)) {
            clean_hex += std::toupper(c);
        }
    }
    
    // Must have even number of hex digits
    if (clean_hex.length() % 2 != 0) {
        return bytes; // Return empty vector on error
    }
    
    for (size_t i = 0; i < clean_hex.length(); i += 2) {
        std::string byte_str = clean_hex.substr(i, 2);
        
        // Handle wildcards (? character)
        if (byte_str == "??") {
            bytes.push_back(0x00); // Use 0 for wildcards, caller should use mask
        } else {
            try {
                uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
                bytes.push_back(byte);
            } catch (const std::exception&) {
                return std::vector<uint8_t>(); // Return empty on parse error
            }
        }
    }
    
    return bytes;
}

std::string BytesToHexString(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0) {
            ss << " ";
        }
        ss << std::setw(2) << static_cast<unsigned>(bytes[i]);
    }
    
    return ss.str();
}

} // namespace MemoryForensics