# esp32-espidf-freertos-gpio-baseline

A native C99/GNU99 execution baseline on ESP32 transitioning from high-overhead Arduino/C++ wrappers to direct ESP-IDF drivers (`driver/gpio.h`). Implements non-blocking heartbeat task scheduling via FreeRTOS (`vTaskDelay`) to guarantee deterministic CPU yields and prevent Task Watchdog Timer (TWDT) starvation.

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
| Status Indicator | Onboard User LED | GPIO Digital Output | 1 | Heartbeat indicator toggled via `app_main` task |
| Debug Interface | USB-to-UART Bridge (CH340 / CP2102) | UART0 Protocol | 1 | Physical log streaming interface for `ESP_LOGI` |

### Pinout & Electrical Contract
| ESP32 Pin | Peripheral / Module | Direction | Logic Mode / Protocol | Hardware Pull | Description & Operational Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `GPIO_NUM_2` | Onboard LED | Output | Push-Pull (GPIO_MODE_OUTPUT) | Default Internal Pull-down | Active HIGH (1 = ON, 0 = OFF); Strapping Pin (must not be externally pulled HIGH during power-up) |
| `GPIO_NUM_1` | Serial Console TX | Output | UART TX (115200 8N1) | None | Log streaming output channel mapped to UART0 via `ESP_LOGI` |
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

### Target Build Specification

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

## Quick Start & Build Instructions

### System Prerequisites & Environment Setup

Before beginning the build process, ensure that your host development environment meets the following version requirements:

| Tool / Dependency | Required Version | Verification Command | Expected Output |
| --- | --- | --- | --- |
| **ESP-IDF** | `v5.1.2` (or `v5.x` compatible) | `idf.py --version` | `ESP-IDF v5.1.2` |
| **CMake** | `^3.16.0` | `cmake --version` | `cmake version 3.16.x` or higher |
| **Ninja** | `v1.10.0+` | `ninja --version` | `1.10.0` or higher |
| **Python** | `3.8+` | `python --version` | `Python 3.x.x` |

---

### Step-by-Step Build & Deployment Workflow

#### Step 1: Repository Ingestion

Clone the target repository and switch to the main branch:

```bash
git clone [https://github.com/whitelirliang-blip/esp32-espidf-freertos-gpio-baseline.git](https://github.com/whitelirliang-blip/esp32-espidf-freertos-gpio-baseline.git)
cd esp32-espidf-freertos-gpio-baseline

```

#### Step 2: Target Selection & Environment Export

Load the ESP-IDF environment variables and explicitly set the target chip to the ESP32:

```bash
# Linux / WSL Environment Export
. $HOME/esp/esp-idf/export.sh

# Set Target Microcontroller (Generates a fresh sdkconfig)
idf.py set-target esp32

```

#### Step 3: Clean Compilation & ELF Generation

Clear the cache and trigger a full parallel build:

```bash
# Clean the artifact tree and trigger a build:
idf.py fullclean
idf.py build

```

> **Verification of Success**: Once compilation is complete, the `build/` directory must contain three core firmware files: `bootloader.bin`, `partition-table/partition-table.bin`, and the main application binary, `esp32-espidf-freertos-gpio-baseline.bin`.

#### Step 4: Flashing & Real-time Console Diagnostics

Connect your ESP32 development board (ensure the onboard CP2102/CH340 serial interface is detected), then execute the command to flash the firmware and launch the serial monitor:

```bash
# Replace /dev/ttyUSB0 (Linux) or COMx (Windows) with your actual port
idf.py -p /dev/ttyUSB0 -b 460800 flash monitor

```

> **Pro-Tip**: To exit the `idf.py monitor` interface, use the keyboard shortcut `Ctrl + ]`.

---

### Minimalist Verification Steps

After launching `idf.py monitor`, observe the serial output and the onboard hardware to verify that the system is operating as expected:

1. **BOOT Phase Assertions**:
* **Expected Behavior**: The bootloader should display the system clock frequency (240MHz), the flash memory size (4MB), and the FreeRTOS startup sequence.


2. **Application-Layer Periodic Diagnostic Assertion**:
* **Expected UART Log Output (115200 Baud 8N1)**:



```text
I (314) main: Starting esp32-espidf-freertos-gpio-baseline...
I (324) main: GPIO_NUM_2 initialized as Push-Pull Output.
I (1324) BLINK: LED ON
I (2324) BLINK: LED OFF
I (3324) BLINK: LED ON

```

* **Success Criteria**: The increment between consecutive log timestamps must be exactly `1000ms` (allowing for a kernel tick margin of error of $\pm 10\text{ms}$), and the onboard blue LED (connected to `GPIO_NUM_2`) must blink synchronously at a frequency of 1Hz.

## Engineering Challenges & Debugging

### Matrix of Intercepted Failure Modes

| Hazard / Symptom | Root Cause (System/Kernel Level) | Mitigation & Fix Strategy | Structural Impact |
| --- | --- | --- | --- |
| **System Crash / Reset Loop**<br>

<br>`Task watchdog got triggered (TWDT)` | An infinite loop (`while(1)`) in `app_main` lacks a kernel-blocking API call, preventing the low-priority `IDLE` task from executing and, consequently, failing to "feed" (reset) the watchdog timer. | Use `vTaskDelay(pdMS_TO_TICKS(1000))` to explicitly yield CPU control, transitioning the task into the FreeRTOS `Blocked` state. | Ensures deterministic time-slice allocation by the scheduler; eliminates TWDT-triggered resets. |
| **Boot Mode Corruption**<br>

