#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "crypto.hpp"
#include "script.hpp"

using namespace btc;

void test_crypto_primitives() {
    std::cout << "[TEST] Running Crypto Primitives Verification..." << std::endl;

    // Test 1: SHA-256 of empty string is e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    std::string sha_empty = crypto::sha256_hex("");
    assert(sha_empty == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    // Test 2: SHA-256 of "bitcoin"
    std::string sha_btc = crypto::sha256_hex("bitcoin");
    assert(sha_btc == "6b88c087247aa2f07ee1c5956b8e1a9f4c7f892a70e324f1bb3d161e05ca107b");

    // Test 3: Double SHA-256 (Hash256) of "hello"
    std::string hash256_hello = crypto::hash256_hex("hello");
    assert(!hash256_hello.empty());
    assert(hash256_hello.length() == 64);

    // Test 4: Hex to Bytes and Bytes to Hex roundtrip
    std::string original_hex = "00112233445566778899aabbccddeeff";
    auto bytes = crypto::hex_to_bytes(original_hex);
    std::string roundtrip_hex = crypto::bytes_to_hex(bytes);
    assert(original_hex == roundtrip_hex);

    std::cout << "  ✓ Crypto Primitives passed (SHA-256, Hash256, Hex codecs)" << std::endl;
}

void test_script_stack_opcodes() {
    std::cout << "[TEST] Running Bitcoin Script Stack Operations..." << std::endl;

    // Test OP_DUP
    auto res_dup = script::evaluate_script("OP_DUP", {"42"});
    assert(res_dup.valid);
    assert(res_dup.final_stack.size() == 2);
    assert(res_dup.final_stack[0] == "42");
    assert(res_dup.final_stack[1] == "42");

    // Test OP_SWAP
    auto res_swap = script::evaluate_script("OP_SWAP", {"1", "2"});
    assert(res_swap.valid);
    assert(res_swap.final_stack.size() == 2);
    assert(res_swap.final_stack[0] == "2");
    assert(res_swap.final_stack[1] == "1");

    // Test OP_DROP
    auto res_drop = script::evaluate_script("OP_DROP", {"1", "2"});
    assert(res_drop.valid);
    assert(res_drop.final_stack.size() == 1);
    assert(res_drop.final_stack[0] == "1");

    std::cout << "  ✓ Stack operations passed (OP_DUP, OP_SWAP, OP_DROP)" << std::endl;
}

void test_script_logic_and_math() {
    std::cout << "[TEST] Running Bitcoin Script Arithmetic & Logic..." << std::endl;

    // Test OP_ADD
    auto res_add = script::evaluate_script("OP_5 OP_3 OP_ADD");
    assert(res_add.valid);
    assert(res_add.final_stack.size() == 1);
    assert(res_add.final_stack[0] == "8");

    // Test OP_SUB
    auto res_sub = script::evaluate_script("OP_10 OP_4 OP_SUB");
    assert(res_sub.valid);
    assert(res_sub.final_stack.size() == 1);
    assert(res_sub.final_stack[0] == "6");

    // Test OP_EQUAL matching
    auto res_eq = script::evaluate_script("OP_EQUAL", {"abc", "abc"});
    assert(res_eq.valid);
    assert(res_eq.final_stack.back() == "1");

    // Test OP_EQUAL mismatch
    auto res_neq = script::evaluate_script("OP_EQUAL", {"abc", "xyz"});
    assert(!res_neq.valid); // Top of stack is 0, so script evaluates to false
    assert(!res_neq.final_stack.empty());
    assert(res_neq.final_stack.back() == "0");

    std::cout << "  ✓ Arithmetic & Equality passed (OP_ADD, OP_SUB, OP_EQUAL)" << std::endl;
}

void test_script_p2pkh_flow() {
    std::cout << "[TEST] Running Standard P2PKH Script Execution..." << std::endl;

    std::string fake_pubkey = "0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798";
    auto pubkey_bytes = crypto::hex_to_bytes(fake_pubkey);
    std::string pubkey_hash = crypto::hash160_hex(pubkey_bytes);

    // Initial stack has signature and pubkey: { <sig>, <pubkey> }
    std::vector<std::string> initial_stack = {"30440220...fake_sig", fake_pubkey};

    // ScriptPubKey: OP_DUP OP_HASH160 <pubkey_hash> OP_EQUALVERIFY OP_CHECKSIG
    std::string script_asm = "OP_DUP OP_HASH160 " + pubkey_hash + " OP_EQUALVERIFY OP_CHECKSIG";

    auto res = script::evaluate_script(script_asm, initial_stack);
    assert(res.valid);
    assert(!res.final_stack.empty());
    assert(res.final_stack.back() == "1"); // Checksig evaluated to true

    std::cout << "  ✓ P2PKH script execution passed (OP_DUP OP_HASH160 OP_EQUALVERIFY OP_CHECKSIG -> 1)" << std::endl;
}

int main() {
    std::cout << "=================================================" << std::endl;
    std::cout << "  Bitcoin Core Testnet4 C++ Unit Test Runner     " << std::endl;
    std::cout << "=================================================" << std::endl;

    test_crypto_primitives();
    test_script_stack_opcodes();
    test_script_logic_and_math();
    test_script_p2pkh_flow();

    std::cout << "=================================================" << std::endl;
    std::cout << "  ALL 4 BITCOIN SCRIPT TEST SUITES PASSED!       " << std::endl;
    std::cout << "=================================================" << std::endl;
    return 0;
}
