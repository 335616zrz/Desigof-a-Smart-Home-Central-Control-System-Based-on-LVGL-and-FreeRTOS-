package iot.music.dto;

import lombok.Data;

@Data
public class MusicTrackDTO {
    /**
     * Opaque id used by API clients. We use "URL-encoded filename" so it is safe in path segments.
     * Example: "128%20%E6%99%B4%E5%A4%A9.flac"
     */
    private String id;

    private String title;

    /**
     * Playable URL for clients (WeChat): mp3/m4a/aac direct, others -> cached mp3 URL when ready.
     * Can be null when cache not ready.
     */
    private String url;

    private String duration;
    private String format;

    /** Original filename on disk (not encoded). */
    private String filename;

    /** Original URL stored in index.json (ESP32 uses this). */
    private String sourceUrl;

    private boolean cacheReady;
}

