// Bitcoin Core C++ Testnet Suite - Frontend Client

const API_BASE = '';

function showToast(msg, type = 'info') {
  const toast = document.getElementById('toast');
  toast.textContent = msg;
  toast.style.borderColor = type === 'error' ? 'var(--color-red)' : 'var(--btc-orange)';
  toast.style.display = 'block';
  setTimeout(() => {
    toast.style.display = 'none';
  }, 3500);
}

// Tab Switching
document.querySelectorAll('.tab-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
    document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
    
    btn.classList.add('active');
    const target = document.getElementById(`pane-${btn.dataset.tab}`);
    if (target) target.classList.add('active');
  });
});

// Quick navigation buttons
document.getElementById('btnOpenFaucet')?.addEventListener('click', () => {
  document.querySelector('[data-tab="faucet"]').click();
});
document.getElementById('btnOpenDebugger')?.addEventListener('click', () => {
  document.querySelector('[data-tab="debugger"]').click();
});
document.getElementById('btnRunBench')?.addEventListener('click', () => {
  document.querySelector('[data-tab="diagnostics"]').click();
  document.getElementById('btnRunBenchmarkAction').click();
});

// 1. Fetch Node Info
async function fetchNodeInfo() {
  try {
    const res = await fetch(`${API_BASE}/api/info`);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    
    document.getElementById('navHeight').textContent = `#${data.blocks}`;
    document.getElementById('statHeight').textContent = `#${data.blocks}`;
    document.getElementById('statBestHash').textContent = `Best: ${data.bestblockhash.substring(0, 16)}...`;
    document.getElementById('statMempool').textContent = `${data.mempool_size} TXs`;
    document.getElementById('statUptime').textContent = `${data.uptime_seconds}s`;
    document.getElementById('statCompiler').textContent = data.compiler || 'GCC C++17';
    
    document.getElementById('nodeDiagJson').textContent = JSON.stringify(data, null, 2);
  } catch (err) {
    console.error('Failed to fetch node info:', err);
  }
}

// 2. Fetch Latest Blocks
async function fetchBlocks() {
  const tbody = document.getElementById('blocksTableBody');
  try {
    const res = await fetch(`${API_BASE}/api/blocks?limit=10`);
    const blocks = await res.json();
    
    if (!Array.isArray(blocks) || blocks.length === 0) {
      tbody.innerHTML = '<tr><td colspan="6" style="text-align:center;">No blocks mined yet.</td></tr>';
      return;
    }
    
    tbody.innerHTML = blocks.map(b => {
      const date = new Date(b.timestamp * 1000).toLocaleTimeString();
      return `
        <tr>
          <td><strong style="color: var(--btc-gold);">#${b.height}</strong></td>
          <td class="hash-cell" onclick="inspectBlock('${b.hash}')" title="${b.hash}">${b.hash}</td>
          <td>${b.tx_count}</td>
          <td>${b.size_bytes} B</td>
          <td>${b.nonce}</td>
          <td>${date}</td>
        </tr>
      `;
    }).join('');
  } catch (err) {
    tbody.innerHTML = `<tr><td colspan="6" style="text-align:center; color:var(--color-red);">Error loading blocks: ${err.message}</td></tr>`;
  }
}

async function inspectBlock(hashOrHeight) {
  const card = document.getElementById('blockDetailCard');
  const content = document.getElementById('blockDetailContent');
  card.style.display = 'block';
  content.textContent = 'Fetching block from C++ Core...';
  
  try {
    const res = await fetch(`${API_BASE}/api/block/${hashOrHeight}`);
    const data = await res.json();
    content.textContent = JSON.stringify(data, null, 2);
    card.scrollIntoView({ behavior: 'smooth' });
  } catch (err) {
    content.textContent = `Error: ${err.message}`;
  }
}

// 3. Quick Mine Button
document.getElementById('btnMineQuick')?.addEventListener('click', async () => {
  showToast('Mining new block in C++ Engine...', 'info');
  try {
    const res = await fetch(`${API_BASE}/api/mine`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({})
    });
    const data = await res.json();
    if (data.success) {
      showToast(`⛏️ Block #${data.block_height} mined! (Nonce: ${data.nonce})`, 'info');
      fetchNodeInfo();
      fetchBlocks();
    } else {
      showToast(`Mining failed: ${data.error}`, 'error');
    }
  } catch (err) {
    showToast(`Error: ${err.message}`, 'error');
  }
});

// 4. Generate Wallet
async function generateWallet() {
  try {
    const res = await fetch(`${API_BASE}/api/wallet/generate`, { method: 'POST' });
    const data = await res.json();
    
    document.getElementById('genSegWit').value = data.segwit_address;
    document.getElementById('genTaproot').value = data.taproot_address;
    document.getElementById('genLegacy').value = data.legacy_address;
    document.getElementById('genWIF').value = data.private_key_wif;
    
    showToast('🔑 New Testnet Keypair Generated!', 'info');
  } catch (err) {
    showToast(`Failed to generate wallet: ${err.message}`, 'error');
  }
}

