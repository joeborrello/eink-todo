#include <Arduino.h>
#include <unity.h>
#include "checklist_model.h"

// ============================================================================
// Helpers — build the double-encoded format the server actually sends
// ============================================================================

// Wrap a backlog JSON array and streak value into the outer KV object
static String makeStateJSON(const String& backlogArray, int streak) {
    // Outer values are JSON-encoded strings (the server double-encodes them)
    String escaped = backlogArray;
    escaped.replace("\"", "\\\"");
    return "{\"otta-backlog\":\"" + escaped + "\",\"otta-streak\":\"" + String(streak) + "\"}";
}

// ============================================================================
// Tests
// ============================================================================

// Basic happy-path: two tasks, one with difficulty, streak=3
void test_parse_valid_state() {
    String backlog = R"([{"text":"Buy milk","difficulty":"easy"},{"text":"Write report","difficulty":"hard"}])";
    String json = makeStateJSON(backlog, 3);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(3, list.streak);
    TEST_ASSERT_EQUAL_INT(2, (int)list.items.size());
    TEST_ASSERT_EQUAL_STRING("Buy milk",     list.items[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("easy",         list.items[0].difficulty.c_str());
    TEST_ASSERT_EQUAL_STRING("Write report", list.items[1].text.c_str());
    TEST_ASSERT_EQUAL_STRING("hard",         list.items[1].difficulty.c_str());
}

// Null difficulty field should map to empty string
void test_parse_null_difficulty() {
    String backlog = R"([{"text":"Task A","difficulty":null}])";
    String json = makeStateJSON(backlog, 0);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, (int)list.items.size());
    TEST_ASSERT_EQUAL_STRING("", list.items[0].difficulty.c_str());
}

// Missing difficulty key should also map to empty string
void test_parse_missing_difficulty() {
    String backlog = R"([{"text":"Task B"}])";
    String json = makeStateJSON(backlog, 0);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("", list.items[0].difficulty.c_str());
}

// Empty backlog array — valid, zero items
void test_parse_empty_backlog() {
    String json = makeStateJSON("[]", 5);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(5, list.streak);
    TEST_ASSERT_EQUAL_INT(0, (int)list.items.size());
}

// Items with empty text should be skipped
void test_parse_skips_empty_text() {
    String backlog = R"([{"text":"","difficulty":"easy"},{"text":"Real task","difficulty":"medium"}])";
    String json = makeStateJSON(backlog, 0);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, (int)list.items.size());
    TEST_ASSERT_EQUAL_STRING("Real task", list.items[0].text.c_str());
}

// Truncation: more than max_items (20) should be capped
void test_parse_truncate_items() {
    String backlog = "[";
    for (int i = 1; i <= 25; i++) {
        backlog += "{\"text\":\"Task " + String(i) + "\",\"difficulty\":\"easy\"}";
        if (i < 25) backlog += ",";
    }
    backlog += "]";
    String json = makeStateJSON(backlog, 0);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(20, (int)list.items.size());
}

// Malformed outer JSON should return false
void test_parse_malformed_outer_json() {
    String json = R"({"otta-backlog": "not closed)";

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_FALSE(result);
}

// Malformed inner backlog string should return false
void test_parse_malformed_backlog_json() {
    // Outer is valid JSON but the backlog value is not a valid JSON array
    String json = R"({"otta-backlog":"[{\"text\":\"broken\"","otta-streak":"0"})";

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_FALSE(result);
}

// Missing otta-streak key should default to 0
void test_parse_missing_streak() {
    String backlog = R"([{"text":"Task","difficulty":"medium"}])";
    // Build outer without streak key
    String escaped = backlog;
    escaped.replace("\"", "\\\"");
    String json = "{\"otta-backlog\":\"" + escaped + "\"}";

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, list.streak);
}

// Round-trip: parse then serialize should produce equivalent content
void test_serialize_roundtrip() {
    String backlog = R"([{"text":"Buy milk","difficulty":"easy"},{"text":"Write report","difficulty":"hard"}])";
    String json = makeStateJSON(backlog, 3);

    TaskList list;
    TEST_ASSERT_TRUE(parseStateJSON(json, list));
    TEST_ASSERT_EQUAL_INT(2, (int)list.items.size());

    String serialized = serializeBacklog(list);

    // Re-parse the serialized output
    JsonDocument doc;
    TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, serialized));
    JsonArray arr = doc.as<JsonArray>();
    TEST_ASSERT_EQUAL_INT(2, (int)arr.size());
    TEST_ASSERT_EQUAL_STRING("Buy milk", arr[0]["text"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("easy",     arr[0]["difficulty"].as<const char*>());
}

// Serialize after removing an item
void test_serialize_after_remove() {
    String backlog = R"([{"text":"Task A","difficulty":"easy"},{"text":"Task B","difficulty":"medium"},{"text":"Task C","difficulty":"hard"}])";
    String json = makeStateJSON(backlog, 0);

    TaskList list;
    TEST_ASSERT_TRUE(parseStateJSON(json, list));

    // Remove middle item
    list.items.erase(list.items.begin() + 1);
    TEST_ASSERT_EQUAL_INT(2, (int)list.items.size());

    String serialized = serializeBacklog(list);

    JsonDocument doc;
    TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, serialized));
    JsonArray arr = doc.as<JsonArray>();
    TEST_ASSERT_EQUAL_INT(2, (int)arr.size());
    TEST_ASSERT_EQUAL_STRING("Task A", arr[0]["text"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Task C", arr[1]["text"].as<const char*>());
}

// Serialize empty list produces "[]"
void test_serialize_empty_list() {
    TaskList list;
    String serialized = serializeBacklog(list);
    TEST_ASSERT_EQUAL_STRING("[]", serialized.c_str());
}

// Null difficulty round-trips as JSON null
void test_serialize_null_difficulty() {
    String backlog = R"([{"text":"Task A","difficulty":null}])";
    String json = makeStateJSON(backlog, 0);

    TaskList list;
    TEST_ASSERT_TRUE(parseStateJSON(json, list));
    TEST_ASSERT_EQUAL_STRING("", list.items[0].difficulty.c_str());

    String serialized = serializeBacklog(list);

    JsonDocument doc;
    deserializeJson(doc, serialized);
    TEST_ASSERT_TRUE(doc[0]["difficulty"].isNull());
}

// ============================================================================
// Runner
// ============================================================================

void setup() {
    delay(2000);  // Wait for serial monitor
    UNITY_BEGIN();

    RUN_TEST(test_parse_valid_state);
    RUN_TEST(test_parse_null_difficulty);
    RUN_TEST(test_parse_missing_difficulty);
    RUN_TEST(test_parse_empty_backlog);
    RUN_TEST(test_parse_skips_empty_text);
    RUN_TEST(test_parse_truncate_items);
    RUN_TEST(test_parse_malformed_outer_json);
    RUN_TEST(test_parse_malformed_backlog_json);
    RUN_TEST(test_parse_missing_streak);
    RUN_TEST(test_serialize_roundtrip);
    RUN_TEST(test_serialize_after_remove);
    RUN_TEST(test_serialize_empty_list);
    RUN_TEST(test_serialize_null_difficulty);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
