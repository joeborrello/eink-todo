#!/usr/bin/env bash
# Daily backup of the SQLite DB. Runs from cron or a systemd timer.
# Uses sqlite3 .backup so it's safe while the app is writing.
#
# Install:
#   sudo cp ops/backup.sh /usr/local/bin/lisa-tasks-backup
#   sudo chmod +x /usr/local/bin/lisa-tasks-backup
#   sudo crontab -e
#     0 4 * * *  /usr/local/bin/lisa-tasks-backup >> /var/log/lisa-tasks-backup.log 2>&1

set -euo pipefail

DB="${LISA_TASKS_DB:-/var/lib/lisa-tasks/tasks.db}"
BACKUP_DIR="${LISA_TASKS_BACKUP_DIR:-/var/backups/lisa-tasks}"
KEEP_DAYS="${LISA_TASKS_BACKUP_KEEP_DAYS:-30}"

mkdir -p "$BACKUP_DIR"
TS=$(date -u +%Y%m%dT%H%M%SZ)
OUT="$BACKUP_DIR/tasks-$TS.db"

sqlite3 "$DB" ".backup '$OUT'"
gzip -9 "$OUT"

# Prune old backups
find "$BACKUP_DIR" -maxdepth 1 -name 'tasks-*.db.gz' -type f -mtime +"$KEEP_DAYS" -delete

echo "[$(date -u +%FT%TZ)] backed up to ${OUT}.gz"
