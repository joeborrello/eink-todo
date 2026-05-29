#ifndef CHECKLIST_MODEL_H
#define CHECKLIST_MODEL_H

#include <Arduino.h>
#include <vector>

/**
 * @brief Single checklist item
 */
struct ChecklistItem {
    uint16_t id;
    String text;
    bool checked;
};

/**
 * @brief Complete checklist with metadata
 */
struct Checklist {
    String title;
    String updated_at;
    std::vector<ChecklistItem> items;
    uint8_t max_items = 20;  // Display limit (adjustable per screen size)
    
    void clear() {
        title = "";
        updated_at = "";
        items.clear();
    }
};

/**
 * @brief Parse JSON string into Checklist struct
 * @param json Raw JSON string from server
 * @param checklist Output checklist object
 * @return true if parsing succeeded, false otherwise
 */
bool parseChecklistJSON(const String& json, Checklist& checklist);

/**
 * @brief Serialize toggle request to JSON
 * @param id Item ID to toggle
 * @param checked New checked state
 * @return JSON string for POST body
 */
String createToggleJSON(uint16_t id, bool checked);

#endif // CHECKLIST_MODEL_H
