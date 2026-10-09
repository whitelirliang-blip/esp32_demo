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