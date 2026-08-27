#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "json.hpp"

namespace btc::script {

struct StackItem {
    std::string hex;
    std::string text;
    int64_t num_val;
    bool is_num;
};

struct ExecutionStep {
    int step_index;
    std::string opcode;
    std::vector<std::string> stack_before;
    std::vector<std::string> stack_after;
    bool success;
    std::string error_message;
};

struct ScriptResult {
    bool valid;
    std::string final_status;
    std::vector<std::string> final_stack;
    std::vector<ExecutionStep> steps;
    std::string script_hex;
    std::string script_asm;
};

// Evaluate Bitcoin Script (ASM format: "OP_DUP OP_HASH160 <pubKeyHash> OP_EQUALVERIFY OP_CHECKSIG")
ScriptResult evaluate_script(const std::string& script_asm, const std::vector<std::string>& initial_stack = {});

// Helper to convert ScriptResult to JSON
nlohmann::json script_result_to_json(const ScriptResult& res);

} // namespace btc::script
