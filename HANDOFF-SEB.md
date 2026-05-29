# E-Ink To-Do Display — Handoff for Seb

## Context

Lisa's to-do app ("One Thing at a Time") is fully deployed on Joe's DigitalOcean
droplet. The server side is complete and live. Your job is to update the ESP32
firmware (in `firmware/`) to pull data from the live server instead of the
reference Flask server, and render it on the e-ink display.

---

## Repo structure

```
eink-todo/
├── firmware/               ← your side (ESP32 / PlatformIO)
│   ├── platformio.ini
│   ├── include/
│   │   ├── config.h        ← update SERVER_URL here
│   │   ├── pins.h
│   │   ├── checklist_model.h
│   │   └── wifi_manager.h
│   ├── src/
│   │   ├── main.cpp        ← update JSON parsing here
│   │   ├── checklist_model.cpp
│   │   └── wifi_manager.cpp
│   ├── test/
│   └── data/
├── server/                 ← live server (FastAPI / SQLite)
│   ├── app.py
│   ├── requirements.txt
│   └── static/
│       └── tasks.html
├── ops/                    ← deployment config
│   ├── lisa-tasks.service
│   ├── nginx-location.conf
│   └── backup.sh
└── HANDOFF-SEB.md          ← you are here
```

The `firmware/server/` reference Flask server has been removed — it's superseded
by the live FastAPI server documented below.

---

## Server

| | |
|---|---|
| Host | `droplet.josephborrello.com` |
| Provider | DigitalOcean VPS, always-on |
| TLS | Let's Encrypt (Certbot) |
| Auth | None — open to the public internet |

---

## API — what the firmware needs to call

### `GET https://droplet.josephborrello.com/tasks/api/state`

Returns all task data in one HTTPS call. No auth required.

**Response:**
```json
{
  "otta-backlog":  "[{\"text\":\"Reply to dentist\",\"difficulty\":\"easy\"},{\"text\":\"Draft Q2 notes\",\"difficulty\":\"medium\"}]",
  "otta-someday":  "[{\"text\":\"Learn Figma basics\",\"difficulty\":null}]",
  "otta-streak":   "4",
  "otta-sessions": "[{\"emoji\":\"🏆\",\"date\":\"May 18\",\"tasks\":5,\"skips\":0,\"mode\":\"ondeck\"}]",
  "otta-archive":  "[{\"text\":\"Pay electricity bill\",\"difficulty\":\"easy\",\"date\":\"May 18\",\"spentSecs\":47,\"allocatedMins\":null}]"
}
```

**Important:** Every value is a **JSON-encoded string**. Parse the outer object
first, then `json.loads()` / `deserializeJson()` each value individually.

### `GET https://droplet.josephborrello.com/tasks/api/health`

Returns `{"ok": true}`. Use this for a connectivity check on boot.

---

## Data schema

### Keys

| Key | UI label | Useful for display? |
|---|---|---|
| `otta-backlog` | On Deck | ✅ Primary — committed task list |
| `otta-someday` | Raw Tasks | Optional — unfiltered captures |
| `otta-streak` | Streak | ✅ Good motivational counter |
| `otta-sessions` | Trophy Case | Low priority for display |
| `otta-archive` | Archive | Low priority for display |

### Task object (from `otta-backlog` / `otta-someday`)
```json
{ "text": "Task description", "difficulty": "easy" }
```
`difficulty` is `"easy"`, `"medium"`, `"hard"`, or `null`.

### Streak (from `otta-streak`)
Plain integer stored as a JSON string, e.g. `"4"`. Parse with `atoi()` /
`parseInt()` after stripping quotes, or `json.loads()`.

---

## Firmware changes needed

The existing firmware was written against the reference Flask server
(`GET /api/checklist` → `{items: [{id, text, checked}]}`). Two things need
updating to target the live server:

### 1. `firmware/include/config.h` — update `SERVER_URL`

```cpp
// Before:
#define SERVER_URL "http://192.168.1.100:5000"

// After:
#define SERVER_URL "https://droplet.josephborrello.com"
```

The fetch path in `main.cpp` should become `/tasks/api/state`.

### 2. `firmware/src/main.cpp` — update JSON parsing

The live API shape differs from the reference server:

| | Reference server | Live server |
|---|---|---|
| Endpoint | `/api/checklist` | `/tasks/api/state` |
| Top-level | `{items: [...]}` | `{"otta-backlog": "[...]", ...}` |
| Task fields | `{id, text, checked}` | `{text, difficulty}` |
| Double-encoded? | No | Yes — each value is a JSON string |

Suggested parse sequence in Arduino/C++ (ArduinoJson):
```cpp
// 1. Parse the outer KV object
JsonDocument outer;
deserializeJson(outer, payload);

// 2. Extract and re-parse the backlog string
const char* backlogStr = outer["otta-backlog"];
JsonDocument tasks;
deserializeJson(tasks, backlogStr);

// 3. Iterate
for (JsonObject task : tasks.as<JsonArray>()) {
  const char* text = task["text"];
  const char* difficulty = task["difficulty"];  // may be null
  // render to display...
}

// 4. Streak
const char* streakStr = outer["otta-streak"];  // e.g. "4"
int streak = atoi(streakStr);
```

---

## If a simpler endpoint would help

The Claude Code agent (Root Access session) can add a dedicated
`GET /tasks/api/display` endpoint that returns a flat, pre-parsed structure
sized for the display — no double-decoding, just what the ESP32 needs. Ask Joe
to relay the exact payload shape you want and it'll be added to `server/app.py`
and deployed in minutes.

---

## Server management (for reference)

```bash
# Logs
journalctl -u lisa-tasks -f

# Restart
systemctl restart lisa-tasks

# DB location
/var/lib/lisa-tasks/tasks.db

# Backups (daily, 30-day retention)
/var/backups/lisa-tasks/
```
