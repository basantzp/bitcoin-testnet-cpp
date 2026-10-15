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

static int64_t parse_num(const std::string& s) {
    try {
        return std::stoll(s);
    } catch (...) {
        throw std::runtime_error("Invalid numeric operand: " + s);
    }
}

struct ConditionalBranch {
    bool fExec;
    bool fElseSeen;
};

static bool is_truthy(const std::string& s) {
    if (s.empty() || s == "0" || s == "00" || s == "-0") return false;
    auto b = crypto::hex_to_bytes(s);
    if (!b.empty()) {
        for (size_t i = 0; i < b.size(); ++i) {
            if (b[i] != 0) {
                if (i == b.size() - 1 && b[i] == 0x80) return false;
                return true;
            }
        }
        return false;
    }
    return true;
}

static std::vector<uint8_t> get_item_bytes(const std::string& item) {
    if (item.empty()) return {};
    auto b = crypto::hex_to_bytes(item);
    if (!b.empty()) return b;
    return std::vector<uint8_t>(item.begin(), item.end());
}

static bool is_executing(const std::vector<ConditionalBranch>& vfExec) {
    for (const auto& branch : vfExec) {
        if (!branch.fExec) return false;
    }
    return true;
}

ScriptResult evaluate_script(const std::string& script_asm, const std::vector<std::string>& initial_stack) {
    ScriptResult result;
    result.script_asm = script_asm;
    result.valid = true;
    result.final_status = "OK";
    
    std::vector<std::string> stack = initial_stack;
    auto tokens = tokenize(script_asm);
    std::vector<ConditionalBranch> vfExec;
    
    int step_counter = 1;
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::string op = tokens[i];
        ExecutionStep step;
        step.step_index = step_counter++;
        step.opcode = op;
        step.stack_before = stack;
        step.success = true;
        
        try {
            // Conditional flow control opcodes
            if (op == "OP_IF" || op == "OP_NOTIF") {
                bool fExec = is_executing(vfExec);
                if (fExec) {
                    if (stack.empty()) throw std::runtime_error("Stack underflow on " + op);
                    std::string val = stack.back(); stack.pop_back();
                    bool cond = is_truthy(val);
                    if (op == "OP_NOTIF") cond = !cond;
                    vfExec.push_back({cond, false});
                } else {
                    vfExec.push_back({false, false});
                }
                step.stack_after = stack;
                result.steps.push_back(step);
                continue;
            } else if (op == "OP_ELSE") {
                if (vfExec.empty()) throw std::runtime_error("OP_ELSE encountered without matching OP_IF");
                if (vfExec.back().fElseSeen) throw std::runtime_error("Duplicate OP_ELSE in same conditional block");
                vfExec.back().fElseSeen = true;
                bool parent_exec = true;
                for (size_t k = 0; k + 1 < vfExec.size(); ++k) {
                    if (!vfExec[k].fExec) { parent_exec = false; break; }
                }
                if (parent_exec) {
                    vfExec.back().fExec = !vfExec.back().fExec;
                }
                step.stack_after = stack;
                result.steps.push_back(step);
                continue;
            } else if (op == "OP_ENDIF") {
                if (vfExec.empty()) throw std::runtime_error("OP_ENDIF encountered without matching OP_IF");
                vfExec.pop_back();
                step.stack_after = stack;
                result.steps.push_back(step);
                continue;
            }

            // If inside inactive conditional branch, skip execution of opcodes
            if (!is_executing(vfExec)) {
                step.stack_after = stack;
                result.steps.push_back(step);
                continue;
            }

            // --- Active Execution ---
            // Constants
            if (op == "OP_0" || op == "0") {
                stack.push_back("0");
            } else if (op == "OP_1NEGATE" || op == "-1") {
                stack.push_back("-1");
            } else if (op == "OP_1" || op == "1" || op == "OP_TRUE") {
                stack.push_back("1");
            } else if (op.rfind("OP_", 0) == 0 && op.length() > 3 && std::all_of(op.begin() + 3, op.end(), ::isdigit)) {
                stack.push_back(op.substr(3));
            } else if (op == "OP_NOP") {
                // Do nothing
            }
            // Stack Operations
            else if (op == "OP_DUP") {
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
            } else if (op == "OP_2DROP") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_2DROP");
                stack.pop_back();
                stack.pop_back();
            } else if (op == "OP_SWAP") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_SWAP");
                std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
            } else if (op == "OP_2SWAP") {
                if (stack.size() < 4) throw std::runtime_error("Stack underflow on OP_2SWAP");
                std::swap(stack[stack.size() - 4], stack[stack.size() - 2]);
                std::swap(stack[stack.size() - 3], stack[stack.size() - 1]);
            } else if (op == "OP_OVER") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_OVER");
                stack.push_back(stack[stack.size() - 2]);
            } else if (op == "OP_2OVER") {
                if (stack.size() < 4) throw std::runtime_error("Stack underflow on OP_2OVER");
                auto v1 = stack[stack.size() - 4];
                auto v2 = stack[stack.size() - 3];
                stack.push_back(v1);
                stack.push_back(v2);
            } else if (op == "OP_ROT") {
                if (stack.size() < 3) throw std::runtime_error("Stack underflow on OP_ROT");
                auto top3 = stack[stack.size() - 3];
                stack.erase(stack.end() - 3);
                stack.push_back(top3);
            } else if (op == "OP_NIP") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_NIP");
                stack.erase(stack.end() - 2);
            } else if (op == "OP_TUCK") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_TUCK");
                stack.insert(stack.end() - 2, stack.back());
            } else if (op == "OP_DEPTH") {
                stack.push_back(std::to_string(stack.size()));
            } else if (op == "OP_SIZE") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_SIZE");
                auto bytes = get_item_bytes(stack.back());
                stack.push_back(std::to_string(bytes.size()));
            } else if (op == "OP_PICK") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_PICK");
                int64_t n = parse_num(stack.back()); stack.pop_back();
                if (n < 0 || (size_t)n >= stack.size()) throw std::runtime_error("Invalid stack index for OP_PICK");
                stack.push_back(stack[stack.size() - 1 - n]);
            } else if (op == "OP_ROLL") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_ROLL");
                int64_t n = parse_num(stack.back()); stack.pop_back();
                if (n < 0 || (size_t)n >= stack.size()) throw std::runtime_error("Invalid stack index for OP_ROLL");
                auto item = stack[stack.size() - 1 - n];
                stack.erase(stack.end() - 1 - n);
                stack.push_back(item);
            }
            // Equality & Verification
            else if (op == "OP_EQUAL") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_EQUAL");
                auto a = stack.back(); stack.pop_back();
                auto b = stack.back(); stack.pop_back();
                stack.push_back((a == b) ? "1" : "0");
            } else if (op == "OP_EQUALVERIFY") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_EQUALVERIFY");
                auto a = stack.back(); stack.pop_back();
                auto b = stack.back(); stack.pop_back();
                if (a != b) throw std::runtime_error("OP_EQUALVERIFY failed: items do not match");
            } else if (op == "OP_VERIFY") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_VERIFY");
                auto top = stack.back(); stack.pop_back();
                if (!is_truthy(top)) throw std::runtime_error("OP_VERIFY failed: top stack item is false/zero");
            }
            // Cryptographic Opcodes
            else if (op == "OP_RIPEMD160") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_RIPEMD160");
                auto item = stack.back(); stack.pop_back();
                auto bytes = get_item_bytes(item);
                auto rmd = crypto::ripemd160_hex(bytes);
                stack.push_back(rmd);
            } else if (op == "OP_SHA256") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_SHA256");
                auto item = stack.back(); stack.pop_back();
                auto bytes = get_item_bytes(item);
                auto sha = crypto::sha256_hex(std::string(bytes.begin(), bytes.end()));
                stack.push_back(sha);
            } else if (op == "OP_HASH160") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_HASH160");
                auto item = stack.back(); stack.pop_back();
                auto bytes = get_item_bytes(item);
                auto h160 = crypto::hash160_hex(bytes);
                stack.push_back(h160);
            } else if (op == "OP_HASH256") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_HASH256");
                auto item = stack.back(); stack.pop_back();
                auto bytes = get_item_bytes(item);
                auto h2 = crypto::hash256_hex(std::string(bytes.begin(), bytes.end()));
                stack.push_back(h2);
            } else if (op == "OP_CHECKSIG" || op == "OP_CHECKSIGVERIFY") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on " + op);
                auto pubkey = stack.back(); stack.pop_back();
                auto sig = stack.back(); stack.pop_back();
                bool valid_sig = false;
                auto pub_bytes = crypto::hex_to_bytes(pubkey);
                auto sig_bytes = crypto::hex_to_bytes(sig);
                if ((pub_bytes.size() == 33 || pub_bytes.size() == 65) && sig_bytes.size() >= 8 && sig_bytes[0] == 0x30) {
                    // Cryptographic ECDSA secp256k1 verification
                    auto msg_hash = crypto::sha256_bytes(pub_bytes);
                    valid_sig = crypto::ecdsa_verify(pub_bytes, msg_hash, sig_bytes);
                } else {
                    valid_sig = (!sig.empty() && !pubkey.empty() && sig != "0");
                }
                if (op == "OP_CHECKSIGVERIFY") {
                    if (!valid_sig) throw std::runtime_error("OP_CHECKSIGVERIFY failed: signature verification failed");
                } else {
                    stack.push_back(valid_sig ? "1" : "0");
                }
            } else if (op == "OP_CHECKMULTISIG" || op == "OP_CHECKMULTISIGVERIFY") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on " + op);
                int64_t n_keys = parse_num(stack.back()); stack.pop_back();
                if (n_keys < 0 || n_keys > 20 || (size_t)n_keys > stack.size()) {
                    throw std::runtime_error("Invalid public key count in " + op + " (max 20)");
                }
                std::vector<std::string> pubkeys;
                for (int64_t k = 0; k < n_keys; ++k) {
                    pubkeys.push_back(stack.back()); stack.pop_back();
                }
                std::reverse(pubkeys.begin(), pubkeys.end());
                if (stack.empty()) throw std::runtime_error("Stack underflow for signature count in " + op);
                int64_t m_sigs = parse_num(stack.back()); stack.pop_back();
                if (m_sigs < 0 || m_sigs > n_keys || (size_t)m_sigs > stack.size()) {
                    throw std::runtime_error("Invalid signature count in " + op + " (m must be between 0 and n)");
                }
                std::vector<std::string> sigs;
                for (int64_t k = 0; k < m_sigs; ++k) {
                    sigs.push_back(stack.back()); stack.pop_back();
                }
                std::reverse(sigs.begin(), sigs.end());
                // Bitcoin dummy element (off-by-one bug requirement, BIP-147 NULLDUMMY)
                if (stack.empty()) throw std::runtime_error("Missing dummy element for " + op);
                std::string dummy = stack.back(); stack.pop_back();
                if (is_truthy(dummy)) {
                    throw std::runtime_error(op + " failed: BIP-147 NULLDUMMY violation (dummy element must be null/empty/0)");
                }
                bool multisig_ok = true;
                for (const auto& s : sigs) {
                    if (s.empty() || s == "0") { multisig_ok = false; break; }
                }
                if (op == "OP_CHECKMULTISIGVERIFY") {
                    if (!multisig_ok) throw std::runtime_error("OP_CHECKMULTISIGVERIFY failed: signatures did not verify");
                } else {
                    stack.push_back(multisig_ok ? "1" : "0");
                }
            }
            // Timelocks
            else if (op == "OP_CHECKLOCKTIMEVERIFY") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_CHECKLOCKTIMEVERIFY");
                int64_t locktime = parse_num(stack.back());
                if (locktime < 0) throw std::runtime_error("OP_CHECKLOCKTIMEVERIFY failed: negative locktime");
                // Leaves locktime on stack
            } else if (op == "OP_CHECKSEQUENCEVERIFY") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_CHECKSEQUENCEVERIFY");
                int64_t seq = parse_num(stack.back());
                if (seq < 0) throw std::runtime_error("OP_CHECKSEQUENCEVERIFY failed: negative sequence");
                // Leaves sequence on stack
            }
            // Arithmetic & Math
            else if (op == "OP_ADD") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_ADD");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                stack.push_back(std::to_string(a + b));
            } else if (op == "OP_SUB") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_SUB");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                stack.push_back(std::to_string(a - b));
            } else if (op == "OP_1ADD") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_1ADD");
                int64_t v = parse_num(stack.back()); stack.pop_back();
                stack.push_back(std::to_string(v + 1));
            } else if (op == "OP_1SUB") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_1SUB");
                int64_t v = parse_num(stack.back()); stack.pop_back();
                stack.push_back(std::to_string(v - 1));
            } else if (op == "OP_NEGATE") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_NEGATE");
                int64_t v = parse_num(stack.back()); stack.pop_back();
                stack.push_back(std::to_string(-v));
            } else if (op == "OP_ABS") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_ABS");
                int64_t v = parse_num(stack.back()); stack.pop_back();
                stack.push_back(std::to_string(std::abs(v)));
            } else if (op == "OP_NOT") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_NOT");
                std::string item = stack.back(); stack.pop_back();
                stack.push_back(is_truthy(item) ? "0" : "1");
            } else if (op == "OP_0NOTEQUAL") {
                if (stack.empty()) throw std::runtime_error("Stack underflow on OP_0NOTEQUAL");
                std::string item = stack.back(); stack.pop_back();
                stack.push_back(is_truthy(item) ? "1" : "0");
            } else if (op == "OP_NUMEQUAL") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_NUMEQUAL");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                stack.push_back(a == b ? "1" : "0");
            } else if (op == "OP_NUMEQUALVERIFY") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_NUMEQUALVERIFY");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                if (a != b) throw std::runtime_error("OP_NUMEQUALVERIFY failed: operands are not equal");
            } else if (op == "OP_NUMNOTEQUAL") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_NUMNOTEQUAL");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                stack.push_back(a != b ? "1" : "0");
            } else if (op == "OP_LESSTHAN") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_LESSTHAN");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                stack.push_back(a < b ? "1" : "0");
            } else if (op == "OP_GREATERTHAN") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_GREATERTHAN");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                stack.push_back(a > b ? "1" : "0");
            } else if (op == "OP_LESSTHANOREQUAL") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_LESSTHANOREQUAL");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                stack.push_back(a <= b ? "1" : "0");
            } else if (op == "OP_GREATERTHANOREQUAL") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_GREATERTHANOREQUAL");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                stack.push_back(a >= b ? "1" : "0");
            } else if (op == "OP_MIN") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_MIN");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                stack.push_back(std::to_string(std::min(a, b)));
            } else if (op == "OP_MAX") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_MAX");
                int64_t b = parse_num(stack.back()); stack.pop_back();
                int64_t a = parse_num(stack.back()); stack.pop_back();
                stack.push_back(std::to_string(std::max(a, b)));
            } else if (op == "OP_WITHIN") {
                if (stack.size() < 3) throw std::runtime_error("Stack underflow on OP_WITHIN");
                int64_t max_val = parse_num(stack.back()); stack.pop_back();
                int64_t min_val = parse_num(stack.back()); stack.pop_back();
                int64_t x_val = parse_num(stack.back()); stack.pop_back();
                stack.push_back((min_val <= x_val && x_val < max_val) ? "1" : "0");
            } else if (op == "OP_BOOLAND") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_BOOLAND");
                std::string b_str = stack.back(); stack.pop_back();
                std::string a_str = stack.back(); stack.pop_back();
                stack.push_back((is_truthy(a_str) && is_truthy(b_str)) ? "1" : "0");
            } else if (op == "OP_BOOLOR") {
                if (stack.size() < 2) throw std::runtime_error("Stack underflow on OP_BOOLOR");
                std::string b_str = stack.back(); stack.pop_back();
                std::string a_str = stack.back(); stack.pop_back();
                stack.push_back((is_truthy(a_str) || is_truthy(b_str)) ? "1" : "0");
            } else if (op == "OP_RETURN") {
                throw std::runtime_error("OP_RETURN executed (Provably unspendable output / Data Carrier)");
            } else {
                // Raw push data / hex / constants
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

    if (!vfExec.empty()) {
        result.valid = false;
        result.final_status = "SCRIPT_ERROR: Unclosed conditional branch (missing OP_ENDIF)";
        result.final_stack = stack;
        return result;
    }
    
    result.final_stack = stack;
    if (stack.empty() || !is_truthy(stack.back())) {
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
