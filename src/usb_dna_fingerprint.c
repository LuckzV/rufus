/*
 * Rufus: The Reliable USB Formatting Utility
 * USB Drive DNA Fingerprinting System Implementation
 * Copyright © 2011-2025 Pete Batard <pete@akeo.ie>
 */

#include "usb_dna_fingerprint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

static usb_dna_context_t g_dna_context = { 0 };
static BOOL g_dna_initialized = FALSE;

// Initialize USB DNA fingerprinting
BOOL InitUSBDNAFingerprinting(const char* drive_path)
{
    if (g_dna_initialized) {
        return TRUE;
    }

    if (!drive_path) {
        return FALSE;
    }

    memset(&g_dna_context, 0, sizeof(g_dna_context));
    strncpy(g_dna_context.drive_path, drive_path, sizeof(g_dna_context.drive_path) - 1);

    // Extract all characteristics
    if (!GetUSBDNACharacteristics(drive_path, &g_dna_context.characteristics)) {
        uprintf("Failed to extract USB drive characteristics");
        return FALSE;
    }

    // Generate fingerprint
    if (!GenerateUSBDNAFingerprint(drive_path, &g_dna_context.fingerprint)) {
        uprintf("Failed to generate USB drive DNA fingerprint");
        return FALSE;
    }

    g_dna_context.is_initialized = TRUE;
    g_dna_context.last_scan_time = GetTickCount64();

    uprintf("USB DNA fingerprinting initialized for drive %s", drive_path);
    return TRUE;
}

// Cleanup USB DNA fingerprinting
void CleanupUSBDNAFingerprinting(void)
{
    if (g_dna_initialized) {
        memset(&g_dna_context, 0, sizeof(g_dna_context));
        g_dna_initialized = FALSE;
    }
}

// Generate USB drive DNA fingerprint
BOOL GenerateUSBDNAFingerprint(const char* drive_path, usb_dna_fingerprint_t* fingerprint)
{
    usb_dna_characteristics_t characteristics;

    if (!drive_path || !fingerprint) {
        return FALSE;
    }

    // Extract characteristics
    if (!GetUSBDNACharacteristics(drive_path, &characteristics)) {
        return FALSE;
    }

    // Generate fingerprint from characteristics
    if (!GenerateDNAFromCharacteristics(&characteristics, fingerprint)) {
        return FALSE;
    }

    // Set metadata
    fingerprint->timestamp = GetUSBDNATimestamp();
    fingerprint->version = 1;
    fingerprint->is_unique = IsUSBDNAUnique(fingerprint);
    fingerprint->is_verified = VerifyUSBDNAFingerprint(fingerprint);

    uprintf("Generated USB DNA fingerprint for drive %s", drive_path);
    return TRUE;
}

// Compare USB DNA fingerprints
BOOL CompareUSBDNAFingerprints(const usb_dna_fingerprint_t* fingerprint1, 
                              const usb_dna_fingerprint_t* fingerprint2,
                              usb_dna_comparison_result_t* result)
{
    uint32_t matching_bytes = 0;
    uint32_t total_bytes = USB_DNA_FINGERPRINT_SIZE;
    uint64_t start_time, end_time;

    if (!fingerprint1 || !fingerprint2 || !result) {
        return FALSE;
    }

    start_time = GetTickCount64();

    // Compare fingerprint bytes
    for (uint32_t i = 0; i < USB_DNA_FINGERPRINT_SIZE; i++) {
        if (fingerprint1->fingerprint[i] == fingerprint2->fingerprint[i]) {
            matching_bytes++;
        }
    }

    // Calculate similarity score
    result->similarity_score = (float)matching_bytes / (float)total_bytes;
    result->is_match = (result->similarity_score >= 0.95f);  // 95% similarity threshold
    result->matching_characteristics = matching_bytes;
    result->total_characteristics = total_bytes;

    // Generate comparison details
    static_sprintf(result->comparison_details,
        "Fingerprint comparison: %d/%d bytes match (%.2f%% similarity)",
        matching_bytes, total_bytes, result->similarity_score * 100.0f);

    end_time = GetTickCount64();
    result->comparison_time = end_time - start_time;

    uprintf("USB DNA fingerprint comparison completed in %llu ms", result->comparison_time);
    return TRUE;
}

