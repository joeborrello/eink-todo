/**
 * @file test_wifi_manager.cpp
 * @brief Unit tests for WiFi manager (requires hardware)
 * 
 * Run with: pio test -e waveshare (or -e lilygo_t5)
 */

#include <Arduino.h>
#include <unity.h>
#include "wifi_manager.h"
#include "config.h"

WiFiManager wifiManager;

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
    wifiManager.disconnect();
    delay(100);
}

// ============================================================================
// Connection Tests
// ============================================================================

void test_wifi_connect_success(void) {
    bool connected = wifiManager.connect(3, 10000);
    TEST_ASSERT_TRUE(connected);
    TEST_ASSERT_TRUE(wifiManager.isConnected());
    TEST_ASSERT_EQUAL(WiFiState::CONNECTED, wifiManager.getState());
}

void test_wifi_rssi_valid(void) {
    wifiManager.connect(3, 10000);
    int rssi = wifiManager.getRSSI();
    TEST_ASSERT_GREATER_THAN(-100, rssi);  // Typical range: -30 to -90 dBm
    TEST_ASSERT_LESS_THAN(0, rssi);
}

void test_wifi_disconnect(void) {
    wifiManager.connect(3, 10000);
    wifiManager.disconnect();
    TEST_ASSERT_FALSE(wifiManager.isConnected());
    TEST_ASSERT_EQUAL(WiFiState::DISCONNECTED, wifiManager.getState());
}

// ============================================================================
// HTTP Tests
// ============================================================================

void test_http_get_success(void) {
    wifiManager.connect(3, 10000);
    
    String url = String(SERVER_URL) + HEALTH_ENDPOINT;
    HttpResponse response = wifiManager.httpGet(url, 5000);
    
    TEST_ASSERT_TRUE(response.success);
    TEST_ASSERT_EQUAL(200, response.statusCode);
    TEST_ASSERT_GREATER_THAN(0, response.body.length());
    TEST_ASSERT_TRUE(response.body.indexOf("ok") > 0);  // {"ok": true}
}

void test_http_get_invalid_url(void) {
    wifiManager.connect(3, 10000);
    
    HttpResponse response = wifiManager.httpGet("http://invalid.local/test", 2000);
    
    TEST_ASSERT_FALSE(response.success);
    TEST_ASSERT_NOT_EQUAL(200, response.statusCode);
}

void test_http_get_state(void) {
    wifiManager.connect(3, 10000);
    
    String url = String(SERVER_URL) + STATE_ENDPOINT;
    HttpResponse response = wifiManager.httpGet(url, 10000);
    
    TEST_ASSERT_TRUE(response.success);
    TEST_ASSERT_EQUAL(200, response.statusCode);
    TEST_ASSERT_GREATER_THAN(0, response.body.length());
    TEST_ASSERT_TRUE(response.body.indexOf("otta-backlog") > 0);  // Should contain state keys
}

void test_http_without_wifi(void) {
    // Ensure WiFi is disconnected
    wifiManager.disconnect();
    
    HttpResponse response = wifiManager.httpGet("http://example.com", 1000);
    
    TEST_ASSERT_FALSE(response.success);
    TEST_ASSERT_EQUAL_STRING("WiFi not connected", response.error.c_str());
}

// ============================================================================
// Retry Logic Tests
// ============================================================================

void test_wifi_retry_on_failure(void) {
    // This test requires temporarily setting invalid credentials
    // For now, just verify the retry mechanism doesn't crash
    
    // Note: This will fail to connect, but should handle gracefully
    bool connected = wifiManager.connect(2, 2000);  // Short timeout, 2 retries
    
    // Should return false but not crash
    TEST_ASSERT_FALSE(connected);
    TEST_ASSERT_EQUAL(WiFiState::FAILED, wifiManager.getState());
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000);  // Wait for serial monitor
    
    UNITY_BEGIN();
    
    // Connection tests
    RUN_TEST(test_wifi_connect_success);
    RUN_TEST(test_wifi_rssi_valid);
    RUN_TEST(test_wifi_disconnect);
    
    // HTTP tests
    RUN_TEST(test_http_get_success);
    RUN_TEST(test_http_get_invalid_url);
    RUN_TEST(test_http_get_state);
    RUN_TEST(test_http_without_wifi);
    
    // Retry tests
    RUN_TEST(test_wifi_retry_on_failure);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
