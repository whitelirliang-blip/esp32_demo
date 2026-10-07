# esp32-espidf-freertos-gpio-baseline

A native C11 execution baseline on ESP32 transitioning from high-overhead Arduino/C++ wrappers to direct ESP-IDF drivers (`driver/gpio.h`). Implements non-blocking heartbeat task scheduling via FreeRTOS (`vTaskDelay`) to guarantee deterministic CPU yields and prevent Task Watchdog Timer (TWDT) starvation.

## Key Features

- **Native Driver Integration**: Bypasses high-overhead Arduino abstractions in favor of direct ESP-IDF driver layer (`driver/gpio.h`). Configures pin modes, directional control, and logic levels directly to reduce cycle latency and RAM footprint.
- **Deterministic RTOS Task Yielding**: Replaced blocking delay loops with FreeRTOS `vTaskDelay()` and `pdMS_TO_TICKS()` macro conversions. Forces the heartbeat task into the `Blocked` state, allowing the FreeRTOS scheduler to execute lower-priority tasks and reset the Task Watchdog Timer (TWDT).
- **Structured System Observability**: Implements Espressif system logger (`ESP_LOGI`) using static string tags (`TAG = "main"`) for thread-safe, runtime-filterable diagnostics over UART at 115200 baud.
- **Modular CMake Target Build**: Fully integrated with the native ESP-IDF CMake target management toolchain, enforcing clear separation between application components (`main/main.c`) and build artifacts.