# E-Ink To-Do Display — Handoff for Seb

## Context

Lisa's to-do app ("One Thing at a Time") is a fully deployed web app on Joe's
DigitalOcean droplet. The server side is complete and live. Your job is to build
the ESP32 + e-ink display side that reads task data from the server and renders
it on the display.

---

## Server

| | |
|---|---|
| Host | `droplet.josephborrello.com` |
| Provider | DigitalOcean VPS |
| OS | Ubuntu (Linux) |
| TLS | Let's Encrypt (Certbot), auto-renewing |

The FastAPI app runs as a systemd service (`lisa-tasks`) behind nginx, listening
only on `127.0.0.1:8765`. Nginx terminates TLS and reverse-proxies
`/tasks/ → 127.0.0.1:8765`.

**No authentication is required.** The app is currently open to the public internet.

---

## API — what the ESP32 needs

### `GET https://droplet.josephborrello.com/tasks/api/state`

Returns all task data in one call. This is the only endpoint the display needs.

**Example response:**
```json
{
  "otta-backlog":  "[{\"text\":\"Reply to dentist\",\"difficulty\":\"easy\"},{\"text\":\"Draft Q2 notes\",\"difficulty\":\"medium\"}]",
  "otta-someday":  "[{\"text\":\"Learn Figma basics\",\"difficulty\":null}]",
  "otta-streak":   "4",
  "otta-sessions": "[{\"emoji\":\"🏆\",\"date\":\"May 18\",\"tasks\":5,\"skips\":0,\"mode\":\"ondeck\"}]",
  "otta-archive":  "[{\"text\":\"Pay electricity bill\",\"difficulty\":\"easy\",\"date\":\"May 18\",\"spentSecs\":47,\"allocatedMins\":null}]"
}
```

**Important:** Every value is a **JSON-encoded string** (not a nested object).
You must `JSON.parse()` / `json.loads()` each value after parsing the outer object.

### Health check

`GET https://droplet.josephborrello.com/tasks/api/health` → `{"ok": true}`

Useful for connectivity checks on boot.

---

## Data schema

### Keys

| Key | UI label | Content |
|---|---|---|
| `otta-backlog` | On Deck | Committed task list |
| `otta-someday` | Raw Tasks | Unfiltered captures |
| `otta-streak` | Streak | Integer (sessions completed), stored as string |
| `otta-sessions` | Trophy Case | Completed session log |
| `otta-archive` | Archive | Per-task completion log |

### Task object (`otta-backlog`, `otta-someday`)
```json
{ "text": "Task description", "difficulty": "easy" }
```
`difficulty` is `"easy"`, `"medium"`, `"hard"`, or `null`.

### Session object (`otta-sessions`)
```json
{ "emoji": "🏆", "date": "May 18", "tasks": 5, "skips": 0, "mode": "ondeck" }
```

### Archive entry (`otta-archive`)
```json
{ "text": "Task description", "difficulty": "easy", "date": "May 18", "spentSecs": 47, "allocatedMins": null }
```

---

## Suggested display layout

The most useful data for an e-ink display is probably:

1. **On Deck list** (`otta-backlog`) — what Lisa has committed to doing
2. **Streak** (`otta-streak`) — motivational counter
3. Optionally: item count from Raw Tasks (`otta-someday`)

The archive and sessions data is less useful for a passive display.

---

## If you need a slimmer endpoint

The Claude Code agent on the droplet (Root Access session) can add a dedicated
lightweight endpoint (e.g. `GET /tasks/api/display`) that returns only what the
ESP32 needs, pre-parsed, in a minimal JSON structure. Just ask Joe to relay the
request with the exact payload shape you want.

---

## Infrastructure details (for reference)

**Systemd service**
```
Unit:    lisa-tasks.service
User:    lisa-tasks (system user, no shell)
Start:   /opt/lisa-tasks/venv/bin/uvicorn app:app --host 127.0.0.1 --port 8765
Logs:    journalctl -u lisa-tasks -f
Restart: automatic on failure
```

**File locations**
```
App:     /opt/lisa-tasks/server/
Venv:    /opt/lisa-tasks/venv/
DB:      /var/lib/lisa-tasks/tasks.db
Backups: /var/backups/lisa-tasks/  (daily at 4am, 30-day retention)
```

**Database** — SQLite, single writer, no migrations needed for read-only access.
Table: `kv(key TEXT PRIMARY KEY, value TEXT NOT NULL)`

---

## Repo structure

```
eink-todo/
├── server/
│   ├── app.py               FastAPI backend (~150 LOC)
│   ├── requirements.txt     fastapi + uvicorn[standard]
│   └── static/
│       └── tasks.html       The web SPA (single file)
├── ops/
│   ├── lisa-tasks.service   systemd unit
│   ├── nginx-location.conf  nginx location block
│   └── backup.sh            daily SQLite backup script
└── HANDOFF-SEB.md           ← you are here
```
