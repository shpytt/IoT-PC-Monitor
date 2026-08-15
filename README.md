# 🖥️ IoT PC Resource Monitor

Hardware-software system for remote monitoring of PC metrics (CPU, GPU, RAM) and management using an ESP8266 microcontroller and a Python backend. 

## 🚀 Features
- **Real-time Monitoring:** Displays CPU, GPU, and RAM usage dynamically on a 1.8" TFT display (ST7735).
- **Smart Logging:** Aggregates and saves telemetry data to a local SQLite database to optimize disk space, while also creating hardware-level TSV logs directly on the ESP8266 via LittleFS.
- **Web Control Panel:** Features an adaptive Dark Mode UI hosted on the microcontroller for remote PC management (clearing Temp files, locking the workstation) from any mobile device.
- **Weather Integration:** Pulls local weather data via the Open-Meteo API.

## 🛠️ Tech Stack
- **Backend (PC):** Python (`psutil`, `pynvml`, `requests`), SQLite
- **Firmware (Microcontroller):** C++ (Arduino IDE), ArduinoJson, LittleFS
- **Hardware:** ESP8266 (Wemos D1 Mini), ST7735 1.8" TFT Display
- **Network:** HTTP/JSON, Wi-Fi 2.4GHz

## ⚙️ How It Works
1. A Python script runs in the background on the PC, collecting hardware metrics using OS-level APIs and NVIDIA NVML.
2. Data is formatted as JSON and sent via HTTP POST to the ESP8266.
3. The microcontroller processes the payload, updates the SPI-driven display dynamically, and handles local data logging.
4. The device simultaneously acts as a local web server, listening for user commands to execute system operations on the PC.
