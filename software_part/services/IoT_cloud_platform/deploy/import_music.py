#!/usr/bin/env python3
import os
import glob
import mysql.connector
from urllib.parse import quote

MUSIC_DIR = os.environ.get("MUSIC_DIR", "/home/zrz/projects/2026_undergraduate_graduation_project/software_part/services/Music_Server")
MUSIC_BASE_URL = os.environ.get("MUSIC_BASE_URL", "http://localhost:8000")

db = mysql.connector.connect(
    host=os.environ.get("MYSQL_HOST", "localhost"),
    port=int(os.environ.get("MYSQL_PORT", 3306)),
    user=os.environ.get("MYSQL_USER", "iot_user"),
    password=os.environ.get("MYSQL_PASSWORD", ""),  # Must be set via environment
    database=os.environ.get("MYSQL_DATABASE", "iot_cloud")
)

cursor = db.cursor()

music_files = sorted(glob.glob(os.path.join(MUSIC_DIR, "*.mp3")) +
                     glob.glob(os.path.join(MUSIC_DIR, "*.wav")))

for idx, file_path in enumerate(music_files):
    filename = os.path.basename(file_path)
    title = os.path.splitext(filename)[0]

    if title[:3].isdigit():
        title = title[4:].strip()

    encoded_filename = quote(filename)
    url = f"{MUSIC_BASE_URL}/{encoded_filename}"

    cursor.execute(
        "INSERT INTO music_tracks (title, url, track_index) VALUES (%s, %s, %s)",
        (title, url, idx)
    )
    print(f"Imported: {title}")

db.commit()
cursor.close()
db.close()

print(f"\nTotal imported: {len(music_files)} tracks")