// Verify USB DNA fingerprint
BOOL VerifyUSBDNAFingerprint(const usb_dna_fingerprint_t* fingerprint)
{
    uint8_t calculated_signature[USB_DNA_SIGNATURE_SIZE];

    if (!fingerprint) {
        return FALSE;
    }

    // Generate signature
    if (!GenerateDNASignature(fingerprint, calculated_signature)) {
        return FALSE;
    }

    // Compare signatures
    return (memcmp(fingerprint->signature, calculated_signature, USB_DNA_SIGNATURE_SIZE) == 0);
}

// Get USB drive characteristics
BOOL GetUSBDNACharacteristics(const char* drive_path, usb_dna_characteristics_t* characteristics)
{
    if (!drive_path || !characteristics) {
        return FALSE;
    }

    memset(characteristics, 0, sizeof(usb_dna_characteristics_t));

    // Extract basic characteristics
    ExtractVendorID(drive_path, &characteristics->vendor_id);
    ExtractProductID(drive_path, &characteristics->product_id);
    ExtractSerialNumber(drive_path, characteristics->serial_number, sizeof(characteristics->serial_number));
    ExtractFirmwareVersion(drive_path, characteristics->firmware_version, sizeof(characteristics->firmware_version));
    ExtractControllerChip(drive_path, characteristics->controller_chip, sizeof(characteristics->controller_chip));
    ExtractMemoryType(drive_path, characteristics->memory_type, sizeof(characteristics->memory_type));
    ExtractCapacity(drive_path, &characteristics->capacity);
    ExtractSectorSize(drive_path, &characteristics->sector_size);

    // Measure performance characteristics
    MeasureReadSpeed(drive_path, &characteristics->read_speed);
    MeasureWriteSpeed(drive_path, &characteristics->write_speed);
    MeasurePowerConsumption(drive_path, &characteristics->power_consumption);

    // Measure physical characteristics
    MeasureTemperatureRange(drive_path, &characteristics->temperature_min, &characteristics->temperature_max);
    MeasureVibrationPattern(drive_path, &characteristics->vibration_frequency);
    MeasureEMSignature(drive_path, &characteristics->em_signature_strength);

    // Extract manufacturing information
    ExtractManufacturingDate(drive_path, characteristics->manufacturing_date, sizeof(characteristics->manufacturing_date));
    ExtractBatchNumber(drive_path, characteristics->batch_number, sizeof(characteristics->batch_number));

    // Identify unique characteristics
    characteristics->characteristics_count = 0;
    for (int i = 0; i < USB_DNA_MAX_CHARACTERISTICS; i++) {
        if (IsCharacteristicUnique(characteristics, (usb_dna_characteristic_t)i)) {
            characteristics->unique_characteristics[characteristics->characteristics_count] = i;
            characteristics->characteristics_count++;
        }
    }

    uprintf("Extracted %d USB drive characteristics", characteristics->characteristics_count);
    return TRUE;
}

// Extract vendor ID
BOOL ExtractVendorID(const char* drive_path, uint16_t* vendor_id)
{
    // Simplified vendor ID extraction
    // In reality, query USB device descriptors
    
    if (!drive_path || !vendor_id) {
        return FALSE;
    }

    // Simulate vendor ID extraction
    *vendor_id = 0x1234;  // Example vendor ID
    return TRUE;
}

// Extract product ID
BOOL ExtractProductID(const char* drive_path, uint16_t* product_id)
{
    // Simplified product ID extraction
    // In reality, query USB device descriptors
    
    if (!drive_path || !product_id) {
        return FALSE;
    }

    // Simulate product ID extraction
    *product_id = 0x5678;  // Example product ID
    return TRUE;
}

// Extract serial number
BOOL ExtractSerialNumber(const char* drive_path, char* serial_number, size_t max_len)
{
    // Simplified serial number extraction
    // In reality, query USB device descriptors
    
    if (!drive_path || !serial_number || max_len == 0) {
        return FALSE;
    }

    // Simulate serial number extraction
    strncpy(serial_number, "USB123456789", max_len - 1);
    serial_number[max_len - 1] = '\0';
    return TRUE;
}

