#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace btc::crypto {

// SHA-256 hash of a string/buffer
std::string sha256_hex(const std::string& input);
std::vector<uint8_t> sha256_bytes(const std::vector<uint8_t>& input);

// RIPEMD-160 hash
std::string ripemd160_hex(const std::vector<uint8_t>& input);

// Double SHA-256 (Hash256 used in Bitcoin blocks and txids)
std::string hash256_hex(const std::string& input);
std::vector<uint8_t> hash256_bytes(const std::vector<uint8_t>& input);

// Hash160 = RIPEMD160(SHA256(input))
std::string hash160_hex(const std::vector<uint8_t>& input);

// Base58 and Base58Check encoding for Bitcoin Testnet (Legacy P2PKH / P2SH)
std::string base58_encode(const std::vector<uint8_t>& data);
std::string base58check_encode(uint8_t version, const std::vector<uint8_t>& payload);

// Bech32 / Bech32m for Testnet SegWit (tb1q...) and Taproot (tb1p...)
std::string bech32_encode(const std::string& hrp, int witness_version, const std::vector<uint8_t>& witness_program);

// Generate random secure bytes
std::vector<uint8_t> random_bytes(size_t len);

// Convert hex string to byte vector and vice versa
std::vector<uint8_t> hex_to_bytes(const std::string& hex);
std::string bytes_to_hex(const std::vector<uint8_t>& bytes);

// Keypair & Address Generator for Testnet
struct TestnetWallet {
    std::string private_key_wif;
    std::string private_key_hex;
    std::string public_key_hex;
    std::string legacy_address;  // m... or n... (Testnet P2PKH)
    std::string segwit_address;  // tb1q... (Testnet Native SegWit P2WPKH)
    std::string taproot_address; // tb1p... (Testnet Taproot P2TR)
};

// secp256k1 Elliptic Curve Cryptography (Bitcoin native curve)
std::vector<uint8_t> derive_public_key(const std::vector<uint8_t>& privkey, bool compressed = true);
std::vector<uint8_t> ecdsa_sign(const std::vector<uint8_t>& privkey, const std::vector<uint8_t>& message_hash);
bool ecdsa_verify(const std::vector<uint8_t>& pubkey, const std::vector<uint8_t>& message_hash, const std::vector<uint8_t>& signature_der);

TestnetWallet generate_testnet_wallet();

} // namespace btc::crypto
