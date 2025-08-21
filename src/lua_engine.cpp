#include "lua_engine.hpp"
#include "app_logger.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace MemoryForensics {

LuaEngine::LuaEngine(std::shared_ptr<MemoryScanner> scanner,
                     std::shared_ptr<DecryptionEngine> decryptor)
    : scanner_(scanner), decryptor_(decryptor) {
    InitializeLuaState();
}

bool LuaEngine::ExecuteScript(const std::string& script_path) {
    LOG_INFO("Executing Lua script: {}", script_path);
    
    if (!std::filesystem::exists(script_path)) {
        last_error_ = "Script file not found: " + script_path;
        LOG_ERROR(last_error_);
        return false;
    }
    
    try {
        // Read script content
        std::ifstream file(script_path);
        if (!file.is_open()) {
            last_error_ = "Failed to open script file: " + script_path;
            LOG_ERROR(last_error_);
            return false;
        }
        
        std::string script_content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
        file.close();
        
        // Validate script before execution
        if (!ValidateScript(script_content)) {
            last_error_ = "Script validation failed for: " + script_path;
            LOG_ERROR(last_error_);
            return false;
        }
        
        // Execute the script
        LOG_DEBUG("Script content length: {} bytes", script_content.length());
        auto result = lua_.safe_script(script_content, sol::script_pass_on_error);
        
        if (!result.valid()) {
            sol::error err = result;
            LogScriptError(err);
            return false;
        }
        
        LOG_INFO("Successfully executed script: {}", script_path);
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = "Exception during script execution: " + std::string(e.what());
        LOG_ERROR(last_error_);
        return false;
    }
}

bool LuaEngine::ExecuteCode(const std::string& lua_code) {
    LOG_DEBUG("Executing Lua code snippet (length: {} bytes)", lua_code.length());
    
    try {
        auto result = lua_.safe_script(lua_code, sol::script_pass_on_error);
        
        if (!result.valid()) {
            sol::error err = result;
            LogScriptError(err);
            return false;
        }
        
        LOG_DEBUG("Successfully executed Lua code snippet");
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = "Exception during code execution: " + std::string(e.what());
        LOG_ERROR(last_error_);
        return false;
    }
}

void LuaEngine::StartInteractiveMode() {
    LOG_INFO("Starting Lua interactive mode");
    LOG_INFO("Type 'exit' or 'quit' to return to main application");
    LOG_INFO("Available functions: read_memory, scan_pattern, find_encrypted_bigintegers, decrypt_biginteger");
    LOG_INFO("Utility functions: address_to_hex, hex_to_address, log");
    
    std::string input;
    std::cout << "\nLua> ";
    
    while (std::getline(std::cin, input)) {
        if (input == "exit" || input == "quit") {
            LOG_INFO("Exiting Lua interactive mode");
            break;
        }
        
        if (input.empty()) {
            std::cout << "Lua> ";
            continue;
        }
        
        if (!ExecuteInteractiveCommand(input)) {
            std::cout << "Error: " << GetLastError() << std::endl;
        }
        
        std::cout << "Lua> ";
    }
}

bool LuaEngine::ExecuteInteractiveCommand(const std::string& command) {
    try {
        auto result = lua_.safe_script(command, sol::script_pass_on_error);
        
        if (!result.valid()) {
            sol::error err = result;
            LogScriptError(err);
            return false;
        }
        
        // Print result if it's not nil
        if (result.get_type() != sol::type::lua_nil) {
            std::cout << "Result: ";
            
            switch (result.get_type()) {
                case sol::type::boolean:
                    std::cout << (result.get<bool>() ? "true" : "false");
                    break;
                case sol::type::number:
                    std::cout << result.get<double>();
                    break;
                case sol::type::string:
                    std::cout << "\"" << result.get<std::string>() << "\"";
                    break;
                case sol::type::table:
                    std::cout << "[table]";
                    break;
                default:
                    std::cout << "[" << sol::type_name(lua_.lua_state(), result.get_type()) << "]";
                    break;
            }
            std::cout << std::endl;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = "Exception during interactive command: " + std::string(e.what());
        return false;
    }
}

void LuaEngine::LoadScriptDirectory(const std::string& directory) {
    LOG_INFO("Loading scripts from directory: {}", directory);
    
    available_scripts_.clear();
    
    if (!std::filesystem::exists(directory)) {
        LOG_WARN("Script directory does not exist: {}", directory);
        return;
    }
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                std::string script_path = entry.path().string();
                available_scripts_.push_back(script_path);
                LOG_DEBUG("Found script: {}", script_path);
            }
        }
        
        LOG_INFO("Loaded {} Lua scripts", available_scripts_.size());
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading script directory: {}", e.what());
    }
}