// Extract firmware version
BOOL ExtractFirmwareVersion(const char* drive_path, char* firmware_version, size_t max_len)
{
    // Simplified firmware version extraction
    // In reality, query device firmware
    
    if (!drive_path || !firmware_version || max_len == 0) {
        return FALSE;
    }

    // Simulate firmware version extraction
    strncpy(firmware_version, "1.2.3.4", max_len - 1);
    firmware_version[max_len - 1] = '\0';
    return TRUE;
}

// Extract controller chip
BOOL ExtractControllerChip(const char* drive_path, char* controller_chip, size_t max_len)
{
    // Simplified controller chip extraction
    // In reality, query device hardware information
    
    if (!drive_path || !controller_chip || max_len == 0) {
        return FALSE;
    }

    // Simulate controller chip extraction
    strncpy(controller_chip, "USB3.0 Controller v2.1", max_len - 1);
    controller_chip[max_len - 1] = '\0';
    return TRUE;
}

// Extract memory type
BOOL ExtractMemoryType(const char* drive_path, char* memory_type, size_t max_len)
{
    // Simplified memory type extraction
    // In reality, query device memory specifications
    
    if (!drive_path || !memory_type || max_len == 0) {
        return FALSE;
    }

    // Simulate memory type extraction
    strncpy(memory_type, "NAND Flash", max_len - 1);
    memory_type[max_len - 1] = '\0';
    return TRUE;
}

// Extract capacity
BOOL ExtractCapacity(const char* drive_path, uint64_t* capacity)
{
    // Simplified capacity extraction
    // In reality, query device capacity
    
    if (!drive_path || !capacity) {
        return FALSE;
    }

    // Simulate capacity extraction
    *capacity = 32ULL * 1024 * 1024 * 1024;  // 32GB
    return TRUE;
}

// Extract sector size
BOOL ExtractSectorSize(const char* drive_path, uint32_t* sector_size)
{
    // Simplified sector size extraction
    // In reality, query device sector size
    
    if (!drive_path || !sector_size) {
        return FALSE;
    }

    // Simulate sector size extraction
    *sector_size = 512;  // 512 bytes
    return TRUE;
}

// Measure read speed
BOOL MeasureReadSpeed(const char* drive_path, float* read_speed)
{
    // Simplified read speed measurement
    // In reality, perform actual speed tests
    
    if (!drive_path || !read_speed) {
        return FALSE;
    }

    // Simulate read speed measurement
    *read_speed = 25.5f;  // 25.5 MB/s
    return TRUE;
}

// Measure write speed
BOOL MeasureWriteSpeed(const char* drive_path, float* write_speed)
{
    // Simplified write speed measurement
    // In reality, perform actual speed tests
    
    if (!drive_path || !write_speed) {
        return FALSE;
    }

    // Simulate write speed measurement
    *write_speed = 18.2f;  // 18.2 MB/s
    return TRUE;
}

// Measure power consumption
BOOL MeasurePowerConsumption(const char* drive_path, float* power_consumption)
{
    // Simplified power consumption measurement
    // In reality, measure actual power usage
    
    if (!drive_path || !power_consumption) {
        return FALSE;
    }

    // Simulate power consumption measurement
    *power_consumption = 2.5f;  // 2.5W
    return TRUE;
}

// Measure temperature range
BOOL MeasureTemperatureRange(const char* drive_path, float* temp_min, float* temp_max)
{
    // Simplified temperature measurement
    // In reality, measure actual temperature
    
    if (!drive_path || !temp_min || !temp_max) {
        return FALSE;
    }

    // Simulate temperature measurement
    *temp_min = 20.0f;  // 20°C
    *temp_max = 45.0f;  // 45°C
    return TRUE;
}

// Measure vibration pattern
BOOL MeasureVibrationPattern(const char* drive_path, float* vibration_frequency)
{
    // Simplified vibration measurement
    // In reality, measure actual vibration patterns
    
    if (!drive_path || !vibration_frequency) {
        return FALSE;
    }

    // Simulate vibration measurement
    *vibration_frequency = 120.5f;  // 120.5 Hz
    return TRUE;
}

