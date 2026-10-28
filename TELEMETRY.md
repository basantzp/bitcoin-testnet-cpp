# ⚡ Bitcoin Core Testnet4 C++ Engine & Script VM Telemetry

> **Repository**: [`basantzp/bitcoin-testnet-cpp`](https://github.com/basantzp/bitcoin-testnet-cpp)  
> **Compiler / Toolchain**: `g++ (C++17)`, OpenSSL 3.0  
> **Last Verification Run**: `2026-10-28 22:42:13 +0545`  
> **Test Status**: 🟢 **10/10 Test Suites Passing (100% Coverage)**  
> **Author**: `basantzp <basantpok90@gmail.com>`

---

## 🧪 Verified Bitcoin Script VM & Cryptographic Primitives
- **secp256k1 Point Multiplication**: Authentic scalar multiplication producing 33-byte compressed public keys.
- **ECDSA Sign / Verify**: DER-encoded signature generation & cryptographic verification against 32-byte message hashes.
- **BIP-147 NULLDUMMY**: Strict NULLDUMMY dummy element validation in `OP_CHECKMULTISIG`.
- **Conditional Flow Control**: Nested `OP_IF`/`OP_ELSE` branches with duplicate `OP_ELSE` prevention.
- **P2PKH & Lightning HTLC**: Evaluated standard Pay-to-PubKey-Hash and hash time-locked contracts.

### Verification Summary
```text
[TEST 1/10] Running Crypto Primitives Verification...
  ✓ Crypto Primitives passed (SHA-256, Hash256, RIPEMD160, secp256k1 EC point mul, ECDSA sign/verify, Base58Check, Bech32)
[TEST 2/10] Running Bitcoin Script Stack Operations...
  ✓ Stack operations passed (OP_DUP, OP_2DUP, OP_DROP, OP_2DROP, OP_SWAP, OP_2SWAP, OP_OVER, OP_2OVER, OP_ROT, OP_NIP, OP_TUCK, OP_DEPTH, OP_SIZE byte accuracy, OP_PICK, OP_ROLL)
[TEST 3/10] Running Bitcoin Script Arithmetic & Logic...
  ✓ Arithmetic & Logic passed (OP_ADD, OP_SUB, OP_1ADD, OP_1SUB, OP_NEGATE, OP_ABS, OP_NOT, OP_0NOTEQUAL, OP_MIN, OP_MAX, OP_WITHIN, OP_BOOLAND, OP_BOOLOR)
[TEST 4/10] Running Bitcoin Script Conditional Flow Control...
  ✓ Conditional Flow Control passed (OP_IF, OP_NOTIF, OP_ELSE, OP_ENDIF, nested branches, duplicate else prevention)
[TEST 5/10] Running Standard P2PKH Script Execution...
  ✓ P2PKH script execution passed (Simulated & Real Cryptographic secp256k1 ECDSA)
[TEST 6/10] Running Bitcoin Lightning HTLC Verification...
  ✓ Lightning HTLC script passed (Preimage Claim Path & Timelock Refund Path)
[TEST 7/10] Running Bitcoin 2-of-3 Multisig & BIP-147 NULLDUMMY...
  ✓ 2-of-3 Multisig execution passed (OP_CHECKMULTISIG -> 1, BIP-147 NULLDUMMY enforced)
[TEST 8/10] Running OP_CHECKSIGVERIFY, OP_CHECKMULTISIGVERIFY, and OP_VERIFY...
  ✓ Verify opcodes passed (OP_VERIFY, OP_CHECKSIGVERIFY, OP_CHECKMULTISIGVERIFY)
[TEST 9/10] Running Edge Cases & Robustness (Deep Nesting, OP_RETURN, Underflow)...
  ✓ Edge cases and robustness passed (OP_RETURN, underflow checks, 50-level nested conditionals)
[TEST 10/10] Running Bitcoin Testnet4 Node & Block Engine...
  ✓ Bitcoin Node Engine passed (Genesis, Faucet, UTXO tracking, Mempool, Block Mining, PoW, Benchmark)
```
