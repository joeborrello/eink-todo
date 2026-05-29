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

#endif // CHECKLIST_MODEL_H
