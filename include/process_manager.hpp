#pragma once

#include "common.hpp"

namespace MemoryForensics {
    
    class ProcessManager {
    public:
        ProcessManager();
        ~ProcessManager();
        
        // Process discovery and attachment
        bool AttachToProcess(const std::string& process_name);
        bool AttachToProcess(ProcessID pid);
        void DetachFromProcess();
        
        // Process information
        ProcessID GetProcessID() const { return process_id_; }
        HANDLE GetProcessHandle() const { return process_handle_; }
        bool IsAttached() const { return process_handle_ != nullptr; }
        
        // Memory operations
        std::vector<MemoryRegion> EnumerateMemoryRegions();
        bool ReadMemory(MemoryAddress address, void* buffer, size_t size);
        bool WriteMemory(MemoryAddress address, const void* buffer, size_t size);
        
        // Process enumeration
        static std::vector<std::pair<ProcessID, std::string>> ListRunningProcesses();
        static std::optional<ProcessID> FindProcessByName(const std::string& name);
        
        // Module information
        std::vector<MODULEENTRY32> GetLoadedModules();
        std::optional<MemoryAddress> GetModuleBaseAddress(const std::string& module_name);
        
    private:
        ProcessID process_id_;
        HANDLE process_handle_;
        std::string process_name_;
        
        bool ValidateProcessAccess();
        void LogProcessInfo();
    };
    
} // namespace MemoryForensics