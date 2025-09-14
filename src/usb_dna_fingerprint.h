/*
 * Rufus: The Reliable USB Formatting Utility
 * USB Drive DNA Fingerprinting System
 * Copyright © 2011-2025 Pete Batard <pete@akeo.ie>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "rufus.h"

// USB DNA fingerprinting constants
#define USB_DNA_FINGERPRINT_SIZE  64
#define USB_DNA_METADATA_SIZE     256
#define USB_DNA_SIGNATURE_SIZE    32
#define USB_DNA_MAX_CHARACTERISTICS 32

// USB drive characteristics
typedef enum {
    USB_DNA_CHAR_VENDOR_ID = 0,
    USB_DNA_CHAR_PRODUCT_ID,
    USB_DNA_CHAR_SERIAL_NUMBER,
    USB_DNA_CHAR_FIRMWARE_VERSION,
    USB_DNA_CHAR_CONTROLLER_CHIP,
    USB_DNA_CHAR_MEMORY_TYPE,
    USB_DNA_CHAR_CAPACITY,
    USB_DNA_CHAR_SECTOR_SIZE,
    USB_DNA_CHAR_READ_SPEED,
    USB_DNA_CHAR_WRITE_SPEED,
    USB_DNA_CHAR_POWER_CONSUMPTION,
    USB_DNA_CHAR_TEMPERATURE_RANGE,
    USB_DNA_CHAR_VIBRATION_PATTERN,
    USB_DNA_CHAR_ELECTROMAGNETIC_SIGNATURE,
    USB_DNA_CHAR_MANUFACTURING_DATE,
    USB_DNA_CHAR_BATCH_NUMBER,
    USB_DNA_CHAR_MAX
} usb_dna_characteristic_t;

// USB drive DNA fingerprint
typedef struct {
    uint8_t fingerprint[USB_DNA_FINGERPRINT_SIZE];
    uint8_t signature[USB_DNA_SIGNATURE_SIZE];
    char metadata[USB_DNA_METADATA_SIZE];
    uint64_t timestamp;
    uint32_t version;
    uint32_t confidence_level;
    BOOL is_unique;
    BOOL is_verified;
} usb_dna_fingerprint_t;

// USB drive characteristics data
typedef struct {
    uint16_t vendor_id;
    uint16_t product_id;
    char serial_number[64];
    char firmware_version[32];
    char controller_chip[64];
    char memory_type[32];
    uint64_t capacity;
    uint32_t sector_size;
    float read_speed;
    float write_speed;
    float power_consumption;
    float temperature_min;
    float temperature_max;
    float vibration_frequency;
    float em_signature_strength;
    char manufacturing_date[16];
    char batch_number[32];
    uint32_t unique_characteristics[USB_DNA_MAX_CHARACTERISTICS];
    uint32_t characteristics_count;
} usb_dna_characteristics_t;

// DNA fingerprinting context
typedef struct {
    usb_dna_characteristics_t characteristics;
    usb_dna_fingerprint_t fingerprint;
    char drive_path[MAX_PATH];
    BOOL is_initialized;
    uint64_t last_scan_time;
} usb_dna_context_t;

// DNA comparison result
typedef struct {
    BOOL is_match;
    float similarity_score;
    uint32_t matching_characteristics;
    uint32_t total_characteristics;
    char comparison_details[512];
    uint64_t comparison_time;
} usb_dna_comparison_result_t;

// Function prototypes
BOOL InitUSBDNAFingerprinting(const char* drive_path);
void CleanupUSBDNAFingerprinting(void);
BOOL GenerateUSBDNAFingerprint(const char* drive_path, usb_dna_fingerprint_t* fingerprint);
BOOL CompareUSBDNAFingerprints(const usb_dna_fingerprint_t* fingerprint1, 
                              const usb_dna_fingerprint_t* fingerprint2,
                              usb_dna_comparison_result_t* result);
BOOL VerifyUSBDNAFingerprint(const usb_dna_fingerprint_t* fingerprint);
BOOL SaveUSBDNAFingerprint(const char* drive_path, const usb_dna_fingerprint_t* fingerprint);
BOOL LoadUSBDNAFingerprint(const char* drive_path, usb_dna_fingerprint_t* fingerprint);
BOOL IsUSBDNAUnique(const usb_dna_fingerprint_t* fingerprint);
BOOL GetUSBDNACharacteristics(const char* drive_path, usb_dna_characteristics_t* characteristics);

// Characteristic extraction functions
BOOL ExtractVendorID(const char* drive_path, uint16_t* vendor_id);
BOOL ExtractProductID(const char* drive_path, uint16_t* product_id);
BOOL ExtractSerialNumber(const char* drive_path, char* serial_number, size_t max_len);
BOOL ExtractFirmwareVersion(const char* drive_path, char* firmware_version, size_t max_len);
BOOL ExtractControllerChip(const char* drive_path, char* controller_chip, size_t max_len);
BOOL ExtractMemoryType(const char* drive_path, char* memory_type, size_t max_len);
BOOL ExtractCapacity(const char* drive_path, uint64_t* capacity);
BOOL ExtractSectorSize(const char* drive_path, uint32_t* sector_size);
BOOL MeasureReadSpeed(const char* drive_path, float* read_speed);
BOOL MeasureWriteSpeed(const char* drive_path, float* write_speed);
BOOL MeasurePowerConsumption(const char* drive_path, float* power_consumption);
BOOL MeasureTemperatureRange(const char* drive_path, float* temp_min, float* temp_max);
BOOL MeasureVibrationPattern(const char* drive_path, float* vibration_frequency);
BOOL MeasureEMSignature(const char* drive_path, float* em_strength);
BOOL ExtractManufacturingDate(const char* drive_path, char* manufacturing_date, size_t max_len);
BOOL ExtractBatchNumber(const char* drive_path, char* batch_number, size_t max_len);

// DNA generation functions
BOOL GenerateDNAFromCharacteristics(const usb_dna_characteristics_t* characteristics, 
                                   usb_dna_fingerprint_t* fingerprint);
BOOL CalculateDNAHash(const usb_dna_characteristics_t* characteristics, uint8_t* hash);
BOOL GenerateDNASignature(const usb_dna_fingerprint_t* fingerprint, uint8_t* signature);
BOOL VerifyDNASignature(const usb_dna_fingerprint_t* fingerprint, const uint8_t* signature);

// Utility functions
float CalculateSimilarityScore(const usb_dna_characteristics_t* char1, 
                              const usb_dna_characteristics_t* char2);
uint32_t CountMatchingCharacteristics(const usb_dna_characteristics_t* char1, 
                                    const usb_dna_characteristics_t* char2);
BOOL IsCharacteristicUnique(const usb_dna_characteristics_t* characteristics, 
                           usb_dna_characteristic_t characteristic);
char* GetCharacteristicName(usb_dna_characteristic_t characteristic);
BOOL ValidateUSBDNAFingerprint(const usb_dna_fingerprint_t* fingerprint);
uint64_t GetUSBDNATimestamp(void);
