package iot.music;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import lombok.Data;

/**
 * Item stored in Music_Server/index.json.
 *
 * ESP32 only requires: title + url (duration optional).
 * Other fields are for management / WeChat compatibility and are ignored by ESP32.
 */
@Data
@JsonIgnoreProperties(ignoreUnknown = true)
public class IndexEntry {
    private String title;
    private String url;
    private String duration;

    // management fields (optional)
    private String filename;
    private String format;
    private String cachedMp3;
}

