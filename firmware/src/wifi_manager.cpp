/**
 * @file wifi_manager.cpp
 * @brief WiFi connection management implementation
 */

#include "wifi_manager.h"

WiFiManager::WiFiManager() 
    : state(WiFiState::DISCONNECTED)
    , lastConnectAttempt(0)
    , consecutiveFailures(0) {
}

bool WiFiManager::connect(uint8_t maxRetries, uint32_t timeoutMs) {
    Serial.println("[WiFi] Starting connection...");
    Serial.printf("[WiFi] SSID: %s\n", WIFI_SSID);
    
    // Disconnect if already connected
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Already connected, disconnecting first");
        WiFi.disconnect();
        delay(100);
    }
    
    // Set WiFi mode
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false); // We handle reconnection manually
    
    // Retry loop
    for (uint8_t attempt = 1; attempt <= maxRetries; attempt++) {
        Serial.printf("[WiFi] Attempt %d/%d\n", attempt, maxRetries);
        
        state = WiFiState::CONNECTING;
        lastConnectAttempt = millis();
        
        // Begin connection
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        // Wait for connection
        if (waitForConnection(timeoutMs)) {
            state = WiFiState::CONNECTED;
            consecutiveFailures = 0;
            logConnectionInfo();
            return true;
        }
        
        // Connection failed
        Serial.printf("[WiFi] Attempt %d failed (status: %d)\n", attempt, WiFi.status());
        consecutiveFailures++;
        
        // Wait before retry (exponential backoff)
        if (attempt < maxRetries) {
            uint32_t backoffMs = 1000 * attempt; // 1s, 2s, 3s...
            Serial.printf("[WiFi] Retrying in %dms...\n", backoffMs);
            delay(backoffMs);
        }
    }
    
    // All attempts failed
    state = WiFiState::FAILED;
    Serial.printf("[WiFi] Connection failed after %d attempts\n", maxRetries);
    return false;
}

void WiFiManager::disconnect() {
    Serial.println("[WiFi] Disconnecting...");
    WiFi.disconnect(true); // true = turn off WiFi radio
    WiFi.mode(WIFI_OFF);
    state = WiFiState::DISCONNECTED;
    Serial.println("[WiFi] Disconnected and powered down");
}

bool WiFiManager::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

WiFiState WiFiManager::getState() {
    return state;
}

int WiFiManager::getRSSI() {
    if (!isConnected()) {
        return 0;
    }
    return WiFi.RSSI();
}

HttpResponse WiFiManager::httpGet(const String& url, uint32_t timeoutMs) {
    HttpResponse response;
    response.success = false;
    response.statusCode = 0;

    if (!isConnected()) {
        response.error = "WiFi not connected";
        Serial.println("[HTTP] Error: WiFi not connected");
        return response;
    }

    HTTPClient http;
    http.setTimeout(timeoutMs);

    Serial.printf("[HTTP] GET %s\n", url.c_str());

    bool begun = false;
    WiFiClientSecure secureClient;
    if (url.startsWith("https://")) {
        // TODO: pin root CA cert for production use
        secureClient.setInsecure();
        begun = http.begin(secureClient, url);
    } else {
        begun = http.begin(url);
    }

    if (!begun) {
        response.error = "Failed to begin HTTP connection";
        Serial.println("[HTTP] Error: Failed to begin connection");
        return response;
    }

    // Add headers
    http.addHeader("Accept", "application/json");
    http.addHeader("User-Agent", "ESP32-Checklist/1.0");

    // Perform request
    int httpCode = http.GET();
    response.statusCode = httpCode;

    if (httpCode > 0) {
        Serial.printf("[HTTP] Response code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK) {
            response.body = http.getString();
            response.success = true;
            Serial.printf("[HTTP] Received %d bytes\n", response.body.length());
        } else {
            response.error = "HTTP error: " + String(httpCode);
            Serial.printf("[HTTP] Error: %s\n", response.error.c_str());
        }
    } else {
        response.error = http.errorToString(httpCode);
        Serial.printf("[HTTP] Request failed: %s\n", response.error.c_str());
    }

    http.end();
    return response;
}

