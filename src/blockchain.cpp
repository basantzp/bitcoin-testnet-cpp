#include "blockchain.hpp"
#include "crypto.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <thread>

namespace btc::chain {

static int64_t current_timestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

BitcoinTestnetNode::BitcoinTestnetNode() 
    : network_name_("testnet4"),
      protocol_version_(70016),
      user_agent_("/Satoshi:28.0.0(Bitcoin Core C++ Testnet Engine)/") {
    start_time_ = current_timestamp();
    init_genesis_block();
}

void BitcoinTestnetNode::init_genesis_block() {
    std::lock_guard<std::mutex> lock(node_mutex_);
    
    // Testnet Genesis Block
    Block genesis;
    genesis.height = 0;
    genesis.prev_block_hash = "0000000000000000000000000000000000000000000000000000000000000000";
    genesis.timestamp = 1716508800; // Testnet4 Genesis timestamp
    genesis.bits = 0x1d00ffff;
    genesis.nonce = 2083236893;
    
    Transaction cb;
    cb.txid = "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b";
    cb.version = 1;
    cb.is_coinbase = true;
    cb.locktime = 0;
    cb.fee_sats = 0;
    cb.timestamp = genesis.timestamp;
    cb.block_height = 0;
    cb.block_hash = "";
    
    TxInput in;
    in.txid = "0000000000000000000000000000000000000000000000000000000000000000";
    in.vout = 0xffffffff;
    in.script_sig = "04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73";
    in.sequence = 0xffffffff;
    in.amount_sats = 0;
    in.address = "Coinbase";
    cb.inputs.push_back(in);
    
    TxOutput out;
    out.n = 0;
    out.value_sats = 5000000000ULL; // 50 tBTC
    out.address = "tb1q908pkk7578p3lkl3d40p78c9e7d77c62031btest";
    out.script_pubkey = "0014" + crypto::hash160_hex({0x02, 0x03, 0x04});
    cb.outputs.push_back(out);
    
    genesis.transactions.push_back(cb);
    genesis.tx_count = 1;
    genesis.merkle_root = cb.txid;
    genesis.size_bytes = 285;
    
    // Hash of genesis header
    std::string header = std::to_string(genesis.height) + genesis.prev_block_hash + genesis.merkle_root + std::to_string(genesis.timestamp);
    genesis.hash = "00000000da84f2bafbbc60da91f48607bc8682617e40ac794e1e0dd731ec17d4";
    
    cb.block_hash = genesis.hash;
    tx_index_[cb.txid] = cb;
    utxo_set_[out.address].push_back(out);
    blockchain_.push_back(genesis);
}

std::string BitcoinTestnetNode::calculate_merkle_root(const std::vector<Transaction>& txs) const {
    if (txs.empty()) return "0000000000000000000000000000000000000000000000000000000000000000";
    std::vector<std::string> tree;
    for (const auto& tx : txs) {
        tree.push_back(tx.txid);
    }
    while (tree.size() > 1) {
        if (tree.size() % 2 != 0) {
            tree.push_back(tree.back());
        }
        std::vector<std::string> next_level;
        for (size_t i = 0; i < tree.size(); i += 2) {
            std::string combined = tree[i] + tree[i + 1];
            next_level.push_back(crypto::hash256_hex(combined));
        }
        tree = next_level;
    }
    return tree[0];
}

nlohmann::json BitcoinTestnetNode::get_node_info() const {
    std::lock_guard<std::mutex> lock(node_mutex_);
    nlohmann::json info;
    info["version"] = 280000;
    info["subversion"] = user_agent_;
    info["protocolversion"] = protocol_version_;
    info["chain"] = network_name_;
    info["blocks"] = blockchain_.size() - 1;
    info["headers"] = blockchain_.size() - 1;
    info["bestblockhash"] = blockchain_.empty() ? "" : blockchain_.back().hash;
    info["difficulty"] = 1.0;
    info["mempool_size"] = mempool_.size();
    info["connections"] = 18; // Simulated active testnet peers
    info["uptime_seconds"] = current_timestamp() - start_time_;
    info["relayfee"] = 0.00001000;
    info["warnings"] = "";
    info["compiler"] = "GCC " __VERSION__ " (C++17 x86_64)";
    return info;
}

std::vector<Block> BitcoinTestnetNode::get_latest_blocks(size_t limit) const {
    std::lock_guard<std::mutex> lock(node_mutex_);
    std::vector<Block> result;
    size_t count = 0;
    for (auto it = blockchain_.rbegin(); it != blockchain_.rend() && count < limit; ++it, ++count) {
        result.push_back(*it);
    }
    return result;
}

nlohmann::json BitcoinTestnetNode::get_block_by_hash(const std::string& hash) const {
    std::lock_guard<std::mutex> lock(node_mutex_);
    for (const auto& b : blockchain_) {
        if (b.hash == hash) {
            nlohmann::json j;
            j["hash"] = b.hash;
            j["height"] = b.height;
            j["prev_block_hash"] = b.prev_block_hash;
            j["merkle_root"] = b.merkle_root;
            j["timestamp"] = b.timestamp;
            j["bits"] = b.bits;
            j["nonce"] = b.nonce;
            j["tx_count"] = b.tx_count;
            j["size_bytes"] = b.size_bytes;
            nlohmann::json txs = nlohmann::json::array();
            for (const auto& tx : b.transactions) {
                txs.push_back(tx.txid);
            }
            j["tx"] = txs;
            return j;
        }
    }
    return nullptr;
}

nlohmann::json BitcoinTestnetNode::get_block_by_height(uint32_t height) const {
    std::lock_guard<std::mutex> lock(node_mutex_);
    if (height < blockchain_.size()) {
        const auto& b = blockchain_[height];
        nlohmann::json j;
        j["hash"] = b.hash;
        j["height"] = b.height;
        j["prev_block_hash"] = b.prev_block_hash;
        j["merkle_root"] = b.merkle_root;
        j["timestamp"] = b.timestamp;
        j["bits"] = b.bits;
        j["nonce"] = b.nonce;
        j["tx_count"] = b.tx_count;
        j["size_bytes"] = b.size_bytes;
        nlohmann::json txs = nlohmann::json::array();
        for (const auto& tx : b.transactions) {
            txs.push_back(tx.txid);
        }
        j["tx"] = txs;
        return j;
    }
    return nullptr;
}

nlohmann::json BitcoinTestnetNode::get_transaction(const std::string& txid) const {
    std::lock_guard<std::mutex> lock(node_mutex_);
    auto it = tx_index_.find(txid);
    if (it != tx_index_.end()) {
        const auto& tx = it->second;
        nlohmann::json j;
        j["txid"] = tx.txid;
        j["version"] = tx.version;
        j["locktime"] = tx.locktime;
        j["timestamp"] = tx.timestamp;
        j["fee_sats"] = tx.fee_sats;
        j["is_coinbase"] = tx.is_coinbase;
        j["block_height"] = tx.block_height;
        j["block_hash"] = tx.block_hash;
        
        nlohmann::json vin = nlohmann::json::array();
        for (const auto& in : tx.inputs) {
            nlohmann::json vi;
            vi["txid"] = in.txid;
            vi["vout"] = in.vout;
            vi["scriptSig"] = in.script_sig;
            vi["sequence"] = in.sequence;
            vi["address"] = in.address;
            vi["amount_sats"] = in.amount_sats;
            vin.push_back(vi);
        }
        j["vin"] = vin;
        
        nlohmann::json vout = nlohmann::json::array();
        for (const auto& out : tx.outputs) {
            nlohmann::json vo;
            vo["n"] = out.n;
            vo["value_sats"] = out.value_sats;
            vo["value_btc"] = (double)out.value_sats / 100000000.0;
            vo["scriptPubKey"] = out.script_pubkey;
            vo["address"] = out.address;
            vout.push_back(vo);
        }
        j["vout"] = vout;
        return j;
    }
    return nullptr;
}

AddressInfo BitcoinTestnetNode::get_address_info(const std::string& address) const {
    std::lock_guard<std::mutex> lock(node_mutex_);
    AddressInfo info;
    info.address = address;
    info.balance_sats = 0;
    info.tx_count = 0;
    
    auto it = utxo_set_.find(address);
    if (it != utxo_set_.end()) {
        info.utxos = it->second;
        for (const auto& u : info.utxos) {
            info.balance_sats += u.value_sats;
        }
    }
    info.balance_btc = (double)info.balance_sats / 100000000.0;
    
    // Find all txids involving this address
    for (const auto& pair : tx_index_) {
        const auto& tx = pair.second;
        bool found = false;
        for (const auto& in : tx.inputs) {
            if (in.address == address) { found = true; break; }
        }
        if (!found) {
            for (const auto& out : tx.outputs) {
                if (out.address == address) { found = true; break; }
            }
        }
        if (found) {
            info.txids.push_back(tx.txid);
        }
    }
    info.tx_count = info.txids.size();
    return info;
}

std::vector<Transaction> BitcoinTestnetNode::get_mempool() const {
    std::lock_guard<std::mutex> lock(node_mutex_);
    return mempool_;
}

Block BitcoinTestnetNode::mine_block(const std::string& miner_addr) {
    std::lock_guard<std::mutex> lock(node_mutex_);
    
    std::string miner = miner_addr.empty() ? "tb1q908pkk7578p3lkl3d40p78c9e7d77c62031btest" : miner_addr;
    
    Block new_block;
    new_block.height = blockchain_.size();
    new_block.prev_block_hash = blockchain_.empty() ? "0000000000000000000000000000000000000000000000000000000000000000" : blockchain_.back().hash;
    new_block.timestamp = current_timestamp();
    new_block.bits = 0x1d00ffff;
    
    // Create Coinbase Tx (50 tBTC block reward + fees)
    uint64_t total_fees = 0;
    for (const auto& tx : mempool_) {
        total_fees += tx.fee_sats;
    }
    
    Transaction cb;
    cb.version = 2;
    cb.is_coinbase = true;
    cb.locktime = 0;
    cb.fee_sats = 0;
    cb.timestamp = new_block.timestamp;
    cb.block_height = new_block.height;
    
    TxInput cb_in;
    cb_in.txid = "0000000000000000000000000000000000000000000000000000000000000000";
    cb_in.vout = 0xffffffff;
    cb_in.script_sig = "03" + std::to_string(new_block.height) + "2f5361746f7368693a432b2b2d436f72652f";
    cb_in.sequence = 0xffffffff;
    cb_in.amount_sats = 0;
    cb_in.address = "Coinbase";
    cb.inputs.push_back(cb_in);
    
    TxOutput cb_out;
    cb_out.n = 0;
    cb_out.value_sats = 5000000000ULL + total_fees;
    cb_out.address = miner;
    cb_out.script_pubkey = "0014" + crypto::hash160_hex({0x02, 0x05, (uint8_t)(new_block.height % 255)});
    cb.outputs.push_back(cb_out);
    
    cb.txid = crypto::hash256_hex("coinbase_" + std::to_string(new_block.height) + "_" + std::to_string(new_block.timestamp));
    new_block.transactions.push_back(cb);
    
    // Add mempool transactions
    for (auto& tx : mempool_) {
        tx.block_height = new_block.height;
        new_block.transactions.push_back(tx);
    }
    mempool_.clear();
    
    new_block.tx_count = new_block.transactions.size();
    new_block.merkle_root = calculate_merkle_root(new_block.transactions);
    new_block.size_bytes = 200 + new_block.tx_count * 250;
    
    // Proof of Work simulation (find hash with "0000")
    uint32_t nonce = 0;
    std::string b_hash;
    while (true) {
        std::string header = std::to_string(new_block.height) + new_block.prev_block_hash + new_block.merkle_root + std::to_string(new_block.timestamp) + std::to_string(nonce);
        b_hash = crypto::hash256_hex(header);
        if (b_hash.rfind("0000", 0) == 0 || nonce > 100000) {
            break;
        }
        nonce++;
    }
    
    new_block.nonce = nonce;
    new_block.hash = b_hash;
    
    // Update block hash in transactions and index
    for (auto& tx : new_block.transactions) {
        tx.block_hash = new_block.hash;
        tx_index_[tx.txid] = tx;
        for (const auto& out : tx.outputs) {
            utxo_set_[out.address].push_back(out);
        }
    }
    
    blockchain_.push_back(new_block);
    return new_block;
}

Transaction BitcoinTestnetNode::request_faucet(const std::string& recipient, uint64_t amount_sats) {
    std::string from_faucet = "tb1q_faucet_hot_wallet_cpp_core_testnet";
    
    Transaction tx;
    tx.version = 2;
    tx.is_coinbase = false;
    tx.locktime = 0;
    tx.fee_sats = 1000;
    tx.timestamp = current_timestamp();
    tx.block_height = 0;
    
    TxInput in;
    in.txid = crypto::hash256_hex("faucet_utxo_" + std::to_string(tx.timestamp));
    in.vout = 0;
    in.script_sig = "483045022100...faucet_sig";
    in.sequence = 0xffffffff;
    in.address = from_faucet;
    in.amount_sats = amount_sats + tx.fee_sats;
    tx.inputs.push_back(in);
    
    TxOutput out;
    out.n = 0;
    out.value_sats = amount_sats;
    out.address = recipient;
    out.script_pubkey = "0014" + crypto::hash160_hex(crypto::hex_to_bytes(crypto::sha256_hex(recipient)));
    tx.outputs.push_back(out);
    
    tx.txid = crypto::hash256_hex(recipient + std::to_string(amount_sats) + std::to_string(tx.timestamp));
    
    {
        std::lock_guard<std::mutex> lock(node_mutex_);
        mempool_.push_back(tx);
    }
    
    // Auto mine block to make it confirmed immediately
    mine_block("tb1q_faucet_miner_cpp_core");
    
    return tx;
}

Transaction BitcoinTestnetNode::create_and_broadcast_tx(const std::string& from_addr, const std::string& to_addr, uint64_t amount_sats, uint64_t fee_sats) {
    std::lock_guard<std::mutex> lock(node_mutex_);
    
    auto it = utxo_set_.find(from_addr);
    if (it == utxo_set_.end() || it->second.empty()) {
        throw std::runtime_error("Insufficient funds or no UTXOs available for address: " + from_addr);
    }
    
    uint64_t total_available = 0;
    for (const auto& u : it->second) {
        total_available += u.value_sats;
    }
    
    if (total_available < (amount_sats + fee_sats)) {
        throw std::runtime_error("Insufficient balance. Have: " + std::to_string(total_available) + " sats, Need: " + std::to_string(amount_sats + fee_sats) + " sats");
    }
    
    // Consume UTXOs
    it->second.clear();
    uint64_t change_sats = total_available - (amount_sats + fee_sats);
    
    Transaction tx;
    tx.version = 2;
    tx.is_coinbase = false;
    tx.locktime = 0;
    tx.fee_sats = fee_sats;
    tx.timestamp = current_timestamp();
    tx.block_height = 0;
    
    TxInput in;
    in.txid = crypto::hash256_hex("spend_" + from_addr + "_" + std::to_string(tx.timestamp));
    in.vout = 0;
    in.script_sig = "483045022100...user_sig";
    in.sequence = 0xffffffff;
    in.address = from_addr;
    in.amount_sats = total_available;
    tx.inputs.push_back(in);
    
    TxOutput out1;
    out1.n = 0;
    out1.value_sats = amount_sats;
    out1.address = to_addr;
    out1.script_pubkey = "0014" + crypto::hash160_hex(crypto::hex_to_bytes(crypto::sha256_hex(to_addr)));
    tx.outputs.push_back(out1);
    
    if (change_sats > 0) {
        TxOutput change_out;
        change_out.n = 1;
        change_out.value_sats = change_sats;
        change_out.address = from_addr;
        change_out.script_pubkey = "0014" + crypto::hash160_hex(crypto::hex_to_bytes(crypto::sha256_hex(from_addr)));
        tx.outputs.push_back(change_out);
    }
    
    tx.txid = crypto::hash256_hex(from_addr + to_addr + std::to_string(amount_sats) + std::to_string(tx.timestamp));
    mempool_.push_back(tx);
    return tx;
}

nlohmann::json BitcoinTestnetNode::decode_raw_transaction(const std::string& hex_data) const {
    nlohmann::json j;
    j["raw_hex"] = hex_data;
    j["size_bytes"] = hex_data.length() / 2;
    j["txid"] = crypto::hash256_hex(hex_data);
    j["hash"] = j["txid"];
    j["version"] = 2;
    j["locktime"] = 0;
    
    nlohmann::json vin = nlohmann::json::array();
    nlohmann::json vout = nlohmann::json::array();
    
    // Sample decoded format
    nlohmann::json in1;
    in1["txid"] = "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b";
    in1["vout"] = 0;
    in1["scriptSig"]["asm"] = "3045022100... 02...";
    in1["scriptSig"]["hex"] = hex_data.substr(0, std::min((size_t)64, hex_data.length()));
    in1["sequence"] = 4294967295;
    vin.push_back(in1);
    
    nlohmann::json out1;
    out1["value"] = 0.50000000;
    out1["n"] = 0;
    out1["scriptPubKey"]["asm"] = "OP_DUP OP_HASH160 0x... OP_EQUALVERIFY OP_CHECKSIG";
    out1["scriptPubKey"]["type"] = "witness_v0_keyhash";
    out1["scriptPubKey"]["address"] = "tb1q908pkk7578p3lkl3d40p78c9e7d77c62031btest";
    vout.push_back(out1);
    
    j["vin"] = vin;
    j["vout"] = vout;
    return j;
}

nlohmann::json BitcoinTestnetNode::run_benchmark(size_t iterations) const {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::string test_data = "Bitcoin Core C++ SHA-256 Mining Benchmark Data String 1234567890";
    std::string current_hash = test_data;
    
    for (size_t i = 0; i < iterations; ++i) {
        current_hash = crypto::sha256_hex(current_hash);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    double hashes_per_sec = (double)iterations / (duration_ms / 1000.0);
    
    nlohmann::json j;
    j["engine"] = "C++17 Native OpenSSL Pipeline";
    j["iterations"] = iterations;
    j["duration_ms"] = duration_ms;
    j["hashes_per_second"] = hashes_per_sec;
    j["final_hash"] = current_hash;
    j["threads_utilized"] = std::thread::hardware_concurrency();
    return j;
}

} // namespace btc::chain
