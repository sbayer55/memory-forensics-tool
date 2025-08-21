#pragma once

#include "common.hpp"
#include "process_manager.hpp"

namespace MemoryForensics {
    
    // .NET object structures (based on CoreCLR implementation)
    struct ObjectHeader {
        uint32_t sync_block_index;
    };
    
    struct MethodTable {
        uint32_t flags;
        uint32_t base_size;
        uint16_t flags2;
        uint16_t token;
        uint16_t num_vtable_slots;
        uint16_t num_interfaces;
        MemoryAddress parent_method_table;
        MemoryAddress module_ptr;
        MemoryAddress ee_class_ptr;
    };
    
    class DotNetParser {
    public:
        explicit DotNetParser(std::shared_ptr<ProcessManager> process_mgr);
        ~DotNetParser() = default;
        
        // Object analysis
        bool IsValidObject(MemoryAddress object_addr);
        std::optional<MethodTable> GetMethodTable(MemoryAddress object_addr);
        std::string GetTypeName(MemoryAddress method_table_addr);
        
        // Managed heap traversal
        std::vector<MemoryAddress> FindObjectsOfType(const std::string& type_name);
        std::vector<MemoryAddress> ScanForBigIntegers();
        
        // GC heap analysis
        std::vector<MemoryRegion> GetManagedHeapRegions();
        bool IsInManagedHeap(MemoryAddress address);
        
        // Method table cache
        void CacheMethodTable(MemoryAddress addr, const MethodTable& mt);
        std::optional<MethodTable> GetCachedMethodTable(MemoryAddress addr);
        void ClearMethodTableCache();
        
        // Unity specific
        MemoryAddress FindUnityEngine();
        std::vector<MemoryAddress> FindGameObjects();
        std::vector<MemoryAddress> FindMonoBehaviours();
        
    private:
        std::shared_ptr<ProcessManager> process_mgr_;
        std::unordered_map<MemoryAddress, MethodTable> method_table_cache_;
        std::unordered_map<std::string, MemoryAddress> type_name_cache_;
        
        // Internal validation
        bool ValidateObjectHeader(const ObjectHeader& header);
        bool ValidateMethodTable(const MethodTable& mt);
        
        // String reading from managed heap
        std::string ReadManagedString(MemoryAddress string_addr);
        
        // Type system navigation
        std::string GetTypeNameFromEEClass(MemoryAddress ee_class_addr);
        MemoryAddress GetTypeHandle(MemoryAddress method_table_addr);
        
        // Memory validation helpers
        bool IsValidPointer(MemoryAddress addr);
        bool IsInExecutableMemory(MemoryAddress addr);
        
        // Constants for .NET object validation
        static constexpr uint32_t METHOD_TABLE_MIN_SIZE = 0x28;
        static constexpr uint32_t METHOD_TABLE_MAX_SIZE = 0x1000;
        static constexpr uint32_t BIGINTEGER_TYPE_TOKEN = 0x02000001; // Example token
    };
    
} // namespace MemoryForensics