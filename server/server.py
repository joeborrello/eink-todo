#!/usr/bin/env python3
"""
Reference Flask server for e-ink checklist display.
Provides simple REST API for checklist management.

Run: python server.py
API: http://localhost:5000/api/checklist
"""

from flask import Flask, request, jsonify
from datetime import datetime
import json

app = Flask(__name__)

# In-memory checklist storage (replace with database for production)
checklist = {
    "items": [
        {"id": 1, "text": "Morning workout", "checked": False},
        {"id": 2, "text": "Review pull requests", "checked": False},
        {"id": 3, "text": "Team standup at 10am", "checked": False},
        {"id": 4, "text": "Finish firmware refactor", "checked": False},
        {"id": 5, "text": "Update documentation", "checked": False},
        {"id": 6, "text": "Code review for Sarah", "checked": False},
        {"id": 7, "text": "Deploy staging build", "checked": False},
        {"id": 8, "text": "Evening run", "checked": False},
    ],
    "last_updated": datetime.now().isoformat()
}

@app.route('/')
def index():
    """Simple status page."""
    return jsonify({
        "service": "E-Ink Checklist Server",
        "status": "running",
        "endpoints": {
            "GET /api/checklist": "Fetch current checklist",
            "POST /api/toggle": "Toggle item by ID",
            "POST /api/update": "Update entire checklist"
        },
        "item_count": len(checklist["items"]),
        "last_updated": checklist["last_updated"]
    })

@app.route('/api/checklist', methods=['GET'])
def get_checklist():
    """
    GET /api/checklist
    Returns current checklist.
    """
    app.logger.info(f"Checklist fetched: {len(checklist['items'])} items")
    return jsonify(checklist)

@app.route('/api/toggle', methods=['POST'])
def toggle_item():
    """
    POST /api/toggle
    Body: {"id": 1}
    Toggles the checked state of an item.
    """
    data = request.get_json()
    
    if not data or 'id' not in data:
        return jsonify({"error": "Missing 'id' in request"}), 400
    
    item_id = data['id']
    
    # Find and toggle item
    for item in checklist['items']:
        if item['id'] == item_id:
            item['checked'] = not item['checked']
            checklist['last_updated'] = datetime.now().isoformat()
            
            app.logger.info(f"Toggled item {item_id}: {item['text']} -> {item['checked']}")
            
            return jsonify({
                "success": True,
                "item": item
            })
    
    return jsonify({"error": f"Item {item_id} not found"}), 404

@app.route('/api/update', methods=['POST'])
def update_checklist():
    """
    POST /api/update
    Body: {"items": [...]}
    Replaces entire checklist.
    """
    data = request.get_json()
    
    if not data or 'items' not in data:
        return jsonify({"error": "Missing 'items' in request"}), 400
    
    # Validate items
    for item in data['items']:
        if 'id' not in item or 'text' not in item:
            return jsonify({"error": "Each item must have 'id' and 'text'"}), 400
        if 'checked' not in item:
            item['checked'] = False  # Default to unchecked
    
    checklist['items'] = data['items']
    checklist['last_updated'] = datetime.now().isoformat()
    
    app.logger.info(f"Checklist updated: {len(checklist['items'])} items")
    
    return jsonify({
        "success": True,
        "item_count": len(checklist['items'])
    })

@app.route('/api/reset', methods=['POST'])
def reset_checklist():
    """
    POST /api/reset
    Resets all items to unchecked.
    """
    for item in checklist['items']:
        item['checked'] = False
    
    checklist['last_updated'] = datetime.now().isoformat()
    
    app.logger.info("Checklist reset: all items unchecked")
    
    return jsonify({
        "success": True,
        "message": "All items reset to unchecked"
    })

if __name__ == '__main__':
    print("=" * 60)
    print("E-Ink Checklist Server")
    print("=" * 60)
    print(f"Starting server on http://0.0.0.0:5000")
    print(f"Checklist items: {len(checklist['items'])}")
    print("\nEndpoints:")
    print("  GET  /api/checklist  - Fetch current checklist")
    print("  POST /api/toggle     - Toggle item (body: {\"id\": 1})")
    print("  POST /api/update     - Update checklist (body: {\"items\": [...]})")
    print("  POST /api/reset      - Reset all to unchecked")
    print("\nPress Ctrl+C to stop")
    print("=" * 60)
    
    app.run(host='0.0.0.0', port=5000, debug=True)
