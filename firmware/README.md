# E-Ink Checklist Display

Battery-powered wireless e-ink checklist display with dual hardware support.

## Hardware Variants

### Waveshare Modular
- **Display**: Waveshare 4.26" e-Paper (800×480, SKU 26175)
- **MCU**: Waveshare ESP32 Driver Board (SKU 15823)
- **Battery**: 2500-3000mAh LiPo (8-9 months per charge)
- **Features**: Hourly updates, low power consumption

### LILYGO T5 Touch
- **Display**: LILYGO T5 4.7" (960×540) with capacitive touch
- **MCU**: ESP32-S3 with 16MB Flash, 8MB PSRAM
- **Battery**: 3500-4000mAh LiPo (12-14 months per charge)
- **Features**: Touch interaction, 2-hour updates, wake-on-touch

## Quick Start

### 1. Install PlatformIO
```bash
# VS Code extension (recommended)
# Or CLI:
pip install platformio
```

### 2. Configure WiFi & Server
```bash
cd eink-checklist-display
cp include/config.h include/config_local.h
# Edit config_local.h with your WiFi credentials and server URL
```

### 3. Build & Upload

**Waveshare:**
```bash
pio run -e waveshare
pio run -e waveshare -t upload  # From Windows/PowerShell (not WSL)
```

**LILYGO T5:**
```bash
pio run -e lilygo_t5
pio run -e lilygo_t5 -t upload  # From Windows/PowerShell (not WSL)
```

### 4. Run Reference Server
```bash
cd server
pip install -r requirements.txt
python server.py
# Server runs on http://0.0.0.0:5000
```

## Project Structure

```
eink-checklist-display/
├── platformio.ini          # Build configuration (dual environments)
├── include/
│   ├── config.h            # Configuration template
│   └── pins.h              # Pin definitions for both boards
├── src/
│   └── main.cpp            # Main application logic
├── server/
│   ├── server.py           # Flask reference server
│   └── requirements.txt    # Python dependencies
└── data/                   # (Future: SPIFFS data for offline mode)
```

## Configuration

Edit `include/config_local.h`:

```cpp
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"
#define SERVER_URL "http://192.168.1.100:5000"
```

## API Endpoints

### GET /api/checklist
Fetch current checklist.

**Response:**
```json
{
  "items": [
    {"id": 1, "text": "Task 1", "checked": false},
    {"id": 2, "text": "Task 2", "checked": true}
  ]
}
```

### POST /api/toggle
Toggle a checklist item (LILYGO touch only).

**Request:**
```json
{"id": 1}
```

### POST /api/update
Update entire checklist (for server-side management).

**Request:**
```json
{
  "items": [
    {"id": 1, "text": "New task", "checked": false}
  ]
}
```

## Power Consumption

### Waveshare (hourly updates)
- Deep sleep: ~10µA
- Wake + WiFi + update: ~80mA for 20-30s
- **Battery life**: 8 months (2500mAh)

### LILYGO T5 (2-hour updates + touch)
- Deep sleep: ~15µA
- Wake + WiFi + update: ~120mA for 25-35s
- Touch wake: ~30mA for 5s (partial refresh)
- **Battery life**: 12.5 months (3500mAh)

## Troubleshooting

### Build fails with "GxEPD2_426_GDEQ0426T82 not found"
The GxEPD2 library must be v1.5.3 or later for 4.26" support. If your display uses a different panel, check the datasheet and update `main.cpp`:
```cpp
// Replace GxEPD2_426_GDEQ0426T82 with your controller (e.g., GxEPD2_420_GDEY042T81 for 4.2")
```

### LILYGO framebuffer allocation fails
Ensure PSRAM is enabled in `platformio.ini`:
```ini
build_flags = -DBOARD_HAS_PSRAM
```

### WiFi won't connect
- Check credentials in `config_local.h`
- Verify 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Increase `WIFI_TIMEOUT_MS` if your network is slow

### Display shows nothing
- **Waveshare**: Check `EPD_PWR` pin (GPIO12) is HIGH during refresh
- **LILYGO**: Verify `epd_poweron()` is called before drawing

## License

MIT