document.getElementById('btnGenWallet')?.addEventListener('click', generateWallet);
document.getElementById('btnUseInFaucet')?.addEventListener('click', () => {
  const segwit = document.getElementById('genSegWit').value;
  if (segwit) {
    document.getElementById('faucetAddress').value = segwit;
    showToast('Copied to faucet input!', 'info');
  } else {
    showToast('Generate a wallet first.', 'error');
  }
});

// 5. Faucet Request
document.getElementById('btnRequestFaucet')?.addEventListener('click', async () => {
  const address = document.getElementById('faucetAddress').value.trim();
  const amount_sats = parseInt(document.getElementById('faucetAmount').value, 10);
  const resultDiv = document.getElementById('faucetResult');
  
  if (!address) {
    showToast('Please enter a recipient testnet address', 'error');
    return;
  }
  
  resultDiv.innerHTML = '<div style="color:var(--btc-gold);">Dispensing satoshis & mining block...</div>';
  
  try {
    const res = await fetch(`${API_BASE}/api/faucet`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ address, amount_sats })
    });
    const data = await res.json();
    
    if (data.success) {
      resultDiv.innerHTML = `
        <div style="background:rgba(63,185,80,0.1); border:1px solid var(--color-green); padding:1rem; border-radius:6px;">
          <strong style="color:var(--color-green);">✓ Dispensed ${data.amount_btc} tBTC!</strong><br>
          <span style="font-size:0.8rem; color:var(--text-muted);">TXID:</span> <code class="code-box" style="display:block; margin-top:4px; font-size:0.75rem;">${data.txid}</code>
        </div>
      `;
      showToast('🚰 Faucet payout mined successfully!', 'info');
      fetchNodeInfo();
      fetchBlocks();
    } else {
      resultDiv.innerHTML = `<div style="color:var(--color-red);">Error: ${data.error}</div>`;
    }
  } catch (err) {
    resultDiv.innerHTML = `<div style="color:var(--color-red);">Error: ${err.message}</div>`;
  }
});

// 6. Script Presets and Debugger
const SCRIPT_PRESETS = {
  p2pkh: {
    script: "OP_DUP OP_HASH160 0x89abcdefabbaabbaabbaabbaabbaabbaabbaabba OP_EQUALVERIFY OP_CHECKSIG",
    stack: "sig_3045022100... pubkey_02a1b2c3d4..."
  },
  math: {
    script: "OP_5 OP_3 OP_ADD OP_8 OP_EQUAL",
    stack: ""
  },
  hashlock: {
    script: "OP_SHA256 0x5d41402abc4b2a76b9719d911017c592 OP_EQUAL",
    stack: "hello"
  },
  equalverify: {
    script: "OP_EQUALVERIFY OP_1",
    stack: "secret_preimage secret_preimage"
  }
};

document.getElementById('scriptPresets')?.addEventListener('change', (e) => {
  const preset = SCRIPT_PRESETS[e.target.value];
  if (preset) {
    document.getElementById('scriptCode').value = preset.script;
    document.getElementById('initialStack').value = preset.stack;
  }
});

document.getElementById('btnEvalScript')?.addEventListener('click', async () => {
  const script = document.getElementById('scriptCode').value.trim();
  const stackRaw = document.getElementById('initialStack').value.trim();
  const stack = stackRaw ? stackRaw.split(/\s+/) : [];
  
  const visualizer = document.getElementById('stackVisualizer');
  const statusDiv = document.getElementById('scriptResultStatus');
  const stepsContainer = document.getElementById('scriptStepsContainer');
  
  if (!script) {
    showToast('Enter a script to evaluate', 'error');
    return;
  }
  
  visualizer.innerHTML = '<div style="color:var(--btc-gold);">Executing script in C++ Engine...</div>';
  stepsContainer.innerHTML = '';
  
  try {
    const res = await fetch(`${API_BASE}/api/script/eval`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ script, stack })
    });
    const data = await res.json();
    
    // Stack Visualizer
    if (data.final_stack && data.final_stack.length > 0) {
      visualizer.innerHTML = data.final_stack.map((item, idx) => `
        <div class="stack-item">[${idx}] ${item}</div>
      `).join('');
    } else {
      visualizer.innerHTML = '<div style="color:var(--text-dim); text-align:center;">(Stack is Empty)</div>';
    }
    
    // Status
    const isSuccess = data.valid;
    statusDiv.innerHTML = `
      <div style="background:${isSuccess ? 'rgba(63,185,80,0.15)' : 'rgba(248,81,73,0.15)'}; border:1px solid ${isSuccess ? 'var(--color-green)' : 'var(--color-red)'}; padding:0.75rem; border-radius:6px; font-family:var(--font-mono); font-size:0.85rem;">
        <strong style="color:${isSuccess ? 'var(--color-green)' : 'var(--color-red)'};">
          ${isSuccess ? '✓ SUCCESS' : '✕ FAILED'}
        </strong>: ${data.final_status}
      </div>
    `;
    
    // Steps
    if (data.steps && data.steps.length > 0) {
      stepsContainer.innerHTML = data.steps.map(s => `
        <div class="step-row ${s.success ? '' : 'error'}">
          <div><strong>Step ${s.step_index}:</strong> <code>${s.opcode}</code></div>
          <div style="color:var(--text-muted); font-size:0.75rem;">
            Stack: [${s.stack_after.join(', ')}]
          </div>
        </div>
      `).join('');
    }
    
    showToast(isSuccess ? '✓ Script Validated!' : 'Script Error Detected', isSuccess ? 'info' : 'error');
  } catch (err) {
    statusDiv.innerHTML = `<div style="color:var(--color-red);">Error: ${err.message}</div>`;
  }
});

