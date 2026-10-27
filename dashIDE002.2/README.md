# dashIDE002.2 — Home Assistant touch controls

Version based on `dashIDE002.1`. The four hardware profiles,
Arduino IDE workflow and previous dashboard objects remain available.
Installed and confirmed by the user on the 2.8-inch ILI9341/XPT2046 board.
This folder is the clean source distribution; credentials, tests, build outputs
and backups are excluded.

## New object: DrawHAButton

A single rectangle both displays a Home Assistant entity's state and acts as
its button. Touching inside the rectangle toggles that entity. Touching outside
uses the existing left/right screen navigation when enabled. Object controls
work even when root `touchEnable` disables navigation. A held finger sends
only one action: release before pressing again.

```json
{
  "Type": "DrawHAButton",
  "entity": "switch.tomada_pc",
  "X": 10,
  "Y": 65,
  "Width": 300,
  "Height": 50,
  "Text": "PC Luzes",
  "Font": 2,
  "Background": "0x0861",
  "OnColor": "Yellow",
  "OffColor": "DarkGrey",
  "Enabled": true
}
```

Add the same entity to an ordinary HA source to keep its state current:

```json
{
  "Type": "HA",
  "Id": "office_controls",
  "Refresh": 2000,
  "Entities": [
    "switch.tomada_pc",
    "light.escritorio_work_bar_light_gledopto_gl_lb_001p"
  ]
}
```

| Property | Meaning / default |
| --- | --- |
| `entity` | One complete HA entity ID, up to 63 bytes; supported domains: `light`, `switch`, `input_boolean`. |
| `X`, `Y`, `Width`, `Height` | Drawing and hit-test rectangle. Default size 178×44; minimum 64×40. Must fit entirely on the display. |
| `Text`, `Font` | Label and font (default 2). Labels are shortened to fit. |
| `Background` | Tile background, default RGB565 `0x0861`. |
| `OnColor`, `OffColor` | State colors; defaults Yellow / DarkGrey. |
| `OnImage`, `OffImage` | Optional built-in image names, without `DrawImage:`. Icons must be at most 24 pixels. Default: a drawn power symbol. |
| `Enabled` | Default true. False keeps the status display but disables commands. |

Text shows `ON` / `OFF`, `--` for uninitialized/unavailable/offline status,
`...` for an unsent queued request, and `ERR` if the latest command or its
immediate state readback failed. Errors have a red border; pending uses cyan.
The last result remains until another interaction. Buttons with unknown states
are inert. The same object's rectangle consumes the touch even when disabled,
so touching it cannot unexpectedly change screens. Overlapping buttons are
hit-tested in reverse drawing order, with the last object taking precedence.

The command is a bearer-authenticated POST to
`/api/services/<entity-domain>/toggle`, followed by an authenticated state GET.
Displayed state is never inverted optimistically. Commands run in the existing
data task under its state lock, with a three-second timeout per request and
one pending command for the board. Other taps while it is pending are consumed.
There are no automatic command retries after timeouts: HA may already have
acted. An error does not prove that the device did not switch.
A fresh HA group poll follows the action to pick up delayed device changes.
Existing blocking data requests can delay touch handling and command execution.
A quick release/press occurring entirely between hardware polls may be missed.

Button fields reuse existing `ObjectInfo` buffers; no framebuffer, extra
per-object allocation or new task is introduced. Global control bookkeeping
is one small fixed structure. Release detection is shared by all four touch
backends; bus errors do not count as a confirmed release.

## Test layout: simple.v6.json

The hosted file [simple.v6.json](../dashboards/simple.v6.json) is based on hosted
`simple.v5.json`. Screen 1 retains its graph, weather and door information.
Screen 2 adds **PC Luzes** (`switch.tomada_pc`) and **Barra LED**
(`light.escritorio_work_bar_light_gledopto_gl_lb_001p`). Tap the right side of
screen 1 to reach controls; tap outside either button on screen 2 to navigate.
Replace these installation-specific IDs when reusing the layout elsewhere.

Publish the JSON manually in the repository's existing `dashboards/` folder
before using the default configuration. Both local private `secrets.h` and the
public template select `simple.v6.json`. SD overrides still take precedence.
Native control tests use mocked HTTP and never toggle physical devices.
Physical button operation and navigation were confirmed by the user.
Service API reference: [Home Assistant REST API](https://developers.home-assistant.io/docs/api/rest/).


## Validation of dashIDE002.2

All four hardware profiles compiled with ESP32 core 3.3.7, the library
versions listed in the repository setup guide and plain Arduino CLI using the sketch's IDE build options.
The final selector configuration is restored to `esp32_ili9341_xpt2046`.

| Profile | Program | Static RAM |
| --- | --- | --- |
| `esp32_ili9341_xpt2046` | 1253447 B (95%) | 98116 B (29%) |
| `esp32_ili9341_cap_touch` | 1273255 B (97%) | 98964 B (30%) |
| `esp32s3_axs15231b_qspi` | 1309266 B (41%) | 97636 B (29%) |
| `esp32_st7789_parallel_cst820` | 1282243 B (97%) | 98796 B (30%) |

Native Power/Ping regressions and new control queue/HTTP-mock/JSON/hit-test/
release-edge tests passed, along with the four selector tests, four launcher
tests and nine GUI tests. The JSON was checked for valid syntax, matching
HA entities and in-bounds control rectangles. No physical control service was
called by those automated tests. Firmware build #6 was subsequently uploaded
to the 2.8-inch board; physical operation was confirmed by the user.

## Setup and compilation

See the [repository setup guide](../README.md#quick-start-with-arduino-ide)
for Arduino IDE, Python, libraries, private configuration, board selection and
Windows/Linux instructions. Open **dashIDE002.2.ino** after selecting a profile.

Copy `secrets_template.h` to local `secrets.h` and configure it before building.
The template selects `simple.v6.json` from the hosted dashboards directory.
The original private configuration is deliberately absent from this folder.

Use `select_board_gui.py` (or the platform wrapper) to choose the board and
**Save and Execute IDE** to open the sketch. Configure Tools manually using
the settings shown by the selector. Display setups are included under
`hardware/User_Setups`; global TFT_eSPI configuration edits are unnecessary.

CLI build helpers remain available. They expect installed libraries in the
sibling `../libraries` directory. For plain Arduino CLI, configure your
sketchbook libraries, run the selector, and use the FQBN it prints.

The development test suites are not included in this clean export.
Driver license notices and attributions remain in their source files.
