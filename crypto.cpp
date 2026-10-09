#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <bcrypt.h>
#include "crypto.h"

#pragma comment(lib, "bcrypt.lib")

#include "argon2/argon2.h"
#include "argon2/core.h"

#define AES_KEY_SIZE_256 32
#define ITERATIONS 8

static BOOL DeriveKeyFromPassword(const char* password, const uint8_t* salt, uint8_t* out_key) {
    if (!password || !salt || !out_key) return FALSE;
    size_t pass_len = strlen(password);
    if (pass_len < 1 || pass_len > 1024) return FALSE;

    uint32_t t_cost = ITERATIONS;
    uint32_t m_cost = 64 * 1024; // 64 MiB RAM
    uint32_t parallelism = 1;

    int res = argon2_hash(
        t_cost,
        m_cost,
        parallelism,
        password, pass_len,
        salt, ARGON_SALT_SIZE,
        out_key, AES_KEY_SIZE_256,
        NULL, 0,
        Argon2_id,
        ARGON2_VERSION_13
    );

    return (res == ARGON2_OK);
}

uint8_t* AES_Encrypt(const uint8_t* data, uint32_t len, const char* password, uint32_t* out_len) {
    if (!data || len == 0 || !password || !out_len) return NULL;

    uint32_t total_len = CRYPTO_HEADER_SIZE + len;
    uint8_t* cryptobuffer = (uint8_t*)malloc(total_len);
    if (!cryptobuffer) return NULL;

    uint8_t* salt  = cryptobuffer;
    uint8_t* nonce = cryptobuffer + ARGON_SALT_SIZE;
    uint8_t* tag   = cryptobuffer + ARGON_SALT_SIZE + GCM_NONCE_SIZE;
    uint8_t* ct    = cryptobuffer + CRYPTO_HEADER_SIZE;

    // 1. Generate random Salt & Nonce
    if (!BCRYPT_SUCCESS(BCryptGenRandom(NULL, salt, ARGON_SALT_SIZE, BCRYPT_USE_SYSTEM_PREFERRED_RNG)) ||
        !BCRYPT_SUCCESS(BCryptGenRandom(NULL, nonce, GCM_NONCE_SIZE, BCRYPT_USE_SYSTEM_PREFERRED_RNG))) {
        free(cryptobuffer);
        return NULL;
    }

    // 2. Derive key dynamically using unique Salt
    uint8_t key[AES_KEY_SIZE_256];
    if (!DeriveKeyFromPassword(password, salt, key)) {
        free(cryptobuffer);
        return NULL;
    }

    // 3. Open Windows CNG AES-GCM Provider
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status)) {
        SecureZeroMemory(key, sizeof(key));
        free(cryptobuffer);
        return NULL;
    }

    status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    if (BCRYPT_SUCCESS(status)) {
        status = BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, key, AES_KEY_SIZE_256, 0);
    }
    SecureZeroMemory(key, sizeof(key)); // Wipe master key immediately

    if (!BCRYPT_SUCCESS(status)) {
        if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
        free(cryptobuffer);
        return NULL;
    }

    // 4. Authenticated Encryption
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = nonce;
    authInfo.cbNonce = GCM_NONCE_SIZE;
    authInfo.pbTag   = tag;
    authInfo.cbTag   = GCM_TAG_SIZE;

    ULONG cbResult = 0;
    status = BCryptEncrypt(hKey, (PUCHAR)data, len, &authInfo, NULL, 0, ct, len, &cbResult, 0);

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (!BCRYPT_SUCCESS(status)) {
        free(cryptobuffer);
        return NULL;
    }

    *out_len = total_len;
    return cryptobuffer;
}

BOOL AES_Decrypt(uint8_t* data, uint32_t len, const char* password, uint32_t* out_len) {
    if (!data || len < CRYPTO_HEADER_SIZE || !password || !out_len) return FALSE;

    uint8_t* salt  = data;
    uint8_t* nonce = data + ARGON_SALT_SIZE;
    uint8_t* tag   = data + ARGON_SALT_SIZE + GCM_NONCE_SIZE;
    uint8_t* ct    = data + CRYPTO_HEADER_SIZE;
    uint32_t ct_len = len - CRYPTO_HEADER_SIZE;

    // 1. Derive key dynamically using payload's Salt
    uint8_t key[AES_KEY_SIZE_256];
    if (!DeriveKeyFromPassword(password, salt, key)) {
        SecureZeroMemory(data, len);
        return FALSE;
    }

    uint8_t* temp_pt = (uint8_t*)malloc(ct_len > 0 ? ct_len : 1);
    if (!temp_pt) {
        SecureZeroMemory(key, sizeof(key));
        SecureZeroMemory(data, len);
        return FALSE;
    }

    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status)) {
        SecureZeroMemory(key, sizeof(key));
        SecureZeroMemory(temp_pt, ct_len);
        free(temp_pt);
        return FALSE;
    }

    status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    if (BCRYPT_SUCCESS(status)) {
        status = BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, key, AES_KEY_SIZE_256, 0);
    }
    SecureZeroMemory(key, sizeof(key)); // Wipe master key immediately

    if (!BCRYPT_SUCCESS(status)) {
        if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
        SecureZeroMemory(temp_pt, ct_len);
        free(temp_pt);
        return FALSE;
    }

    // 2. Verify Authentication Tag & Decrypt
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = nonce;
    authInfo.cbNonce = GCM_NONCE_SIZE;
    authInfo.pbTag   = tag;
    authInfo.cbTag   = GCM_TAG_SIZE;

    ULONG cbResult = 0;
    status = BCryptDecrypt(hKey, ct, ct_len, &authInfo, NULL, 0, temp_pt, ct_len, &cbResult, 0);

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (BCRYPT_SUCCESS(status)) {
        memcpy(data, temp_pt, ct_len);
        SecureZeroMemory(data + ct_len, len - ct_len);
        *out_len = ct_len;
    } else {
        SecureZeroMemory(data, len);
        *out_len = 0;
    }

    SecureZeroMemory(temp_pt, ct_len > 0 ? ct_len : 1);
    free(temp_pt);
    return BCRYPT_SUCCESS(status);
}

char* hex_encode(const uint8_t* data, uint32_t len, uint32_t* out_len) {
    if (!data || len == 0 || !out_len) return NULL;

    static const char hex_digits[] = "0123456789abcdef";
    uint32_t hex_len = len * 2;
    char* hexbuffer = (char*)malloc(hex_len + 1);
    if (!hexbuffer) return NULL;

    for (uint32_t i = 0; i < len; i++) {
        hexbuffer[i * 2]     = hex_digits[(data[i] >> 4) & 0x0F];
        hexbuffer[i * 2 + 1] = hex_digits[data[i] & 0x0F];
    }
    hexbuffer[hex_len] = '\0';
    *out_len = hex_len;
    return hexbuffer;
}
