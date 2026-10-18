# esp32-espidf-freertos-gpio-baseline

A native C11 execution baseline on ESP32 transitioning from high-overhead Arduino/C++ wrappers to direct ESP-IDF drivers (`driver/gpio.h`). Implements non-blocking heartbeat task scheduling via FreeRTOS (`vTaskDelay`) to guarantee deterministic CPU yields and prevent Task Watchdog Timer (TWDT) starvation.

## Key Features

- **Native Driver Integration**: Bypasses high-overhead Arduino abstractions in favor of direct ESP-IDF driver layer (`driver/gpio.h`). Configures pin modes, directional control, and logic levels directly to reduce cycle latency and RAM footprint.
- **Deterministic RTOS Task Yielding**: Replaced blocking delay loops with FreeRTOS `vTaskDelay()` and `pdMS_TO_TICKS()` macro conversions. Forces the heartbeat task into the `Blocked` state, allowing the FreeRTOS scheduler to execute lower-priority tasks and reset the Task Watchdog Timer (TWDT).
- **Structured System Observability**: Implements Espressif system logger (`ESP_LOGI`) using static string tags (`TAG = "main"`) for thread-safe, runtime-filterable diagnostics over UART at 115200 baud.
- **Modular CMake Target Build**: Fully integrated with the native ESP-IDF CMake target management toolchain, enforcing clear separation between application components (`main/main.c`) and build artifacts.

## Hardware Architecture & Pin Mapping

### Bill of Materials (BOM)
| Component | Model / Specification | Interface / Protocol | Qty | Functional Role |
| :--- | :--- | :--- | :--- | :--- |
| Microcontroller | ESP32-WROOM-32 (Xtensa Dual-Core) | Onboard System Bus | 1 | Main execution node running FreeRTOS kernel |
| Status Indicator | Onboard User LED | GPIO Digital Output | 1 | Heartbeat indicator toggled via app_main task |
| Debug Interface | USB-to-UART Bridge (CH340) | UART0 Protocol | 1 | Physical log streaming interface for ESP_LOGI |

### Pinout & Electrical Contract
| ESP32 Pin | Peripheral / Module | Direction | Logic Mode / Protocol | Hardware Pull | Description & Operational Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `GPIO_NUM_2` | Onboard LED | Output | Push-Pull | Internal Pull-down | Active HIGH (1 = ON, 0 = OFF); Strapping Pin (must not be externally pulled HIGH during power-up) |
| `GPIO_NUM_1` | Serial Console TX | Output | UART TX (115200 8N1) | None | Log streaming output channel mapped to UART0 via ESP_LOGI |
| `GPIO_NUM_3` | Serial Console RX | Input | UART RX (115200 8N1) | None | System control & CLI input channel mapped to UART0 |

## System Design & Data Flow

### Software Execution Logic & Pipeline
```text
 [ Hardware Power-On / Reset ]
               |
               v
 [ ESP-IDF System Bootloader ]
               |
               v  (Spawns System Tasks & Calls app_main)
+-------------------------------------------------------------------+
| FreeRTOS Application Context: app_main Task                       |
| - Priority: 1                                                     |
| - Stack Allocation: 4096 bytes (Configured via sdkconfig)         |
+-------------------------------------------------------------------+
   |                                                           |
   | 1. Reset & Set Direction                                  | 2. Stream Log Diagnostics
   v                                                           v
+-----------------------+                           +-----------------------+
| driver/gpio HAL Layer |                           | esp_log Subsystem     |
+-----------------------+                           +-----------------------+
   |                                                           |
   | (Push-Pull Drive)                                         | (UART0 TX at 115200 8N1)
   v                                                           v
[ GPIO_NUM_2: Onboard LED ]                         [ Serial Monitor / Console ]
   |                                                           |
   +--- High Level (3.3V) -> LED ON  <=========================> Logs: "BLINK: LED ON"
   |    vTaskDelay(pdMS_TO_TICKS(1000)) -> Yield CPU to IDLE
   |
   +--- Low Level  (0.0V) -> LED OFF <=========================> Logs: "BLINK: LED OFF"
        vTaskDelay(pdMS_TO_TICKS(1000)) -> Yield CPU to IDLE
```

