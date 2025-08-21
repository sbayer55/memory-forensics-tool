#pragma once

#include "common.hpp"

namespace MemoryForensics {
    
    class DecryptionEngine {
    public:
        DecryptionEngine();
        ~DecryptionEngine() = default;
        
        // Main decryption interface
        bool DecryptBigInteger(EncryptedBigInteger& encrypted_obj);
        ByteVector DecryptData(const ByteVector& encrypted_data, const ByteVector& key);
        
        // Key management
        bool ValidateDecryptionKey(const ByteVector& key);
        std::optional<ByteVector> ExtractKeyFromMemory(MemoryAddress key_address);
        
        // Decryption algorithms (based on reverse-engineered source)
        ByteVector XORDecrypt(const ByteVector& data, const ByteVector& key);
        ByteVector AESDecrypt(const ByteVector& data, const ByteVector& key);
        ByteVector CustomDecrypt(const ByteVector& data, const ByteVector& key);
        
        // BigInteger specific operations
        std::string BigIntegerToString(const ByteVector& decrypted_data);
        bool IsBigIntegerValid(const ByteVector& data);
        
        // Batch operations
        std::vector<EncryptedBigInteger> DecryptMultiple(
            std::vector<EncryptedBigInteger>& encrypted_objects);
        
        // Configuration
        void SetDecryptionMethod(const std::string& method_name);
        void LoadDecryptionConfig(const nlohmann::json& config);
        
        // Statistics and logging
        size_t GetSuccessfulDecryptions() const { return successful_decryptions_; }
        size_t GetFailedDecryptions() const { return failed_decryptions_; }
        void ResetStatistics();
        
    private:
        std::string current_method_;
        size_t successful_decryptions_;
        size_t failed_decryptions_;
        
        // Internal decryption helpers
        bool ValidateDecryptedData(const ByteVector& data);
        ByteVector ApplyDecryptionMethod(const ByteVector& data, const ByteVector& key);
        
        // Key validation
        bool IsKeyLengthValid(const ByteVector& key);
        bool IsKeyPatternValid(const ByteVector& key);
        
        // BigInteger parsing (from .NET format)
        struct BigIntegerHeader {
            uint32_t length;
            bool is_negative;
            uint32_t data_offset;
        };
        
        std::optional<BigIntegerHeader> ParseBigIntegerHeader(const ByteVector& data);
    };
    
} // namespace MemoryForensics