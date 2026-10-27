# Dashboard data sources — dashIDE002.1

The board JSON defines **what to query** in `DataSources` and **where to display
it** in `Screens[].Objects`. Declaring a source does not create screen objects.
Implemented source types are `Weather`, `HA`, `Ping` and `Power`.

The examples below are valid JSON fragments. Place source arrays in
`DataSources` and object arrays in `Screens[].Objects`. When combining examples,
merge their elements into one array rather than repeating root keys. Key names
and capitalization must match the examples.

Example entity IDs, source IDs and private network addresses are retained from
the existing layouts. Replace entities and hosts with your own. IDs such as
`casa` and `sala` are identifiers, not translated configuration keywords.

## Board structure

Complete example with a household power graph:

```json
{
  "TIMEZONE": "Europe/Lisbon",
  "touchEnable": true,
  "Log": {"Weather": false, "HA": false, "Power": false},
  "DataSources": [
    {"Type": "Power", "Id": "casa", "Entity": "sensor.energia_pj2101a_local_power", "Unit": "W", "Refresh": 2000}
  ],
  "Screens": [
    {
      "Number": 1,
      "Rotation": 1,
      "Invert": true,
      "Refresh": 1000,
      "Objects": [
        {"Type": "FillScreen", "Color": "Black"},
        {"Type": "DrawText", "X": 10, "Y": 10, "Text": "function:Clock", "Font": 4, "Color": "White"},
        {"Type": "DrawPowerGraph", "Source": "casa", "X": 10, "Y": 100, "Width": 178, "Height": 90},
        {"Type": "DrawText", "X": 10, "Y": 196, "Text": "power:casa", "Prefix": "House:", "Suffix": "W", "Font": 2}
      ]
    }
  ]
}
```

`DataSources[].Refresh` controls polling in milliseconds; `Screens[].Refresh`
controls drawing. Network requests and other sources can increase the actual
interval. Drawing reads cached values. Sources are global to the board and
remain active when switching screens.

Positions are pixels measured from the upper-left corner, without automatic
scaling. With `Rotation: 1`, the configured ILI9341 and ST7789 profiles use
320×240, and the AXS15231B profile uses 480×320. Adjust positions when combining
examples to avoid overlaps. Display inversion must match your panel.

`simple.v5.json` replaces the ping area with a graph and the household power
value underneath. It does not declare Ping sources or the living-room sensors.

## Sources used by the example layouts

| Id | Type | Contents | Screen reference |
| --- | --- | --- | --- |
| `weather_aveiro` | Weather | Weather at the coordinates in the URL | `datasource:weather_aveiro:<field>` |
| `doors` | HA | Six doors | `DrawHASensor` with `entity` |
| `people` | HA | Four presence sensors | `DrawHASensor` with `entity` |
| `sala` | HA | Living-room temperature, humidity and illuminance | `entity:sensor...` in `DrawText` |
| `casa` | Power | Household power and history | `DrawPowerGraph` and `power:casa` |
| `server1` | Ping | `192.168.1.204` | `ping:server1` |
| `server2` | Ping | `192.168.1.75` | `ping:server2` |
| `server3` | Ping | `192.168.1.79` | `ping:server3` |

`sala` is documented for reuse and is not part of `simple.v5.json`. The three
server sources come from v4 and are also absent from v5. IDs can be renamed
provided their corresponding screen references are updated.

## Weather

### Define the source

```json
[
  {
    "Type": "Weather",
    "Id": "weather_aveiro",
    "Refresh": 600000,
    "url": "https://api.open-meteo.com/v1/forecast?latitude=41.343&longitude=-8.477&current=temperature_2m,relative_humidity_2m,pressure_msl,weather_code,wind_speed_10m",
    "Map": {
      "temperature": "current.temperature_2m",
      "humidity": "current.relative_humidity_2m",
      "pressure": "current.pressure_msl",
      "wind": "current.wind_speed_10m",
      "code": "current.weather_code"
    },
    "Codes": {
      "0": {"label": "Clear sky", "image": "sun"},
      "2": {"label": "Partly cloudy", "image": "cloud_sun"},
      "3": {"label": "Overcast", "image": "cloud"},
      "61": {"label": "Light rain", "image": "rain_light"}
    }
  }
]
```

- Lowercase `url` is the service address. Coordinates determine the location; the source ID does not.
- `Map` maps dashboard fields to dot-separated paths in the returned JSON. The firmware does not add units or convert values.
- `Codes` maps weather codes to labels and built-in `images.h` icons. This abbreviated table can be extended using [simple.v4.json](simple.v4.json).
- Unmapped codes leave the label and icon name empty. A per-code `url` property found in some layouts does not download an image.