// Measure electromagnetic signature
BOOL MeasureEMSignature(const char* drive_path, float* em_strength)
{
    // Simplified EM signature measurement
    // In reality, measure actual electromagnetic emissions
    
    if (!drive_path || !em_strength) {
        return FALSE;
    }

    // Simulate EM signature measurement
    *em_strength = 0.75f;  // 0.75 (normalized strength)
    return TRUE;
}

// Extract manufacturing date
BOOL ExtractManufacturingDate(const char* drive_path, char* manufacturing_date, size_t max_len)
{
    // Simplified manufacturing date extraction
    // In reality, query device manufacturing information
    
    if (!drive_path || !manufacturing_date || max_len == 0) {
        return FALSE;
    }

    // Simulate manufacturing date extraction
    strncpy(manufacturing_date, "2024-01-15", max_len - 1);
    manufacturing_date[max_len - 1] = '\0';
    return TRUE;
}

// Extract batch number
BOOL ExtractBatchNumber(const char* drive_path, char* batch_number, size_t max_len)
{
    // Simplified batch number extraction
    // In reality, query device batch information
    
    if (!drive_path || !batch_number || max_len == 0) {
        return FALSE;
    }

    // Simulate batch number extraction
    strncpy(batch_number, "BATCH-2024-001", max_len - 1);
    batch_number[max_len - 1] = '\0';
    return TRUE;
}

// Generate DNA from characteristics
BOOL GenerateDNAFromCharacteristics(const usb_dna_characteristics_t* characteristics, 
                                   usb_dna_fingerprint_t* fingerprint)
{
    uint8_t hash[USB_DNA_FINGERPRINT_SIZE];
    uint8_t signature[USB_DNA_SIGNATURE_SIZE];

    if (!characteristics || !fingerprint) {
        return FALSE;
    }

    memset(fingerprint, 0, sizeof(usb_dna_fingerprint_t));

    // Calculate DNA hash
    if (!CalculateDNAHash(characteristics, hash)) {
        return FALSE;
    }

    // Copy hash to fingerprint
    memcpy(fingerprint->fingerprint, hash, USB_DNA_FINGERPRINT_SIZE);

    // Generate signature
    if (!GenerateDNASignature(fingerprint, signature)) {
        return FALSE;
    }

    // Copy signature to fingerprint
    memcpy(fingerprint->signature, signature, USB_DNA_SIGNATURE_SIZE);

    // Generate metadata
    static_sprintf(fingerprint->metadata,
        "Vendor: 0x%04X, Product: 0x%04X, Serial: %s, Firmware: %s",
        characteristics->vendor_id, characteristics->product_id,
        characteristics->serial_number, characteristics->firmware_version);

    return TRUE;
}

// Calculate DNA hash
BOOL CalculateDNAHash(const usb_dna_characteristics_t* characteristics, uint8_t* hash)
{
    uint8_t data[1024];
    uint32_t data_size = 0;

    if (!characteristics || !hash) {
        return FALSE;
    }

    // Pack characteristics into data array
    memcpy(data + data_size, &characteristics->vendor_id, sizeof(characteristics->vendor_id));
    data_size += sizeof(characteristics->vendor_id);

    memcpy(data + data_size, &characteristics->product_id, sizeof(characteristics->product_id));
    data_size += sizeof(characteristics->product_id);

    memcpy(data + data_size, characteristics->serial_number, strlen(characteristics->serial_number));
    data_size += (uint32_t)strlen(characteristics->serial_number);

    memcpy(data + data_size, &characteristics->capacity, sizeof(characteristics->capacity));
    data_size += sizeof(characteristics->capacity);

    memcpy(data + data_size, &characteristics->read_speed, sizeof(characteristics->read_speed));
    data_size += sizeof(characteristics->read_speed);

    memcpy(data + data_size, &characteristics->write_speed, sizeof(characteristics->write_speed));
    data_size += sizeof(characteristics->write_speed);

    // Calculate hash
    memset(hash, 0, USB_DNA_FINGERPRINT_SIZE);
    for (uint32_t i = 0; i < data_size; i++) {
        hash[i % USB_DNA_FINGERPRINT_SIZE] ^= data[i];
    }

    return TRUE;
}

