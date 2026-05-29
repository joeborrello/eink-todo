/**
 * @file wifi_manager.h
 * @brief WiFi connection management with retry logic and timeout handling
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"

// Connection states
enum class WiFiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED
};

// HTTP response structure
struct HttpResponse {
    bool success;
    int statusCode;
    String body;
    String error;
};

class WiFiManager {
public:
    WiFiManager();
    
    /**
     * @brief Connect to WiFi with retry logic
     * @param maxRetries Maximum connection attempts (default: 3)
     * @param timeoutMs Timeout per attempt in milliseconds (default: 10000)
     * @return true if connected, false otherwise
     */
    bool connect(uint8_t maxRetries = 3, uint32_t timeoutMs = 10000);
    
    /**
     * @brief Disconnect from WiFi and power down radio
     */
    void disconnect();
    
    /**
     * @brief Check if currently connected
     * @return true if connected to WiFi
     */
    bool isConnected();
    
    /**
     * @brief Get current WiFi state
     * @return Current WiFiState enum value
     */
    WiFiState getState();
    
    /**
     * @brief Get signal strength (RSSI)
     * @return RSSI in dBm, or 0 if not connected
     */
    int getRSSI();
    
    /**
     * @brief Perform HTTP GET request
     * @param url Target URL
     * @param timeoutMs Request timeout in milliseconds (default: 5000)
     * @return HttpResponse structure with result
     */
    HttpResponse httpGet(const String& url, uint32_t timeoutMs = 5000);
    
    /**
     * @brief Perform HTTP POST request
     * @param url Target URL
     * @param jsonPayload JSON body to send
     * @param timeoutMs Request timeout in milliseconds (default: 5000)
     * @return HttpResponse structure with result
     */
    HttpResponse httpPost(const String& url, const String& jsonPayload, uint32_t timeoutMs = 5000);

    /**
     * @brief Perform HTTP PUT request
     * @param url Target URL
     * @param rawBody Raw body to send
     * @param timeoutMs Request timeout in milliseconds (default: 10000)
     * @return HttpResponse structure with result
     */
    HttpResponse httpPut(const String& url, const String& rawBody, uint32_t timeoutMs = 10000);

private:
    WiFiState state;
    uint32_t lastConnectAttempt;
    uint8_t consecutiveFailures;
    
    /**
     * @brief Wait for WiFi connection with timeout
     * @param timeoutMs Timeout in milliseconds
     * @return true if connected within timeout
     */
    bool waitForConnection(uint32_t timeoutMs);
    
    /**
     * @brief Log connection diagnostics
     */
    void logConnectionInfo();
};

#endif // WIFI_MANAGER_H
