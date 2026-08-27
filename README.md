# ₿ Bitcoin Core C++ Testnet Suite & Explorer

[![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Network: Bitcoin Testnet4](https://img.shields.io/badge/Network-Bitcoin_Testnet4-F7931A?style=for-the-badge&logo=bitcoin&logoColor=white)](https://github.com/bitcoin/bitcoin)
[![OpenSSL: 3.0+](https://img.shields.io/badge/OpenSSL-3.0%2B-721412?style=for-the-badge&logo=openssl&logoColor=white)](https://www.openssl.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

> **A high-performance Bitcoin Testnet4 node state engine, interactive block explorer, and Bitcoin Script execution environment written natively in modern C++ (the language of Bitcoin Core).**

---

## 🌟 Key Features

- ⚡ **Native C++17 Engine**: High-performance, multi-threaded C++ state engine simulating Bitcoin Core node behaviors, mempool queue, and block validation.
- 📦 **Live Block Explorer**: Real-time inspection of Testnet4 blocks, Merkle roots, block sizes, double-SHA256 hashes, and nonces.
- 📜 **Bitcoin Script Debugger & Opcode VM**: Step-by-step evaluation of Bitcoin Script opcodes (`OP_DUP`, `OP_HASH160`, `OP_EQUALVERIFY`, `OP_CHECKSIG`, `OP_ADD`, `OP_RETURN`) with live stack visualization.
- 🔑 **Cryptographic Address Generator**: Generates Native SegWit (`tb1q...`, BIP-173), Taproot (`tb1p...`, BIP-350), and Legacy (`m...`/`n...`) Testnet keypairs with WIF private keys.
- 🚰 **Instant Testnet Faucet**: Dispenses testnet satoshis (`0.5 - 2.5 tBTC`) directly to any address with immediate Proof-of-Work block mining.
- 🔬 **Raw Transaction Decoder**: Decodes raw Bitcoin hex transactions into human-readable JSON formats.
- 📊 **C++ SHA-256 Benchmark**: Low-level multi-threaded CPU hashing benchmark measuring raw hashes per second.
- 🌐 **100% SEO Optimized Web UI**: Modern dark theme with Bitcoin orange accents, OpenGraph, Twitter Cards, and Schema.org JSON-LD structured data.

---

## 🏗️ Architecture & C++ Components

```
bitcoin-testnet-cpp/
├── Makefile                       # Optimized C++ build pipeline
├── bin/
│   └── bitcoin-testnet-core       # Compiled native binary
├── include/
│   ├── httplib.h                  # High-performance C++ HTTP server
│   └── json.hpp                   # Modern JSON for C++ (nlohmann)
├── src/
│   ├── crypto.hpp / .cpp          # SHA-256, RIPEMD-160, Base58Check, Bech32/Bech32m
│   ├── script.hpp / .cpp          # Bitcoin Script Opcode execution VM & Stack
│   ├── blockchain.hpp / .cpp      # Blocks, Mempool, UTXO Set & PoW Miner
│   └── server.cpp                 # Multi-threaded REST API & Static File Router
├── public/                        # Responsive Web UI & Explorer Assets
│   ├── index.html                 # Semantic, SEO-optimized HTML5 interface
│   ├── style.css                  # Bitcoin Core Dark & Orange styling
│   ├── app.js                     # Dynamic REST API client & Script visualizer
│   ├── favicon.svg                # Bitcoin vector icon
│   ├── robots.txt                 # Search engine crawling rules
│   └── sitemap.xml                # Sitemap metadata
└── scripts/
    ├── start.sh                   # Server startup script
    └── stop.sh                    # Graceful shutdown script
```

---

## 🚀 Quick Start & Installation

### 1. Prerequisites
- **GCC / Clang** supporting C++17 (`g++ >= 9.0`)
- **OpenSSL Development Headers** (`libssl-dev` or `openssl`)
- **Make**

On Arch Linux:
```bash
sudo pacman -S base-devel openssl
```

### 2. Build from Source
```bash
git clone https://github.com/basantzp/bitcoin-testnet-cpp.git
cd bitcoin-testnet-cpp
make
```

### 3. Run the Server
```bash
# Start locally on default port 8332
./scripts/start.sh

# Or run directly:
./bin/bitcoin-testnet-core 8332
```
Open your browser at **`http://localhost:8332`**.

---

## 📡 REST API Documentation

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/info` | Get node status, block height, difficulty, and compiler info |
| `GET` | `/api/blocks?limit=15` | List latest mined Testnet4 blocks |
| `GET` | `/api/block/:hash_or_height`| Inspect detailed block structure |
| `GET` | `/api/tx/:txid` | Inspect transaction inputs, outputs, and status |
| `GET` | `/api/address/:address` | Get UTXOs and satoshi balance for an address |
| `GET` | `/api/mempool` | List unconfirmed transactions in mempool |
| `POST`| `/api/wallet/generate` | Generate fresh Legacy, SegWit, and Taproot keypairs |
| `POST`| `/api/faucet` | Request testnet satoshis (`{"address": "tb1q...", "amount_sats": 100000000}`) |
| `POST`| `/api/script/eval` | Evaluate Bitcoin Script (`{"script": "OP_1 OP_1 OP_ADD", "stack": []}`) |
| `POST`| `/api/tx/create` | Build and broadcast transaction to mempool |
| `POST`| `/api/tx/decode` | Decode raw transaction hex |
| `POST`| `/api/mine` | Mine a new Proof-of-Work block |
| `GET` | `/api/benchmark` | Run native C++ SHA-256 performance benchmark |

---

## 💖 Support & Crypto Donations

If you find this Bitcoin C++ suite and learning tools useful, donations to support ongoing Web3 open-source development are warmly appreciated!

| Network | Address |
| :--- | :--- |
| **🔹 Ethereum / EVM** | [`0xD8401133F356B4e18f0B087E78C9e7d77c62031b`](https://etherscan.io/address/0xD8401133F356B4e18f0B087E78C9e7d77c62031b) |
| **🟣 Solana** | [`D9RwKA5qQJJf2gNBNUcjaPm3KB8SXyw8U47sGXuSSAk8`](https://solscan.io/account/D9RwKA5qQJJf2gNBNUcjaPm3KB8SXyw8U47sGXuSSAk8) |
| **🟢 Filecoin** | [`f1p3ok7v3tratrzkrioqxt5gtylxjytothvibcwni`](https://filfox.info/en/address/f1p3ok7v3tratrzkrioqxt5gtylxjytothvibcwni) |

---

## 📄 License
Released under the [MIT License](LICENSE).
Built with passion by [Basant Pokhrel](https://github.com/basantzp).