// Generate DNA signature
BOOL GenerateDNASignature(const usb_dna_fingerprint_t* fingerprint, uint8_t* signature)
{
    if (!fingerprint || !signature) {
        return FALSE;
    }

    // Create deterministic signature based on fingerprint
    for (uint32_t i = 0; i < USB_DNA_SIGNATURE_SIZE; i++) {
        signature[i] = (uint8_t)((fingerprint->fingerprint[i % USB_DNA_FINGERPRINT_SIZE] + i) ^ 0xAA);
    }

    return TRUE;
}

// Check if USB DNA is unique
BOOL IsUSBDNAUnique(const usb_dna_fingerprint_t* fingerprint)
{
    // Simplified uniqueness check
    // In reality, compare against database of known fingerprints
    
    if (!fingerprint) {
        return FALSE;
    }

    // Check for obvious patterns that indicate non-uniqueness
    uint32_t zero_count = 0;
    for (uint32_t i = 0; i < USB_DNA_FINGERPRINT_SIZE; i++) {
        if (fingerprint->fingerprint[i] == 0) {
            zero_count++;
        }
    }

    // If more than 50% zeros, likely not unique
    return (zero_count < (USB_DNA_FINGERPRINT_SIZE / 2));
}

// Check if characteristic is unique
BOOL IsCharacteristicUnique(const usb_dna_characteristics_t* characteristics, 
                           usb_dna_characteristic_t characteristic)
{
    if (!characteristics) {
        return FALSE;
    }

    // Simplified uniqueness check
    // In reality, compare against database of known characteristics
    
    switch (characteristic) {
        case USB_DNA_CHAR_SERIAL_NUMBER:
            return (strlen(characteristics->serial_number) > 0);
        case USB_DNA_CHAR_VENDOR_ID:
            return (characteristics->vendor_id != 0);
        case USB_DNA_CHAR_PRODUCT_ID:
            return (characteristics->product_id != 0);
        case USB_DNA_CHAR_CAPACITY:
            return (characteristics->capacity > 0);
        default:
            return TRUE;  // Assume other characteristics are unique
    }
}

// Get characteristic name
char* GetCharacteristicName(usb_dna_characteristic_t characteristic)
{
    switch (characteristic) {
        case USB_DNA_CHAR_VENDOR_ID: return "Vendor ID";
        case USB_DNA_CHAR_PRODUCT_ID: return "Product ID";
        case USB_DNA_CHAR_SERIAL_NUMBER: return "Serial Number";
        case USB_DNA_CHAR_FIRMWARE_VERSION: return "Firmware Version";
        case USB_DNA_CHAR_CONTROLLER_CHIP: return "Controller Chip";
        case USB_DNA_CHAR_MEMORY_TYPE: return "Memory Type";
        case USB_DNA_CHAR_CAPACITY: return "Capacity";
        case USB_DNA_CHAR_SECTOR_SIZE: return "Sector Size";
        case USB_DNA_CHAR_READ_SPEED: return "Read Speed";
        case USB_DNA_CHAR_WRITE_SPEED: return "Write Speed";
        case USB_DNA_CHAR_POWER_CONSUMPTION: return "Power Consumption";
        case USB_DNA_CHAR_TEMPERATURE_RANGE: return "Temperature Range";
        case USB_DNA_CHAR_VIBRATION_PATTERN: return "Vibration Pattern";
        case USB_DNA_CHAR_ELECTROMAGNETIC_SIGNATURE: return "EM Signature";
        case USB_DNA_CHAR_MANUFACTURING_DATE: return "Manufacturing Date";
        case USB_DNA_CHAR_BATCH_NUMBER: return "Batch Number";
        default: return "Unknown";
    }
}

// Get USB DNA timestamp
uint64_t GetUSBDNATimestamp(void)
{
    return (uint64_t)time(NULL);
}
