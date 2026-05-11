/**
 * @file pins.h
 * @brief Pin definitions for both hardware variants
 */

#ifndef PINS_H
#define PINS_H

#if defined(HARDWARE_WAVESHARE)

// ============================================================================
// Waveshare ESP32 Driver Board Pin Definitions
// ============================================================================

// SPI e-Paper pins
#define EPD_CS      15
#define EPD_DC      27
#define EPD_RST     26
#define EPD_BUSY    25
#define EPD_SCK     13
#define EPD_MOSI    14

// Power control
#define EPD_PWR     12  // Display power enable (HIGH = on)

// Battery monitoring
#define BATT_ADC    35  // ADC1_CH7

// SD card (shared SPI bus)
#define SD_CS       5
#define SD_MISO     19
#define SD_MOSI     14  // Shared with EPD
#define SD_SCK      13  // Shared with EPD

// User button (if available on your board variant)
#define USER_BTN    0   // BOOT button

#elif defined(HARDWARE_LILYGO_T5)

// ============================================================================
// LILYGO T5-4.7-S3 Pin Definitions
// ============================================================================

// E-paper parallel interface (handled by EPD47 library internally)
// No manual pin definitions needed - library manages all EPD pins

// Touch controller (GT911 on I2C)
#define TOUCH_SDA   18
#define TOUCH_SCL   17
#define TOUCH_IRQ   47
#define TOUCH_RST   -1  // Not connected on T5-4.7

// Battery monitoring
#define BATT_ADC    14  // ADC1_CH3 (GPIO14)

// SD card
#define SD_CS       42
#define SD_MISO     16
#define SD_MOSI     15
#define SD_SCK      11

// User button
#define USER_BTN    21  // BTN_IO21

// Power control (EPD47 library handles this)
// EPD_PWR is managed internally by epd_poweroff()/epd_poweron()

#else
  #error "No hardware variant defined in pins.h"
#endif

#endif // PINS_H
