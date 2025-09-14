/*
 * Rufus: The Reliable USB Formatting Utility
 * Blockchain-Based Drive Verification System
 * Copyright © 2011-2025 Pete Batard <pete@akeo.ie>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "rufus.h"

// Blockchain verification constants
#define BLOCKCHAIN_HASH_SIZE       32
#define BLOCKCHAIN_SIGNATURE_SIZE  64
#define BLOCKCHAIN_MERKLE_DEPTH    16
#define BLOCKCHAIN_NETWORK_TIMEOUT 30000  // 30 seconds

// Blockchain networks
typedef enum {
    BLOCKCHAIN_MAINNET = 0,
    BLOCKCHAIN_TESTNET,
    BLOCKCHAIN_LOCAL,
    BLOCKCHAIN_MAX
} blockchain_network_t;

// Drive verification record
typedef struct {
    uint8_t drive_hash[BLOCKCHAIN_HASH_SIZE];
    uint8_t signature[BLOCKCHAIN_SIGNATURE_SIZE];
    uint64_t timestamp;
    uint64_t block_number;
    char drive_serial[64];
    char creator[32];
    uint32_t version;
    uint32_t checksum;
} drive_verification_record_t;

// Merkle tree node
typedef struct {
    uint8_t hash[BLOCKCHAIN_HASH_SIZE];
    struct merkle_node* left;
    struct merkle_node* right;
    struct merkle_node* parent;
} merkle_node_t;

// Blockchain context
typedef struct {
    blockchain_network_t network;
    char node_url[256];
    char private_key[128];
    char public_key[128];
    uint64_t gas_price;
    uint64_t gas_limit;
    BOOL is_connected;
} blockchain_context_t;

// Verification result
typedef struct {
    BOOL is_verified;
    BOOL is_tampered;
    uint64_t verification_time;
    char verification_proof[512];
    uint32_t confidence_level;
    char error_message[256];
} verification_result_t;

// Function prototypes
BOOL InitBlockchainVerifier(blockchain_network_t network, const char* node_url);
void CleanupBlockchainVerifier(void);
BOOL CreateDriveVerificationRecord(const char* drive_path, drive_verification_record_t* record);
BOOL VerifyDriveIntegrity(const char* drive_path, verification_result_t* result);
BOOL SubmitToBlockchain(drive_verification_record_t* record);
BOOL QueryBlockchainVerification(const char* drive_serial, verification_result_t* result);
BOOL BuildMerkleTree(drive_verification_record_t* records, int count, merkle_node_t** root);
BOOL VerifyMerkleProof(merkle_node_t* root, const char* target_hash);
BOOL IsDriveTampered(const char* drive_path, drive_verification_record_t* record);

// Cryptographic functions
BOOL GenerateDriveHash(const char* drive_path, uint8_t* hash);
BOOL GenerateSignature(const uint8_t* data, size_t data_size, uint8_t* signature);
BOOL VerifySignature(const uint8_t* data, size_t data_size, const uint8_t* signature, const char* public_key);
BOOL GenerateKeyPair(char* private_key, char* public_key);
BOOL HashData(const uint8_t* data, size_t data_size, uint8_t* hash);

// Blockchain network functions
BOOL ConnectToBlockchain(blockchain_network_t network);
BOOL DisconnectFromBlockchain(void);
BOOL SendTransaction(const char* transaction_data);
BOOL GetBlockHeight(uint64_t* height);
BOOL GetTransactionStatus(const char* tx_hash, BOOL* confirmed);

// Utility functions
char* HashToHexString(const uint8_t* hash, size_t hash_size);
BOOL HexStringToHash(const char* hex_string, uint8_t* hash);
uint64_t GetCurrentBlockchainTime(void);
BOOL IsBlockchainAvailable(void);
