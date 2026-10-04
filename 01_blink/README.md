# Stage 1: Native ESP-IDF GPIO Driver & FreeRTOS Baseline (`01_blink`)

An industrial-grade C-language implementation transitioning from Arduino/C++ rapid prototyping (`main.cpp`) to native Espressif IoT Development Framework (ESP-IDF) drivers (`driver/gpio.h`) and FreeRTOS task management on ESP32.

## Key Features
- **Native Driver Integration**: Direct usage of ESP-IDF HAL/driver layer (`gpio_reset_pin`, `gpio_set_direction`, `gpio_set_level`) bypassing high-overhead Arduino wrappers.
- **Non-blocking RTOS Execution**: Replaced blocking delays with `vTaskDelay()` and `pdMS_TO_TICKS()` to yield CPU core control back to the FreeRTOS scheduler.
- **Structured Logging**: Implemented Espressif system logger (`ESP_LOGI`) with static tag filtering for deterministic runtime diagnostics.
- **CMake Build System Integration**: Clean separation of source code (`main/main.c`) and build targets managed via modular CMake configuration.

---

## Hardware Setup

### Bill of Materials (BOM)
- **Microcontroller**: ESP-32 Development Board (ESP-WROOM-32)
- **Indicator**: Onboard User LED

### Pin Mapping
| ESP32 Pin | Peripheral / Module | Mode | Active Level | Description |
| :--- | :--- | :--- | :--- | :--- |
| `GPIO_NUM_2` | Onboard LED | Output | High (`1`) | Heartbeat indicator toggled via main task |

---

## System Design & Execution Flow

### Execution Flowchart
```text
[ ESP-IDF Bootloader ]
         |
         v
    [ app_main() ] 
         |
         +--> [ GPIO Initialization ] (Reset & set direction to OUTPUT)
         |
         v
    [ Infinite Task Loop ]
         |
         +--> Set GPIO High  --> ESP_LOGI("LED ON")
         +--> vTaskDelay(1000ms)  (Yield CPU to Scheduler)
         +--> Set GPIO Low   --> ESP_LOGI("LED OFF")
         +--> vTaskDelay(1000ms)  (Yield CPU to Scheduler)

FreeRTOS Task Architecture
Task Context: Runs under the system-created main task context managed by the FreeRTOS scheduler.

Priority Level: Default app_main priority (1).

Memory Allocation: Configured via sdkconfig default stack size (4096 bytes).

Scheduler Interaction: Calls vTaskDelay() to trigger a context switch, preventing Watchdog Timer (WDT) starvation.

Software Stack & Toolchain
Target Chip: ESP32 (xtensa-esp32)

SDK / Framework: ESP-IDF v6.x / v5.x

Language Standard: C11 (Native C Runtime)

Build System: CMake (v3.16+) & Ninja

Compiler: xtensa-esp32-elf-gcc

IDE: Visual Studio Code with ESP-IDF Extension

Quick Start & Build Guide
1. Prerequisite
Ensure ESP-IDF environment variables are properly exported in your terminal.

2. Compilation and Flashing via CLI
Navigate into the 01_blink directory and execute the unified idf.py chain:

# Navigate to the project folder
cd 01_blink

# Set target microcontroller (required for initial build)
idf.py set-target esp32

# Build, flash at 115200 baud rate, and launch serial monitor
idf.py -p COM3 -b 115200 flash monitor

(Note: Replace COM3 with your actual serial port identified in Device Manager).

3. Exit Serial Monitor
Press Ctrl + ] to terminate the idf.py monitor process.

Engineering Challenges & Debugging
Challenge 1: Flash Connection Failure (CMakeFiles/flash.util)
Issue: esptool threw Unable to verify flash chip connection (No more data to read from the serial port) during high-speed firmware writing.

Root Cause: High default flashing baud rates (460800 / 921600 bps) caused signal corruption and voltage drops across USB-to-UART bridge hardware during SPI Flash activation.

Solution: Explicitly set the baud rate limit to 115200 bps using the -b flag (idf.py -p COMx -b 115200 flash) and held down the BOOT button during hardware handshake initialization.

Challenge 2: VS Code C/C++ IntelliSense Path Resolution Failure
Issue: Editor displayed red squiggly error lines under system headers (cannot open source file "freertos/FreeRTOS.h").

Root Cause: Opening the parent root directory (esp32_demo) prevented the C/C++ Extension from indexing component include paths generated in 01_blink/build/.

Solution: Isolated workspace context by opening the 01_blink subdirectory directly in VS Code (File -> Open Folder), triggering automatic CMake header path generation.

Challenge 3: FreeRTOS Header Ordering Dependency
Issue: Compilation errors regarding missing kernel macros when importing freertos/task.h.

Root Cause: FreeRTOS architecture requires core kernel structures defined in FreeRTOS.h to be parsed before any subsystem headers.

Solution: Enforced strict inclusion order in main.c:

#include "freertos/FreeRTOS.h" // Must precede task.h
#include "freertos/task.h"

License & Author Info
License: MIT License

Author: Xiquan Liang 

Focus: Embedded Systems, IoT Engineering & FreeRTOS Development

GitHub: [https://github.com/whitelirliang-blip]