<br>ESP32 hangs in the bootloader or outputs garbled text upon power-up | `GPIO_NUM_2` functions as a "strapping pin"; external circuitry or strong pull-up/pull-down interference during power-on reset causes the chip to enter an incorrect boot/programming mode. | Ensure any external LED is connected in series with a current-limiting resistor of $\ge 1\text{k}\Omega$; avoid forcing the pin to a high state during the boot sequence. Include a warning in the pinout specification. | Guarantees that the chip’s power-on boot logic consistently defaults to the intended SPI Flash startup mode. |
| **Garbage Characters in UART Log**<br>

<br>UART output contains garbled or nonsensical characters | Mismatch in baud rate configuration; or a conflict between the baud rate of the host's monitor port (115200 8N1) and the initial baud rate of the chip's bootloader (74880/115200). | Explicitly set `CONFIG_ESP_CONSOLE_UART_BAUDRATE=115200` in `sdkconfig`, and consistently specify `-b 115200` in the CLI. | Establish a reliable and deterministic physical-layer communication channel for diagnostics. |

---

### Deep Dive: Task Watchdog Timer (TWDT) Starvation Analysis

#### 1. Defective Implementation

```c
// BAD PRACTICE: CPU Starvation & Non-deterministic Timing
void app_main(void) {
    gpio_reset_pin(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(GPIO_NUM_2, 1);
        // ❌ DEADLY BUG: Magic-number busy-waiting loop locks CPU Core 0, starving the IDLE task
        for (volatile uint32_t i = 0; i < 10000000; i++);
        gpio_set_level(GPIO_NUM_2, 0);
        for (volatile uint32_t i = 0; i < 10000000; i++);
    }
}

```

#### 2. Crash Log Diagnostic

```text
E (5320) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
E (5320) task_wdt: - main (CPU 0)
E (5320) task_wdt: Tasks currently running:
E (5320) task_wdt: CPU 0: main
E (5320) task_wdt: Aborting program.
Guru Meditation Error: Core 0 panic'ed (Interrupt wdt timeout on CPU0).

```

#### 3. Root Cause & Architectural Fix

* **Root Cause Analysis**: FreeRTOS is a preemptive RTOS. The `app_main` task has a default priority of `1`, while the watchdog "feeding" (resetting) mechanism relies on the `IDLE0` task, which runs at priority `0`. Busy-waiting drives CPU usage to 100%, preventing the `IDLE0` task from ever executing. This triggers a watchdog timeout (5000ms), forcing the system to panic and crash.
* **Refactoring Strategy**: Use an API to explicitly trigger a context switch:

```c
// GOOD PRACTICE: Deterministic Kernel Yielding
while (1) {
    gpio_set_level(GPIO_NUM_2, 1);
    ESP_LOGI(TAG, "LED ON");
    // ✅ Moves app_main to the Blocked list; yields execution to the IDLE task
    vTaskDelay(pdMS_TO_TICKS(1000));

    gpio_set_level(GPIO_NUM_2, 0);
    ESP_LOGI(TAG, "LED OFF");
    vTaskDelay(pdMS_TO_TICKS(1000));
}

```

---

### Minimalist Verification Steps (Troubleshooting & Validation)

After addressing the aforementioned pitfalls, follow these steps to verify system stability:

1. **Long-Run Stability Test**:
* Keep `idf.py monitor` running for at least 30 minutes.


2. **Log-Based Verification**:
* Ensure that no `task_wdt` or "Guru Meditation Error" messages appear in the UART output. Check the log timestamps to confirm that `LED ON` and `LED OFF` are output consistently every second; this indicates that the kernel scheduler has reached a stable, deterministic state.



## License & Author Information

### Software License & Open-Source Compliance

This project is licensed and distributed under the **MIT Open-Source License**.

```text
Copyright (c) 2026 Xiquan Liang 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

```

#### Third-Party Dependency & Component Attribution

Under the premise of complying with the MIT license, this project relies on and integrates the following upstream open source components:

| Component / Layer | Licensing Standard | Copyright Holder / Origin | Compliance Obligations & Notes |
| --- | --- | --- | --- |
| **ESP-IDF Framework** | Apache License 2.0 | Espressif Systems (Shanghai) Co., Ltd. | Includes Espressif HAL drivers and register mappings. |
| **FreeRTOS Kernel** | MIT License | Amazon.com, Inc. or its affiliates | Espressif port of FreeRTOS kernel (`v10.4.3-LTS`). |

---

### Author Provenance & Maintainer Contact

* **Lead Architect / Primary Author**: Xiquan Liang (Liang Xiquan)
* **Academic/Professional Role**: Undergraduate Student in IoT Engineering
* **Focus**: Embedded Systems, Real-Time Operating Systems (RTOS), & Edge IoT Infrastructure
* **Repository Context**: Baseline reference implementation for deterministic GPIO pin control and FreeRTOS execution contexts (`esp32-espidf-freertos-gpio-baseline`).

#### Official Channels & Code Repositories

* **GitHub Repository**: [`whitelirliang-blip/esp32_demo`](https://github.com/whitelirliang-blip/esp32_demo)
* **Issue Tracker**: Report architectural bugs or API compliance issues via GitHub Issues.

---

### Minimalist Verification Steps (Compliance and Metadata Validation)

1. **License and Copyright Notice Consistency Check**:
* Verify the presence of a standard `LICENSE` file in the root directory.
* **Verification Command**:

```bash
head -n 5 LICENSE

```

* **Success Criteria**: The output must include the `Copyright (c) 2026 Xiquan Liang` statement and the MIT license header information.