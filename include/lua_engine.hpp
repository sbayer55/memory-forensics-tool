#pragma once

#include "common.hpp"
#include "memory_scanner.hpp"
#include "decryption_engine.hpp"

#include <sol/sol.hpp>

namespace MemoryForensics {
    
    class LuaEngine {
    public:
        explicit LuaEngine(std::shared_ptr<MemoryScanner> scanner,
                          std::shared_ptr<class DecryptionEngine> decryptor);
        ~LuaEngine() = default;
        
        // Script execution
        bool ExecuteScript(const std::string& script_path);
        bool ExecuteCode(const std::string& lua_code);
        
        // Interactive mode
        void StartInteractiveMode();
        bool ExecuteInteractiveCommand(const std::string& command);
        
        // Script management
        void LoadScriptDirectory(const std::string& directory);
        std::vector<std::string> GetAvailableScripts() const;
        
        // Variable management
        void SetGlobalVariable(const std::string& name, const sol::object& value);
        sol::object GetGlobalVariable(const std::string& name);
        
        // Error handling
        std::string GetLastError() const { return last_error_; }
        
    private:
        sol::state lua_;
        std::shared_ptr<MemoryScanner> scanner_;
        std::shared_ptr<DecryptionEngine> decryptor_;
        std::string last_error_;
        std::vector<std::string> available_scripts_;
        
        // Lua API setup
        void InitializeLuaState();
        void RegisterMemoryAPI();
        void RegisterDecryptionAPI();
        void RegisterUtilityAPI();
        
        // Memory API functions exposed to Lua
        void LuaReadMemory(MemoryAddress address, size_t size);
        std::vector<MemoryAddress> LuaScanPattern(const std::string& hex_pattern);
        sol::table LuaFindEncryptedBigIntegers();
        bool LuaDecryptBigInteger(MemoryAddress container_addr);
        
        // Utility API functions
        std::string LuaAddressToHex(MemoryAddress address);
        MemoryAddress LuaHexToAddress(const std::string& hex);
        void LuaLog(const std::string& message, const std::string& level = "info");
        
        // Script validation
        bool ValidateScript(const std::string& script_content);
        void LogScriptError(const sol::error& error);
    };
    
} // namespace MemoryForensics