"""
Lisa's To-Do App — FastAPI backend.

Tiny KV-style server that mirrors the original frontend's localStorage shape.
The frontend treats storage as `string in, string out` (values are JSON-encoded
strings) — so the API does too: GET returns a dict of string values, PUT
accepts a raw JSON string and stores it verbatim.

HTTP Basic Auth is handled at the Nginx layer (auth_basic). This service
trusts that anything reaching it has been authenticated, and listens only on
loopback (127.0.0.1) — so it must not be exposed directly to the internet.
"""

from __future__ import annotations

import json
import os
import sqlite3
from contextlib import contextmanager
from pathlib import Path

from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import FileResponse, JSONResponse, RedirectResponse
from fastapi.staticfiles import StaticFiles

# ───────────────────────── config ─────────────────────────

DB_PATH = Path(os.environ.get("LISA_TASKS_DB", "/var/lib/lisa-tasks/tasks.db"))
STATIC_DIR = Path(__file__).parent / "static"

# Keys carried over 1:1 from the original frontend's localStorage. Do NOT
# rename — the frontend JS hardcodes these and the names are deliberately
# different from the user-facing labels (see ONBOARDING.md).
ALLOWED_KEYS = {
    "otta-backlog",     # "On Deck"
    "otta-someday",     # "Raw Tasks"
    "otta-streak",      # integer-as-string
    "otta-sessions",    # trophy case
    "otta-archive",     # completed-task log
}

DEFAULTS = {
    "otta-backlog": "[]",
    "otta-someday": "[]",
    "otta-streak": "0",
    "otta-sessions": "[]",
    "otta-archive": "[]",
}

# ───────────────────────── db ─────────────────────────

def init_db() -> None:
    DB_PATH.parent.mkdir(parents=True, exist_ok=True)
    with sqlite3.connect(DB_PATH) as c:
        c.execute(
            "CREATE TABLE IF NOT EXISTS kv ("
            "  key   TEXT PRIMARY KEY,"
            "  value TEXT NOT NULL"
            ")"
        )
        for k, v in DEFAULTS.items():
            c.execute(
                "INSERT OR IGNORE INTO kv (key, value) VALUES (?, ?)",
                (k, v),
            )


@contextmanager
def db():
    conn = sqlite3.connect(DB_PATH)
    try:
        yield conn
        conn.commit()
    finally:
        conn.close()


# ───────────────────────── app ─────────────────────────

app = FastAPI(title="Lisa's To-Do App", docs_url=None, redoc_url=None)


@app.on_event("startup")
def _startup() -> None:
    init_db()


# Health check — useful for nginx upstream checks and post-deploy smoke test.
@app.get("/tasks/api/health")
def health():
    return {"ok": True}


@app.get("/tasks/api/state")
def get_state():
    """Return all stored values as JSON strings (matching the frontend
    contract: `store.get(k)` returns a string)."""
    with db() as c:
        rows = c.execute("SELECT key, value FROM kv").fetchall()
    state = dict(rows)
    for k, v in DEFAULTS.items():
        state.setdefault(k, v)
    return state


@app.put("/tasks/api/list/{key}")
async def put_value(key: str, request: Request):
    """Replace the value for a single key.

    Body is the raw JSON-encoded string the frontend would have stored
    via `localStorage.setItem(k, JSON.stringify(arr))`. We validate it
    parses as JSON, then store it verbatim — no schema enforced beyond that.
    """
    if key not in ALLOWED_KEYS:
        raise HTTPException(status_code=404, detail=f"Unknown key: {key}")

    raw = await request.body()
    if not raw:
        raise HTTPException(status_code=400, detail="Empty body")
    try:
        json.loads(raw)
    except ValueError as e:
        raise HTTPException(status_code=400, detail=f"Body is not valid JSON: {e}")

    with db() as c:
        c.execute(
            "INSERT INTO kv (key, value) VALUES (?, ?) "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value",
            (key, raw.decode("utf-8")),
        )
    return {"ok": True, "key": key, "bytes": len(raw)}


# ───────────────────────── static / index ─────────────────────────

# Redirect bare /tasks → /tasks/ so the relative path doesn't 404.
@app.get("/tasks")
def _redirect_to_slash():
    return RedirectResponse(url="/tasks/", status_code=308)


@app.get("/tasks/")
def index():
    return FileResponse(STATIC_DIR / "tasks.html", media_type="text/html")


# Any future assets (favicon, CSS, etc.) drop into static/ and are served
# under /tasks/static/. The HTML itself is at /tasks/ above.
app.mount("/tasks/static", StaticFiles(directory=STATIC_DIR), name="static")
