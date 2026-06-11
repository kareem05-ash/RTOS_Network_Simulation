CC = gcc
CFLAGS  = -Wall -g -O0 -D posix \
          -I. \
          -I./FreeRTOS/include \
          -I./FreeRTOS/portable/ThirdParty/GCC/Posix \
          -I./src/config \
          -I./src/common \
          -I./src/utils \
          -I./src/stats \
          -I./src/generator \
          -I./src/sender \
          -I./src/receiver \
          -I./src/link

# ── Python & venv ─────────────────────────────────────────────────────────────
VENV            = .venv
PYTHON          = $(VENV)/bin/python3
PIP             = $(VENV)/bin/pip
REQ_FILE        = scripts/requirements.txt
CHECK_DEPS      = scripts/check_deps.py

# ── Script paths ──────────────────────────────────────────────────────────────
PARSE_SCRIPT    = scripts/parse_logs.py
PLOT_TP_SCRIPT  = scripts/plot_throughput.py
PLOT_RTX_SCRIPT = scripts/plot_retransmissions.py
LOGS_DIR        = simulation_results/raw_logs
CSV_OUT         = simulation_results/csv/results.csv
PLOTS_DIR       = simulation_results/plots

# ── FreeRTOS Kernel + POSIX Port ──────────────────────────────────────────────
FREERTOS_SRC = \
	FreeRTOS/tasks.c \
	FreeRTOS/queue.c \
	FreeRTOS/timers.c \
	FreeRTOS/list.c \
	FreeRTOS/event_groups.c \
	FreeRTOS/croutine.c \
	FreeRTOS/stream_buffer.c \
	FreeRTOS/portable/ThirdParty/GCC/Posix/port.c \
	FreeRTOS/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c \
	FreeRTOS/portable/MemMang/heap_3.c

# ── Project Source Files ──────────────────────────────────────────────────────
PROJECT_SRC = \
	main.c \
	src/utils/utils.c \
	src/generator/packet_generator.c \
	src/sender/sender.c \
	src/link/comm_link.c \
	src/receiver/receiver.c \
	src/stats/statistics.c

TARGET      = build/simulation

.PHONY: all clean run help install_req \
        parse_logs plot_throughput plot_retransmissions \
        plot_all run_pipeline package

# ─────────────────────────────────────────────────────────────────────────────
# BUILD TARGETS
# ─────────────────────────────────────────────────────────────────────────────

