#pragma once

#include "common.hpp"
#include "process_manager.hpp"

namespace MemoryForensics {
    
    class MemoryScanner {
    public:
        explicit MemoryScanner(std::shared_ptr<ProcessManager> process_mgr);
        ~MemoryScanner() = default;
        
        // Pattern scanning
        std::vector<MemoryAddress> ScanForPattern(const ByteVector& pattern, 
                                                 const ByteVector& mask = {});
        std::vector<MemoryAddress> ScanForPattern(const std::string& hex_pattern);
        
        // Specific structure scanning
        std::vector<MemoryAddress> FindContainerStructs();
        std::vector<EncryptedBigInteger> FindEncryptedBigIntegers();
        
        // Memory reading utilities
        template<typename T>
        std::optional<T> ReadValue(MemoryAddress address);
        
        ByteVector ReadBytes(MemoryAddress address, size_t size);
        std::string ReadString(MemoryAddress address, size_t max_length = 256);
        
        // Primitive type reading functions
        std::optional<int32_t> ReadInt32(MemoryAddress address);
        std::optional<uint32_t> ReadUInt32(MemoryAddress address);
        std::optional<uint64_t> ReadUInt64(MemoryAddress address);
        std::optional<bool> ReadBool(MemoryAddress address);
        std::optional<std::vector<uint32_t>> ReadUInt32Array(MemoryAddress address, size_t count);
        
        // Pointer following
        std::optional<MemoryAddress> FollowPointer(MemoryAddress ptr_address);
        std::vector<MemoryAddress> FollowPointerChain(MemoryAddress base, 
                                                     const std::vector<size_t>& offsets);
        
        // Signature management
        void AddSignature(const std::string& name, const ByteVector& pattern);
        void LoadSignaturesFromConfig(const nlohmann::json& config);
        
        // Scanning configuration
        void SetScanRegions(const std::vector<MemoryRegion>& regions);
        void SetScanRange(MemoryAddress start, MemoryAddress end);
        void EnableProgressCallback(std::function<void(float)> callback);
        
    private:
        std::shared_ptr<ProcessManager> process_mgr_;
        std::vector<MemoryRegion> scan_regions_;
        std::unordered_map<std::string, ByteVector> signatures_;
        std::function<void(float)> progress_callback_;
        
        // Internal scanning methods
        bool IsValidScanRegion(const MemoryRegion& region);
        std::vector<MemoryAddress> ScanRegionForPattern(const MemoryRegion& region,
                                                       const ByteVector& pattern,
                                                       const ByteVector& mask);
        
        // Container struct detection
        bool IsContainerStruct(MemoryAddress address);
        bool ValidateContainerStruct(MemoryAddress address);
        
        // Performance optimization
        static constexpr size_t SCAN_ALIGNMENT = 4;
        static constexpr size_t MIN_VALID_POINTER = 0x10000;
        static constexpr size_t MAX_VALID_POINTER = 0x7FFFFFFFFFFF;
    };
    
    // Template implementation
    template<typename T>
    std::optional<T> MemoryScanner::ReadValue(MemoryAddress address) {
        T value;
        if (process_mgr_->ReadMemory(address, &value, sizeof(T))) {
            return value;
        }
        return std::nullopt;
    }
    
} // namespace MemoryForensics