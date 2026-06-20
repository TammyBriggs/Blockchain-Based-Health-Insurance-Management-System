# ALU Health Insurance Blockchain Management System

A decentralized, low-level blockchain platform built entirely in C for the African Leadership University (ALU) community. This system simulates a secure, immutable, and transparent health insurance platform utilizing a native cryptocurrency called the ALU Health Token (AHT).

## 🚀 Core Features

This project was built from scratch without the use of high-level blockchain frameworks. It implements core blockchain architecture, including:

* **Hybrid State Engine:** Simultaneously implements both the UTXO model (for primary asset transfers and double-spending protection) and the Account-Based model (for balance tracking and fee deduction).
* **Cryptographic Security:** Utilizes the OpenSSL `secp256k1` curve for real ECDSA keypair generation and transaction signing.
* **Custom Merkle Trees:** Implements bottom-up Merkle Tree construction and verification from scratch to summarize block transactions and detect tampering.
* **Consensus & Mining:** Features Proof-of-Work (PoW) consensus supporting both Solo and Proportional Pool Mining, complete with a 10-block rolling average automatic difficulty retargeting algorithm.
* **Automated Business Logic:** Enforces strict 365-day policy lifecycles and an automated 5% premium split to a dedicated Reinsurance Pool.
* **Fraud Detection Heuristics:** Automatically analyzes the mempool and on-chain history to flag duplicate transactions, high-frequency claims (>3 per 24hr), and abnormal provider settlement requests.
* **Binary Persistence:** Serializes and saves the full memory state to disk, allowing the chain to be safely shut down, loaded, and cryptographically re-verified upon reboot.

---

## 🛠️ Prerequisites & Dependencies

To compile and run this node, your environment must have the following installed:

* **GCC:** The GNU Compiler Collection.
* **OpenSSL:** Specifically the `libcrypto` and `libssl` development libraries.
    * *Ubuntu/Debian:* `sudo apt-get install libssl-dev`
    * *MacOS:* `brew install openssl` (You may need to map the include/lib paths manually).
    * *Windows:* Install OpenSSL via MSYS2 or MinGW.

---

## ⚙️ Compilation Instructions

Ensure all source files (`.c` and `.h`) are in the same directory. Compile the monolithic application using the following strict GCC command:

```bash
gcc -Wall main.c crypto.c state.c mempool.c chain.c insurance.c persistence.c -o health_chain -lcrypto -lssl
```

> **Note:** The `-Wall` flag ensures all warnings are displayed. The code is designed to compile cleanly with zero warnings.

---

## 💻 Usage & CLI Operations

Run the compiled executable to launch the interactive Read-Eval-Print Loop (REPL) dashboard:

```bash
./health_chain
```

### Recommended Demo Workflow

If you are testing the system for the first time, follow this sequence to observe the full lifecycle:

1. **Enroll:** Type `2 ALU_GOLD` to enroll the default session wallet in a policy.
2. **Pay Premium:** Type `5 1000 50`. This deducts 1,000 AHT and applies the 50 AHT fee.
3. **Check Mempool:** Type `8` to view the mempool. You will see the transaction automatically split into a 950 AHT primary payment and a 50 AHT reinsurance contribution.
4. **Mine Block:** Type `9` to execute Proof-of-Work, confirm the transactions, generate UTXOs, and claim the 50 AHT block reward.
5. **Submit Claim:** Type `6 PROVIDER_ADDRESS 1500 POL_123` to request a medical settlement.
6. **Settle Claim:** Type `7 TX_ID PROVIDER_ADDRESS 1500`. Because the amount exceeds 1,000 AHT, the system will programmatically split the payout between the primary insurance pool and the reinsurance pool.
7. **Verify Chain:** Type `12` to run a deep cryptographic audit of every block, hash, and signature in the chain's history.

---

## 📁 Project Structure

| File | Description |
|------|-------------|
| `core.h` | Core data structures (Block, Transaction, ChainState). |
| `crypto.c / .h` | OpenSSL wrappers for SHA-256 hashing and ECDSA digital signatures. |
| `state.c / .h` | The Hybrid Ledger Engine managing Account balances, nonces, and UTXO sets. |
| `mempool.c / .h` | Priority queue for unconfirmed transactions and reinsurance split logic. |
| `chain.c / .h` | Merkle tree construction, PoW mining loops, and difficulty retargeting. |
| `insurance.c / .h` | Policy lifecycles, claim settlement splits, and the fraud detection pipeline. |
| `persistence.c / .h` | Binary file I/O for saving/loading state and the cryptographic verification loop. |
| `main.c` | The interactive Command-Line Interface and routing logic. |

---

## ⚠️ Security Notes

> **DO NOT COMMIT PRIVATE KEYS OR COMPILED BINARIES TO VERSION CONTROL.**

This repository includes a strict `.gitignore` file. Ensure it is active to prevent the accidental upload of `.pem` key files, `.bin` database files, or the `health_chain` executable. Private keys must remain strictly local.