### Display on the board

```json
[
  {"Type": "DrawImage", "X": 10, "Y": 70, "Image": "datasource:weather_aveiro:code_image", "Zoom": 1, "Color": "White"},
  {"Type": "DrawText", "X": 40, "Y": 75, "Text": "datasource:weather_aveiro:codeText", "Font": 2, "Color": "White"},
  {"Type": "DrawText", "X": 10, "Y": 100, "Text": "datasource:weather_aveiro:temperature", "Prefix": "Outside: ", "Suffix": " C", "Font": 2, "Color": "Yellow"}
]
```

Fields for `datasource:<Id>:<field>` are `temperature`, `humidity`, `pressure`,
`wind`, `code`, `codeText` and `code_image`. Aliases include `code_text`,
`codeImage` and `image`. Keep IDs short enough for the full expression to fit
inside the object's text/image field.

## HA — Home Assistant

`HA_URL` and `HA_TOKEN` come from local `secrets.h`, optionally overridden by
SD `/secrets.json`. All HA sources share this configuration. Each entity is
queried individually at `/api/states/<entity_id>`.

`Entities` accepts an array or a comma-separated string; an array is recommended.
The group `Id` organizes polling. Screen objects use the **full entity ID**,
without the group ID. `entity:<entity_id>` returns the state text, not attributes.

### Doors — source doors

```json
[
  {
    "Type": "HA",
    "Id": "doors",
    "Refresh": 5000,
    "Entities": [
      "binary_sensor.porta_entrada",
      "binary_sensor.porta_cozinha",
      "binary_sensor.porta_alcapao",
      "binary_sensor.porta_centro",
      "binary_sensor.porta_cozinha_cave",
      "binary_sensor.porta_oficina"
    ]
  }
]
```

One door object; repeat with different entities and positions:

```json
[
  {
    "Type": "DrawHASensor",
    "IconX": 198,
    "IconY": 35,
    "X": 228,
    "Y": 35,
    "Text": "Entrance",
    "Font": 2,
    "Color": "White",
    "entity": "binary_sensor.porta_entrada",
    "state:on": "DrawImage:door_open",
    "state:off": "DrawImage:door_closed",
    "else": "DrawImage:warning"
  }
]
```

`IconX/IconY` position the icon; `X/Y` position its label. `state:<value>` rules
match the exact returned state. Add `open`/`closed` rules if your integration
uses those states. `else` is the fallback, including unknown states. Each
object supports up to six rules, one of which can be `else`.

### Presence — source people

```json
[
  {
    "Type": "HA",
    "Id": "people",
    "Refresh": 5000,
    "Entities": [
      "binary_sensor.pessoas_escada",
      "binary_sensor.pessoas_cozinha_cave",
      "binary_sensor.pessoas_na_salinha",
      "binary_sensor.pessoas_na_oficina"
    ]
  }
]
```

```json
[
  {
    "Type": "DrawHASensor",
    "IconX": 10,
    "IconY": 180,
    "X": 40,
    "Y": 180,
    "Text": "Stairs",
    "Font": 2,
    "Color": "White",
    "entity": "binary_sensor.pessoas_escada",
    "state:on": "DrawImage:person_on",
    "state:off": "DrawImage:person_off",
    "else": "DrawImage:warning"
  }
]
```

The `people` source is declared in the example layout, but it needs screen
objects such as this one to become visible. Reading states on this dashboard
does not enable Telegram alerts from the companion Raspberry Pi Python project.

### Living room — temperature, humidity and light

To restore these sensors, add this source to `DataSources`:

```json
[
  {
    "Type": "HA",
    "Id": "sala",
    "Refresh": 5000,
    "Entities": [
      "sensor.pessoas_sala_temperature",
      "sensor.pessoas_sala_humidity",
      "sensor.pessoas_sala_illuminance"
    ]
  }
]
```

Add these objects to `Screens[].Objects`:

```json
[
  {"Type": "DrawText", "X": 10, "Y": 100, "Text": "entity:sensor.pessoas_sala_temperature", "Prefix": "Temp: ", "Suffix": " C", "Font": 4, "Color": "Yellow"},
  {"Type": "DrawText", "X": 10, "Y": 130, "Text": "entity:sensor.pessoas_sala_humidity", "Prefix": "Humidity: ", "Suffix": " %", "Font": 2, "Color": "Green"},
  {"Type": "DrawText", "X": 10, "Y": 155, "Text": "entity:sensor.pessoas_sala_illuminance", "Prefix": "Light: ", "Suffix": " lux", "Font": 2, "Color": "Cyan"}
]
```

