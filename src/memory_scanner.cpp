#include "memory_scanner.hpp"
#include <algorithm>

namespace MemoryForensics {

MemoryScanner::MemoryScanner(std::shared_ptr<ProcessManager> process_mgr)
    : process_mgr_(process_mgr) {
}

// Primitive type reading functions
std::optional<int32_t> MemoryScanner::ReadInt32(MemoryAddress address) {
    return ReadValue<int32_t>(address);
}

std::optional<uint32_t> MemoryScanner::ReadUInt32(MemoryAddress address) {
    return ReadValue<uint32_t>(address);
}

std::optional<uint64_t> MemoryScanner::ReadUInt64(MemoryAddress address) {
    return ReadValue<uint64_t>(address);
}

std::optional<bool> MemoryScanner::ReadBool(MemoryAddress address) {
    return ReadValue<bool>(address);
}

std::optional<std::vector<uint32_t>> MemoryScanner::ReadUInt32Array(MemoryAddress address, size_t count) {
    if (count == 0) {
        return std::vector<uint32_t>();
    }
    
    std::vector<uint32_t> result;
    result.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        auto value = ReadUInt32(address + (i * sizeof(uint32_t)));
        if (!value) {
            return std::nullopt;  // Failed to read element
        }
        result.push_back(*value);
    }
    
    return result;
}

// Pattern scanning
std::vector<MemoryAddress> MemoryScanner::ScanForPattern(const ByteVector& pattern, const ByteVector& mask) {
    std::vector<MemoryAddress> results;
    
    for (const auto& region : scan_regions_) {
        if (!IsValidScanRegion(region)) {
            continue;
        }
        
        auto region_results = ScanRegionForPattern(region, pattern, mask);
        results.insert(results.end(), region_results.begin(), region_results.end());
    }
    
    return results;
}

std::vector<MemoryAddress> MemoryScanner::ScanForPattern(const std::string& hex_pattern) {
    auto pattern = HexStringToBytes(hex_pattern);
    return ScanForPattern(pattern);
}

// Specific structure scanning
std::vector<MemoryAddress> MemoryScanner::FindContainerStructs() {
    // Implementation would scan for container struct signatures
    return {};
}

std::vector<EncryptedBigInteger> MemoryScanner::FindEncryptedBigIntegers() {
    // Implementation would find and return encrypted BigInteger objects
    return {};
}

// Memory reading utilities
ByteVector MemoryScanner::ReadBytes(MemoryAddress address, size_t size) {
    ByteVector buffer(size);
    if (process_mgr_->ReadMemory(address, buffer.data(), size)) {
        return buffer;
    }
    return {};
}

std::string MemoryScanner::ReadString(MemoryAddress address, size_t max_length) {
    ByteVector buffer = ReadBytes(address, max_length);
    if (buffer.empty()) {
        return "";
    }
    
    // Find null terminator
    auto null_pos = std::find(buffer.begin(), buffer.end(), 0);
    if (null_pos != buffer.end()) {
        buffer.resize(std::distance(buffer.begin(), null_pos));
    }
    
    return std::string(buffer.begin(), buffer.end());
}

// Pointer following
std::optional<MemoryAddress> MemoryScanner::FollowPointer(MemoryAddress ptr_address) {
    return ReadValue<MemoryAddress>(ptr_address);
}

std::vector<MemoryAddress> MemoryScanner::FollowPointerChain(MemoryAddress base, const std::vector<size_t>& offsets) {
    std::vector<MemoryAddress> chain;
    MemoryAddress current = base;
    
    for (size_t offset : offsets) {
        auto next = FollowPointer(current + offset);
        if (!next) {
            break;
        }
        current = *next;
        chain.push_back(current);
    }
    
    return chain;
}

// Signature management
void MemoryScanner::AddSignature(const std::string& name, const ByteVector& pattern) {
    signatures_[name] = pattern;
}

void MemoryScanner::LoadSignaturesFromConfig(const nlohmann::json& config) {
    if (config.contains("memory_scanning") && config["memory_scanning"].contains("signatures")) {
        for (const auto& [name, sig_config] : config["memory_scanning"]["signatures"].items()) {
            if (sig_config.contains("pattern")) {
                auto pattern = HexStringToBytes(sig_config["pattern"]);
                AddSignature(name, pattern);
            }
        }
    }
}

// Scanning configuration
void MemoryScanner::SetScanRegions(const std::vector<MemoryRegion>& regions) {
    scan_regions_ = regions;
}

void MemoryScanner::SetScanRange(MemoryAddress start, MemoryAddress end) {
    scan_regions_.clear();
    MemoryRegion region;
    region.base_address = start;
    region.size = end - start;
    region.protection = PAGE_READWRITE;
    scan_regions_.push_back(region);
}

void MemoryScanner::EnableProgressCallback(std::function<void(float)> callback) {
    progress_callback_ = callback;
}

// Private methods
bool MemoryScanner::IsValidScanRegion(const MemoryRegion& region) {
    return region.size > 0 && region.base_address != 0;
}

std::vector<MemoryAddress> MemoryScanner::ScanRegionForPattern(const MemoryRegion& region,
                                                              const ByteVector& pattern,
                                                              const ByteVector& mask) {
    std::vector<MemoryAddress> results;
    
    if (pattern.empty()) {
        return results;
    }
    
    auto region_data = ReadBytes(region.base_address, region.size);
    if (region_data.empty()) {
        return results;
    }
    
    // Simple pattern matching (could be optimized with Boyer-Moore or similar)
    for (size_t i = 0; i <= region_data.size() - pattern.size(); i += SCAN_ALIGNMENT) {
        bool match = true;
        
        for (size_t j = 0; j < pattern.size(); ++j) {
            if (!mask.empty() && mask[j] == 0) {
                continue; // Skip masked bytes
            }
            
            if (region_data[i + j] != pattern[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            results.push_back(region.base_address + i);
        }
    }
    
    return results;
}

bool MemoryScanner::IsContainerStruct(MemoryAddress address) {
    // Implementation would validate container struct signature
    return false;
}

bool MemoryScanner::ValidateContainerStruct(MemoryAddress address) {
    // Implementation would perform additional validation
    return false;
}

} // namespace MemoryForensics