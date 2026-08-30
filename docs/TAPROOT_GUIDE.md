# Bitcoin Taproot & Schnorr Implementation Guide

## Taproot Fundamentals (BIP 340, 341, 342)
- **Schnorr Signatures (BIP 340)**: Linear signature aggregation allowing $n$-of-$n$ keys to aggregate into a single 64-byte signature.
- **Taproot Outputs (BIP 341)**: Single output script format `OP_1 <32-byte-pubkey>`.
- **Tapscript (BIP 342)**: Updated opcode semantics for conditional script path execution via Merkle trees.
