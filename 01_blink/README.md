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