all:
	mkdir -p build
	$(CC) $(CFLAGS) $(FREERTOS_SRC) $(PROJECT_SRC) -o $(TARGET) -lpthread -lrt

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(PROJECT_SRC) $(FREERTOS_SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(FREERTOS_SRC) $(PROJECT_SRC) -o $(TARGET) -lpthread -lrt

# ─────────────────────────────────────────────────────────────────────────────
# VENV + DEPENDENCIES
# ─────────────────────────────────────────────────────────────────────────────

$(VENV)/bin/activate:
	@echo "[install_req] Creating virtual environment in $(VENV)/ ..."
	python3 -m venv $(VENV)
	@echo "[install_req] Virtual environment ready."

install_req: $(VENV)/bin/activate
	@echo "[install_req] Installing packages from $(REQ_FILE) ..."
	$(PIP) install --upgrade pip -q
	$(PIP) install -r $(REQ_FILE)
	@echo "[install_req] All packages installed into $(VENV)/."

.check_deps:
	@test -f $(PYTHON) || { \
	    echo ""; \
	    echo "[ERROR] Virtual environment not found."; \
	    echo "        Run:  make install_req"; \
	    echo ""; \
	    exit 1; \
	}
	@$(PYTHON) $(CHECK_DEPS)

# ─────────────────────────────────────────────────────────────────────────────
# EXPERIMENT RUNNER
# ─────────────────────────────────────────────────────────────────────────────

# Builds the binary if missing, then hands off to run_experiments.sh
run_pipeline: $(TARGET)
	bash run_experiments.sh

# ─────────────────────────────────────────────────────────────────────────────
# ANALYSIS SCRIPTS
# ─────────────────────────────────────────────────────────────────────────────

parse_logs: .check_deps
	@echo "[parse_logs] Scanning $(LOGS_DIR) ..."
	@mkdir -p simulation_results/csv
	$(PYTHON) $(PARSE_SCRIPT) --logs_dir $(LOGS_DIR) --out $(CSV_OUT)

plot_throughput: .check_deps
	@echo "[plot_throughput] Generating throughput plots ..."
	@mkdir -p $(PLOTS_DIR)
	$(PYTHON) $(PLOT_TP_SCRIPT) --csv $(CSV_OUT) --out $(PLOTS_DIR)

plot_retransmissions: .check_deps
	@echo "[plot_retransmissions] Generating retransmission plots ..."
	@mkdir -p $(PLOTS_DIR)
	$(PYTHON) $(PLOT_RTX_SCRIPT) --csv $(CSV_OUT) --out $(PLOTS_DIR)

plot_all: parse_logs plot_throughput plot_retransmissions
	@echo ""
	@echo "================================================"
	@echo "  All plots saved to: $(PLOTS_DIR)/"
	@echo "================================================"

# ─────────────────────────────────────────────────────────────────────────────
# PACKAGING
# ─────────────────────────────────────────────────────────────────────────────

ZIP_NAME := RTOS_Network_Simulation.zip

package:
		@command -v zip >/dev/null 2>&1 || { \
                echo "Error: zip is not installed. Install it via:"; \
                echo "  sudo apt install zip"; \
                exit 1; \
        }

		@echo "Creating $(ZIP_NAME)..."

		@if [ -f $(ZIP_NAME) ]; then \
                echo "Removing existing $(ZIP_NAME)"; \
                rm -f $(ZIP_NAME); \
		fi

		@zip -r $(ZIP_NAME) ./ \
                -x "*__pycache__*" \
                   "build/*" \
                   "docs/*" \
                   "imgs/*" \
                   ".vscode/*" \
                   ".git/*"  \
				   "FreeRTOS/*" \
				   ".venv/*" \
				   "simulation_results/*" \
				   ".gitignore" \

        @echo "Done: $(ZIP_NAME) created successfully"

# ─────────────────────────────────────────────────────────────────────────────
# HELP
# ─────────────────────────────────────────────────────────────────────────────

help:
	@echo ""
	@echo "  RTOS Network Simulation — Makefile Targets"
	@echo "  ─────────────────────────────────────────────────────────────────"
	@printf "  %-28s %s\n" "Target" "Description"
	@echo "  ─────────────────────────────────────────────────────────────────"
	@printf "  %-28s %s\n" "make all"                  "Compile the simulation binary → build/simulation"
	@printf "  %-28s %s\n" "make run"                  "Compile (if needed) and run with current types.h config"
	@printf "  %-28s %s\n" "make clean"                "Delete the compiled binary"
	@echo "  ─────────────────────────────────────────────────────────────────"
	@printf "  %-28s %s\n" "make install_req"          "Create .venv/ and pip install scripts/requirements.txt"
	@printf "  %-28s %s\n" "make package"          	   "Create a zip file that contains {scripts/, src/, Makefile, main.c, README.md, run_experiments.sh}"
	@echo "  ─────────────────────────────────────────────────────────────────"
	@printf "  %-28s %s\n" "make run_pipeline"         "Build (if needed) then run all 16 combos via run_experiments.sh"
	@printf "  %-28s %s\n" "make parse_logs"           "Parse raw_logs/*.txt  →  csv/results.csv"
	@printf "  %-28s %s\n" "make plot_throughput"      "Fig1 & Fig2: Throughput vs Pdrop / Tout"
	@printf "  %-28s %s\n" "make plot_retransmissions" "Fig3 & Fig4: AvgTx & Dropped-after-4 bar chart"
	@printf "  %-28s %s\n" "make plot_all"             "Run all 3 scripts in order (parse → plot)"
	@echo "  ─────────────────────────────────────────────────────────────────"
	@printf "  %-28s %s\n" "make help"                 "Show this message"
	@echo ""
	@echo "  Typical workflow:"
	@echo "    1.  make install_req          # first time only"
	@echo "    2.  make run_pipeline         # build + run all 16 experiment configs"
	@echo "    3.  make plot_all             # parse logs and generate all plots"
	@echo ""