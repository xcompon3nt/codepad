
#pragma once
#include <stdint.h>
#include <windows.h>

#define ARGON_SALT_SIZE    16   // 128-bit random salt
#define GCM_NONCE_SIZE     12   // 96-bit standard GCM Nonce
#define GCM_TAG_SIZE       16   // 128-bit Authentication Tag
#define CRYPTO_HEADER_SIZE (ARGON_SALT_SIZE + GCM_NONCE_SIZE + GCM_TAG_SIZE) // 44 bytes (88 hex characters)

uint8_t* AES_Encrypt(const uint8_t* data, uint32_t len, const char* password, uint32_t* out_len);
BOOL AES_Decrypt(uint8_t* data, uint32_t len, const char* password, uint32_t* out_len);
char* hex_encode(const uint8_t* data, uint32_t len, uint32_t* out_len);
