#pragma once

// Platform compatibility layer for cross-platform compilation
// This allows the Windows-focused memory forensics tool to compile on macOS for development

#ifdef __APPLE__
    #define MACOS_BUILD
#elif defined(_WIN32) || defined(_WIN64)
    #define WINDOWS_BUILD
#else
    #define LINUX_BUILD
#endif

#ifdef MACOS_BUILD
    // macOS compatibility layer for Windows APIs
    #include <cstdint>
    #include <string>
    #include <vector>
    #include <cstdarg>
    
    // Windows types
    using DWORD = uint32_t;
    using HANDLE = void*;
    using BOOL = int;
    using BYTE = uint8_t;
    using LPSTR = char*;
    using LPCSTR = const char*;
    using LPVOID = void*;
    using LPCVOID = const void*;
    using SIZE_T = size_t;
    
    // Windows constants
    #define TRUE 1
    #define FALSE 0
    #define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
    #define MAX_PATH 260
    
    // Memory protection constants (dummy values for compilation)
    #define PAGE_READWRITE 0x04
    #define PAGE_READONLY 0x02
    #define PAGE_EXECUTE 0x10
    #define PAGE_EXECUTE_READ 0x20
    #define PAGE_EXECUTE_READWRITE 0x40
    #define PAGE_EXECUTE_WRITECOPY 0x80
    #define PAGE_WRITECOPY 0x08
    
    // Memory allocation constants
    #define MEM_COMMIT 0x1000
    #define MEM_IMAGE 0x1000000
    #define MEM_MAPPED 0x40000
    #define MEM_PRIVATE 0x20000
    
    // Process access rights (dummy values)
    #define PROCESS_ALL_ACCESS 0x1F0FFF
    #define PROCESS_VM_READ 0x0010
    #define PROCESS_QUERY_INFORMATION 0x0400
    
    // ToolHelp32 constants
    #define TH32CS_SNAPPROCESS 0x00000002
    #define TH32CS_SNAPMODULE 0x00000008
    #define TH32CS_SNAPMODULE32 0x00000010
    
    // Windows structures (simplified for compilation)
    struct MEMORY_BASIC_INFORMATION {
        LPVOID BaseAddress;
        LPVOID AllocationBase;
        DWORD AllocationProtect;
        SIZE_T RegionSize;
        DWORD State;
        DWORD Protect;
        DWORD Type;
    };
    
    struct PROCESSENTRY32 {
        DWORD dwSize;
        DWORD cntUsage;
        DWORD th32ProcessID;
        uintptr_t th32DefaultHeapID;
        DWORD th32ModuleID;
        DWORD cntThreads;
        DWORD th32ParentProcessID;
        long pcPriClassBase;
        DWORD dwFlags;
        char szExeFile[MAX_PATH];
    };
    
    struct MODULEENTRY32 {
        DWORD dwSize;
        DWORD th32ModuleID;
        DWORD th32ProcessID;
        DWORD GlblcntUsage;
        DWORD ProccntUsage;
        BYTE* modBaseAddr;
        DWORD modBaseSize;
        HANDLE hModule;
        char szModule[256];
        char szExePath[MAX_PATH];
    };
    
    struct PROCESS_MEMORY_COUNTERS {
        DWORD cb;
        DWORD PageFaultCount;
        SIZE_T PeakWorkingSetSize;
        SIZE_T WorkingSetSize;
        SIZE_T QuotaPeakPagedPoolUsage;
        SIZE_T QuotaPagedPoolUsage;
        SIZE_T QuotaPeakNonPagedPoolUsage;
        SIZE_T QuotaNonPagedPoolUsage;
        SIZE_T PagefileUsage;
        SIZE_T PeakPagefileUsage;
    };
    
    // Language constants
    #define LANG_NEUTRAL 0x00
    #define SUBLANG_DEFAULT 0x01
    #define MAKELANGID(p, s) ((((DWORD)(s)) << 10) | (DWORD)(p))
    
    // Format message constants
    #define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
    #define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
    #define FORMAT_MESSAGE_IGNORE_INSERTS 0x00000200
    
    // Stub functions for Windows API (non-functional on macOS)
    inline HANDLE OpenProcess(DWORD, BOOL, DWORD) { return INVALID_HANDLE_VALUE; }
    inline BOOL CloseHandle(HANDLE) { return FALSE; }
    inline BOOL ReadProcessMemory(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*) { return FALSE; }
    inline BOOL WriteProcessMemory(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*) { return FALSE; }
    inline SIZE_T VirtualQueryEx(HANDLE, LPCVOID, MEMORY_BASIC_INFORMATION*, SIZE_T) { return 0; }
    inline HANDLE CreateToolhelp32Snapshot(DWORD, DWORD) { return INVALID_HANDLE_VALUE; }
    inline BOOL Process32First(HANDLE, PROCESSENTRY32*) { return FALSE; }
    inline BOOL Process32Next(HANDLE, PROCESSENTRY32*) { return FALSE; }
    inline BOOL Module32First(HANDLE, MODULEENTRY32*) { return FALSE; }
    inline BOOL Module32Next(HANDLE, MODULEENTRY32*) { return FALSE; }
    inline BOOL QueryFullProcessImageNameA(HANDLE, DWORD, LPSTR, DWORD*) { return FALSE; }
    inline BOOL GetProcessMemoryInfo(HANDLE, PROCESS_MEMORY_COUNTERS*, DWORD) { return FALSE; }
    inline DWORD GetLastError() { return 0; }
    inline void SetLastError(DWORD) {}
    inline DWORD FormatMessageA(DWORD, LPCVOID, DWORD, DWORD, LPSTR, DWORD, va_list*) { return 0; }
    inline HANDLE LocalFree(HANDLE) { return nullptr; }
    
#else
    // Windows build - include actual Windows headers
    #include <Windows.h>
    #include <TlHelp32.h>
    #include <Psapi.h>
#endif