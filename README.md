# FreeRTOS Stop-and-Wait Simulation

A multi-task simulation of a Stop-and-Wait (S&W) ARQ protocol over a noisy communication link, built with FreeRTOS on Eclipse CDT Embedded.

---

## 📁 Project Structure
``` 
.
├── README.md
├── build
│   └── simulation
├── docs
│   └── 91240568_91240572.docx
├── main.c
├── makefile
├── scripts
│   ├── parse_logs.py
│   ├── plot_retransmissions.py
│   ├── plot_throughput.py
│   └── requirements.txt
├── simulation_results
│   ├── csv
│   ├── plots
│   └── raw_logs
└── src
    ├── common
    │   ├── packet.c
    │   ├── packet.h
    │   └── types.h
    ├── config
    │   └── FreeRTOSConfig.h
    ├── generator
    │   ├── packet_generator.c
    │   └── packet_generator.h
    ├── link
    │   ├── comm_link.c
    │   └── comm_link.h
    ├── receiver
    │   ├── receiver.c
    │   └── receiver.h
    ├── sender
    │   ├── sender.c
    │   └── sender.h
    ├── stats
    │   ├── statistics.c
    │   └── statistics.h
    └── utils
        ├── utils.c
        └── utils.h
```
---

## 🚀 Phases Overview

### Phase 0 — Setup & Shared Foundations
- Configure Eclipse CDT Embedded with FreeRTOS emulation target.
- Clone the repo and verify the build.
- Write `types.h`: all shared constants (`L1`, `L2`, `T1`, `T2`, `C`, `D`, `K`, `P_drop[]`, `P_ack`, `Tout[]`) and packet/ACK struct definitions.
- Write `utils.c`: `rand_uniform_int()`, `rand_uniform_float()`, `get_tick_ms()` wrappers.
- Agree on global queue and semaphore handles in a shared header.

**Deliverable:** Project compiles with an empty `main()`, all types and constants defined.

---

### Phase 1 — Core Components

#### Sender Side
- **`packet_generator.c`** — Task that creates packets with correct headers, dynamically allocated, enqueued to `packet_queue`.
- **`sender.c`** — Full S&W logic:
  - Dequeue → TX buffer → start FreeRTOS software timer (`Tout`).
  - On timer expiry: retransmit up to 4 times, then `free()` and advance.
  - On ACK received: validate `seq_num`, `free()` packet, advance.
  - Timer callback posts event to sender task via queue (no logic in callback directly).

#### Link + Receiver Side
- **`comm_link.c`** — Simulates the channel in both directions:
  - Forward path: apply `P_drop`, compute delay = `D + (L×8)/C` ms, enqueue to `rx_queue`.
  - ACK return path: apply `P_ack`, compute delay, enqueue to `ack_queue`.
- **`receiver.c`** — Dequeues from `rx_queue`, sends ACK (`K=40` bytes), tracks received count, `free()`s packet.
- **`statistics.c`** — Global counters: packets/bytes received, timestamps, retransmit count, dropped-after-4 count.

---

### Phase 2 — Integration & Debugging
- Wire all tasks and queues in `src/main.c`.
- Task priorities:

| Task | Priority |
|---|---|
| Link (forward) | 3 |
| Link (ACK return) | 3 |
| Sender | 2 |
| Receiver | 2 |
| Generator | 1 |

- Run with a fixed config first (e.g., `P_drop=0.01`, `Tout=200ms`) and verify end-to-end flow.
- Log format: `[TICK][COMPONENT] message`
- Verify no memory leaks — every `malloc` must have a corresponding `free`.

**Deliverable:** Single clean simulation run that terminates correctly and prints throughput.

---

### Phase 3 — Experiments & Data Collection
- Run all **16 combinations**: 4 values of `P_drop` × 4 values of `Tout`.
- Redirect stdout to log files:
...
- Each run prints a result summary line:
RESULT: Pdrop=X Tout=Y ThroughputBytes=Z AvgTransmissions=W DroppedAfter4=V Duration_ms=U

---

### Phase 4 — Python Analysis & Plots
- **`parse_logs.py`** — Extracts `RESULT:` lines → builds a pandas DataFrame → saves to `simulation_results/csv/results.csv`.
- **`plot_throughput.py`** — Generates:
  - **Fig 1:** Throughput (bytes/sec) vs `P_drop` — one curve per `Tout` value.
  - **Fig 2:** Throughput (bytes/sec) vs `Tout` — one curve per `P_drop` value.
- **`plot_retransmissions.py`** — Generates:
  - **Fig 3:** Average transmissions per packet vs `P_drop`.
  - Prints total packets dropped after 4 attempts per config.

All plots saved as PNG to `simulation_results/plots/`.

---

### Phase 5 — Documentation
Report follows the provided `.docx` template (max 5 pages):

- **Section 1 — System Design:** Block diagram, task table, S&W logic explanation, key code snippets.
- **Section 2 — Results:** Embedded plots with interpretation.
- **Section 3 — References:** FreeRTOS docs, course slides.

---

### Phase 6 — Merging & Submission Prep
- Flatten all `.c` and `.h` files into a single `main.c` (order: `types → utils → packet → generator → link → receiver → sender → main`).
- Confirm it compiles and runs identically to the modular version.
- Name files: `StudentID1_StudentID2.docx` and `main.c`.
- Zip with relative paths and verify structure:
main.c

---

## ⚙️ Build & Run

1. Open the project in **Eclipse CDT Embedded**.
2. Confirm FreeRTOS emulation target is configured.
3. Build and run — output prints to the console.
4. To switch configurations, update `P_drop` and `Tout` via `#define` in `types.h` (or via `argv` if supported).

---

## 📊 Running the Analysis

```bash
python parse_logs.py
python plot_throughput.py
python plot_retransmissions.py
```

---
