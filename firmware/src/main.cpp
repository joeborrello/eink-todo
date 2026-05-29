/**
 * @file main.cpp
 * @brief E-ink checklist display - main entry point
 * 
 * Dual-hardware support:
 * - Waveshare 4.26" (800x480) SPI e-paper + ESP32 driver board
 * - LILYGO T5 4.7" (960x540) parallel e-paper + capacitive touch
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Local includes
#include "config.h"
#include "pins.h"
#include "checklist_model.h"
#include "wifi_manager.h"

// Hardware-specific display drivers
#if defined(HARDWARE_WAVESHARE)
  #include <GxEPD2_BW.h>
  #include <Fonts/FreeMonoBold12pt7b.h>
  
  // GxEPD2 driver for Waveshare 4.26" (GDEY042T81)
  GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(
    GxEPD2_420(/*CS=*/EPD_CS, /*DC=*/EPD_DC, /*RST=*/EPD_RST, /*BUSY=*/EPD_BUSY)
  );

#elif defined(HARDWARE_LILYGO_T5)
  #include "epd_driver.h"
  #include "firasans.h"
  #include <Wire.h>
  #include <TAMC_GT911.h>
  
  // Touch controller
  TAMC_GT911 touch = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_IRQ, TOUCH_RST, EPD_WIDTH, EPD_HEIGHT);
  
  // Framebuffer for EPD47 (allocated in setup)
  uint8_t *framebuffer = nullptr;

#endif

// ============================================================================
// Global State
// ============================================================================

Checklist checklist;
WiFiManager wifiManager;

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint64_t lastUpdateTime = 0;

// ============================================================================
// Function Prototypes
// ============================================================================

void setupDisplay();
void setupWiFi();
void fetchChecklist();
void renderChecklist();
void enterDeepSleep(uint64_t sleepTimeSeconds);
uint16_t readBatteryVoltage();
void handleTouch();

// ============================================================================
// Setup
// ============================================================================

void setup() {
  #if DEBUG_SERIAL
    Serial.begin(DEBUG_BAUD);
    delay(1000);  // Wait for serial monitor
  #endif
  
  bootCount++;
  DEBUG_PRINTF("\n=== Boot #%d - %s ===\n", bootCount, HARDWARE_NAME);
  
  // Initialize display hardware
  setupDisplay();
  
  // Check battery voltage
  uint16_t batteryMv = readBatteryVoltage();
  DEBUG_PRINTF("Battery: %d mV\n", batteryMv);
  
  if (batteryMv < BATT_CRITICAL_MV) {
    DEBUG_PRINTLN("CRITICAL: Battery too low, entering indefinite sleep");
    // TODO: Show "Low Battery" screen before sleeping
    enterDeepSleep(0);  // Sleep indefinitely (0 = no timer wake)
  }
  
  // Determine wake reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  bool shouldUpdate = false;
  bool touchWake = false;
  
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      DEBUG_PRINTLN("Wake: Timer");
      shouldUpdate = true;
      break;
      
    case ESP_SLEEP_WAKEUP_EXT0:
      DEBUG_PRINTLN("Wake: Touch/Button");
      touchWake = true;
      #if HAS_TOUCH
        shouldUpdate = false;  // Handle touch locally, don't fetch new data
      #else
        shouldUpdate = true;  // Button press = force update
      #endif
      break;
      
    default:
      DEBUG_PRINTLN("Wake: Power-on reset");
      shouldUpdate = true;
      break;
  }
  
  // Handle touch interaction (LILYGO only)
  #if HAS_TOUCH
  if (touchWake) {
    handleTouch();
    enterDeepSleep(UPDATE_INTERVAL);
    return;  // Never reached
  }
  #endif
  
  // Fetch new checklist data from server
  if (shouldUpdate) {
    setupWiFi();
    fetchChecklist();
    wifiManager.disconnect();
  }
  
  // Render checklist to display
  renderChecklist();
  
  // Enter deep sleep
  lastUpdateTime = millis();
  enterDeepSleep(UPDATE_INTERVAL);
}

// ============================================================================
// Loop (never reached - device sleeps in setup)
// ============================================================================

void loop() {
  // Not used - all logic in setup() before deep sleep
}

// ============================================================================
// Display Initialization
// ============================================================================