These positions overlap the power graph: move the graph or replace its objects.
`Prefix` is the recommended spelling; the legacy `Preffix` spelling is accepted.

Values are displayed as received from HA, without rounding or automatic unit
lookup. `Prefix`/`Suffix` compose the label. Empty, `unknown` and `unavailable`
states display `--`, retaining the prefix and suffix. Failed HTTP requests may
leave the last received state in the cache, so the text is not a freshness indicator.

## Power — household power graph

`simple.v5.json` uses this source for the household power sensor:

```json
[
  {
    "Type": "Power",
    "Id": "casa",
    "Entity": "sensor.energia_pj2101a_local_power",
    "Unit": "W",
    "Refresh": 2000
  }
]
```

`Power` uses the same HA URL/token as HA sources. There is no need to include
the entity in an HA group as well. `Entity` is required and supports up to
63 bytes; `Id` supports up to 31 bytes. One Power source is shared across screens.
`Unit` accepts `W` or `kW`; kW readings are converted to W before storage and display.

`Refresh` is the minimum interval from the end of one polling attempt to the
next: default 2000 ms, clamped to 2000–3600000 ms. Other sources and HTTP
requests may increase the actual interval. Polling runs in the existing data
task with a three-second HTTP timeout; the graph does not create another task.

### Display the graph and value underneath

```json
[
  {"Type": "DrawPowerGraph", "Source": "casa", "X": 10, "Y": 100, "Width": 178, "Height": 90},
  {"Type": "DrawText", "X": 10, "Y": 196, "Text": "power:casa", "Prefix": "House:", "Suffix": "W", "Font": 2}
]
```

- `Source` is the Power source ID. `Width`/`Height` include the border and default to 178×90. At least 64×40 pixels are required. Drawing is bounded by the physical screen; negative coordinates or insufficient space are not drawn.
- The fixed **five-minute** window places newer samples on the right using actual timestamps. During startup, history fills from the right. Up to 151 timestamp/power samples use about 1.2 KB plus configuration on the heap.
- The vertical axis shows **absolute power**, with a minimum of **0 W**. Maximum = largest absolute value retained in the window + `max(5 W, 5%)`. A -800 W peak gives 840 W; a +50 W peak gives 55 W. The maximum can decrease when an old peak expires.
- Default colors are **bright green for negative** and **yellow for zero or positive**. A sign change is interpolated through zero at the baseline, where the color changes. A negative segment touching zero remains green, matching the Raspberry Pi graph.
- Without history, the scale is 0–100 W. All-zero samples give a 5 W maximum. With fewer than two samples, the current firmware displays the literal waiting message `aguarda...`.
- `power:casa` displays the latest valid watts truncated to an integer, for example `House:-382W`. The text uses the sign color regardless of its `Color` field. Before the first reading it displays `House:--W` in white.
- Empty, `unknown`, `unavailable`, nonnumeric, NaN/Inf and failed HTTP readings do not add samples; the last valid value is retained. Absolute values up to 1,000,000,000 W are accepted. Samples expire from the graph, while the text retains the latest valid reading. A line may connect valid readings across a temporary failure.
- Screen changes retain history; restarting or reloading the board JSON clears it. History is not saved to SD/flash. Drawing uses no graph framebuffer or additional graphics library.

This is **power in watts**, not accumulated energy in kWh.

### Configure graph colors

Each graph object can override its palette independently:

```json
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
}
```

Omitted, empty or non-string properties keep their individual defaults:
background RGB565 `0x0861`, positive yellow and negative green. `Background`
is the preferred key; `background` and the historical typo `backgroud` are
accepted aliases. Use supported color names or RGB565 strings such as
`0x0861`, not CSS `#RRGGBB` values. Graph palette overrides apply to the graph;
the `power:` text retains its automatic sign colors.

For diagnostics, enable `Log.Power`: logs include value, sample count, free
heap and read failures. Other source logs can remain disabled.

## Ping — server availability

Ping remains supported in the firmware, although `simple.v5.json` does not
use it. The following examples are alternatives to the graph in the same area.
The board sends pings to these hosts; this is not a pushed status feed.

### Define sources

```json
[
  {"Type": "Ping", "Id": "server1", "Host": "192.168.1.204", "Refresh": 10000},
  {"Type": "Ping", "Id": "server2", "Host": "192.168.1.75", "Refresh": 10000},
  {"Type": "Ping", "Id": "server3", "Host": "192.168.1.79", "Refresh": 10000}
]
```

