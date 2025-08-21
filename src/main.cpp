#include "common.hpp"
#include "process_manager.hpp"
#include "memory_scanner.hpp"
#include "lua_engine.hpp"
#include "decryption_engine.hpp"
#include "dotnet_parser.hpp"

#include <CLI/CLI.hpp>
#include <fstream>

using namespace MemoryForensics;

int main(int argc, char** argv) {
    // Initialize logging
    auto logger = spdlog::stdout_color_mt("main");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    
    // Command line arguments
    CLI::App app{"Memory Forensics Tool for Revolution Idol"};
    
    std::string process_name = TARGET_PROCESS_NAME;
    ProcessID target_pid = 0;
    std::string script_file;
    std::string output_file;
    bool interactive_mode = false;
    bool decrypt_mode = false;
    bool verbose = false;
    
    app.add_option("-p,--process", process_name, "Target process name")
       ->default_val(TARGET_PROCESS_NAME);
    app.add_option("--pid", target_pid, "Target process ID (overrides process name)");
    app.add_option("-s,--script", script_file, "Lua script to execute");
    app.add_option("-o,--output", output_file, "Output file for results");
    app.add_flag("-i,--interactive", interactive_mode, "Start interactive Lua shell");
    app.add_flag("-d,--decrypt", decrypt_mode, "Enable decryption of found objects");
    app.add_flag("-v,--verbose", verbose, "Enable verbose logging");
    
    CLI11_PARSE(app, argc, argv);
    
    // Set logging level
    if (verbose) {
        spdlog::set_level(spdlog::level::debug);
    }
    
    try {
        // Initialize core components
        auto process_mgr = std::make_shared<ProcessManager>();
        auto decryption_engine = std::make_shared<DecryptionEngine>();
        auto memory_scanner = std::make_shared<MemoryScanner>(process_mgr);
        auto lua_engine = std::make_shared<LuaEngine>(memory_scanner, decryption_engine);
        
        // Attach to target process
        bool attached = false;
        if (target_pid != 0) {
            spdlog::info("Attempting to attach to process ID: {}", target_pid);
            attached = process_mgr->AttachToProcess(target_pid);
        } else {
            spdlog::info("Attempting to attach to process: {}", process_name);
            attached = process_mgr->AttachToProcess(process_name);
        }
        
        if (!attached) {
            spdlog::error("Failed to attach to target process");
            return 1;
        }
        
        spdlog::info("Successfully attached to process ID: {}", process_mgr->GetProcessID());
        
        // Load configuration
        std::ifstream config_file("config/default_config.json");
        if (config_file.is_open()) {
            nlohmann::json config;
            config_file >> config;
            decryption_engine->LoadDecryptionConfig(config);
            memory_scanner->LoadSignaturesFromConfig(config);
            spdlog::debug("Loaded configuration from file");
        }
        
        // Execute based on mode
        if (interactive_mode) {
            spdlog::info("Starting interactive Lua shell...");
            lua_engine->StartInteractiveMode();
        } else if (!script_file.empty()) {
            spdlog::info("Executing script: {}", script_file);
            if (!lua_engine->ExecuteScript(script_file)) {
                spdlog::error("Script execution failed: {}", lua_engine->GetLastError());
                return 1;
            }
        } else {
            // Default behavior: scan for encrypted BigIntegers
            spdlog::info("Scanning for encrypted BigInteger objects...");
            auto encrypted_objects = memory_scanner->FindEncryptedBigIntegers();
            
            if (encrypted_objects.empty()) {
                spdlog::warn("No encrypted BigInteger objects found");
                return 0;
            }
            
            spdlog::info("Found {} encrypted BigInteger objects", encrypted_objects.size());
            
            if (decrypt_mode) {
                spdlog::info("Decrypting found objects...");
                auto decrypted = decryption_engine->DecryptMultiple(encrypted_objects);
                
                spdlog::info("Successfully decrypted {}/{} objects", 
                           decryption_engine->GetSuccessfulDecryptions(),
                           encrypted_objects.size());
                
                // Output results
                if (!output_file.empty()) {
                    nlohmann::json results;
                    for (const auto& obj : decrypted) {
                        if (obj.is_decrypted) {
                            results["decrypted_objects"].push_back({
                                {"address", obj.container_address},
                                {"bigint_ptr", obj.bigint_ptr},
                                {"key_ptr", obj.key_ptr},
                                {"decrypted_data", BytesToHexString(obj.encrypted_data)}
                            });
                        }
                    }
                    
                    std::ofstream out(output_file);
                    out << results.dump(2);
                    spdlog::info("Results written to: {}", output_file);
                }
            } else {
                // Just report found objects
                for (size_t i = 0; i < encrypted_objects.size(); ++i) {
                    const auto& obj = encrypted_objects[i];
                    spdlog::info("Object {}: Container=0x{:X}, BigInt=0x{:X}, Key=0x{:X}",
                               i, obj.container_address, obj.bigint_ptr, obj.key_ptr);
                }
            }
        }
        
    } catch (const ProcessException& e) {
        spdlog::error("Process error: {}", e.what());
        return 1;
    } catch (const MemoryException& e) {
        spdlog::error("Memory error: {}", e.what());
        return 1;
    } catch (const std::exception& e) {
        spdlog::error("Unexpected error: {}", e.what());
        return 1;
    }
    
    spdlog::info("Tool execution completed successfully");
    return 0;
}