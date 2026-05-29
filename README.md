# E-Ink To-Do Display

Battery-powered wireless e-ink display for Lisa's "One Thing at a Time" to-do app.

## Components

- **Server** (`server/`) — FastAPI/SQLite backend, live at `https://droplet.josephborrello.com/tasks`
- **Firmware** (`firmware/`) — ESP32 firmware for the e-ink display (PlatformIO)
- **Ops** (`ops/`) — systemd service, nginx config, backup script

## Quick links

- Web app: `https://droplet.josephborrello.com/tasks`
- API: `https://droplet.josephborrello.com/tasks/api/state`
- Firmware setup: see `firmware/README.md`
- Server + API details: see `HANDOFF-SEB.md`