void setupDisplay() {
  #if defined(HARDWARE_WAVESHARE)
    // Power on display
    pinMode(EPD_PWR, OUTPUT);
    digitalWrite(EPD_PWR, HIGH);
    delay(100);
    
    // Initialize GxEPD2
    display.init(115200, true, 2, false);  // (serial_diag_bitrate, initial, reset_duration, pulldown_rst_mode)
    display.setRotation(1);  // Landscape
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold12pt7b);
    
    DEBUG_PRINTLN("Display: Waveshare 4.26\" initialized");
    
  #elif defined(HARDWARE_LILYGO_T5)
    // Initialize EPD47 driver
    epd_init();
    epd_poweron();
    epd_clear();
    
    // Allocate framebuffer (PSRAM required)
    framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    if (!framebuffer) {
      DEBUG_PRINTLN("ERROR: Failed to allocate framebuffer!");
      esp_restart();
    }
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);  // White background
    
    // Initialize touch controller
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    touch.begin();
    
    DEBUG_PRINTLN("Display: LILYGO T5 4.7\" initialized");
  #endif
}

// ============================================================================
// WiFi Connection
// ============================================================================

void setupWiFi() {
  DEBUG_PRINTLN("Initializing WiFi...");
  
  if (!wifiManager.connect(WIFI_MAX_RETRIES, WIFI_TIMEOUT_MS)) {
    DEBUG_PRINTLN("WiFi connection failed - will use cached data");
  }
}

// ============================================================================
// Fetch Checklist from Server
// ============================================================================

void fetchChecklist() {
  if (!wifiManager.isConnected()) {
    DEBUG_PRINTLN("No WiFi - skipping fetch");
    return;
  }
  
  String url = String(SERVER_URL) + CHECKLIST_ENDPOINT;
  DEBUG_PRINTF("Fetching checklist from: %s\n", url.c_str());
  
  HttpResponse response = wifiManager.httpGet(url, HTTP_TIMEOUT_MS);
  
  if (response.success) {
    DEBUG_PRINTF("Received %d bytes\n", response.body.length());
    
    // Parse JSON using checklist_model
    if (parseChecklistJSON(response.body, checklist)) {
      DEBUG_PRINTF("Parsed checklist: %s (%d items)\n", 
                   checklist.title.c_str(), checklist.items.size());
      lastUpdateTime = millis();
    } else {
      DEBUG_PRINTLN("Failed to parse checklist JSON");
    }
  } else {
    DEBUG_PRINTF("Fetch failed: %s\n", response.error.c_str());
  }
}

// ============================================================================
// Render Checklist to Display
// ============================================================================

void renderChecklist() {
  #if defined(HARDWARE_WAVESHARE)
    display.setFullWindow();
    display.firstPage();
    
    do {
      display.fillScreen(GxEPD_WHITE);
      
      // Title
      display.setCursor(10, 30);
      display.print("Checklist");
      
      // Items
      for (uint8_t i = 0; i < checklist.items.size(); i++) {
        int16_t y = 70 + (i * ITEM_HEIGHT);
        
        // Checkbox
        int16_t boxX = 10;
        int16_t boxY = y - CHECKBOX_SIZE + 5;
        display.drawRect(boxX, boxY, CHECKBOX_SIZE, CHECKBOX_SIZE, GxEPD_BLACK);
        
        if (checklist.items[i].checked) {
          // Draw X
          display.drawLine(boxX + 5, boxY + 5, boxX + CHECKBOX_SIZE - 5, boxY + CHECKBOX_SIZE - 5, GxEPD_BLACK);
          display.drawLine(boxX + CHECKBOX_SIZE - 5, boxY + 5, boxX + 5, boxY + CHECKBOX_SIZE - 5, GxEPD_BLACK);
        }
        
        // Text
        display.setCursor(TEXT_MARGIN_LEFT, y);
        display.print(checklist.items[i].text);
      }
      
      // Battery indicator
      uint16_t battMv = readBatteryVoltage();
      display.setCursor(650, 30);
      display.printf("%dmV", battMv);
      
    } while (display.nextPage());
    
    // Power off display
    digitalWrite(EPD_PWR, LOW);
    
  #elif defined(HARDWARE_LILYGO_T5)
    epd_poweron();
    epd_clear();
    
    // Title
    int cursor_x = 20;
    int cursor_y = 50;
    writeln((GFXfont *)&FiraSans, "Checklist", &cursor_x, &cursor_y, framebuffer);
    
    cursor_y += 30;
    
    // Items
    for (uint8_t i = 0; i < checklist.items.size(); i++) {
      cursor_x = 20;
      
      // Checkbox
      int boxX = cursor_x;
      int boxY = cursor_y - CHECKBOX_SIZE + 5;
      epd_draw_rect(boxX, boxY, CHECKBOX_SIZE, CHECKBOX_SIZE, 0x00, framebuffer);
      
      if (checklist.items[i].checked) {
        // Draw X
        epd_draw_line(boxX + 5, boxY + 5, boxX + CHECKBOX_SIZE - 5, boxY + CHECKBOX_SIZE - 5, 0x00, framebuffer);
        epd_draw_line(boxX + CHECKBOX_SIZE - 5, boxY + 5, boxX + 5, boxY + CHECKBOX_SIZE - 5, 0x00, framebuffer);
      }
      
      // Text
      cursor_x = TEXT_MARGIN_LEFT;
      write_string((GFXfont *)&FiraSans, checklist.items[i].text.c_str(), &cursor_x, &cursor_y, framebuffer);
      
      cursor_y += ITEM_HEIGHT;
    }
    
    // Battery indicator
    uint16_t battMv = readBatteryVoltage();
    cursor_x = EPD_WIDTH - 150;
    cursor_y = 50;
    char battStr[16];
    snprintf(battStr, sizeof(battStr), "%dmV", battMv);
    write_string((GFXfont *)&FiraSans, battStr, &cursor_x, &cursor_y, framebuffer);
    
    // Draw framebuffer to display
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    epd_poweroff();
  #endif
  
  DEBUG_PRINTLN("Display updated");
}

