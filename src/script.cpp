#include "script.hpp"
#include "crypto.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace btc::script {

static std::vector<std::string> tokenize(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

static bool is_number(const std::string& s) {
    if (s.empty()) return false;
    size_t start = 0;
    if (s[0] == '-' || s[0] == '+') start = 1;
    if (start >= s.length()) return false;
    return std::all_of(s.begin() + start, s.end(), ::isdigit);
}

ScriptResult evaluate_script(const std::string& script_asm, const std::vector<std::string>& initial_stack) {
    ScriptResult result;
    result.script_asm = script_asm;
    result.valid = true;
    result.final_status = "OK";
    
    std::vector<std::string> stack = initial_stack;
    auto tokens = tokenize(script_asm);
    
    int step_counter = 1;
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::string op = tokens[i];
        ExecutionStep step;
        step.step_index = step_counter++;
        step.opcode = op;
        step.stack_before = stack;
        step.success = true;
        
        try {
            if (op == "OP_0" || op == "0") {
                stack.push_back("0");
            } else if (op == "OP_1" || op == "1" || op == "OP_TRUE") {
                stack.push_back("1");
            } else if (op == "OP_2" || op == "2") {
                stack.push_back("2");
            } else if (op == "OP_3" || op == "3") {
                stack.push_back("3");
            } else if (op == "OP_4" || op == "4") {
                stack.push_back("4");
            } else if (op == "OP_5" || op == "5") {
                stack.push_back("5");
            } else if (op == "OP_6" || op == "6") {
                stack.push_back("6");
            } else if (op == "OP_7" || op == "7") {
                stack.push_back("7");
            } else if (op == "OP_8" || op == "8") {
                stack.push_back("8");
            } else if (op == "OP_9" || op == "9") {
                stack.push_back("9");
            } else if (op == "OP_10" || op == "10") {
                stack.push_back("10");
            } else if (op == "OP_16" || op == "16") {
                stack.push_back("16");
            } else if (op == "OP_DUP") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_DUP");
                stack.push_back(stack.back());
            } else if (op == "OP_2DUP") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_2DUP");
                auto v1 = stack[stack.size() - 2];
                auto v2 = stack[stack.size() - 1];
                stack.push_back(v1);
                stack.push_back(v2);
            } else if (op == "OP_DROP") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_DROP");
                stack.pop_back();
            } else if (op == "OP_SWAP") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_SWAP");
                std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
            } else if (op == "OP_OVER") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_OVER");
                stack.push_back(stack[stack.size() - 2]);
            } else if (op == "OP_ROT") {
                if (stack.size() < 3) throw std::runtime_error("Stack underflow on OP_ROT");
                auto top3 = stack[stack.size() - 3];
                stack.erase(stack.end() - 3);
                stack.push_back(top3);
            } else if (op == "OP_EQUAL") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_EQUAL");
                auto a = stack.back(); stack.pop_back();
                auto b = stack.back(); stack.pop_back();
                stack.push_back((a == b) ? "1" : "0");
            } else if (op == "OP_EQUALVERIFY") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_EQUALVERIFY");
                auto a = stack.back(); stack.pop_back();
                auto b = stack.back(); stack.pop_back();
                if (a != b) throw std::runtime_error("OP_EQUALVERIFY failed: items do not match");
            } else if (op == "OP_HASH160") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_HASH160");
                auto item = stack.back(); stack.pop_back();
                auto bytes = crypto::hex_to_bytes(item);
                if (bytes.empty() && !item.empty()) {
                    bytes = std::vector<uint8_t>(item.begin(), item.end());
                }
                auto h160 = crypto::hash160_hex(bytes);
                stack.push_back(h160);
            } else if (op == "OP_SHA256") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_SHA256");
                auto item = stack.back(); stack.pop_back();
                auto bytes = crypto::hex_to_bytes(item);
                if (bytes.empty() && !item.empty()) {
                    bytes = std::vector<uint8_t>(item.begin(), item.end());
                }
                auto sha = crypto::sha256_hex(std::string(bytes.begin(), bytes.end()));
                stack.push_back(sha);
            } else if (op == "OP_HASH256") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_HASH256");
                auto item = stack.back(); stack.pop_back();
                auto bytes = crypto::hex_to_bytes(item);
                if (bytes.empty() && !item.empty()) {
                    bytes = std::vector<uint8_t>(item.begin(), item.end());
                }
                auto h2 = crypto::hash256_hex(std::string(bytes.begin(), bytes.end()));
                stack.push_back(h2);
            } else if (op == "OP_CHECKSIG") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_CHECKSIG");
                auto pubkey = stack.back(); stack.pop_back();
                auto sig = stack.back(); stack.pop_back();
                // In script evaluation simulator, non-empty sig + pubkey verifies
                bool valid_sig = (!sig.empty() && !pubkey.empty() && sig != "0");
                stack.push_back(valid_sig ? "1" : "0");
            } else if (op == "OP_ADD") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_ADD");
                int64_t b = std::stoll(stack.back()); stack.pop_back();
                int64_t a = std::stoll(stack.back()); stack.pop_back();
                stack.push_back(std::to_string(a + b));
            } else if (op == "OP_SUB") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_SUB");
                int64_t b = std::stoll(stack.back()); stack.pop_back();
                int64_t a = std::stoll(stack.back()); stack.pop_back();
                stack.push_back(std::to_string(a - b));
            } else if (op == "OP_VERIFY") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_VERIFY");
                auto top = stack.back(); stack.pop_back();
                if (top == "0" || top.empty()) throw std::runtime_error("OP_VERIFY failed: top stack item is false/zero");
            } else if (op == "OP_RETURN") {
                throw std::runtime_error("OP_RETURN executed (Provably unspendable output / Data Carrier)");
            } else {
                // Push raw data / hex / constants
                std::string push_val = op;
                if (push_val.rfind("<", 0) == 0 && push_val.back() == '>') {
                    push_val = push_val.substr(1, push_val.length() - 2);
                }
                stack.push_back(push_val);
            }
        } catch (const std::exception& ex) {
            step.success = false;
            step.error_message = ex.what();
            step.stack_after = stack;
            result.steps.push_back(step);
            result.valid = false;
            result.final_status = std::string("SCRIPT_ERROR: ") + ex.what();
            result.final_stack = stack;
            return result;
        }
        
        step.stack_after = stack;
        result.steps.push_back(step);
    }
    
    result.final_stack = stack;
    if (stack.empty() || stack.back() == "0" || stack.back() == "") {
        result.valid = false;
        result.final_status = "SCRIPT_FAILED: Final top stack element is FALSE (0)";
    } else {
        result.valid = true;
        result.final_status = "SCRIPT_SUCCESS: Validated True (1)";
    }
    
    return result;
}

nlohmann::json script_result_to_json(const ScriptResult& res) {
    nlohmann::json j;
    j["valid"] = res.valid;
    j["final_status"] = res.final_status;
    j["final_stack"] = res.final_stack;
    j["script_asm"] = res.script_asm;
    
    nlohmann::json steps_json = nlohmann::json::array();
    for (const auto& step : res.steps) {
        nlohmann::json s;
        s["step_index"] = step.step_index;
        s["opcode"] = step.opcode;
        s["stack_before"] = step.stack_before;
        s["stack_after"] = step.stack_after;
        s["success"] = step.success;
        s["error_message"] = step.error_message;
        steps_json.push_back(s);
    }
    j["steps"] = steps_json;
    return j;
}

} // namespace btc::script
