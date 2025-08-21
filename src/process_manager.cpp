#include "process_manager.hpp"
#include "app_logger.hpp"
#include <algorithm>
#include <iostream>

namespace MemoryForensics {

ProcessManager::ProcessManager() 
    : process_id_(0), process_handle_(nullptr) {
}

ProcessManager::~ProcessManager() {
    DetachFromProcess();
}

bool ProcessManager::AttachToProcess(const std::string& process_name) {
    LOG_INFO("Attempting to attach to process: {}", process_name);
    
    auto pid_opt = FindProcessByName(process_name);
    if (!pid_opt) {
        LOG_ERROR("Process '{}' not found", process_name);
        return false;
    }
    
    return AttachToProcess(*pid_opt);
}

bool ProcessManager::AttachToProcess(ProcessID pid) {
    LOG_INFO("Attempting to attach to process ID: {}", pid);
    
    // Detach from any existing process first
    DetachFromProcess();
    
    // Open the target process with full access
    HANDLE handle = OpenProcess(
        PROCESS_ALL_ACCESS,
        FALSE,
        pid
    );
    
    if (handle == nullptr) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to open process {}: {} ({})", pid, GetLastErrorString(), error);
        
        // Try with reduced privileges if full access fails
        handle = OpenProcess(
            PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
            FALSE,
            pid
        );
        
        if (handle == nullptr) {
            LOG_ERROR("Failed to open process {} with reduced privileges: {}", pid, GetLastErrorString());
            return false;
        }
        
        LOG_WARN("Opened process {} with reduced privileges", pid);
    }
    
    process_handle_ = handle;
    process_id_ = pid;
    
    // Get process name for logging
    CHAR process_name[MAX_PATH];
    DWORD name_size = MAX_PATH;
    if (QueryFullProcessImageNameA(process_handle_, 0, process_name, &name_size)) {
        process_name_ = std::string(process_name);
        // Extract just the filename
        size_t last_slash = process_name_.find_last_of("\\/");
        if (last_slash != std::string::npos) {
            process_name_ = process_name_.substr(last_slash + 1);
        }
    } else {
        process_name_ = "Unknown";
    }
    
    if (!ValidateProcessAccess()) {
        LOG_WARN("Process access validation failed for PID {}", pid);
    }
    
    LogProcessInfo();
    LOG_INFO("Successfully attached to process {} (PID: {})", process_name_, process_id_);
    
    return true;
}

void ProcessManager::DetachFromProcess() {
    if (process_handle_ != nullptr) {
        LOG_INFO("Detaching from process {} (PID: {})", process_name_, process_id_);
        CloseHandle(process_handle_);
        process_handle_ = nullptr;
        process_id_ = 0;
        process_name_.clear();
    }
}

std::vector<MemoryRegion> ProcessManager::EnumerateMemoryRegions() {
    std::vector<MemoryRegion> regions;
    
    if (!IsAttached()) {
        LOG_ERROR("Not attached to any process");
        return regions;
    }
    
    LOG_DEBUG("Enumerating memory regions for process {}", process_id_);
    
    MEMORY_BASIC_INFORMATION mbi;
    MemoryAddress current_address = 0;
    
    while (VirtualQueryEx(process_handle_, 
                         reinterpret_cast<LPCVOID>(current_address), 
                         &mbi, 
                         sizeof(mbi)) == sizeof(mbi)) {
        
        if (mbi.State == MEM_COMMIT) {
            MemoryRegion region;
            region.base_address = reinterpret_cast<MemoryAddress>(mbi.BaseAddress);
            region.size = mbi.RegionSize;
            region.protection = mbi.Protect;
            
            // Determine region type/name
            if (mbi.Type == MEM_IMAGE) {
                region.name = "IMAGE";
            } else if (mbi.Type == MEM_MAPPED) {
                region.name = "MAPPED";
            } else if (mbi.Type == MEM_PRIVATE) {
                region.name = "PRIVATE";
            } else {
                region.name = "UNKNOWN";
            }
            
            // Add protection information to name
            std::string protection_str;
            if (mbi.Protect & PAGE_EXECUTE) protection_str += "X";
            if (mbi.Protect & PAGE_READWRITE) protection_str += "RW";
            else if (mbi.Protect & PAGE_READONLY) protection_str += "R";
            if (mbi.Protect & PAGE_WRITECOPY) protection_str += "WC";
            
            if (!protection_str.empty()) {
                region.name += "_" + protection_str;
            }
            
            regions.push_back(region);
        }
        
        current_address = reinterpret_cast<MemoryAddress>(mbi.BaseAddress) + mbi.RegionSize;
    }
    
    LOG_DEBUG("Found {} memory regions", regions.size());
    return regions;
}

