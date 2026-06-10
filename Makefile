CC = gcc
CFLAGS = -Wall -g -O0 -D posix \
         -I. \
         -I./FreeRTOS/include \
         -I./FreeRTOS/portable/ThirdParty/GCC/Posix \
         -I./src/config \
         -I./src/common \
         -I./src/utils

# FreeRTOS Kernel + POSIX Port
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

# Your Project Files (add more later)
PROJECT_SRC = main.c src/utils/utils.c

TARGET = build/simulation

all: $(TARGET)

$(TARGET): $(FREERTOS_SRC) $(PROJECT_SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ -lpthread -lrt

clean:
	rm -rf build/

run: $(TARGET)
	./$(TARGET)

# ===========================================================================================
# PACKAGING
# ===========================================================================================

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
		   ".git/*"

	@echo "Done: $(ZIP_NAME) created successfully"