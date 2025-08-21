#include "dotnet_parser.hpp"
#include "app_logger.hpp"
#include <algorithm>
#include <regex>

namespace MemoryForensics {

DotNetParser::DotNetParser(std::shared_ptr<ProcessManager> process_mgr)
    : process_mgr_(process_mgr) {
}

bool DotNetParser::IsValidObject(MemoryAddress object_addr) {
    if (!IsValidPointer(object_addr)) {
        return false;
    }
    
    // Read the object header
    ObjectHeader header;
    if (!process_mgr_->ReadMemory(object_addr, &header, sizeof(ObjectHeader))) {
        return false;
    }
    
    if (!ValidateObjectHeader(header)) {
        return false;
    }
    
    // Get the method table pointer (typically right after the object header)
    MemoryAddress method_table_addr;
    if (!process_mgr_->ReadMemory(object_addr + sizeof(ObjectHeader), &method_table_addr, sizeof(MemoryAddress))) {
        return false;
    }
    
    if (!IsValidPointer(method_table_addr)) {
        return false;
    }
    
    // Read and validate the method table
    MethodTable mt;
    if (!process_mgr_->ReadMemory(method_table_addr, &mt, sizeof(MethodTable))) {
        return false;
    }
    
    return ValidateMethodTable(mt);
}

std::optional<MethodTable> DotNetParser::GetMethodTable(MemoryAddress object_addr) {
    if (!IsValidPointer(object_addr)) {
        return std::nullopt;
    }
    
    // Check cache first
    auto cached = GetCachedMethodTable(object_addr);
    if (cached) {
        return cached;
    }
    
    // Read method table pointer from object
    MemoryAddress method_table_addr;
    if (!process_mgr_->ReadMemory(object_addr + sizeof(ObjectHeader), &method_table_addr, sizeof(MemoryAddress))) {
        return std::nullopt;
    }
    
    if (!IsValidPointer(method_table_addr)) {
        return std::nullopt;
    }
    
    // Read the method table
    MethodTable mt;
    if (!process_mgr_->ReadMemory(method_table_addr, &mt, sizeof(MethodTable))) {
        return std::nullopt;
    }
    
    if (!ValidateMethodTable(mt)) {
        return std::nullopt;
    }
    
    // Cache the result
    CacheMethodTable(method_table_addr, mt);
    
    return mt;
}

std::string DotNetParser::GetTypeName(MemoryAddress method_table_addr) {
    if (!IsValidPointer(method_table_addr)) {
        return "INVALID_ADDRESS";
    }
    
    // Check type name cache first
    auto it = type_name_cache_.find(method_table_addr);
    if (it != type_name_cache_.end()) {
        return ReadManagedString(it->second);
    }
    
    // Read method table
    MethodTable mt;
    if (!process_mgr_->ReadMemory(method_table_addr, &mt, sizeof(MethodTable))) {
        return "READ_FAILED";
    }
    
    if (!ValidateMethodTable(mt)) {
        return "INVALID_METHOD_TABLE";
    }
    
    // Try to get type name from EEClass
    std::string type_name = GetTypeNameFromEEClass(mt.ee_class_ptr);
    if (!type_name.empty() && type_name != "UNKNOWN") {
        return type_name;
    }
    
    // Fallback: use token information
    return fmt::format("UnknownType_0x{:X}", mt.token);
}

std::vector<MemoryAddress> DotNetParser::FindObjectsOfType(const std::string& type_name) {
    std::vector<MemoryAddress> results;
    
    // Get managed heap regions
    auto heap_regions = GetManagedHeapRegions();
    
    for (const auto& region : heap_regions) {
        // Scan through the region looking for objects
        MemoryAddress current_addr = region.base_address;
        MemoryAddress end_addr = region.base_address + region.size;
        
        while (current_addr < end_addr) {
            if (IsValidObject(current_addr)) {
                auto mt = GetMethodTable(current_addr);
                if (mt) {
                    std::string obj_type = GetTypeName(current_addr + sizeof(ObjectHeader));
                    if (obj_type.find(type_name) != std::string::npos) {
                        results.push_back(current_addr);
                        LOG_DEBUG("Found {} object at 0x{:X}", type_name, current_addr);
                    }
                }
            }
            
            // Move to next potential object (align to pointer size)
            current_addr += sizeof(MemoryAddress);
        }
    }
    
    return results;
}

std::vector<MemoryAddress> DotNetParser::ScanForBigIntegers() {
    LOG_INFO("Scanning for .NET BigInteger objects");
    
    std::vector<std::string> bigint_patterns = {
        "BigInteger",
        "System.Numerics.BigInteger",
        "SerializableBigInteger",
        "ObscuredBigInteger"
    };
    
    std::vector<MemoryAddress> all_results;
    
    for (const auto& pattern : bigint_patterns) {
        LOG_DEBUG("Searching for type: {}", pattern);
        auto results = FindObjectsOfType(pattern);
        all_results.insert(all_results.end(), results.begin(), results.end());
    }
    
    // Remove duplicates
    std::sort(all_results.begin(), all_results.end());
    all_results.erase(std::unique(all_results.begin(), all_results.end()), all_results.end());
    
    LOG_INFO("Found {} BigInteger-related objects", all_results.size());
    return all_results;
}

std::vector<MemoryRegion> DotNetParser::GetManagedHeapRegions() {
    std::vector<MemoryRegion> heap_regions;
    
    // Get all memory regions from the process
    MEMORY_BASIC_INFORMATION mbi;
    MemoryAddress current_addr = 0;
    
    while (VirtualQueryEx(process_mgr_->GetProcessHandle(), 
                         reinterpret_cast<LPCVOID>(current_addr), 
                         &mbi, sizeof(mbi)) == sizeof(mbi)) {
        
        // Look for committed memory regions that could contain managed heap
        if (mbi.State == MEM_COMMIT && 
            (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) &&
            mbi.RegionSize > 64 * 1024) { // At least 64KB regions
            
            MemoryRegion region;
            region.base_address = reinterpret_cast<MemoryAddress>(mbi.BaseAddress);
            region.size = mbi.RegionSize;
            region.protection = mbi.Protect;
            region.name = "PotentialManagedHeap";
            
            heap_regions.push_back(region);
        }
        
        current_addr = reinterpret_cast<MemoryAddress>(mbi.BaseAddress) + mbi.RegionSize;
    }
    
    LOG_DEBUG("Found {} potential managed heap regions", heap_regions.size());
    return heap_regions;
}

bool DotNetParser::IsInManagedHeap(MemoryAddress address) {
    auto heap_regions = GetManagedHeapRegions();
    
    for (const auto& region : heap_regions) {
        if (address >= region.base_address && 
            address < region.base_address + region.size) {
            return true;
        }
    }
    
    return false;
}

void DotNetParser::CacheMethodTable(MemoryAddress addr, const MethodTable& mt) {
    method_table_cache_[addr] = mt;
}

std::optional<MethodTable> DotNetParser::GetCachedMethodTable(MemoryAddress addr) {
    auto it = method_table_cache_.find(addr);
    if (it != method_table_cache_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void DotNetParser::ClearMethodTableCache() {
    method_table_cache_.clear();
    type_name_cache_.clear();
}

MemoryAddress DotNetParser::FindUnityEngine() {
    LOG_INFO("Searching for Unity Engine assembly");
    
    // This is a simplified approach - in reality, you'd need to:
    // 1. Find the .NET runtime structures
    // 2. Locate the Application Domain
    // 3. Enumerate loaded assemblies
    // 4. Find UnityEngine assembly
    
    // For now, return 0 indicating not found
    LOG_WARN("Unity Engine assembly detection not fully implemented");
    return 0;
}

std::vector<MemoryAddress> DotNetParser::FindGameObjects() {
    LOG_INFO("Searching for Unity GameObjects");
    return FindObjectsOfType("GameObject");
}

std::vector<MemoryAddress> DotNetParser::FindMonoBehaviours() {
    LOG_INFO("Searching for Unity MonoBehaviour objects");
    return FindObjectsOfType("MonoBehaviour");
}

bool DotNetParser::ValidateObjectHeader(const ObjectHeader& header) {
    // Basic validation of object header
    // In .NET, sync block index can be 0 or point to a valid sync block
    
    // If sync block index is non-zero, it should be a reasonable value
    if (header.sync_block_index != 0) {
        // Check if it's within reasonable bounds (not too large)
        if (header.sync_block_index > 0x1000000) { // 16M seems reasonable upper bound
            return false;
        }
    }
    
    return true;
}

bool DotNetParser::ValidateMethodTable(const MethodTable& mt) {
    // Validate method table structure
    
    // Check base size is reasonable
    if (mt.base_size < 4 || mt.base_size > METHOD_TABLE_MAX_SIZE) {
        return false;
    }
    
    // Check number of vtable slots is reasonable
    if (mt.num_vtable_slots > 10000) { // Arbitrary but reasonable limit
        return false;
    }
    
    // Check number of interfaces is reasonable
    if (mt.num_interfaces > 1000) { // Arbitrary but reasonable limit
        return false;
    }
    
    // Validate pointer fields
    if (mt.parent_method_table != 0 && !IsValidPointer(mt.parent_method_table)) {
        return false;
    }
    
    if (mt.module_ptr != 0 && !IsValidPointer(mt.module_ptr)) {
        return false;
    }
    
    if (mt.ee_class_ptr != 0 && !IsValidPointer(mt.ee_class_ptr)) {
        return false;
    }
    
    return true;
}

std::string DotNetParser::ReadManagedString(MemoryAddress string_addr) {
    if (!IsValidPointer(string_addr)) {
        return "INVALID_STRING_ADDRESS";
    }
    
    // .NET string structure:
    // - Object header
    // - Method table pointer
    // - String length (4 bytes)
    // - String data (UTF-16)
    
    MemoryAddress data_offset = string_addr + sizeof(ObjectHeader) + sizeof(MemoryAddress);
    
    // Read string length
    uint32_t length;
    if (!process_mgr_->ReadMemory(data_offset, &length, sizeof(uint32_t))) {
        return "LENGTH_READ_FAILED";
    }
    
    // Sanity check on length
    if (length > 10000) { // Reasonable limit for type names
        return "STRING_TOO_LONG";
    }
    
    // Read string data (UTF-16)
    std::vector<uint16_t> utf16_data(length);
    MemoryAddress string_data_addr = data_offset + sizeof(uint32_t);
    
    if (!process_mgr_->ReadMemory(string_data_addr, utf16_data.data(), length * sizeof(uint16_t))) {
        return "STRING_DATA_READ_FAILED";
    }
    
    // Convert UTF-16 to UTF-8 (simplified conversion)
    std::string result;
    for (uint16_t ch : utf16_data) {
        if (ch < 128) {
            result += static_cast<char>(ch);
        } else {
            result += '?'; // Simplified handling of non-ASCII
        }
    }
    
    return result;
}

std::string DotNetParser::GetTypeNameFromEEClass(MemoryAddress ee_class_addr) {
    if (!IsValidPointer(ee_class_addr)) {
        return "UNKNOWN";
    }
    
    // EEClass structure is complex and version-dependent
    // This is a simplified approach that would need to be adapted
    // based on the specific .NET runtime version
    
    // Try to read what might be a type name pointer at various offsets
    std::vector<size_t> possible_offsets = {0x10, 0x18, 0x20, 0x28, 0x30};
    
    for (size_t offset : possible_offsets) {
        MemoryAddress name_ptr;
        if (process_mgr_->ReadMemory(ee_class_addr + offset, &name_ptr, sizeof(MemoryAddress))) {
            if (IsValidPointer(name_ptr)) {
                std::string name = ReadManagedString(name_ptr);
                if (!name.empty() && name != "INVALID_STRING_ADDRESS" && 
                    name != "LENGTH_READ_FAILED" && name != "STRING_DATA_READ_FAILED") {
                    return name;
                }
            }
        }
    }
    
    return "UNKNOWN";
}

MemoryAddress DotNetParser::GetTypeHandle(MemoryAddress method_table_addr) {
    // In .NET, the type handle is typically the method table address itself
    return method_table_addr;
}

bool DotNetParser::IsValidPointer(MemoryAddress addr) {
    // Basic pointer validation
    if (addr == 0) {
        return false;
    }
    
    // Check if it's in reasonable user space range
    if (addr < 0x10000 || addr > 0x7FFFFFFFFFFF) {
        return false;
    }
    
    return true;
}

bool DotNetParser::IsInExecutableMemory(MemoryAddress addr) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(process_mgr_->GetProcessHandle(), 
                      reinterpret_cast<LPCVOID>(addr), 
                      &mbi, sizeof(mbi)) != sizeof(mbi)) {
        return false;
    }
    
    return (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}

} // namespace MemoryForensics