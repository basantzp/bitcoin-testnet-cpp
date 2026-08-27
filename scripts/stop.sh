#!/usr/bin/env bash
# ==============================================================================
# Stop Bitcoin Core C++ Testnet Server
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PID_FILE="${SCRIPT_DIR}/.bitcoin-testnet-cpp.pid"

if [[ -f "$PID_FILE" ]]; then
  PID=$(cat "$PID_FILE")
  if kill -0 "$PID" 2>/dev/null; then
    echo "[+] Stopping Bitcoin Core C++ Testnet (PID: $PID)..."
    kill "$PID" || true
    sleep 1
    if kill -0 "$PID" 2>/dev/null; then
      kill -9 "$PID" || true
    fi
    echo "✓ Stopped."
  else
    echo "[-] Process $PID not running."
  fi
  rm -f "$PID_FILE"
else
  pkill -f "bitcoin-testnet-core" || echo "No running process found."
fi
