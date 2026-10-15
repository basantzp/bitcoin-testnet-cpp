#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include "httplib.h"
#include "json.hpp"
#include "crypto.hpp"
#include "script.hpp"
#include "blockchain.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    int port = 8332; // Standard Bitcoin RPC / Testnet Core default port

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Bitcoin Core Testnet4 Suite & Script VM Engine\n"
                      << "Usage: bitcoin-testnet-core [PORT]\n"
                      << "       bitcoin-testnet-core -p <PORT>\n\n"
                      << "Options:\n"
                      << "  -h, --help        Display this help message and exit\n"
                      << "  -p, --port <PORT> Port to bind HTTP / RPC server (default: 8332 or $PORT)\n";
            return 0;
        } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            try {
                port = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "Error: Invalid port number: " << argv[i] << std::endl;
                return 1;
            }
        } else {
            try {
                port = std::stoi(arg);
            } catch (...) {
                std::cerr << "Error: Invalid port number or argument: " << arg << "\n"
                          << "Run with --help for usage instructions." << std::endl;
                return 1;
            }
        }
    }

    if (const char* env_port = std::getenv("PORT")) {
        try {
            port = std::stoi(env_port);
        } catch (...) {}
    }

    auto node = std::make_shared<btc::chain::BitcoinTestnetNode>();
    httplib::Server svr;

    // Static files mount point
    svr.set_mount_point("/", "./public");

    // Enable CORS for all API routes
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With"}
    });

    svr.Options(R"(/.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
    });

    // 1. Node Info API
    svr.Get("/api/info", [node](const httplib::Request&, httplib::Response& res) {
        res.set_content(node->get_node_info().dump(2), "application/json");
    });

    // 2. Latest Blocks API
    svr.Get("/api/blocks", [node](const httplib::Request& req, httplib::Response& res) {
        size_t limit = 15;
        if (req.has_param("limit")) {
            limit = std::stoul(req.get_param_value("limit"));
        }
        auto blocks = node->get_latest_blocks(limit);
        json j = json::array();
        for (const auto& b : blocks) {
            json item;
            item["height"] = b.height;
            item["hash"] = b.hash;
            item["prev_block_hash"] = b.prev_block_hash;
            item["merkle_root"] = b.merkle_root;
            item["timestamp"] = b.timestamp;
            item["tx_count"] = b.tx_count;
            item["size_bytes"] = b.size_bytes;
            item["nonce"] = b.nonce;
            j.push_back(item);
        }
        res.set_content(j.dump(2), "application/json");
    });

    // 3. Block by Hash or Height
    svr.Get(R"(/api/block/(.+))", [node](const httplib::Request& req, httplib::Response& res) {
        std::string param = req.matches[1];
        json b = nullptr;
        if (std::all_of(param.begin(), param.end(), ::isdigit)) {
            b = node->get_block_by_height(std::stoul(param));
        } else {
            b = node->get_block_by_hash(param);
        }
        if (b.is_null()) {
            res.status = 404;
            json err; err["error"] = "Block not found";
            res.set_content(err.dump(2), "application/json");
        } else {
            res.set_content(b.dump(2), "application/json");
        }
    });

    // 4. Transaction Inspection API
    svr.Get(R"(/api/tx/(.+))", [node](const httplib::Request& req, httplib::Response& res) {
        std::string txid = req.matches[1];
        auto tx = node->get_transaction(txid);
        if (tx.is_null()) {
            res.status = 404;
            json err; err["error"] = "Transaction not found";
            res.set_content(err.dump(2), "application/json");
        } else {
            res.set_content(tx.dump(2), "application/json");
        }
    });

    // 5. Address Info API
    svr.Get(R"(/api/address/(.+))", [node](const httplib::Request& req, httplib::Response& res) {
        std::string address = req.matches[1];
        auto info = node->get_address_info(address);
        json j;
        j["address"] = info.address;
        j["balance_sats"] = info.balance_sats;
        j["balance_btc"] = info.balance_btc;
        j["tx_count"] = info.tx_count;
        j["txids"] = info.txids;
        json utxos = json::array();
        for (const auto& u : info.utxos) {
            json uj;
            uj["n"] = u.n;
            uj["value_sats"] = u.value_sats;
            uj["script_pubkey"] = u.script_pubkey;
            utxos.push_back(uj);
        }
        j["utxos"] = utxos;
        res.set_content(j.dump(2), "application/json");
    });

    // 6. Mempool API
    svr.Get("/api/mempool", [node](const httplib::Request&, httplib::Response& res) {
        auto mempool = node->get_mempool();
        json j = json::array();
        for (const auto& tx : mempool) {
            json item;
            item["txid"] = tx.txid;
            item["fee_sats"] = tx.fee_sats;
            item["timestamp"] = tx.timestamp;
            item["inputs_count"] = tx.inputs.size();
            item["outputs_count"] = tx.outputs.size();
            j.push_back(item);
        }
        res.set_content(j.dump(2), "application/json");
    });

    // 7. Wallet Generation API (Legacy, SegWit tb1q, Taproot tb1p)
    svr.Post("/api/wallet/generate", [](const httplib::Request&, httplib::Response& res) {
        auto w = btc::crypto::generate_testnet_wallet();
        json j;
        j["private_key_wif"] = w.private_key_wif;
        j["private_key_hex"] = w.private_key_hex;
        j["public_key_hex"] = w.public_key_hex;
        j["legacy_address"] = w.legacy_address;
        j["segwit_address"] = w.segwit_address;
        j["taproot_address"] = w.taproot_address;
        j["network"] = "testnet4";
        res.set_content(j.dump(2), "application/json");
    });

    // 8. Testnet Faucet API
    svr.Post("/api/faucet", [node](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string address = body.value("address", "");
            uint64_t amount_sats = body.value("amount_sats", 100000000ULL); // 1.0 tBTC default
            
            if (address.empty()) {
                res.status = 400;
                json err; err["error"] = "Address parameter is required";
                res.set_content(err.dump(2), "application/json");
                return;
            }
            
            auto tx = node->request_faucet(address, amount_sats);
            json j;
            j["success"] = true;
            j["txid"] = tx.txid;
            j["recipient"] = address;
            j["amount_sats"] = amount_sats;
            j["amount_btc"] = (double)amount_sats / 100000000.0;
            j["message"] = "Testnet satoshis successfully dispensed and mined into next block!";
            res.set_content(j.dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            json err; err["error"] = e.what();
            res.set_content(err.dump(2), "application/json");
        }
    });

    // 9. Transaction Creation & Broadcast API
    svr.Post("/api/tx/create", [node](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string from = body.value("from", "");
            std::string to = body.value("to", "");
            uint64_t amount_sats = body.value("amount_sats", 0ULL);
            uint64_t fee_sats = body.value("fee_sats", 1000ULL);

            if (from.empty() || to.empty() || amount_sats == 0) {
                res.status = 400;
                json err; err["error"] = "Invalid transaction parameters ('from', 'to', 'amount_sats')";
                res.set_content(err.dump(2), "application/json");
                return;
            }

            auto tx = node->create_and_broadcast_tx(from, to, amount_sats, fee_sats);
            json j;
            j["success"] = true;
            j["txid"] = tx.txid;
            j["fee_sats"] = tx.fee_sats;
            j["status"] = "broadcasted_to_mempool";
            res.set_content(j.dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json err; err["error"] = e.what();
            res.set_content(err.dump(2), "application/json");
        }
    });

    // 10. Raw Transaction Hex Decoder API
    svr.Post("/api/tx/decode", [node](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string hex_str = body.value("hex", "");
            if (hex_str.empty()) {
                res.status = 400;
                json err; err["error"] = "Hex string is required";
                res.set_content(err.dump(2), "application/json");
                return;
            }
            auto decoded = node->decode_raw_transaction(hex_str);
            res.set_content(decoded.dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json err; err["error"] = e.what();
            res.set_content(err.dump(2), "application/json");
        }
    });

    // 11. Bitcoin Script Evaluator & Debugger API
    svr.Post("/api/script/eval", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string script_asm = body.value("script", "");
            std::vector<std::string> initial_stack;
            if (body.contains("stack") && body["stack"].is_array()) {
                for (const auto& item : body["stack"]) {
                    initial_stack.push_back(item.get<std::string>());
                }
            }
            auto eval_res = btc::script::evaluate_script(script_asm, initial_stack);
            res.set_content(btc::script::script_result_to_json(eval_res).dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json err; err["error"] = e.what();
            res.set_content(err.dump(2), "application/json");
        }
    });

    // 12. Manual Block Mining API
    svr.Post("/api/mine", [node](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string miner = "";
            if (!req.body.empty()) {
                auto body = json::parse(req.body);
                miner = body.value("miner_address", "");
            }
            auto block = node->mine_block(miner);
            json j;
            j["success"] = true;
            j["block_height"] = block.height;
            j["block_hash"] = block.hash;
            j["tx_count"] = block.tx_count;
            j["nonce"] = block.nonce;
            j["timestamp"] = block.timestamp;
            res.set_content(j.dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            json err; err["error"] = e.what();
            res.set_content(err.dump(2), "application/json");
        }
    });

    // 13. C++ SHA-256 Hashing Benchmark API
    svr.Get("/api/benchmark", [node](const httplib::Request& req, httplib::Response& res) {
        size_t iters = 500000;
        if (req.has_param("iterations")) {
            iters = std::stoul(req.get_param_value("iterations"));
        }
        auto bench = node->run_benchmark(iters);
        res.set_content(bench.dump(2), "application/json");
    });

    std::cout << "========================================================\n";
    std::cout << "  ₿ Bitcoin Core C++ Testnet Suite & Explorer Server   \n";
    std::cout << "  Engine: C++17 / Native OpenSSL / cpp-httplib           \n";
    std::cout << "  Listening on: http://localhost:" << port << "              \n";
    std::cout << "========================================================\n";

    svr.listen("0.0.0.0", port);
    return 0;
}
