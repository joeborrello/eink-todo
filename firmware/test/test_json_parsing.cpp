#include <Arduino.h>
#include <unity.h>
#include "checklist_model.h"

// Test valid JSON parsing
void test_parse_valid_json() {
    String json = R"({
        "title": "Test Checklist",
        "updated_at": "2025-05-10T12:00:00Z",
        "items": [
            {"id": 1, "text": "Task 1", "checked": true},
            {"id": 2, "text": "Task 2", "checked": false}
        ]
    })";
    
    Checklist checklist;
    bool result = parseChecklistJSON(json, checklist);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Test Checklist", checklist.title.c_str());
    TEST_ASSERT_EQUAL(2, checklist.items.size());
    TEST_ASSERT_EQUAL(1, checklist.items[0].id);
    TEST_ASSERT_EQUAL_STRING("Task 1", checklist.items[0].text.c_str());
    TEST_ASSERT_TRUE(checklist.items[0].checked);
    TEST_ASSERT_FALSE(checklist.items[1].checked);
}

// Test missing required fields
void test_parse_missing_title() {
    String json = R"({
        "items": [
            {"id": 1, "text": "Task 1", "checked": true}
        ]
    })";
    
    Checklist checklist;
    bool result = parseChecklistJSON(json, checklist);
    
    TEST_ASSERT_FALSE(result);
}

// Test malformed JSON
void test_parse_malformed_json() {
    String json = R"({"title": "Test", "items": [{"id": 1, "text": "Task 1"})";  // Missing closing braces
    
    Checklist checklist;
    bool result = parseChecklistJSON(json, checklist);
    
    TEST_ASSERT_FALSE(result);
}

// Test item truncation at max_items
void test_parse_truncate_items() {
    String json = R"({
        "title": "Long List",
        "items": [)";
    
    // Generate 25 items (exceeds default max_items = 20)
    for (int i = 1; i <= 25; i++) {
        json += "{\"id\": " + String(i) + ", \"text\": \"Task " + String(i) + "\", \"checked\": false}";
        if (i < 25) json += ",";
    }
    json += "]}";
    
    Checklist checklist;
    bool result = parseChecklistJSON(json, checklist);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(20, checklist.items.size());  // Should truncate to max_items
}

// Test toggle JSON creation
void test_create_toggle_json() {
    String json = createToggleJSON(42, true);
    
    TEST_ASSERT_TRUE(json.indexOf("\"id\":42") > 0);
    TEST_ASSERT_TRUE(json.indexOf("\"checked\":true") > 0);
}

// Test optional fields (updated_at)
void test_parse_optional_fields() {
    String json = R"({
        "title": "Minimal",
        "items": [
            {"id": 1, "text": "Task 1"}
        ]
    })";
    
    Checklist checklist;
    bool result = parseChecklistJSON(json, checklist);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("", checklist.updated_at.c_str());
    TEST_ASSERT_FALSE(checklist.items[0].checked);  // Should default to false
}

void setup() {
    delay(2000);  // Wait for serial monitor
    UNITY_BEGIN();
    
    RUN_TEST(test_parse_valid_json);
    RUN_TEST(test_parse_missing_title);
    RUN_TEST(test_parse_malformed_json);
    RUN_TEST(test_parse_truncate_items);
    RUN_TEST(test_create_toggle_json);
    RUN_TEST(test_parse_optional_fields);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
