# E-Ink Task Display

Battery-powered e-ink display that fetches and renders Lisa's "One Thing at a Time" task list from a live FastAPI server over WiFi. The device deep-sleeps between updates and wakes on a timer or touch event.

---

## Hardware

| | LILYGO T5 4.7" *(primary)* | Waveshare 4.26" *(secondary)* |
|---|---|---|
| MCU | ESP32-S3 (240 MHz, 8 MB PSRAM) | ESP32 |
| Display | 960×540 parallel e-paper | 800×480 SPI e-paper (GDEY042T81) |
| Touch | GT911 capacitive (I2C) | None |
| Flash | 16 MB | 4 MB |
| Update interval | 2 hours | 1 hour |
| PlatformIO env | `lilygo_t5` | `waveshare` |

---

## Features

- **Periodic WiFi fetch** — wakes on RTC timer, fetches `/tasks/api/state`, renders, sleeps
- **Deep sleep** — ~10–15 µA between updates; months of battery life
- **Touch-to-complete** *(LILYGO only)* — tap a task row to remove it from the backlog and PUT the updated list back to the server
- **Streak display** — motivational counter from `otta-streak`
- **Difficulty tags** — `[easy]` / `[medium]` / `[hard]` rendered next to each task
- **Battery monitoring** — ADC voltage divider; warns below 3.3 V, sleeps indefinitely below 3.1 V

---

## Server

Live FastAPI server, no auth required:

| | |
|---|---|
| Base URL | `https://droplet.josephborrello.com` |
| State endpoint | `GET /tasks/api/state` |
| Update backlog | `PUT /tasks/api/list/otta-backlog` |
| Health check | `GET /tasks/api/health` |

See [`HANDOFF-SEB.md`](HANDOFF-SEB.md) for the full API shape and double-encoding details.

---

## Setup

### 1. Configure credentials

```bash
cp firmware/include/config.h firmware/include/config_local.h
# Edit config_local.h — fill in WIFI_SSID and WIFI_PASSWORD
```

`config_local.h` is gitignored. The defaults in `config.h` already point at the live server (`https://droplet.josephborrello.com`).

### 2. Install PlatformIO

```bash
# VS Code extension (recommended), or:
pip install platformio
```

### 3. Build

```bash
cd firmware
pio run -e lilygo_t5   # LILYGO T5 4.7"
pio run -e waveshare   # Waveshare 4.26"
```

### 4. Flash

```bash
pio run -t upload -e lilygo_t5
pio run -t upload -e waveshare
```

> **Note:** USB flashing from WSL is unreliable — use Windows/PowerShell or a native Linux host.

---

## Tests

Unit tests run on-device via Unity (requires hardware):

```bash
pio test -e lilygo_t5
```

Tests cover JSON parsing (`parseStateJSON`) and serialization (`serializeBacklog`) round-trips.

---

## Project layout

```
eink-todo/
├── firmware/
│   ├── platformio.ini          # Dual build environments (waveshare, lilygo_t5)
│   ├── include/
│   │   ├── config.h            # Configuration template (copy → config_local.h)
│   │   ├── pins.h              # Pin definitions for both boards
│   │   ├── checklist_model.h   # TaskItem / TaskList structs + parse/serialize API
│   │   └── wifi_manager.h      # WiFiManager class (GET / PUT / connect / disconnect)
│   ├── src/
│   │   ├── main.cpp            # setup() / renderTaskList() / handleTouch() / sleep
│   │   ├── checklist_model.cpp # parseStateJSON() + serializeBacklog()
│   │   └── wifi_manager.cpp    # WiFi + HTTP implementation
│   └── test/
│       ├── test_json_parsing.cpp   # Unity tests for model parse + serialize
│       └── test_wifi_manager.cpp   # Unity tests for WiFiManager helpers
├── server/
│   ├── app.py                  # FastAPI server (live on droplet)
│   ├── requirements.txt
│   └── static/tasks.html       # Web UI
├── ops/
│   ├── lisa-tasks.service      # systemd unit
│   ├── nginx-location.conf     # nginx reverse-proxy snippet
│   └── backup.sh               # Daily SQLite backup (30-day retention)
├── HANDOFF-SEB.md              # API shape, double-encoding details, server notes
└── README.md                   # This file
```

---

## How touch-to-complete works

1. Device wakes from deep sleep on `TOUCH_IRQ` (EXT0).
2. `handleTouch()` reads the GT911 touch point.
3. Hit-tests each task row against the rendered checkbox Y-coordinates.
4. On a hit: erases the item from `taskList.items`, serializes the remaining list with `serializeBacklog()`, builds `{"value": "<escaped JSON array>"}`, and PUTs it to `/tasks/api/list/otta-backlog`.
5. Re-renders the display with the updated list, then returns to deep sleep.
6. If WiFi fails, the local state is still updated and the display re-renders — the server sync is best-effort.