// ============================================================================
// Touch Handling (LILYGO only)
// ============================================================================

void handleTouch() {
  #if HAS_TOUCH
    touch.read();
    
    if (touch.isTouched && touch.touches > 0) {
      TP_Point t = touch.points[0];
      DEBUG_PRINTF("Touch: x=%d, y=%d\n", t.x, t.y);
      
      // Determine which checklist item was touched
      for (uint8_t i = 0; i < checklist.items.size(); i++) {
        int16_t itemY = 70 + (i * ITEM_HEIGHT);
        int16_t boxY = itemY - CHECKBOX_SIZE + 5;
        
        if (t.y >= boxY && t.y <= (boxY + CHECKBOX_SIZE)) {
          // Toggle checkbox
          checklist.items[i].checked = !checklist.items[i].checked;
          DEBUG_PRINTF("Toggled item %d: %s\n", i, checklist.items[i].checked ? "checked" : "unchecked");
          
          // TODO: Send toggle to server
          // For now, just update display with partial refresh
          renderChecklist();
          break;
        }
      }
    }
  #endif
}

// ============================================================================
// Battery Monitoring
// ============================================================================

uint16_t readBatteryVoltage() {
  pinMode(BATT_ADC, INPUT);
  
  // Average multiple readings
  uint32_t sum = 0;
  const uint8_t samples = 10;
  
  for (uint8_t i = 0; i < samples; i++) {
    sum += analogRead(BATT_ADC);
    delay(10);
  }
  
  uint16_t adcValue = sum / samples;
  
  // Convert to millivolts
  // ESP32 ADC: 0-4095 = 0-3.3V (with attenuation)
  // Voltage divider on board typically 2:1
  uint16_t voltage = (adcValue * 3300 / 4095) * BATT_ADC_MULTIPLIER + BATT_ADC_OFFSET;
  
  return voltage;
}

// ============================================================================
// Deep Sleep
// ============================================================================

void enterDeepSleep(uint64_t sleepTimeSeconds) {
  DEBUG_PRINTF("Entering deep sleep for %llu seconds\n", sleepTimeSeconds);
  
  #if HAS_TOUCH
    // Enable wake on touch interrupt
    esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_IRQ, LOW);
  #else
    // Enable wake on button press
    esp_sleep_enable_ext0_wakeup((gpio_num_t)USER_BTN, LOW);
  #endif
  
  // Enable timer wake
  if (sleepTimeSeconds > 0) {
    esp_sleep_enable_timer_wakeup(sleepTimeSeconds * 1000000ULL);
  }
  
  // Enter deep sleep
  esp_deep_sleep_start();
}
