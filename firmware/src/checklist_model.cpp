#include "checklist_model.h"
#include <ArduinoJson.h>

bool parseStateJSON(const String& json, TaskList& list) {
    list.clear();

    // Step 1: parse outer KV object (~4 KB budget)
    JsonDocument outer;
    DeserializationError err = deserializeJson(outer, json);
    if (err) {
        Serial.printf("Outer parse error: %s\n", err.c_str());
        return false;
    }

    // Step 2: streak — stored as a JSON string e.g. "4"
    const char* streakStr = outer["otta-streak"] | "0";
    list.streak = atoi(streakStr);

    // Step 3: backlog — value is itself a JSON-encoded string
    const char* backlogStr = outer["otta-backlog"] | "[]";
    JsonDocument tasks;
    err = deserializeJson(tasks, backlogStr);
    if (err) {
        Serial.printf("Backlog parse error: %s\n", err.c_str());
        return false;
    }

    uint8_t count = 0;
    for (JsonObject task : tasks.as<JsonArray>()) {
        if (count >= list.max_items) break;
        const char* text = task["text"] | "";
        if (strlen(text) == 0) continue;
        TaskItem item;
        item.text = String(text);
        item.difficulty = task["difficulty"].isNull() ? "" : task["difficulty"].as<String>();
        list.items.push_back(item);
        count++;
    }

    Serial.printf("Parsed %d tasks, streak=%d\n", (int)list.items.size(), list.streak);
    return true;
}
