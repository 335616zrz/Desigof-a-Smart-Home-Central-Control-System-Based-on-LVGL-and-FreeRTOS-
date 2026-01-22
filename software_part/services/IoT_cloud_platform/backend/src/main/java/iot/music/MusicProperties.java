package iot.music;

import java.util.List;
import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

@Data
@Component
@ConfigurationProperties(prefix = "music")
public class MusicProperties {
    /**
     * Host path (or container path) to the Music_Server directory.
     * This directory is treated as the source of truth and must be writable by the backend.
     */
    private String storagePath = "/data/music";

    /**
     * Cache directory under storagePath (no leading dot to avoid being hidden by servers).
     * Example: /data/music/cache
     */
    private String cachePath = "/data/music/cache";

    /**
     * Public base URL used by ESP32 to download index.json referenced tracks.
     * Example: https://servers.local
     */
    private String esp32BaseUrl = "https://servers.local";

    /**
     * Audio formats allowed to be indexed from storagePath.
     */
    private List<String> supportedFormats = List.of("mp3", "flac", "m4a", "aac", "wav", "ogg");

    /**
     * Audio formats that WeChat mini program can play directly.
     */
    private List<String> wechatFormats = List.of("mp3", "m4a", "aac");
}

