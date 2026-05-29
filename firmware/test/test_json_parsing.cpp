#include <Arduino.h>
#include <unity.h>
#include "checklist_model.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build a well-formed /tasks/api/state response.
// otta-backlog value is a JSON-encoded string (double-encoded array).
static String makeStateJSON(const String& backlogArrayJSON, int streak) {
    // Escape the inner JSON string for embedding as a JSON string value
    String escaped = backlogArrayJSON;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");

    String out = "{";
    out += "\"otta-streak\":\"" + String(streak) + "\",";
    out += "\"otta-backlog\":\"" + escaped + "\"";
    out += "}";
    return out;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// Test valid double-encoded state response
void test_parse_valid_state() {
    String backlog = "[{\"text\":\"Buy milk\",\"difficulty\":\"easy\"},{\"text\":\"Fix bug\",\"difficulty\":\"hard\"}]";
    String json = makeStateJSON(backlog, 5);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(5, list.streak);
    TEST_ASSERT_EQUAL(2, (int)list.items.size());
    TEST_ASSERT_EQUAL_STRING("Buy milk", list.items[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("easy", list.items[0].difficulty.c_str());
    TEST_ASSERT_EQUAL_STRING("Fix bug", list.items[1].text.c_str());
    TEST_ASSERT_EQUAL_STRING("hard", list.items[1].difficulty.c_str());
}

// Test null difficulty maps to empty string
void test_parse_null_difficulty() {
    String backlog = "[{\"text\":\"Walk dog\",\"difficulty\":null}]";
    String json = makeStateJSON(backlog, 0);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, (int)list.items.size());
    TEST_ASSERT_EQUAL_STRING("Walk dog", list.items[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("", list.items[0].difficulty.c_str());
}

// Test missing difficulty key also maps to empty string
void test_parse_missing_difficulty() {
    String backlog = "[{\"text\":\"Read book\"}]";
    String json = makeStateJSON(backlog, 2);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("", list.items[0].difficulty.c_str());
}

// Test malformed outer JSON
void test_parse_malformed_outer_json() {
    String json = "{\"otta-streak\":\"3\", \"otta-backlog\":";  // truncated

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_FALSE(result);
}

// Test malformed inner backlog JSON
void test_parse_malformed_backlog_json() {
    // Outer is valid but the inner backlog string is not valid JSON
    String json = "{\"otta-streak\":\"1\",\"otta-backlog\":\"[{\\\"text\\\":\\\"Oops\\\"\"}";  // missing ]}

    TaskList list;
    bool result = parseStateJSON(json, list);

    // Outer parse succeeds but inner parse fails → returns false
    TEST_ASSERT_FALSE(result);
}

// Test item truncation at max_items
void test_parse_truncate_items() {
    String backlog = "[";
    for (int i = 0; i < 25; i++) {
        if (i > 0) backlog += ",";
        backlog += "{\"text\":\"Task " + String(i + 1) + "\",\"difficulty\":\"easy\"}";
    }
    backlog += "]";

    String json = makeStateJSON(backlog, 0);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(20, (int)list.items.size());  // capped at max_items
}

// Test empty backlog array
void test_parse_empty_backlog() {
    String json = makeStateJSON("[]", 3);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, (int)list.items.size());
    TEST_ASSERT_EQUAL(3, list.streak);
}

// Test streak defaults to 0 when key is absent
void test_parse_missing_streak() {
    String json = "{\"otta-backlog\":\"[]\"}";

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, list.streak);
}

// Test items with empty text are skipped
void test_parse_skip_empty_text() {
    String backlog = "[{\"text\":\"\",\"difficulty\":\"easy\"},{\"text\":\"Valid\",\"difficulty\":\"medium\"}]";
    String json = makeStateJSON(backlog, 0);

    TaskList list;
    bool result = parseStateJSON(json, list);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, (int)list.items.size());
    TEST_ASSERT_EQUAL_STRING("Valid", list.items[0].text.c_str());
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

void setup() {
    delay(2000);  // Wait for serial monitor
    UNITY_BEGIN();

    RUN_TEST(test_parse_valid_state);
    RUN_TEST(test_parse_null_difficulty);
    RUN_TEST(test_parse_missing_difficulty);
    RUN_TEST(test_parse_malformed_outer_json);
    RUN_TEST(test_parse_malformed_backlog_json);
    RUN_TEST(test_parse_truncate_items);
    RUN_TEST(test_parse_empty_backlog);
    RUN_TEST(test_parse_missing_streak);
    RUN_TEST(test_parse_skip_empty_text);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
