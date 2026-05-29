/**
 * @file config.h
 * @brief Configuration template for e-ink task display
 * 
 * Copy this to config_local.h and fill in your credentials.
 * config_local.h is gitignored.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// WiFi Configuration
// ============================================================================

#ifndef WIFI_SSID
#define WIFI_SSID "your-wifi-ssid"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "your-wifi-password"
#endif

#define WIFI_TIMEOUT_MS 20000  // 20 seconds
#define WIFI_MAX_RETRIES 3

// ============================================================================
// Server Configuration
// ============================================================================

#ifndef SERVER_URL
#define SERVER_URL "https://droplet.josephborrello.com"
#endif

#define STATE_ENDPOINT  "/tasks/api/state"
#define HEALTH_ENDPOINT "/tasks/api/health"

#define HTTP_TIMEOUT_MS 10000  // 10 seconds

// ============================================================================
// Power Management
// ============================================================================

// Update intervals (seconds)
#define UPDATE_INTERVAL_SEC 3600  // 1 hour for Waveshare
#define UPDATE_INTERVAL_TOUCH_SEC 7200  // 2 hours for LILYGO (touch extends battery)

// Battery monitoring
#define BATT_LOW_THRESHOLD_MV 3300  // Show low battery warning below 3.3V
#define BATT_CRITICAL_MV 3100  // Enter deep sleep indefinitely below 3.1V

// ADC calibration (adjust per board)
#define BATT_ADC_MULTIPLIER 2.0  // Voltage divider ratio
#define BATT_ADC_OFFSET 0  // mV offset correction

// ============================================================================
// Display Configuration
// ============================================================================

// Task list rendering
#define MAX_TASK_ITEMS 20
#define ITEM_HEIGHT 40  // Pixels per task item
#define CHECKBOX_SIZE 30
#define TEXT_MARGIN_LEFT 50
#define TEXT_SIZE 2  // Adafruit GFX text size

// Partial refresh (LILYGO only, for touch toggles)
#define ENABLE_PARTIAL_REFRESH true

// ============================================================================
// Debug Configuration
// ============================================================================

#define DEBUG_SERIAL true
#define DEBUG_BAUD 115200

#if DEBUG_SERIAL
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(fmt, ...)
#endif

// ============================================================================
// Hardware Detection (auto-configured by platformio.ini)
// ============================================================================

#if defined(HARDWARE_WAVESHARE)
  #define HARDWARE_NAME "Waveshare 4.26\""
  #define HAS_TOUCH false
  #define UPDATE_INTERVAL UPDATE_INTERVAL_SEC
#elif defined(HARDWARE_LILYGO_T5)
  #define HARDWARE_NAME "LILYGO T5 4.7\""
  #define HAS_TOUCH true
  #define UPDATE_INTERVAL UPDATE_INTERVAL_TOUCH_SEC
#else
  #error "No hardware variant defined! Check platformio.ini build_flags"
#endif

#endif // CONFIG_H
