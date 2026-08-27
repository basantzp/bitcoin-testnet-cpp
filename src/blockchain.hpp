#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include <cstdint>
#include "json.hpp"

namespace btc::chain {

struct TxInput {
    std::string txid;
    uint32_t vout;
    std::string script_sig;
    uint32_t sequence;
    std::string address;
    uint64_t amount_sats;
};

struct TxOutput {
    uint32_t n;
    uint64_t value_sats;
    std::string script_pubkey;
    std::string address;
};

struct Transaction {
    std::string txid;
    int32_t version;
    std::vector<TxInput> inputs;
    std::vector<TxOutput> outputs;
    uint32_t locktime;
    uint64_t fee_sats;
    int64_t timestamp;
    bool is_coinbase;
    uint32_t block_height;
    std::string block_hash;
};

struct Block {
    uint32_t height;
    std::string hash;
    std::string prev_block_hash;
    std::string merkle_root;
    int64_t timestamp;
    uint32_t bits;       // Difficulty target
    uint32_t nonce;
    uint32_t tx_count;
    uint64_t size_bytes;
    std::vector<Transaction> transactions;
};

struct AddressInfo {
    std::string address;
    uint64_t balance_sats;
    double balance_btc;
    uint32_t tx_count;
    std::vector<TxOutput> utxos;
    std::vector<std::string> txids;
};

class BitcoinTestnetNode {
public:
    BitcoinTestnetNode();

    // Node Information
    nlohmann::json get_node_info() const;

    // Blockchain queries
    std::vector<Block> get_latest_blocks(size_t limit = 15) const;
    nlohmann::json get_block_by_hash(const std::string& hash) const;
    nlohmann::json get_block_by_height(uint32_t height) const;
    nlohmann::json get_transaction(const std::string& txid) const;
    AddressInfo get_address_info(const std::string& address) const;

    // Mempool
    std::vector<Transaction> get_mempool() const;

    // Mining
    Block mine_block(const std::string& miner_address = "");

    // Faucet
    Transaction request_faucet(const std::string& recipient_address, uint64_t amount_sats = 100000000); // 1 tBTC default

    // Transaction Creation & Broadcast
    Transaction create_and_broadcast_tx(const std::string& from_addr, const std::string& to_addr, uint64_t amount_sats, uint64_t fee_sats = 1000);

    // Raw Transaction Decoder
    nlohmann::json decode_raw_transaction(const std::string& hex_data) const;

    // Hashing Benchmark
    nlohmann::json run_benchmark(size_t iterations = 500000) const;

private:
    mutable std::mutex node_mutex_;
    std::string network_name_;
    uint32_t protocol_version_;
    std::string user_agent_;
    int64_t start_time_;
    
    std::vector<Block> blockchain_;
    std::vector<Transaction> mempool_;
    std::map<std::string, Transaction> tx_index_;
    std::map<std::string, std::vector<TxOutput>> utxo_set_;

    void init_genesis_block();
    std::string calculate_merkle_root(const std::vector<Transaction>& txs) const;
};

} // namespace btc::chain
