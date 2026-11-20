# DCS - Dryer Control System Firmware

A low-cost 3D filament dryer made from a repurposed old built-in convection oven. The project uses ESP-IDF and FreeRTOS and targets the ESP32-C3 (but it can target any other chip—even an RP2040 or RP2350 with a few modifications—since most of the code is cross-platform).  

## Requirements

- ESP-IDF 5.5.x (the project is configured for ESP-IDF 5.5.4)
- An ESP32-C3 development board (a SEEED STUDIO XIAO ESP32-C3 was used)
- An USB cable and the serial device/port for the board
- For the graphical workflow: Visual Studio Code with the Espressif ESP-IDF extension
- CONFIG_FREERTOS_HZ *MUST* be set to 1000 as in the provided sdkconfig.defaults 

ESP-IDF and its toolchain must already be installed and available in your environment. The instructions below cover building, flashing, and monitoring the firmware.

## Get the source

Clone the DCS repository, then move into the firmware directory:

```sh
git clone https://github.com/dcox69/dcs
cd dcs/firmware
```

Open the `firmware` directory as the project folder in VS Code, or use it as the working directory in an ESP-IDF terminal.

All project components are included in the repository under `components/`; no additional project libraries need to be cloned.

## Build, flash, and monitor in VS Code

1. Open the `firmware` directory in VS Code.
2. Make sure the ESP-IDF extension is using your local ESP-IDF 5.5.x installation and that the target is set to **ESP32-C3**. The project defaults to `esp32c3` in `sdkconfig.defaults`.
3. Connect the board and select its serial port in the ESP-IDF extension. The port varies by computer and board.
4. Click **Build Project** in the ESP-IDF status bar (or run the extension's **Build your project** command).
5. To program the board and view its serial output, click **Build, Flash & Monitor**. You can also use the extension's separate Flash and Monitor actions.

The first build creates the `build/` directory and generates the local `sdkconfig` from `sdkconfig.defaults`. These files are build- and machine-specific and are not required in a fresh checkout. If VS Code does not recognize the project, select the `firmware` directory as the ESP-IDF project directory and run the build again.

The checked-in VS Code settings may contain paths or a serial port specific to another computer. Update the ESP-IDF installation and port in the extension settings if they do not match your setup.

## Build, flash, and monitor from a terminal

Open a terminal where the ESP-IDF environment is available, then change to the firmware project directory:

```sh
cd dcs/firmware
```

For a fresh checkout, set the target once:

```sh
idf.py set-target esp32c3
```

Build the project:

```sh
idf.py build
```

Flash the connected board and start the serial monitor. Replace `PORT` with the serial port for your board, for example `/dev/ttyUSB0` on Linux or `COM3` on Windows. On macOS, it is typically a path under `/dev/cu.*`.

```sh
idf.py -p PORT flash monitor
```

You can also run the operations separately:

```sh
idf.py -p PORT flash
idf.py -p PORT monitor
```

Press `Ctrl+]` in the monitor to exit. If your system has only one connected serial device, ESP-IDF may be able to detect the port automatically, so `-p PORT` can be omitted.

## Project configuration

- `sdkconfig.defaults` contains the project defaults, including the ESP32-C3 target and flash/partition settings.
- `sdkconfig` is generated locally by ESP-IDF and is not part of a clean checkout. Do not commit it; change project-wide defaults in `sdkconfig.defaults` instead.
- `build/` contains generated build files and does not need to be cloned or copied.

The project updates `version.txt` during CMake configuration. As a result, configuring or building the project changes the version value in that file.