std::vector<std::string> LuaEngine::GetAvailableScripts() const {
    return available_scripts_;
}

void LuaEngine::SetGlobalVariable(const std::string& name, const sol::object& value) {
    lua_[name] = value;
    LOG_DEBUG("Set global Lua variable: {}", name);
}

sol::object LuaEngine::GetGlobalVariable(const std::string& name) {
    return lua_[name];
}

void LuaEngine::InitializeLuaState() {
    LOG_DEBUG("Initializing Lua state");
    
    // Open standard Lua libraries
    lua_.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, 
                       sol::lib::table, sol::lib::io, sol::lib::os);
    
    // Register our custom APIs
    RegisterMemoryAPI();
    RegisterDecryptionAPI();
    RegisterUtilityAPI();
    
    LOG_DEBUG("Lua state initialized successfully");
}

void LuaEngine::RegisterMemoryAPI() {
    LOG_DEBUG("Registering Memory API for Lua");
    
    // Memory reading functions
    lua_.set_function("read_memory", [this](MemoryAddress address, size_t size) {
        LuaReadMemory(address, size);
    });
    
    // Pattern scanning
    lua_.set_function("scan_pattern", [this](const std::string& hex_pattern) {
        return LuaScanPattern(hex_pattern);
    });
    
    // BigInteger finding
    lua_.set_function("find_encrypted_bigintegers", [this]() {
        return LuaFindEncryptedBigIntegers();
    });
    
    // Container struct finding
    lua_.set_function("find_container_structs", [this]() {
        if (!scanner_) {
            LOG_ERROR("MemoryScanner not available");
            return sol::make_object(lua_, sol::nil);
        }
        
        auto containers = scanner_->FindContainerStructs();
        sol::table result = lua_.create_table();
        
        for (size_t i = 0; i < containers.size(); ++i) {
            sol::table container = lua_.create_table();
            container["address"] = containers[i];
            result[i + 1] = container;
        }
        
        return sol::make_object(lua_, result);
    });
}

void LuaEngine::RegisterDecryptionAPI() {
    LOG_DEBUG("Registering Decryption API for Lua");
    
    // BigInteger decryption
    lua_.set_function("decrypt_biginteger", [this](MemoryAddress container_addr) {
        return LuaDecryptBigInteger(container_addr);
    });
    
    // Generic data decryption
    lua_.set_function("decrypt_data", [this](sol::table data_table, sol::table key_table) {
        if (!decryptor_) {
            LOG_ERROR("DecryptionEngine not available");
            return sol::make_object(lua_, sol::nil);
        }
        
        // Convert Lua tables to ByteVectors
        ByteVector data, key;
        
        for (const auto& pair : data_table) {
            data.push_back(pair.second.as<uint8_t>());
        }
        
        for (const auto& pair : key_table) {
            key.push_back(pair.second.as<uint8_t>());
        }
        
        auto decrypted = decryptor_->DecryptData(data, key);
        
        // Convert result back to Lua table
        sol::table result = lua_.create_table();
        for (size_t i = 0; i < decrypted.size(); ++i) {
            result[i + 1] = decrypted[i];
        }
        
        return sol::make_object(lua_, result);
    });
}

void LuaEngine::RegisterUtilityAPI() {
    LOG_DEBUG("Registering Utility API for Lua");
    
    // Address conversion functions
    lua_.set_function("address_to_hex", [this](MemoryAddress address) {
        return LuaAddressToHex(address);
    });
    
    lua_.set_function("hex_to_address", [this](const std::string& hex) {
        return LuaHexToAddress(hex);
    });
    
    // Logging function
    lua_.set_function("log", [this](const std::string& message, const std::string& level) {
        LuaLog(message, level);
    });
    
    // Byte array utilities
    lua_.set_function("bytes_to_hex", [this](sol::table bytes_table) {
        ByteVector bytes;
        for (const auto& pair : bytes_table) {
            bytes.push_back(pair.second.as<uint8_t>());
        }
        return BytesToHexString(bytes);
    });
    
    lua_.set_function("hex_to_bytes", [this](const std::string& hex) {
        auto bytes = HexStringToBytes(hex);
        sol::table result = lua_.create_table();
        for (size_t i = 0; i < bytes.size(); ++i) {
            result[i + 1] = bytes[i];
        }
        return result;
    });
}

