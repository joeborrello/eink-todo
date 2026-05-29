/**
 * @file checklist_model.cpp
 * @brief Task list model — parses the double-encoded /tasks/api/state response.
 *
 * Response shape (outer object, values are JSON-encoded strings):
 *   {
 *     "otta-streak":  "4",
 *     "otta-backlog": "[{\"text\":\"Buy milk\",\"difficulty\":\"easy\"}, ...]"
 *   }
 */

#include "checklist_model.h"
#include <ArduinoJson.h>

bool parseStateJSON(const String& json, TaskList& list) {
    list.clear();

    // -------------------------------------------------------------------------
    // Step 1: parse the outer KV object (~4 KB budget covers full state payload)
    // -------------------------------------------------------------------------
    JsonDocument outer;
    DeserializationError err = deserializeJson(outer, json);
    if (err) {
        Serial.printf("[Model] Outer parse error: %s\n", err.c_str());
        return false;
    }

    // -------------------------------------------------------------------------
    // Step 2: streak — stored as a plain integer encoded as a JSON string, e.g. "4"
    // -------------------------------------------------------------------------
    const char* streakStr = outer["otta-streak"] | "0";
    list.streak = atoi(streakStr);

    // -------------------------------------------------------------------------
    // Step 3: backlog — value is itself a JSON-encoded string containing an array
    // -------------------------------------------------------------------------
    const char* backlogStr = outer["otta-backlog"] | "[]";
    JsonDocument tasks;
    err = deserializeJson(tasks, backlogStr);
    if (err) {
        Serial.printf("[Model] Backlog parse error: %s\n", err.c_str());
        return false;
    }

    uint8_t count = 0;
    for (JsonObject task : tasks.as<JsonArray>()) {
        if (count >= list.max_items) break;

        const char* text = task["text"] | "";
        if (strlen(text) == 0) continue;

        TaskItem item;
        item.text = String(text);
        item.difficulty = task["difficulty"].isNull()
                              ? ""
                              : task["difficulty"].as<String>();

        list.items.push_back(item);
        count++;
    }

    Serial.printf("[Model] Parsed %d tasks, streak=%d\n",
                  (int)list.items.size(), list.streak);
    return true;
}
