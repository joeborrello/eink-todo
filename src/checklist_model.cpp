#include "checklist_model.h"
#include <ArduinoJson.h>

bool parseChecklistJSON(const String& json, Checklist& checklist) {
    // Clear existing data
    checklist.clear();
    
    // Allocate JSON document (4KB should handle ~20 items with 100-char text each)
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.print(F("JSON parse error: "));
        Serial.println(error.c_str());
        return false;
    }
    
    // Validate required fields
    if (!doc.containsKey("title") || !doc.containsKey("items")) {
        Serial.println(F("Missing required fields (title, items)"));
        return false;
    }
    
    // Extract metadata
    checklist.title = doc["title"].as<String>();
    checklist.updated_at = doc["updated_at"] | "";  // Optional field
    
    // Parse items array
    JsonArray items = doc["items"];
    if (items.isNull()) {
        Serial.println(F("Items field is not an array"));
        return false;
    }
    
    uint8_t count = 0;
    for (JsonObject item : items) {
        if (count >= checklist.max_items) {
            Serial.println(F("Warning: Truncating checklist to max_items"));
            break;
        }
        
        // Validate item fields
        if (!item.containsKey("id") || !item.containsKey("text")) {
            Serial.println(F("Skipping item: missing id or text"));
            continue;
        }
        
        ChecklistItem checklistItem;
        checklistItem.id = item["id"];
        checklistItem.text = item["text"].as<String>();
        checklistItem.checked = item["checked"] | false;  // Default to unchecked
        
        checklist.items.push_back(checklistItem);
        count++;
    }
    
    Serial.printf("Parsed %d items from checklist '%s'\n", checklist.items.size(), checklist.title.c_str());
    return true;
}

String createToggleJSON(uint16_t id, bool checked) {
    StaticJsonDocument<128> doc;
    doc["id"] = id;
    doc["checked"] = checked;
    
    String output;
    serializeJson(doc, output);
    return output;
}
