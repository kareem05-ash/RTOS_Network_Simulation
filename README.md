# RTOS Network Simulation

---     

## Folder Structure

``` Bash
RTOS_Network_Simulation/
│
├── src/                          # Modular C source (development phase)
│   ├── config/
│   │   └── FreeRTOSConfig.h
│   ├── common/
│   │   ├── types.h               # Shared enums, constants, macros
│   │   ├── packet.h / .c         # Packet & ACK structs + malloc/free logic
│   ├── generator/
│   │   ├── packet_generator.h/.c # Task: generates packets, enqueues to packet_queue
│   ├── sender/
│   │   ├── sender.h/.c           # Task: S&W logic, TX buffer, retransmit, timer callback
│   ├── link/
│   │   ├── comm_link.h/.c        # Task: drop logic, propagation + tx delay (vTaskDelay)
│   ├── receiver/
│   │   ├── receiver.h/.c         # Task: receives packet, sends ACK
│   ├── stats/
│   │   ├── statistics.h/.c       # Global counters: throughput, drops, retransmits
│   └── utils/
│       └── utils.h/.c            # rand_uniform(), get_time_ms(), etc.
│
├── main.c                        # SUBMISSION FILE: flat merged version of all src/
│
├── scripts/                      # Python post-processing
│   ├── parse_logs.py             # Parse simulation output → CSV
│   ├── plot_throughput.py        # Throughput vs Pdrop (multi-Tout) + vs Tout (multi-Pdrop)
│   ├── plot_retransmissions.py   # Avg transmissions per packet vs Pdrop
│   └── requirements.txt          # matplotlib, pandas, numpy
│
├── simulation_results/
│   ├── raw_logs/                 # stdout/stderr dumps per (Pdrop, Tout) run
│   ├── csv/                      # Parsed structured data
│   └── plots/                    # Final PNG graphs for the report
│
├── docs/
│   └── StudentID1_StudentID2.docx
│
├── .gitignore
└── README.md
```

---     