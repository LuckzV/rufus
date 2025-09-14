/*
 * Rufus: The Reliable USB Formatting Utility
 * Quantum-Resistant Encryption System
 * Copyright © 2011-2025 Pete Batard <pete@akeo.ie>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "rufus.h"

// Quantum-resistant encryption constants
#define QUANTUM_KEY_SIZE           32
#define QUANTUM_IV_SIZE           16
#define QUANTUM_SIGNATURE_SIZE    64
#define QUANTUM_HASH_SIZE         32
#define QUANTUM_BLOCK_SIZE        16
#define QUANTUM_MAX_PLAINTEXT     1048576  // 1MB max

// Post-quantum algorithms
typedef enum {
    QUANTUM_ALGO_SPHINCS_PLUS = 0,    // Hash-based signatures
    QUANTUM_ALGO_CRYSTALS_KYBER,      // Key encapsulation
    QUANTUM_ALGO_CRYSTALS_DILITHIUM,  // Digital signatures
    QUANTUM_ALGO_FALCON,              // Lattice-based signatures
    QUANTUM_ALGO_NTRU,                // Lattice-based encryption
    QUANTUM_ALGO_MAX
} quantum_algorithm_t;

// Encryption context
typedef struct {
    quantum_algorithm_t algorithm;
    uint8_t master_key[QUANTUM_KEY_SIZE];
    uint8_t public_key[QUANTUM_KEY_SIZE];
    uint8_t private_key[QUANTUM_KEY_SIZE];
    uint8_t session_key[QUANTUM_KEY_SIZE];
    BOOL is_initialized;
} quantum_encryption_context_t;

// Encrypted data header
typedef struct {
    uint32_t magic;                    // 0x5152454E (QREN)
    uint32_t version;                  // Encryption version
    quantum_algorithm_t algorithm;     // Algorithm used
    uint32_t data_size;               // Original data size
    uint32_t encrypted_size;          // Encrypted data size
    uint8_t iv[QUANTUM_IV_SIZE];      // Initialization vector
    uint8_t signature[QUANTUM_SIGNATURE_SIZE]; // Digital signature
    uint8_t hash[QUANTUM_HASH_SIZE];  // Data integrity hash
    uint64_t timestamp;               // Encryption timestamp
} quantum_encrypted_header_t;

// Encryption result
typedef struct {
    BOOL success;
    uint32_t encrypted_size;
    uint32_t original_size;
    char error_message[256];
    uint64_t encryption_time;
    uint64_t decryption_time;
} quantum_encryption_result_t;

// Function prototypes
BOOL InitQuantumEncryption(quantum_algorithm_t algorithm);
void CleanupQuantumEncryption(void);
BOOL EncryptData(const uint8_t* plaintext, uint32_t plaintext_size, 
                 uint8_t* ciphertext, uint32_t* ciphertext_size);
BOOL DecryptData(const uint8_t* ciphertext, uint32_t ciphertext_size,
                 uint8_t* plaintext, uint32_t* plaintext_size);
BOOL GenerateQuantumKeyPair(quantum_algorithm_t algorithm, 
                           uint8_t* public_key, uint8_t* private_key);
BOOL SignData(const uint8_t* data, uint32_t data_size, uint8_t* signature);
BOOL VerifySignature(const uint8_t* data, uint32_t data_size, 
                    const uint8_t* signature, const uint8_t* public_key);
BOOL EncryptUSBDrive(const char* drive_path, const char* password);
BOOL DecryptUSBDrive(const char* drive_path, const char* password);
BOOL IsQuantumResistant(const char* algorithm_name);

// SPHINCS+ implementation
BOOL SPHINCS_GenerateKeyPair(uint8_t* public_key, uint8_t* private_key);
BOOL SPHINCS_Sign(const uint8_t* message, uint32_t message_size, 
                  const uint8_t* private_key, uint8_t* signature);
BOOL SPHINCS_Verify(const uint8_t* message, uint32_t message_size,
                   const uint8_t* signature, const uint8_t* public_key);

// CRYSTALS-KYBER implementation
BOOL KYBER_GenerateKeyPair(uint8_t* public_key, uint8_t* private_key);
BOOL KYBER_Encrypt(const uint8_t* plaintext, uint32_t plaintext_size,
                   const uint8_t* public_key, uint8_t* ciphertext, uint32_t* ciphertext_size);
BOOL KYBER_Decrypt(const uint8_t* ciphertext, uint32_t ciphertext_size,
                   const uint8_t* private_key, uint8_t* plaintext, uint32_t* plaintext_size);

// CRYSTALS-DILITHIUM implementation
BOOL DILITHIUM_GenerateKeyPair(uint8_t* public_key, uint8_t* private_key);
BOOL DILITHIUM_Sign(const uint8_t* message, uint32_t message_size,
                   const uint8_t* private_key, uint8_t* signature);
BOOL DILITHIUM_Verify(const uint8_t* message, uint32_t message_size,
                     const uint8_t* signature, const uint8_t* public_key);

// Utility functions
BOOL GenerateRandomBytes(uint8_t* buffer, uint32_t size);
BOOL DeriveKeyFromPassword(const char* password, const uint8_t* salt, 
                          uint8_t* derived_key);
BOOL HashData(const uint8_t* data, uint32_t data_size, uint8_t* hash);
BOOL CompareBytes(const uint8_t* a, const uint8_t* b, uint32_t size);
uint64_t GetQuantumResistantTime(void);
BOOL IsPostQuantumAlgorithm(quantum_algorithm_t algorithm);
