package iot.music.dto;

import lombok.Data;

@Data
public class AddMusicRequest {
    /**
     * Original filename as it exists under Music_Server (not encoded).
     * Example: "128 晴天.flac"
     */
    private String filename;

    /** Optional override title. If empty, backend derives from filename. */
    private String title;

    /**
     * Backward-compatible field for existing clients (e.g. WeChat mini program) that post "url".
     * When provided, backend will infer filename from the last path segment.
     */
    private String url;
}