`Host` accepts an IPv4 address or hostname, for example `raspi2` or
`raspi2.local`. Two ICMP IPv4 requests are sent with a two-second timeout per
request; one reply is sufficient for OK. Failed hostname resolution is also
tried with a `.local` mDNS name; IP addresses do not receive that suffix.

The interval starts when the previous check finishes. Default: 10000 ms,
clamped to 1000–3600000 ms. A dedicated task checks hosts sequentially, so
slow hosts can delay a round without holding the drawing mutex during requests.

### Display text

```json
[
  {"Type": "DrawText", "X": 10, "Y": 100, "Text": "ping:server1", "Prefix": "192.168.1.204 ", "Font": 2, "Color": "White"},
  {"Type": "DrawText", "X": 10, "Y": 130, "Text": "ping:server2", "Prefix": "192.168.1.75 ", "Font": 2, "Color": "White"},
  {"Type": "DrawText", "X": 10, "Y": 155, "Text": "ping:server3", "Prefix": "192.168.1.79 ", "Font": 2, "Color": "White"}
]
```

| State | Text and color |
| --- | --- |
| Not checked yet or unknown ID | `--`, white |
| Replied | `OK`, green |
| No reply, resolution/session failure or Wi-Fi disconnected after the first result | `FAIL`, yellow |

For `ping:` text, firmware automatically colors the entire line, including
the prefix, overriding the object's `Color` field.

### Display a boolean icon

```json
[
  {"Type": "DrawImageBool", "X": 10, "Y": 100, "Value": "ping:server1", "true": "DrawImage:wifi_on", "false": "DrawImage:wifi_off"}
]
```

Wi-Fi icons are simply a visual choice for server status. Only OK is `true`;
FAIL, unchecked and unknown IDs are `false`. This replaces the text object at
the same position.

## Local functions — no DataSource required

Clock, IP and Wi-Fi status need no `DataSources` entries:

```json
[
  {"Type": "DrawText", "X": 5, "Y": 5, "Text": "function:Clock", "Font": 4, "Color": "White"},
  {"Type": "DrawText", "X": 160, "Y": 12, "Text": "function:MyIP", "Font": 1, "Color": "White"},
  {"Type": "DrawImageBool", "X": 290, "Y": 5, "Value": "function:IsWIFIConnected", "true": "DrawImage:wifi_on", "false": "DrawImage:wifi_off"}
]
```

Other text functions are `function:Date`, `function:Uptime`,
`function:WifiRSSI` and `function:FreeRAM`. The boolean resolver also supports
`function:IsHAConnected`, but this only indicates that an HA source is marked
valid in the cache, not proof of current connectivity. MQTT is not implemented.

## Logging, limits and applying JSON changes

For polling diagnostics, place this object at the root:

```json
{"Log": {"Weather": true, "HA": true, "Ping": true, "Power": true}}
```

Weather/HA flags also control HTTP logs for those sources. Ping logs initial
results and state changes. Flags can be `false` during normal operation.

| Item | Current limit |
| --- | --- |
| Weather/HA sources combined | 16 |
| Ping sources | 8 |
| Power sources | 1 |
| Power history | 151 samples / 5 minutes |
| HA entities queried per group | 12 |
| Entity list per group | 511 bytes, including commas |
| Distinct HA entities in global cache | 40 |
| Source ID | Up to 31 bytes; use unique IDs |
| Ping host | Up to 63 bytes |
| Weather code mappings | 16 |
| Objects per screen | 30 |
| Object Text/Image/boolean expression | Up to 39 bytes |
| Prefix / Suffix | Up to 23 / 15 bytes |

Text limits are UTF-8 byte limits. Long HA references must fit in `Text`; the
living-room examples above fit. Cached HA state text holds up to 15 bytes,
so longer state strings may be truncated.

Store the JSON on SD or at the base URL configured by `JSON_URL`, and select
its filename with `JSON_FILE`. SD `/secrets.json` overrides can change these
settings. An existing SD layout file takes priority over the network copy.
Then open `http://<board-ip>/?reload=true` to reload the layout and sources.

JSON-only changes do not require recompilation when the installed firmware
supports the requested features. `dashIDE002.1` supports all sources and graph
color options documented here. Earlier firmware needs the corresponding
feature support (Ping from dash001.4; Power from dash001.5).

Implementation references in [dashIDE002.1](../dashIDE002.1/):
`datasources.h`, `datacache.h`, `functions.h`, `ping_sources.h`,
`power_source.h`, `power_history.h`, `power_plot.h`, `power_graph_json.h`,
`objects.h` and `draw.h`.
