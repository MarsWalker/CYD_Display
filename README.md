# CYD Display Dashboard 

Example uses XPT2046

Simple_Example.v2 uses a webpage to change the screen ( http://ip?screen=2 ) will change to the second screen

This project allows you to easily configure and control generic **CYD (Cheap Yellow Display)** boards using dynamic JSON files. 
It features WiFi connectivity, local Home Assistant integration, and an interactive touch interface using the XPT2046 controller.
In the future:
 - Telegram integration
 - More boards with different touchscreens

---

## 🚀 Key Features

* **JSON-Driven Layouts:** The display layout, buttons, and images are dynamically loaded from a remote JSON file.
* **Home Assistant Integration:** Fetch and display real-time data from your local Home Assistant instance.
* **Smart Image Loading:** Support for localized micro-SD assets, remote HTTP/HTTPS JPG streaming, and embedded flash bitmaps.
* **Touch Interactive:** Built for CYD boards equipped with the XPT2046 touch controller.

---

## 📐 Dashboard & Screen Architecture

The project uses a structured JSON system located in the `/dashboards` directory to define how the CYD board behaves and what it displays.

### 1. The Main Config (`board_name.json`)
This is the entry point for your board. It contains:
* **Generic/Global Data:** Shared variables used across screens (for example, weather details).
* **Screen Settings:** Defines how many screens can be loaded, which ones they are, and how they should be loaded (e.g., screen rotation, inverted screen settings).
* **Screen List:** A list pointing to the individual screen JSON files.

### 2. Included Screen Examples
* **`mainSimple.json` (Default Screen):**
  Displays basic and essential information:
  * Current Time
  * Temperature & Humidity
  * Weather status icon
  * WiFi status indicator and local IP address
* **`HA.json` (Home Assistant Screen):**
  Integrates with your local smart home setup to display:
  * Door/Window sensors (dynamic icons showing open/closed state)
  * Occupancy/Motion sensors (dynamic icons showing detected/clear state)
* **`jpgDirect.json` (Web Image Screen):**
  Demonstrates the JPEG streaming engine by rendering a high-performance JPG directly from a web URL.

---

## 🛠️ How It Works (Configuration)

### 1. Device Hostname & JSON URL ( inside secrets.h )
The device uses its **HOSTNAME** (defined in your credentials) to know which configuration file to download. 
* If your `HOSTNAME` is set to `board_name`, the project will fetch `board_name.json` from your defined `JSON_URL`.

const char* JSON_URL = "https://raw.githubusercontent.com/MarsWalker/CYD_Display/refs/heads/main/dashboards/";

const char* HOSTNAME = "board_name"; 

### 2. Secrets Configuration
To get started, you need to configure your network and API credentials:
1. Rename the file `secrets_template.h` to `secrets.h`.
2. Open `secrets.h` and fill in your:
   * WiFi credentials (SSID and Password)
   * `JSON_URL` (Where your screen configuration JSONs are hosted)
   * Home Assistant URL and Long-Lived Access Token (Lovelace API)
   * MQTT Broker details (if using MQTT)

---

## 📚 Required Libraries

Please ensure you have the following libraries installed in your Arduino IDE:

* **XPT2046_Touchscreen** (For touch controls)
* **Time** (For timekeeping and NTP synchronization)
* **TFT_eSPI** (Main display driver)
* **PubSubClient** (For MQTT support)
* **NTPClient** (To fetch accurate network time)
* **JPEGDEC** (High-performance JPEG decoder)
* **ESP32Ping** (To test network connectivity)
* **GFX_Library_for_Arduino** (Graphics helpers)
* **ArduinoJson** (For parsing config files and Home Assistant API payloads)

---

## ⚙️ TFT_eSPI Setup (Crucial)

This project relies on the **TFT_eSPI** library, which requires a custom `User_Setup.h` file to match the CYD board pinout.

1. Download the pre-configured `User_Setup.h` zip file from Random Nerd Tutorials ( https://randomnerdtutorials.com ) :
   👉 [Download User_Setup.zip](https://github.com/RuiSantosdotme/ESP32-TFT-Touchscreen/raw/main/configs/User_Setup.zip)
2. Extract the zip file.
3. Replace the default `User_Setup.h` file inside your Arduino libraries folder:
   `Documents/Arduino/libraries/TFT_eSPI/User_Setup.h` with the downloaded one.

---
Tested on a ESP32-2432S028R with a 2.8 inch ILI9341 240×320 TFT LCD touchscreen.
---

## 🤝 Contributing & License

This project is a work in progress. Feel free to open issues, submit pull requests, or suggest improvements!
