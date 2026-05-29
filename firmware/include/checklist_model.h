#ifndef CHECKLIST_MODEL_H
#define CHECKLIST_MODEL_H

#include <Arduino.h>
#include <vector>

/**
 * @brief Single task item from the otta-backlog list
 */
struct TaskItem {
    String text;
    String difficulty;  // "easy" | "medium" | "hard" | "" (null maps to "")
};

/**
 * @brief Task list with streak counter, parsed from /tasks/api/state
 */
struct TaskList {
    std::vector<TaskItem> items;  // from otta-backlog
    int streak = 0;               // from otta-streak
    uint8_t max_items = 20;

    void clear() {
        items.clear();
        streak = 0;
    }
};

/**
 * @brief Parse the double-encoded /tasks/api/state response into a TaskList.
 *
 * The server returns a flat KV object where every value is a JSON-encoded
 * string (double-encoded).  This function:
 *   1. Parses the outer KV object.
 *   2. Extracts "otta-streak" as an integer (stored as a JSON string, e.g. "4").
 *   3. Re-parses "otta-backlog" (itself a JSON-encoded array string) into items.
 *
 * @param json  Raw JSON string from the server
 * @param list  Output TaskList object
 * @return true if parsing succeeded, false otherwise
 */
bool parseStateJSON(const String& json, TaskList& list);

#endif // CHECKLIST_MODEL_H
