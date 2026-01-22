package iot.music.dto;

import lombok.Data;

@Data
public class MusicFileDTO {
    /** URL-encoded filename (safe as a path segment). */
    private String id;
    private String filename;
    private String format;
    private long size;
    private boolean indexed;
}