bool ProcessManager::ReadMemory(MemoryAddress address, void* buffer, size_t size) {
    if (!IsAttached()) {
        LOG_ERROR("Not attached to any process");
        return false;
    }
    
    if (buffer == nullptr || size == 0) {
        LOG_ERROR("Invalid buffer or size for memory read");
        return false;
    }
    
    if (size > MAX_READ_SIZE) {
        LOG_WARN("Read size {} exceeds maximum allowed size {}", size, MAX_READ_SIZE);
        return false;
    }
    
    SIZE_T bytes_read = 0;
    BOOL result = ReadProcessMemory(
        process_handle_,
        reinterpret_cast<LPCVOID>(address),
        buffer,
        size,
        &bytes_read
    );
    
    if (!result || bytes_read != size) {
        DWORD error = GetLastError();
        LOG_DEBUG("Failed to read {} bytes from 0x{:X}: {} ({})", 
                 size, address, GetLastErrorString(), error);
        return false;
    }
    
    return true;
}

bool ProcessManager::WriteMemory(MemoryAddress address, const void* buffer, size_t size) {
    if (!IsAttached()) {
        LOG_ERROR("Not attached to any process");
        return false;
    }
    
    if (buffer == nullptr || size == 0) {
        LOG_ERROR("Invalid buffer or size for memory write");
        return false;
    }
    
    SIZE_T bytes_written = 0;
    BOOL result = WriteProcessMemory(
        process_handle_,
        reinterpret_cast<LPVOID>(address),
        buffer,
        size,
        &bytes_written
    );
    
    if (!result || bytes_written != size) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to write {} bytes to 0x{:X}: {} ({})", 
                 size, address, GetLastErrorString(), error);
        return false;
    }
    
    LOG_DEBUG("Successfully wrote {} bytes to 0x{:X}", size, address);
    return true;
}

std::vector<std::pair<ProcessID, std::string>> ProcessManager::ListRunningProcesses() {
    std::vector<std::pair<ProcessID, std::string>> processes;
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to create process snapshot: {}", GetLastErrorString());
        return processes;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(snapshot, &pe32)) {
        do {
            processes.emplace_back(pe32.th32ProcessID, std::string(pe32.szExeFile));
        } while (Process32Next(snapshot, &pe32));
    }
    
    CloseHandle(snapshot);
    return processes;
}

std::optional<ProcessID> ProcessManager::FindProcessByName(const std::string& name) {
    auto processes = ListRunningProcesses();
    
    for (const auto& [pid, process_name] : processes) {
        if (process_name == name) {
            LOG_DEBUG("Found process '{}' with PID {}", name, pid);
            return pid;
        }
    }
    
    // Try case-insensitive search
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    for (const auto& [pid, process_name] : processes) {
        std::string lower_process_name = process_name;
        std::transform(lower_process_name.begin(), lower_process_name.end(), 
                      lower_process_name.begin(), ::tolower);
        
        if (lower_process_name == lower_name) {
            LOG_DEBUG("Found process '{}' with PID {} (case-insensitive)", name, pid);
            return pid;
        }
    }
    
    return std::nullopt;
}