void LuaEngine::LuaReadMemory(MemoryAddress address, size_t size) {
    if (!scanner_) {
        LOG_ERROR("MemoryScanner not available");
        return;
    }
    
    auto data = scanner_->ReadBytes(address, size);
    
    if (data.empty()) {
        LOG_WARN("Failed to read {} bytes from 0x{:X}", size, address);
        return;
    }
    
    // Convert to Lua table
    sol::table result = lua_.create_table();
    for (size_t i = 0; i < data.size(); ++i) {
        result[i + 1] = data[i];
    }
    
    // Store result in global variable for easy access
    lua_["last_read_data"] = result;
    
    LOG_DEBUG("Read {} bytes from 0x{:X}", data.size(), address);
}

std::vector<MemoryAddress> LuaEngine::LuaScanPattern(const std::string& hex_pattern) {
    if (!scanner_) {
        LOG_ERROR("MemoryScanner not available");
        return {};
    }
    
    auto results = scanner_->ScanForPattern(hex_pattern);
    LOG_INFO("Pattern scan found {} matches for: {}", results.size(), hex_pattern);
    
    return results;
}

sol::table LuaEngine::LuaFindEncryptedBigIntegers() {
    sol::table result = lua_.create_table();
    
    if (!scanner_) {
        LOG_ERROR("MemoryScanner not available");
        return result;
    }
    
    auto encrypted_objects = scanner_->FindEncryptedBigIntegers();
    
    for (size_t i = 0; i < encrypted_objects.size(); ++i) {
        sol::table obj = lua_.create_table();
        obj["address"] = encrypted_objects[i].container_address;
        obj["bigint_ptr"] = encrypted_objects[i].bigint_ptr;
        obj["key_ptr"] = encrypted_objects[i].key_ptr;
        obj["is_decrypted"] = encrypted_objects[i].is_decrypted;
        
        result[i + 1] = obj;
    }
    
    LOG_INFO("Found {} encrypted BigInteger objects", encrypted_objects.size());
    return result;
}

bool LuaEngine::LuaDecryptBigInteger(MemoryAddress container_addr) {
    if (!decryptor_ || !scanner_) {
        LOG_ERROR("DecryptionEngine or MemoryScanner not available");
        return false;
    }
    
    // This is a simplified approach - in a real implementation,
    // we'd need to create an EncryptedBigInteger object from the address
    LOG_INFO("Attempting to decrypt BigInteger at 0x{:X}", container_addr);
    
    // For now, just log the attempt
    LOG_WARN("BigInteger decryption not fully implemented in Lua API");
    return false;
}

std::string LuaEngine::LuaAddressToHex(MemoryAddress address) {
    return fmt::format("0x{:X}", address);
}

MemoryAddress LuaEngine::LuaHexToAddress(const std::string& hex) {
    try {
        // Remove "0x" prefix if present
        std::string clean_hex = hex;
        if (clean_hex.substr(0, 2) == "0x" || clean_hex.substr(0, 2) == "0X") {
            clean_hex = clean_hex.substr(2);
        }
        
        return std::stoull(clean_hex, nullptr, 16);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to convert hex string '{}' to address: {}", hex, e.what());
        return 0;
    }
}

void LuaEngine::LuaLog(const std::string& message, const std::string& level) {
    if (level == "debug") {
        LOG_DEBUG("[Lua] {}", message);
    } else if (level == "info") {
        LOG_INFO("[Lua] {}", message);
    } else if (level == "warn") {
        LOG_WARN("[Lua] {}", message);
    } else if (level == "error") {
        LOG_ERROR("[Lua] {}", message);
    } else {
        LOG_INFO("[Lua] {}", message);
    }
}

bool LuaEngine::ValidateScript(const std::string& script_content) {
    // Basic validation - check for potentially dangerous functions
    std::vector<std::string> dangerous_patterns = {
        "io.popen",
        "os.execute",
        "loadfile",
        "dofile"
    };
    
    for (const auto& pattern : dangerous_patterns) {
        if (script_content.find(pattern) != std::string::npos) {
            LOG_WARN("Script contains potentially dangerous function: {}", pattern);
            // For now, just warn but don't block - you may want to be more restrictive
        }
    }
    
    // Try to compile the script to check for syntax errors
    try {
        auto result = lua_.safe_script(script_content, sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            last_error_ = "Script syntax error: " + std::string(err.what());
            return false;
        }
    } catch (const std::exception& e) {
        last_error_ = "Script validation exception: " + std::string(e.what());
        return false;
    }
    
    return true;
}

void LuaEngine::LogScriptError(const sol::error& error) {
    last_error_ = "Lua script error: " + std::string(error.what());
    LOG_ERROR(last_error_);
}

} // namespace MemoryForensics