#ifndef CHECKLIST_MODEL_H
#define CHECKLIST_MODEL_H

#include <Arduino.h>
#include <vector>

/**
 * @brief Single task item from otta-backlog
 */
struct TaskItem {
    String text;
    String difficulty;  // "easy" | "medium" | "hard" | "" (null maps to "")
};

/**
 * @brief Complete task list with streak counter
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
 * @brief Parse the double-encoded /tasks/api/state response into a TaskList
 * @param json Raw JSON string from server
 * @param list Output TaskList object
 * @return true if parsing succeeded, false otherwise
 */
bool parseStateJSON(const String& json, TaskList& list);

/**
 * @brief Serialize a TaskList back to the double-encoded string the server expects.
 *
 * The server stores otta-backlog as a JSON-encoded string (double-encoded).
 * This produces the *inner* JSON array string — the caller wraps it in the
 * PUT body as: {"value": <result>}
 *
 * @param list  Source TaskList
 * @return      Inner JSON array string, e.g. "[{\"text\":\"...\",\"difficulty\":\"easy\"}]"
 */
String serializeBacklog(const TaskList& list);

#endif // CHECKLIST_MODEL_H