std::vector<MODULEENTRY32> ProcessManager::GetLoadedModules() {
    std::vector<MODULEENTRY32> modules;
    
    if (!IsAttached()) {
        LOG_ERROR("Not attached to any process");
        return modules;
    }
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id_);
    if (snapshot == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to create module snapshot for PID {}: {}", process_id_, GetLastErrorString());
        return modules;
    }
    
    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);
    
    if (Module32First(snapshot, &me32)) {
        do {
            modules.push_back(me32);
        } while (Module32Next(snapshot, &me32));
    }
    
    CloseHandle(snapshot);
    LOG_DEBUG("Found {} loaded modules", modules.size());
    return modules;
}

std::optional<MemoryAddress> ProcessManager::GetModuleBaseAddress(const std::string& module_name) {
    auto modules = GetLoadedModules();
    
    for (const auto& module : modules) {
        std::string current_name(module.szModule);
        if (current_name == module_name) {
            MemoryAddress base = reinterpret_cast<MemoryAddress>(module.modBaseAddr);
            LOG_DEBUG("Found module '{}' at base address 0x{:X}", module_name, base);
            return base;
        }
    }
    
    // Try case-insensitive search
    std::string lower_name = module_name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    for (const auto& module : modules) {
        std::string current_name(module.szModule);
        std::transform(current_name.begin(), current_name.end(), current_name.begin(), ::tolower);
        
        if (current_name == lower_name) {
            MemoryAddress base = reinterpret_cast<MemoryAddress>(module.modBaseAddr);
            LOG_DEBUG("Found module '{}' at base address 0x{:X} (case-insensitive)", module_name, base);
            return base;
        }
    }
    
    LOG_WARN("Module '{}' not found in process {}", module_name, process_id_);
    return std::nullopt;
}

bool ProcessManager::ValidateProcessAccess() {
    if (!IsAttached()) {
        return false;
    }
    
    // Try to read a small amount of memory from the process
    // This validates that we have the necessary permissions
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(process_handle_, nullptr, &mbi, sizeof(mbi)) != sizeof(mbi)) {
        LOG_WARN("Cannot query memory information for process {}", process_id_);
        return false;
    }
    
    // Check if we can enumerate modules (requires PROCESS_QUERY_INFORMATION)
    auto modules = GetLoadedModules();
    if (modules.empty()) {
        LOG_WARN("Cannot enumerate modules for process {} - may have limited permissions", process_id_);
        return false;
    }
    
    return true;
}

void ProcessManager::LogProcessInfo() {
    if (!IsAttached()) {
        return;
    }
    
    LOG_DEBUG("Process Information:");
    LOG_DEBUG("  Name: {}", process_name_);
    LOG_DEBUG("  PID: {}", process_id_);
    LOG_DEBUG("  Handle: 0x{:X}", reinterpret_cast<uintptr_t>(process_handle_));
    
    // Log some basic process information
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(process_handle_, &pmc, sizeof(pmc))) {
        LOG_DEBUG("  Working Set Size: {} KB", pmc.WorkingSetSize / 1024);
        LOG_DEBUG("  Peak Working Set: {} KB", pmc.PeakWorkingSetSize / 1024);
        LOG_DEBUG("  Page File Usage: {} KB", pmc.PagefileUsage / 1024);
    }
    
    // Log number of loaded modules
    auto modules = GetLoadedModules();
    LOG_DEBUG("  Loaded Modules: {}", modules.size());
    
    // Log some key modules
    for (const auto& module : modules) {
        std::string module_name(module.szModule);
        if (module_name.find("mono") != std::string::npos ||
            module_name.find("unity") != std::string::npos ||
            module_name.find("UnityPlayer") != std::string::npos ||
            module_name == process_name_) {
            LOG_DEBUG("    {} - Base: 0x{:X}, Size: {} KB", 
                     module_name, 
                     reinterpret_cast<uintptr_t>(module.modBaseAddr),
                     module.modBaseSize / 1024);
        }
    }
}

} // namespace MemoryForensics