// 7. Broadcast Transaction
document.getElementById('btnBroadcastTx')?.addEventListener('click', async () => {
  const from = document.getElementById('txFrom').value.trim();
  const to = document.getElementById('txTo').value.trim();
  const amount_sats = parseInt(document.getElementById('txAmount').value, 10);
  const resultDiv = document.getElementById('txBroadcastResult');
  
  if (!from || !to || !amount_sats) {
    showToast('Fill in all transaction fields', 'error');
    return;
  }
  
  resultDiv.innerHTML = '<div style="color:var(--btc-gold);">Constructing & broadcasting...</div>';
  
  try {
    const res = await fetch(`${API_BASE}/api/tx/create`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ from, to, amount_sats })
    });
    const data = await res.json();
    
    if (data.success) {
      resultDiv.innerHTML = `
        <div style="background:rgba(63,185,80,0.1); border:1px solid var(--color-green); padding:1rem; border-radius:6px;">
          <strong style="color:var(--color-green);">✓ Broadcasted to Mempool!</strong><br>
          <span style="font-size:0.8rem;">TXID:</span> <code class="code-box" style="display:block; margin-top:4px; font-size:0.75rem;">${data.txid}</code>
        </div>
      `;
      showToast('🚀 TX in Mempool! Auto-mining recommended.', 'info');
      fetchNodeInfo();
    } else {
      resultDiv.innerHTML = `<div style="color:var(--color-red);">Error: ${data.error}</div>`;
    }
  } catch (err) {
    resultDiv.innerHTML = `<div style="color:var(--color-red);">Error: ${err.message}</div>`;
  }
});

// 8. Raw Hex Decoder
document.getElementById('btnDecodeHex')?.addEventListener('click', async () => {
  const hex = document.getElementById('rawHexInput').value.trim();
  const resultDiv = document.getElementById('hexDecodeResult');
  
  if (!hex) {
    showToast('Enter raw hex to decode', 'error');
    return;
  }
  
  resultDiv.innerHTML = '<div style="color:var(--btc-gold);">Decoding in C++ Engine...</div>';
  
  try {
    const res = await fetch(`${API_BASE}/api/tx/decode`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ hex })
    });
    const data = await res.json();
    resultDiv.innerHTML = `<pre class="code-box">${JSON.stringify(data, null, 2)}</pre>`;
  } catch (err) {
    resultDiv.innerHTML = `<div style="color:var(--color-red);">Error: ${err.message}</div>`;
  }
});

// 9. C++ Hashing Benchmark
document.getElementById('btnRunBenchmarkAction')?.addEventListener('click', async () => {
  const btn = document.getElementById('btnRunBenchmarkAction');
  const benchOut = document.getElementById('benchResultJson');
  
  btn.disabled = true;
  btn.textContent = '⚡ Benchmarking...';
  benchOut.textContent = 'Running 500,000 multi-threaded SHA-256 iterations in native C++...';
  
  try {
    const res = await fetch(`${API_BASE}/api/benchmark?iterations=500000`);
    const data = await res.json();
    benchOut.textContent = JSON.stringify(data, null, 2);
    
    const mhs = (data.hashes_per_second / 1000000).toFixed(2);
    document.getElementById('statHashrate').textContent = `${mhs} MH/s`;
    showToast(`⚡ C++ Benchmark: ${mhs} MH/s (${data.duration_ms.toFixed(1)} ms)`, 'info');
  } catch (err) {
    benchOut.textContent = `Error: ${err.message}`;
  } finally {
    btn.disabled = false;
    btn.textContent = '⚡ Run 500,000 Hash Benchmark';
  }
});

// Initialization
document.addEventListener('DOMContentLoaded', () => {
  fetchNodeInfo();
  fetchBlocks();
  generateWallet();
  
  // Set default script preset
  const defaultPreset = SCRIPT_PRESETS.p2pkh;
  document.getElementById('scriptCode').value = defaultPreset.script;
  document.getElementById('initialStack').value = defaultPreset.stack;
  
  // Auto refresh blocks every 8 seconds
  setInterval(fetchNodeInfo, 8000);
  setInterval(fetchBlocks, 15000);
});
