#!/usr/bin/env bash
# ==============================================================================
# Start Bitcoin Core C++ Testnet Server
# ==============================================================================

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${SCRIPT_DIR}/bin/bitcoin-testnet-core"
PID_FILE="${SCRIPT_DIR}/.bitcoin-testnet-cpp.pid"
PORT="${PORT:-8332}"

if [[ ! -f "$BIN" ]]; then
  echo "[+] Building Bitcoin Core C++ engine..."
  (cd "$SCRIPT_DIR" && make)
fi

if [[ -f "$PID_FILE" ]] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
  echo "[-] Bitcoin Core C++ Testnet is already running on PID $(cat "$PID_FILE") (http://localhost:${PORT})"
  exit 0
fi

echo "[+] Starting Bitcoin Core C++ Testnet on port ${PORT}..."
(cd "$SCRIPT_DIR" && PORT="$PORT" "$BIN" > "${SCRIPT_DIR}/server.log" 2>&1 & echo $! > "$PID_FILE")

sleep 1
if kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
  echo "✓ Bitcoin Core C++ Testnet is live on http://localhost:${PORT}"
  echo "  PID: $(cat "$PID_FILE") | Log: ${SCRIPT_DIR}/server.log"
else
  echo "[-] Failed to start server. Check logs: ${SCRIPT_DIR}/server.log"
  exit 1
fi