HttpResponse WiFiManager::httpPost(const String& url, const String& jsonPayload, uint32_t timeoutMs) {
    HttpResponse response;
    response.success = false;
    response.statusCode = 0;

    if (!isConnected()) {
        response.error = "WiFi not connected";
        Serial.println("[HTTP] Error: WiFi not connected");
        return response;
    }

    HTTPClient http;
    http.setTimeout(timeoutMs);

    Serial.printf("[HTTP] POST %s\n", url.c_str());
    Serial.printf("[HTTP] Payload: %s\n", jsonPayload.c_str());

    bool begun = false;
    WiFiClientSecure secureClient;
    if (url.startsWith("https://")) {
        // TODO: pin root CA cert for production use
        secureClient.setInsecure();
        begun = http.begin(secureClient, url);
    } else {
        begun = http.begin(url);
    }

    if (!begun) {
        response.error = "Failed to begin HTTP connection";
        Serial.println("[HTTP] Error: Failed to begin connection");
        return response;
    }

    // Add headers
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "ESP32-Checklist/1.0");

    // Perform request
    int httpCode = http.POST(jsonPayload);
    response.statusCode = httpCode;

    if (httpCode > 0) {
        Serial.printf("[HTTP] Response code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
            response.body = http.getString();
            response.success = true;
            Serial.printf("[HTTP] Received %d bytes\n", response.body.length());
        } else {
            response.error = "HTTP error: " + String(httpCode);
            Serial.printf("[HTTP] Error: %s\n", response.error.c_str());
        }
    } else {
        response.error = http.errorToString(httpCode);
        Serial.printf("[HTTP] Request failed: %s\n", response.error.c_str());
    }

    http.end();
    return response;
}

HttpResponse WiFiManager::httpPut(const String& url, const String& rawBody, uint32_t timeoutMs) {
    HttpResponse response;
    response.success = false;
    response.statusCode = 0;

    if (!isConnected()) {
        response.error = "WiFi not connected";
        Serial.println("[HTTP] Error: WiFi not connected");
        return response;
    }

    HTTPClient http;
    http.setTimeout(timeoutMs);

    Serial.printf("[HTTP] PUT %s\n", url.c_str());

    bool begun = false;
    WiFiClientSecure secureClient;
    if (url.startsWith("https://")) {
        // TODO: pin root CA cert for production use
        secureClient.setInsecure();
        begun = http.begin(secureClient, url);
    } else {
        begun = http.begin(url);
    }

    if (!begun) {
        response.error = "Failed to begin HTTP connection";
        Serial.println("[HTTP] Error: Failed to begin connection");
        return response;
    }

    // Add headers
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "ESP32-Checklist/1.0");

    // Perform request
    int httpCode = http.PUT(rawBody);
    response.statusCode = httpCode;

    if (httpCode > 0) {
        Serial.printf("[HTTP] Response code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
            response.body = http.getString();
            response.success = true;
            Serial.printf("[HTTP] Received %d bytes\n", response.body.length());
        } else {
            response.error = "HTTP error: " + String(httpCode);
            Serial.printf("[HTTP] Error: %s\n", response.error.c_str());
        }
    } else {
        response.error = http.errorToString(httpCode);
        Serial.printf("[HTTP] Request failed: %s\n", response.error.c_str());
    }

    http.end();
    return response;
}

// Private methods

bool WiFiManager::waitForConnection(uint32_t timeoutMs) {
    uint32_t startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeoutMs) {
            Serial.println("[WiFi] Connection timeout");
            return false;
        }
        
        // Print progress dots every 500ms
        if ((millis() - startTime) % 500 == 0) {
            Serial.print(".");
        }
        
        delay(100);
    }
    
    Serial.println(); // New line after dots
    return true;
}

void WiFiManager::logConnectionInfo() {
    Serial.println("[WiFi] ✓ Connected successfully");
    Serial.printf("[WiFi] IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("[WiFi] DNS: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("[WiFi] MAC: %s\n", WiFi.macAddress().c_str());
}