### FreeRTOS Task Architecture & Memory Footprint

| Task Name | Priority | Stack Size | Core Affinity | State Transition Mechanics |
| --- | --- | --- | --- | --- |
| `app_main` | `1` (Default) | `4096 Bytes` | `tskNO_AFFINITY` | Transitions to `Blocked` state during `vTaskDelay()`, allowing execution of background system tasks (e.g., `IDLE`, `Task WDT`). |

## Software Stack & Toolchain 

### Tiered Software Architecture 

```text
+-----------------------------------------------------------------------+
|  Application Layer                                                    |
|  - main.c (app_main entry point, LED toggle task logic)               |
+-----------------------------------------------------------------------+
                                    |
                                    v
+-----------------------------------------------------------------------+
|  Operating System & Core Services                                     |
|  - FreeRTOS (Preemptive Tick-based Scheduler, vTaskDelay, TWDT)       |
|  - Espressif Logging System (esp_log.h - Thread-safe UART Streaming)  |
+-----------------------------------------------------------------------+
                                    |
                                    v
+-----------------------------------------------------------------------+
|  Hardware Abstraction Layer / Drivers                                 |
|  - ESP-IDF HAL Driver (driver/gpio.h - Direct Register Operations)    |
|  - ROM Driver Interface (esp_rom_gpio.h)                              |
+-----------------------------------------------------------------------+
                                    |
                                    v
+-----------------------------------------------------------------------+
|  Target Hardware Platform                                             |
|  - ESP32 Dual-Core Xtensa LX6 (GPIO_NUM_2, UART0)                     |
+-----------------------------------------------------------------------+

```

### Toolchain & Dependency Matrix 

| Component / Layer | Specification / Name | Version / Standard | Functional Purpose & Dependency Justification |
| --- | --- | --- | --- |
| **Language Standard** | ANSI C (C99 Standard) | GNU99 (`-std=gnu99`) | Ensures portable, type-safe C code with fixed-width types (`stdint.h`). |
| **Build System** | CMake | `^3.16.0` | Declarative, cross-platform build orchestration via `CMakeLists.txt`. |
| **Build Generator** | Ninja | `v1.10.0+` | Fast, deterministic parallel compilation execution engine. |
| **SDK / Framework** | ESP-IDF | `v5.x` (Targeting v5.1.2) | Official Espressif framework providing FreeRTOS port and HAL drivers. |
| **Compiler / Toolchain** | `xtensa-esp32-elf-gcc` | `gcc-12.2.0` (Espressif) | Cross-compiler targeting Xtensa dual-core LX6 architecture. |
| **RTOS Kernel** | FreeRTOS (Espressif Port) | `v10.4.3-LTS` | Preemptive multitasking, deterministic time-slicing, and TWDT feeding. |
| **Flashing / Debug Tool** | `esptool.py` / `idf.py` | `v4.6+` | Serial flashing via USB-to-UART bridge at 460800 baud and UART monitor. |

### 4.3 Target Build Specification 

```cmake
# Root level CMakeLists.txt Specification
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-espidf-freertos-gpio-baseline)

```

```cmake
# main/CMakeLists.txt Specification
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_log freertos
)

```

### Verification Steps 
1. **Verify Toolchain Version Consistency**:
```bash
idf.py --version

```


* **Expected Output**: `ESP-IDF v5.1.2` (or the specific v5.x version in use).


2. **Validate the Complete Build and Flash Workflow**:
```bash
idf.py reconfigure fullclean build

```


* **Success Criteria**: The build process completes with zero warnings, and the necessary flashable `.bin` firmware files (e.g., `bootloader.bin`, `partition-table.bin`, `app.bin`) are generated.





