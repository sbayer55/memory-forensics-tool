-- Basic memory scanning script for Revolution Idol
-- This script demonstrates finding and decrypting BigInteger objects

log("Starting basic memory scan for encrypted BigInteger objects", "info")

-- Find all encrypted BigInteger containers
local containers = find_encrypted_bigintegers()

if #containers == 0 then
    log("No encrypted BigInteger objects found", "warn")
    return
end

log("Found " .. #containers .. " encrypted BigInteger objects", "info")

-- Process each container
local successful_decryptions = 0
local failed_decryptions = 0

for i, container in ipairs(containers) do
    log("Processing container " .. i .. " at address " .. address_to_hex(container.address), "debug")
    
    -- Attempt decryption
    local success = decrypt_biginteger(container.address)
    
    if success then
        successful_decryptions = successful_decryptions + 1
        log("Successfully decrypted container " .. i, "info")
        
        -- Read the decrypted value
        local decrypted_data = read_memory(container.bigint_ptr, 256)
        log("Decrypted BigInteger data: " .. bytes_to_hex(decrypted_data), "info")
    else
        failed_decryptions = failed_decryptions + 1
        log("Failed to decrypt container " .. i, "warn")
    end
end

-- Summary
log("Decryption complete:", "info")
log("  Successful: " .. successful_decryptions, "info")
log("  Failed: " .. failed_decryptions, "info")
log("  Total: " .. #containers, "info")

-- Optional: Save results to global variables for further analysis
_G.last_scan_results = {
    total_found = #containers,
    successful = successful_decryptions,
    failed = failed_decryptions,
    containers = containers
}