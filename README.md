# CYD Display — configurable ESP32 dashboards

Turn a supported ESP32 touchscreen board into a dashboard for Home Assistant,
weather, network availability and household power. Screen layouts are JSON
files: change the layout and reload it from the board's web interface without
recompiling the firmware.

The current firmware is **dashIDE002.2**, prepared for **Arduino IDE 2.x** on
Windows and Linux. A Python selector configures one of four hardware profiles,
using either a graphical interface or the command line.

## Example result

One of the finished dashboards, using the `simple.v6.json` layout:

![CYD Display running the simple.v5 dashboard](https://raw.githubusercontent.com/MarsWalker/CYD_Display/main/images/simple.v5.jpg)

## Repository guide

| Location | Contents |
| --- | --- |
| [dashIDE002.2/](dashIDE002.2/) | Firmware, hardware profiles and selection tools. |
| [Firmware setup guide](dashIDE002.2/README.md) | Detailed installation, IDE settings, CLI builds and validation notes. |
| [dashboards/](https://github.com/MarsWalker/CYD_Display/tree/main/dashboards) | Hosted JSON layouts and their data-source documentation. |

Dashboard files are maintained separately from the firmware folder. The
configuration template selects `simple.v6.json` from this repository's
`dashboards/` directory by default.

## Features

- Standalone and multi-screen JSON dashboards, including external screen files.
- Home Assistant entities and configurable weather HTTP/JSON sources.
- Touch buttons for Home Assistant lights and smart sockets, with live ON/OFF state.
- ICMP ping checks with `OK`, `FAIL` and initial `--` status.
- A five-minute household power graph with automatic scaling and configurable colors.
- Built-in bitmap icons, text, sensor state icons, clock, date and device values.
- Touch navigation and a local web interface for screen selection, JSON reload and restart.
- Wi-Fi hostname and mDNS access at `http://<hostname>.local/` where supported.
- Optional SD configuration overrides, local layout files and rotating logs.
- Hardware configurations kept inside the sketch; no global TFT_eSPI setup edits.

## Supported hardware profiles

Select the profile matching **both the controllers and the board pinout**.
A matching display controller alone does not guarantee compatibility with a
third-party board. Pin definitions are included in the firmware's hardware setups.

| Public profile | Familiar aliases | Display and touch | Arduino IDE board |
| --- | --- | --- | --- |
| `esp32_ili9341_xpt2046` | `amarela2.8` | ILI9341, XPT2046 resistive touch | ESP32 Dev Module |
| `esp32_st7789_parallel_cst820` | `amarela2.2` | ST7789 parallel, CST820 capacitive touch | ESP32 Dev Module |
| `esp32_ili9341_cap_touch` | `pink` | ILI9341, FT6x36-compatible capacitive touch | ESP32 Dev Module |
| `esp32s3_axs15231b_qspi` | `black`, `preta` | AXS15231B QSPI display and touch | ESP32S3 Dev Module |

Public identifiers and aliases select the same hardware. Selection is manual;
the selector does not identify boards by USB. The default profile is
`esp32_ili9341_xpt2046`.

## Quick start with Arduino IDE

### 1. Install the tools

Install Arduino IDE 2.x, Python 3 and **esp32 by Espressif Systems 3.3.7**.
If necessary, add this Boards Manager URL in Arduino IDE Preferences:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Install the libraries used by your hardware profile in your Arduino sketchbook:

| Library | Version used | Required for |
| --- | --- | --- |
| ArduinoJson | 7.4.3 | All profiles |
| TFT_eSPI | 2.5.44 | Both ILI9341 profiles and the AXS15231B profile |
| XPT2046_Touchscreen | 1.4 | XPT2046 profile |
| LovyanGFX | 1.2.0 | ST7789 parallel profile |

These are the versions used for validation. Wi-Fi, WebServer, SD, mDNS and
other ESP32 platform components come with the ESP32 core. LVGL is not required.
Libraries are not bundled with the firmware folder.

**Do not replace TFT_eSPI's global `User_Setup.h` or `User_Setup_Select.h`.**
The sketch supplies its own configuration through `build_opt.h`, `ide_build.h`
and `hardware/User_Setups/`. Keep these files together with the sketch.

### 2. Create your private configuration

Open a terminal in `dashIDE002.2` and copy the template.

Linux:

```bash
cp secrets_template.h secrets.h
```

Windows Command Prompt:

```bat
copy secrets_template.h secrets.h
```

Edit the new `secrets.h` and fill in `WIFI_SSID`, `WIFI_PASSWORD`, `HA_URL` and
`HA_TOKEN`. Use a Home Assistant long-lived access token if your layout reads
HA entities. `JSON_URL` is the base URL for layouts; `JSON_FILE` selects the
initial board layout. `HOSTNAME` sets the device name.

The public defaults are:

```cpp
const char* JSON_URL = "https://raw.githubusercontent.com/MarsWalker/CYD_Display/main/dashboards/";
const char* JSON_FILE = "simple.v6.json";
const char* HOSTNAME = "cyd-dashboard";
```

Adjust example entity IDs, hosts and weather URLs in the selected layout for
your installation. Telegram fields are reserved; Telegram alerts are not implemented.
Keep `secrets.h` and SD `/secrets.json` private. Publish only the template.

### 3. Select the hardware

Save and close an already-open sketch before applying a different profile.
Run the selector from the firmware folder.

Linux:

```bash
python3 select_board_gui.py
```

Alternatively, run `./select_board_gui` when executable permissions are preserved.
On Debian/Ubuntu, install `python3-tk` if Tkinter is missing:

```bash
sudo apt install python3-tk
```

Windows: double-click **`select_board_gui.cmd`**, or run:

```bat
py -3 select_board_gui.py
```

Use Python 3 with Tcl/Tk support. No pip GUI packages are needed.

In the window:

1. Choose the hardware profile. Aliases appear beside its public name.
2. Check the prefilled **Arduino IDE executable** path. Edit it or use **Browse...** if necessary; the file picker starts in the current executable's folder when available.
3. Click **Save and Execute IDE** to save the selection and open `dashIDE002.2.ino`.
4. Configure the Arduino IDE Tools options shown by the selector.

**Apply selection** saves without opening the IDE. **Copy IDE settings** copies
the required menu options. Opening the window or previewing profiles does not
save changes. The executable override lasts for the current selector session;
`ARDUINO_IDE` can also specify an executable path in the environment.

The selector does not change IDE menus, choose a serial port, compile or upload.
If launching the IDE fails, the hardware selection remains saved.

For a terminal-only workflow, choose one profile, for example:

```bash
python3 select_board.py esp32_ili9341_xpt2046
```

On Windows use `py -3 select_board.py esp32_ili9341_xpt2046`.
Then open `dashIDE002.2.ino` manually. The detailed firmware guide includes
all profiles and the shell/Windows wrappers.

### 4. Configure Tools, compile and upload

| Profile | Tools settings |
| --- | --- |
| All three ESP32 profiles | Board: ESP32 Dev Module; Flash Size: 4MB; PSRAM: Disabled; Partition Scheme: Default 4MB with spiffs |
| ESP32-S3 AXS15231B profile | Board: ESP32S3 Dev Module; Flash Size: 16MB; PSRAM: OPI PSRAM; USB CDC On Boot: Enabled; Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS) |

Choose the board's serial port, then use **Verify** and **Upload** normally.
Serial output uses **115200 baud**. No extra compiler flags need to be entered
manually in the IDE. Keep the folder name `dashIDE002.2` matching its `.ino` file.

The selector prints the matching FQBN for Arduino CLI users. CLI build helpers
are also included; see the [firmware setup guide](dashIDE002.2/README.md) for
their library directory requirements and profile options.

## Dashboard JSON

The firmware accepts a board containing `DataSources` and `Screens`, screens
embedded inside that board, or a standalone screen with root-level `Objects`.
A `Screens` entry can reference an external file through `name`.
Refresh intervals are in **milliseconds**. Coordinates and dimensions are in
pixels; layouts do not automatically scale between displays.

Example standalone board with one ping status:

```json
{
  "TIMEZONE": "Europe/Lisbon",
  "DataSources": [
    { "Type": "Ping", "Id": "router", "Host": "192.168.1.1", "Refresh": 10000 }
  ],
  "Screens": [
    {
      "Number": 1,
      "Rotation": 1,
      "Refresh": 1000,
      "Objects": [
        { "Type": "FillScreen", "Color": "Black" },
        { "Type": "DrawText", "X": 10, "Y": 10, "Font": 2, "Color": "White", "Text": "function:Clock" },
        { "Type": "DrawText", "X": 10, "Y": 40, "Font": 2, "Color": "White", "Prefix": "Router: ", "Text": "ping:router" }
      ]
    }
  ]
}
```

### Data sources and display values

| Source | Configuration | Use on screen |
| --- | --- | --- |
| `HA` | `Id`, `Entities`, `Refresh` | `DrawHASensor` with entity state mappings |
| `Weather` | `Id`, `url`, `Map`, optional `Codes`, `Refresh` | `datasource:<id>:<field>` |
| `Ping` | `Id`, `Host`, `Refresh` | `ping:<id>` in text or boolean expressions |
| `Power` | `Id`, `Entity`, `Unit` (`W` or `kW`), `Refresh` | `DrawPowerGraph` and `power:<id>` |

Pings are sent **by the board to the configured hosts**. Status is `--` before
a result, then `OK` or `FAIL`. Up to eight Ping sources are supported.

Built-in text values include `function:Clock`, `function:Date`,
`function:Uptime`, `function:WifiRSSI`, `function:FreeRAM` and `function:MyIP`.
MQTT is not implemented. Objects include `FillScreen`, `DrawText`, `DrawImage`,
`DrawImageBool`, `DrawHASensor`, `DrawPowerGraph` and `DrawHAButton`, with up to 30 per screen.

### Household power graph

Add a source to the board's `DataSources` array, replacing the example entity
with your own signed power sensor:

```json
{
  "Type": "Power",
  "Id": "casa",
  "Entity": "sensor.house_power",
  "Unit": "W",
  "Refresh": 2000
}
```

Add these objects to the screen's `Objects` array:

```json
[
  {
    "Type": "DrawPowerGraph",
    "Source": "casa",
    "X": 10,
    "Y": 100,
    "Width": 178,
    "Height": 90,
    "Background": "0x0861",
    "PositiveColor": "Yellow",
    "NegativeColor": "Green"
  },
  {
    "Type": "DrawText",
    "X": 10,
    "Y": 196,
    "Font": 2,
    "Color": "White",
    "Prefix": "Casa: ",
    "Text": "power:casa",
    "Suffix": " W"
  }
]
```

The graph retains up to 151 samples over five minutes in RAM. It plots the
**magnitude** of power: the vertical minimum is zero, and the maximum is the
largest retained absolute value plus the greater of 5 W or 5%. Sign changes
pass through zero; negative values are green and positive values yellow by
default. The text retains the original sign. This shows power in watts,
not accumulated energy in kWh.

Each optional color independently falls back to its current default if omitted.
`Background` also accepts the aliases `background` and `backgroud`. Colors use
supported names or RGB565 strings such as `0x0861`, not CSS `#RRGGBB`.
Only one Power source is supported. It polls no faster than every two seconds;
invalid readings do not create samples. History resets on reboot or board JSON
reload and is not saved to SD/flash.

### Touch controls for lights and smart sockets

`simple.v6.json` keeps the graph dashboard on screen 1 and adds office controls
on screen 2. It demonstrates a smart socket (`switch.tomada_pc`) and a light
(`light.escritorio_work_bar_light_gledopto_gl_lb_001p`). Replace these IDs with
your own entities when adapting the layout.

Declare each controlled entity in an ordinary `HA` source, then add an object:

```json
{
  "Type": "DrawHAButton",
  "entity": "switch.office_lights",
  "X": 10,
  "Y": 65,
  "Width": 300,
  "Height": 50,
  "Text": "Office lights",
  "Font": 2,
  "Background": "0x0861",
  "OnColor": "Yellow",
  "OffColor": "DarkGrey"
}
```

The entire rectangle displays status and accepts a tap. Only a new press after
release can send another command. Supported domains are `light`, `switch` and
`input_boolean`. Unknown/offline/disabled buttons consume touches without
sending commands. HA confirms the state; a timeout is not retried automatically.
Full fields and examples: [dashboard documentation](dashboards/readme.md#drawhabutton--lights-and-smart-sockets).

## Reload, touch and web controls

After changing a hosted JSON file, open the board's web interface and choose
**Reload JSON**. A firmware rebuild is unnecessary for layout-only changes.
Touch routing gives buttons priority:

- **Inside a button:** toggle that entity; do not change screens.
- **Outside buttons, left half:** previous screen.
- **Outside buttons, right half:** next screen.

Navigation wraps around and requires root `touchEnable: true`. Object controls
remain active independently of that navigation flag. On the v6 control screen,
tap the title area or below the buttons to navigate.

| URL | Action |
| --- | --- |
| `http://<ip>/` | Screen list and controls |
| `http://<ip>/?screen=N` | Select screen number N |
| `http://<ip>/?reload=true` | Fetch and parse the board JSON again |
| `http://<ip>/?restart=true` | Restart the board |

Use `http://<hostname>.local/` as an alternative where mDNS is available.

## Optional SD configuration and logging

When supported and mounted, the SD card can provide `/secrets.json`,
`/<JSON_FILE>` and external screen files. Existing SD layout files take
precedence over network copies: check the card if a web-hosted change appears
to have no effect. Network access is still needed for remote data sources.

SD configuration overrides support `WIFI_SSID`, `WIFI_PASSWORD`, `HA_URL`,
`HA_TOKEN`, `JSON_URL`, `JSON_FILE`, `HOSTNAME` and the reserved Telegram fields.
The compile-time `secrets.h` template is still required for building.

Logs go to Serial and, when SD is mounted, `/logs/<HOSTNAME>.log`, rotating to
`.old` at approximately 50 KB. SD support and pins depend on the selected
hardware profile; SD is optional for network-hosted operation.

## Limits and troubleshooting

- **Blank display or incorrect touch:** confirm the complete board profile and pinout, not just the screen size, then reapply the selection and reopen the sketch.
- **MCU/PSRAM build error:** use the Tools settings for the selected profile, especially ESP32-S3 and OPI PSRAM for the AXS15231B board.
- **Missing Tkinter or display:** install Tcl/Tk support or use the command-line selector.
- **IDE not found:** edit the executable field, use Browse, or set `ARDUINO_IDE`.
- **Missing readings:** check Wi-Fi, HA URL/token, entity IDs and the JSON source configuration.
- **Old layout after reload:** check SD overrides, the configured URL/filename and the hosted file contents.
- **Limited application space:** the standard ESP32 profiles are close to the default application partition limit; additional features may require size work or a deliberate partition change.

The local HTTP interface has no authentication. The current HTTPS client uses
`setInsecure()` and does not validate server certificates. Account for these
limits when choosing the network and endpoints used by the board.

## Development and validation

Firmware builds passed for all four profiles using ESP32 core 3.3.7 and the
IDE build-option mechanism through Arduino CLI. Automated development tests
covered power, pings, button parsing, hit-testing, command dispatch, release
handling and the board selector. Tests and generated artifacts are excluded
from the clean `dashIDE002.2` distribution.

The user confirmed physical operation on the 2.8-inch ILI9341/XPT2046 board,
including the buttons and navigation outside them. The remaining profiles
passed compilation; that is separate from physical validation. The selector
GUI was tested on Linux; Windows desktop behavior has not been verified here.

Driver license notices and attributions are retained in their source files.
