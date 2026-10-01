# CYD Display Dashboard Firmware

This project is firmware for a **CYD (Cheap Yellow Display)** ESP32 board. It
loads one or more screen layouts from JSON, retrieves live values from Home
Assistant and weather services, and renders them on the built-in TFT. Screens
can be changed using the touchscreen or the board's local web interface.

The firmware is designed for the **ESP32-2432S028R**, with a 2.8-inch,
240 x 320 ILI9341 display and XPT2046 touch controller. Its dashboard JSON
format is shared with the companion repository
[MarsWalker/CYD_Display](https://github.com/MarsWalker/CYD_Display), which hosts
the dashboard files.

## Features

- JSON-configured standalone or multi-screen dashboards.
- Home Assistant state sensors and weather data sources, each with its own
  polling interval.
- Built-in 24 x 24 bitmap icons stored in firmware flash.
- Physical touch navigation: tap the left or right side to move between screens.
- HTTP web interface for screen selection, JSON reload and restart.
- mDNS hostname, so the board can be reached at `http://<hostname>.local/`.
- Optional SD card configuration and log files.
- Local clock, date, uptime, Wi-Fi and memory values, with a configurable
  timezone.
- Incremental rendering: text is redrawn when its visible value changes;
  `function:Clock` is refreshed on every screen draw cycle.
- Single-core and dual-core execution, selected automatically for the ESP32.

## Hardware

- ESP32-2432S028R CYD board.
- ILI9341 TFT display, 240 x 320 pixels.
- XPT2046 resistive touchscreen.
- Optional microSD card. The SD chip select is GPIO 4 (`SD_CS`).

The display and touchscreen use the pin configuration selected in the shared
`TFT_eSPI` library. Touch input uses a separate SPI bus. Keep the CYD-specific
TFT setup from the project's `../libraries` directory when building this
sketch.

## TFT_eSPI Setup (Crucial)

`TFT_eSPI` requires a `User_Setup.h` configuration that matches the CYD display
controller and pinout. Without the correct setup, the display may not initialize
or may use the wrong pins.

1. Download the preconfigured [Random Nerd Tutorials User_Setup.zip](https://github.com/RuiSantosdotme/ESP32-TFT-Touchscreen/raw/main/configs/User_Setup.zip).
2. Extract the ZIP archive.
3. Replace the default `User_Setup.h` in
   `Documents/Arduino/libraries/TFT_eSPI/` with the extracted CYD setup file.

If your Arduino libraries are stored in a different directory, replace the
`User_Setup.h` in that installation's `TFT_eSPI` folder. This project is tested
on an **ESP32-2432S028R** with a 2.8-inch ILI9341 240 x 320 TFT touchscreen.

## How the firmware starts

At startup, the firmware initializes Serial, the display, SD logging and the
configuration. It connects to Wi-Fi, loads the board JSON, configures NTP and
the timezone, then parses data sources and screens before drawing screen 1.
The web server starts after the display and Wi-Fi are ready.

The board JSON is read from the SD card first when the requested file exists;
otherwise it is fetched from `JSON_URL`. External screen files in a multi-screen
board use the same SD-first, network-second lookup. Wi-Fi is still needed for
Home Assistant, weather and web access even when the layout comes from SD.

On a dual-core ESP32, data updates run on core 0 and drawing runs on core 1,
protected by a mutex. On a single-core target, both run sequentially in the
main loop. Each data source is fetched according to its own `Refresh` value;
the screen's `Refresh` controls how often objects are evaluated for drawing.

## Configuration and credentials

Compile-time defaults are defined in `secrets.h`. For overrides without a new
firmware build, place `/secrets.json` on the SD card. Supported keys are:

| Key | Purpose |
| --- | --- |
| `WIFI_SSID`, `WIFI_PASSWORD` | Wi-Fi connection |
| `HA_URL`, `HA_TOKEN` | Home Assistant API access |
| `JSON_URL` | Base URL for board and screen JSON files |
| `JSON_FILE` | Board JSON filename, such as `simple.json` |
| `HOSTNAME` | Wi-Fi hostname and mDNS name |
| `TELEGRAM_TOKEN`, `TELEGRAM_CHAT_ID` | Reserved configuration values |

Example with placeholders:

```json
{
  "WIFI_SSID": "your-wifi",
  "WIFI_PASSWORD": "your-password",
  "HA_URL": "http://homeassistant.local:8123",
  "HA_TOKEN": "your-long-lived-token",
  "JSON_URL": "https://example.invalid/dashboards/",
  "JSON_FILE": "simple.json",
  "HOSTNAME": "board1"
}
```

Keep real credentials in the local `secrets.h` or on the SD card. Do not publish
those files or their contents. `secrets_template.h` is provided as a reference
for the configuration fields.

## Dashboard JSON

The firmware accepts three layouts:

1. **Multi-screen board:** the root contains `DataSources` and a `Screens` array.
   Each screen entry can name an external screen file.
2. **Embedded/standalone board:** `Screens` contains screen settings and its
   `Objects` array in the same JSON file.
3. **Bare screen file:** `Objects` and optional screen settings are at the root;
   the parser treats it as screen 1.

Screen settings include `Number`, `name`, `Rotation`, `Invert` and `Refresh`
(milliseconds). The board starts on screen 1. For an external screen, `name`
is appended to `JSON_URL`.

### Data sources

Data sources are declared in the root-level `DataSources` array. Every source
has a `Type`, `Id` and `Refresh` interval in milliseconds.

Weather sources use a URL, JSON field paths in `Map`, and an optional `Codes`
table mapping weather codes to labels and built-in icon names:

```json
{
  "Type": "Weather",
  "Id": "weather",
  "Refresh": 600000,
  "url": "https://weather.example/api/current",
  "Map": {
    "temperature": "current.temperature_2m",
    "humidity": "current.relative_humidity_2m",
    "pressure": "current.pressure_msl",
    "wind": "current.wind_speed_10m",
    "code": "current.weather_code"
  },
  "Codes": {
    "0": { "label": "Clear sky", "image": "sun" }
  }
}
```

Home Assistant sources query each listed entity at
`<HA_URL>/api/states/<entity>` using a bearer token. `Entities` can be an array
or a comma-separated string:

```json
{
  "Type": "HA",
  "Id": "doors",
  "Refresh": 5000,
  "Entities": ["binary_sensor.front_door", "binary_sensor.kitchen_door"]
}
```

An optional root-level `Log` object controls data-source logging:

```json
"Log": { "Weather": false, "HA": true }
```

When a type is disabled, its data and HTTP request logs are suppressed.

### Screen objects

Objects are listed in each screen's `Objects` array. The parser supports up to
30 objects per screen.

| Type | Main fields | Behavior |
| --- | --- | --- |
| `FillScreen` | `Color` | Sets the background on a full screen draw. |
| `DrawText` | `X`, `Y`, `Text`, `Font`, `Color`, `Prefix`/`Preffix`, `Suffix` | Draws plain text or a resolved function/data-source value. |
| `DrawImage` | `X`, `Y`, `Image`, `Zoom`, optional `Color` | Draws a built-in bitmap or a resolved image name. |
| `DrawImageBool` | `X`, `Y`, `Value`, `true`, `false` | Selects an image command based on a boolean value. |
| `DrawHASensor` | `IconX`, `IconY`, `entity`, `state:*`, `X`, `Y`, `Text` | Chooses an icon from one or more Home Assistant entity states and draws a label. |

Example Home Assistant object:

```json
{
  "Type": "DrawHASensor",
  "IconX": 10,
  "IconY": 10,
  "entity": "binary_sensor.front_door",
  "state:on": "DrawImage:door_open",
  "state:off": "DrawImage:door_closed",
  "else": "DrawImage:warning",
  "Font": 2,
  "Color": "White",
  "X": 40,
  "Y": 10,
  "Text": "Front door"
}
```

`DrawHASensor` also accepts multiple comma-separated entities. Mappings use
`state:<value>` keys; `else` is the fallback. `DrawImage:<name>` refers to a
built-in image. `Touch` objects and inline touch zones are currently ignored;
physical left/right navigation and the web interface are supported.

### Values and built-in images

`DrawText` can contain literal text, `function:<name>` or
`datasource:<id>:<field>`. Supported functions include:

- `Clock` (`HH:MM:SS`) and `Date` (`YYYY/MM/DD`), using the configured timezone.
- `Uptime`, `WifiRSSI`, `FreeRAM` and `MyIP`.
- `IsWIFIConnected`, `IsHAConnected` and `IsMQTTConnected` for boolean objects.
  MQTT is not implemented and currently resolves to `false`.

The optional root field `TIMEZONE` configures local time and daylight-saving
rules. Common names such as `Europe/Lisbon`, `UTC`, `Europe/London` and
`America/New_York` are recognized. The default is Lisbon time.

Built-in bitmap names include `heart`, `warning`, `home`, `lock`, `check`,
`door_open`, `door_closed`, `person_on`, `person_off`, `wifi_on`, `wifi_off`,
and weather icons such as `cloud`, `rain`, `sun`, `moon`, `cloud_sun`,
`rain_light`, `rain_heavy`, `shower`, `shower_light` and `shower_heavy`.

## Touch, web control and networking

Touch navigation follows the working XPT2046 example for this board. A tap on
the left half moves to the previous screen; a tap on the right half moves to
the next screen. Navigation wraps around the available screens. It is always
available; JSON touch-enable flags are accepted for compatibility but are not
required.

The HTTP server listens on port 80:

| URL | Action |
| --- | --- |
| `http://<ip>/` | Shows the screen list and available controls. |
| `http://<ip>/?screen=N` | Loads screen number `N`. |
| `http://<ip>/?reload=true` | Reloads the board configuration and current screen. |
| `http://<ip>/?restart=true` | Restarts the ESP32. |

The hostname is set from `HOSTNAME`; mDNS advertises the HTTP service at
`http://<hostname>.local/` where supported by the local network.

## Logging and SD card

Log messages are written to Serial. When an SD card is mounted, logs are also
stored in `/logs/<HOSTNAME>.log`; files rotate to `.old` after reaching 50 KB.
Logging is buffered and flushed periodically. Startup messages may appear on
the TFT, then TFT logging is disabled so the dashboard owns the display.

The SD card can also provide `/secrets.json`, the board JSON at `/<JSON_FILE>`,
and external screen JSON files. Missing SD support does not prevent operation
when configuration and data are available over the network.

## Source layout

| File | Responsibility |
| --- | --- |
| `dash001.1.ino` | Startup, configuration loading, screen changes and runtime hooks. |
| `comms.h` | Wi-Fi and HTTP/HTTPS GET/POST, including bearer-authenticated requests. |
| `datasources.h`, `datacache.h` | JSON data-source parsing, polling and cached values. |
| `screens.h`, `objects.h` | Screen and object parsing. |
| `draw.h`, `functions.h`, `images.h`, `colors.h` | Rendering, value resolution, icons and colors. |
| `engine.h` | Data and drawing loops with single/dual-core coordination. |
| `touch.h` | XPT2046 input and physical screen navigation. |
| `web.h` | HTTP screen control, reload and restart handlers. |
| `log.h` | Serial, TFT and SD logging. |
| `jsons/` | Sample board and screen JSON layouts. |
| `build.sh`, `upload.sh` | Linux build and upload helpers. |
| `serial_monitor.sh`, `serial_monitor.py` | Optional Linux serial monitor with reconnect support. |

## Build and upload

The project uses the shared libraries in the sibling `../libraries` directory.
Do not install a second copy for this sketch: the `TFT_eSPI` setup there is
configured for the CYD. Required libraries include `TFT_eSPI`,
`XPT2046_Touchscreen` and `ArduinoJson`; `WebServer`, Wi-Fi, SD and time support
come from the ESP32 Arduino core.

Install `arduino-cli` and the ESP32 Arduino platform, then run from the sketch
directory on Linux:

```bash
./build.sh
./upload.sh
```

`build.sh` compiles for `esp32:esp32:esp32`, uses `../libraries`, updates the
build number and writes binaries to `build_out/`. `upload.sh` flashes the
existing build using the ESP32 serial-by-id path by default. It coordinates
with the optional serial monitor so the monitor releases the serial port while
uploading.

On Windows, use `build.cmd` from the sketch directory; an optional COM port can
be supplied for compile-and-upload. Serial output uses **115200 baud**. On
Linux, `./serial_monitor.sh` starts the optional reconnecting monitor.

## Example layouts

The `jsons/` directory contains standalone, multi-screen, Home Assistant and
other example layouts, including `simple.json`, `board1.json`, `board2.json`,
`main.json`, `main2.json`, `HA.json`, `HA_Vertical.json` and `ecras2.json`.
Use these as format references; the JSON URL and filename are selected through
the local configuration.

The companion [CYD_Display repository](https://github.com/MarsWalker/CYD_Display)
contains dashboard JSON files intended to be served to the firmware.

## Project notes

`brain.md` is the development memory for this local project. It records
implementation details, operational constraints and change history. Read it
before making code changes. The `backup_AAAAMMDD_HHMMSS.tar.gz` files are local
project snapshots and are not required to build the firmware.
