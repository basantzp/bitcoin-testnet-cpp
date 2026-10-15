#include "crypto.hpp"
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <array>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace btc::crypto {

static const char* BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const char* BECH32_CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

std::vector<uint8_t> sha256_bytes(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(input.data(), input.size(), hash.data());
    return hash;
}

std::string sha256_hex(const std::string& input) {
    std::vector<uint8_t> data(input.begin(), input.end());
    auto hash = sha256_bytes(data);
    return bytes_to_hex(hash);
}

std::string ripemd160_hex(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> hash(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160(input.data(), input.size(), hash.data());
    return bytes_to_hex(hash);
}

std::vector<uint8_t> hash256_bytes(const std::vector<uint8_t>& input) {
    return sha256_bytes(sha256_bytes(input));
}

std::string hash256_hex(const std::string& input) {
    std::vector<uint8_t> data(input.begin(), input.end());
    auto h2 = hash256_bytes(data);
    return bytes_to_hex(h2);
}

std::string hash160_hex(const std::vector<uint8_t>& input) {
    auto sha = sha256_bytes(input);
    std::vector<uint8_t> rmd(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160(sha.data(), sha.size(), rmd.data());
    return bytes_to_hex(rmd);
}

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    std::string clean = hex;
    if (clean.rfind("0x", 0) == 0 || clean.rfind("0X", 0) == 0) {
        clean = clean.substr(2);
    }
    if (clean.empty() || !std::all_of(clean.begin(), clean.end(), ::isxdigit)) {
        return {};
    }
    if (clean.length() % 2 != 0) {
        clean = "0" + clean;
    }
    for (size_t i = 0; i < clean.length(); i += 2) {
        std::string byteString = clean.substr(i, 2);
        uint8_t byte = (uint8_t)strtoul(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    for (uint8_t b : bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    return oss.str();
}

std::vector<uint8_t> random_bytes(size_t len) {
    std::vector<uint8_t> buf(len);
    if (RAND_bytes(buf.data(), len) != 1) {
        // Fallback to random device if OpenSSL RAND fails
        for (size_t i = 0; i < len; i++) {
            buf[i] = rand() % 256;
        }
    }
    return buf;
}

std::string base58_encode(const std::vector<uint8_t>& data) {
    if (data.empty()) return "";
    
    // Count leading zeros
    size_t zeros = 0;
    while (zeros < data.size() && data[zeros] == 0) {
        zeros++;
    }
    
    // Allocate space in big-endian base58 representation
    std::vector<uint8_t> b58((data.size() - zeros) * 138 / 100 + 1);
    size_t length = 0;
    
    for (size_t i = zeros; i < data.size(); i++) {
        int carry = data[i];
        size_t j = 0;
        for (size_t it = 0; it < b58.size(); it++) {
            if (carry == 0 && it >= length) break;
            carry += 256 * b58[it];
            b58[it] = carry % 58;
            carry /= 58;
            j++;
        }
        length = std::max(length, j);
    }
    
    std::string result;
    result.assign(zeros, '1');
    while (length > 0) {
        result += BASE58_ALPHABET[b58[--length]];
    }
    return result;
}

std::string base58check_encode(uint8_t version, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> vdata;
    vdata.push_back(version);
    vdata.insert(vdata.end(), payload.begin(), payload.end());
    
    auto checksum = hash256_bytes(vdata);
    vdata.insert(vdata.end(), checksum.begin(), checksum.begin() + 4);
    
    return base58_encode(vdata);
}

// Bech32 helper functions (BIP-173 / BIP-350)
static uint32_t bech32_polymod(const std::vector<uint8_t>& values) {
    uint32_t chk = 1;
    for (uint8_t v : values) {
        uint8_t top = chk >> 25;
        chk = ((chk & 0x1ffffff) << 5) ^ v;
        if (top & 1) chk ^= 0x3b6a57b2;
        if (top & 2) chk ^= 0x26508e6d;
        if (top & 4) chk ^= 0x1ea119fa;
        if (top & 8) chk ^= 0x3d4233dd;
        if (top & 16) chk ^= 0x2a1462b3;
    }
    return chk;
}

static std::vector<uint8_t> bech32_hrp_expand(const std::string& hrp) {
    std::vector<uint8_t> ret;
    for (char c : hrp) ret.push_back(c >> 5);
    ret.push_back(0);
    for (char c : hrp) ret.push_back(c & 31);
    return ret;
}

static bool convert_bits(const std::vector<uint8_t>& in, int from_bits, int to_bits, bool pad, std::vector<uint8_t>& out) {
    int acc = 0;
    int bits = 0;
    const int maxv = (1 << to_bits) - 1;
    for (uint8_t val : in) {
        acc = (acc << from_bits) | val;
        bits += from_bits;
        while (bits >= to_bits) {
            bits -= to_bits;
            out.push_back((acc >> bits) & maxv);
        }
    }
    if (pad) {
        if (bits > 0) {
            out.push_back((acc << (to_bits - bits)) & maxv);
        }
    } else if (bits >= from_bits || ((acc << (to_bits - bits)) & maxv)) {
        return false;
    }
    return true;
}

std::string bech32_encode(const std::string& hrp, int witness_version, const std::vector<uint8_t>& witness_program) {
    std::vector<uint8_t> data;
    data.push_back(witness_version);
    
    std::vector<uint8_t> converted;
    convert_bits(witness_program, 8, 5, true, converted);
    data.insert(data.end(), converted.begin(), converted.end());
    
    // Constant: 1 for Bech32 (SegWit v0), 0x2bc830a3 for Bech32m (Taproot v1)
    uint32_t const_val = (witness_version == 0) ? 1 : 0x2bc830a3;
    
    std::vector<uint8_t> exp = bech32_hrp_expand(hrp);
    exp.insert(exp.end(), data.begin(), data.end());
    exp.resize(exp.size() + 6, 0);
    
    uint32_t mod = bech32_polymod(exp) ^ const_val;
    for (int i = 0; i < 6; i++) {
        data.push_back((mod >> (5 * (5 - i))) & 31);
    }
    
    std::string ret = hrp + "1";
    for (uint8_t d : data) {
        ret += BECH32_CHARSET[d];
    }
    return ret;
}

std::vector<uint8_t> derive_public_key(const std::vector<uint8_t>& privkey, bool compressed) {
    if (privkey.size() != 32) return {};
    
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key) return {};
    
    BIGNUM* bn = BN_bin2bn(privkey.data(), privkey.size(), NULL);
    if (!bn) {
        EC_KEY_free(key);
        return {};
    }
    
    if (!EC_KEY_set_private_key(key, bn)) {
        BN_free(bn);
        EC_KEY_free(key);
        return {};
    }
    
    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* pub_point = EC_POINT_new(group);
    if (!pub_point) {
        BN_free(bn);
        EC_KEY_free(key);
        return {};
    }
    
    if (!EC_POINT_mul(group, pub_point, bn, NULL, NULL, NULL)) {
        EC_POINT_free(pub_point);
        BN_free(bn);
        EC_KEY_free(key);
        return {};
    }
    
    EC_KEY_set_public_key(key, pub_point);
    
    point_conversion_form_t form = compressed ? POINT_CONVERSION_COMPRESSED : POINT_CONVERSION_UNCOMPRESSED;
    size_t pub_len = EC_POINT_point2oct(group, pub_point, form, NULL, 0, NULL);
    std::vector<uint8_t> pub(pub_len);
    EC_POINT_point2oct(group, pub_point, form, pub.data(), pub.size(), NULL);
    
    EC_POINT_free(pub_point);
    BN_free(bn);
    EC_KEY_free(key);
    return pub;
}

std::vector<uint8_t> ecdsa_sign(const std::vector<uint8_t>& privkey, const std::vector<uint8_t>& message_hash) {
    if (privkey.size() != 32 || message_hash.empty()) return {};
    
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key) return {};
    
    BIGNUM* bn = BN_bin2bn(privkey.data(), privkey.size(), NULL);
    if (!bn) {
        EC_KEY_free(key);
        return {};
    }
    EC_KEY_set_private_key(key, bn);
    
    unsigned int sig_len = ECDSA_size(key);
    std::vector<uint8_t> sig(sig_len);
    if (!ECDSA_sign(0, message_hash.data(), message_hash.size(), sig.data(), &sig_len, key)) {
        BN_free(bn);
        EC_KEY_free(key);
        return {};
    }
    sig.resize(sig_len);
    
    BN_free(bn);
    EC_KEY_free(key);
    return sig;
}

bool ecdsa_verify(const std::vector<uint8_t>& pubkey, const std::vector<uint8_t>& message_hash, const std::vector<uint8_t>& signature_der) {
    if (pubkey.empty() || message_hash.empty() || signature_der.empty()) return false;
    
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key) return false;
    
    const unsigned char* p = pubkey.data();
    if (!o2i_ECPublicKey(&key, &p, pubkey.size())) {
        EC_KEY_free(key);
        return false;
    }
    
    int ok = ECDSA_verify(0, message_hash.data(), message_hash.size(), signature_der.data(), signature_der.size(), key);
    EC_KEY_free(key);
    return (ok == 1);
}

TestnetWallet generate_testnet_wallet() {
    TestnetWallet wallet;
    
    // 1. Generate 32-byte private key
    auto priv_bytes = random_bytes(32);
    wallet.private_key_hex = bytes_to_hex(priv_bytes);
    
    // Testnet WIF prefix is 0xef (239). Add 0x01 for compressed public key.
    std::vector<uint8_t> wif_payload = priv_bytes;
    wif_payload.push_back(0x01);
    wallet.private_key_wif = base58check_encode(0xef, wif_payload);
    
    // 2. Generate Real secp256k1 Compressed Public Key (33 bytes) via EC scalar multiplication Q = d * G
    auto pub_key = derive_public_key(priv_bytes, true);
    if (pub_key.empty()) {
        throw std::runtime_error("Failed to derive secp256k1 public key");
    }
    wallet.public_key_hex = bytes_to_hex(pub_key);
    
    // 3. Compute Hash160 (RIPEMD160(SHA256(pubkey)))
    auto sha_pub = sha256_bytes(pub_key);
    std::vector<uint8_t> hash160_bytes(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160(sha_pub.data(), sha_pub.size(), hash160_bytes.data());
    
    // 4. Testnet Legacy P2PKH Address (prefix 0x6f -> m... or n...)
    wallet.legacy_address = base58check_encode(0x6f, hash160_bytes);
    
    // 5. Testnet Native SegWit Address (hrp "tb", witness version 0 -> tb1q...)
    wallet.segwit_address = bech32_encode("tb", 0, hash160_bytes);
    
    // 6. Testnet Taproot Address (hrp "tb", witness version 1, 32-byte X-only pubkey -> tb1p...)
    // In BIP-340/341, the Taproot internal key is the 32-byte X coordinate (skipping the 0x02/0x03 prefix byte)
    std::vector<uint8_t> x_only_pub(pub_key.begin() + 1, pub_key.end());
    wallet.taproot_address = bech32_encode("tb", 1, x_only_pub);
    
    return wallet;
}

} // namespace btc::crypto

#pragma GCC diagnostic pop
