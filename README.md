
# SmartClimate-I2C: Weather Station with FreeRTOS

An industrial-grade environmental monitoring system using an **STM32F103** microcontroller, running **FreeRTOS**, and reading multiple sensors over a single **I2C bus**.

## 🇬🇧 English Description
This project demonstrates advanced embedded software architecture by interfacing two distinct I2C sensors (**AHT20** and **BMP280**) on the same physical hardware lines (PB8/PB9).
*   **Operating System**: **FreeRTOS** managed with CMSIS_V2.
*   **Inter-task Communication**: Uses **Queues** to safely pass structured data (`Weather_Data_t`) from the measurement task to the display task.
*   **Fixed-Point Math**: Optimized code execution by completely avoiding `float` data types. All data processing is done using integer math to minimize CPU load.
*   **Diagnostics**: NATIVE C PC-side Data Logger included to stream, timestamp, and save raw output into a `.csv` file.

## 🇸🇪 Svensk beskrivning (Swedish)
Detta projekt demonstrerar en avancerad arkitektur för inbyggda system genom att ansluta två olika I2C-sensorer (**AHT20** och **BMP280**) på samma fysiska hårdvarulinjer (PB8/PB9).
*   **Operativsystem**: **FreeRTOS** hanterat med CMSIS_V2.
*   **Kommunikation**: Använder **köer (Queues)** för att säkert skicka strukturerad data (`Weather_Data_t`) från mätuppgiften till displayuppgiften.
*   **Fasttalsteknik**: Optimerad kodexekvering genom att helt undvika datatypen `float`. All databehandling sker med heltalsmatematik för att minimera CPU-belastningen.
*   **Diagnostik**: Inkluderar en datalogger i C för PC (Debian Linux) för att strömma, tidsstämpla och spara rådata till en `.csv`-fil.

## Hardware Used
*   **Board**: STM32F103RB (Nucleo-64)
*   **Peripherals**: AHT20 + BMP280 sensor (I2C).

## How to Build
I use a custom CMake-based build system.
1. Generate the build files:
   ```bash
   ./myCmakeGen2.sh
   ```
2. The project will compile and generate a `build/[project_name].elf` file.

## 🛠️ Hardware Setup (PB8 / PB9)
*   **VCC** ➡️ 3.3V
*   **GND** ➡️ GND
*   **SCL** ➡️ PB8 (I2C1_SCL)
*   **SDA** ➡️ PB9 (I2C1_SDA)

---

## 💻 PC Data Logger (Debian Linux)
Inside the `PC_Logger` directory, you will find a native C application that:
1. Opens `/dev/ttyACM0` at 115200 baud.
2. Reads incoming rows from the STM32.
3. Adds a precise system timestamp to each row.
4. Saves everything automatically into a `log_data.csv` file.

To compile and run:
`gcc -o logger logger.c`
`sudo ./logger`

[![Watch the video](https://img.youtube.com/vi/Wgp3kLLGPkA/0.jpg)](https://www.youtube.com/watch?v=mS1VBagZzfg)

*Developed by PeterSysEng - Electronics Technician & Embedded Systems Developer*
