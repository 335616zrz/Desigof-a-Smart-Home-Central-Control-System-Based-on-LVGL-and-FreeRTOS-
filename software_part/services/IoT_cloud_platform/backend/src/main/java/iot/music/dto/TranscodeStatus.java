package iot.music.dto;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@NoArgsConstructor
@AllArgsConstructor
public class TranscodeStatus {
    /** Original filename on disk (not encoded). */
    private String filename;

    /** pending | transcoding | completed | failed | skipped */
    private String status;

    /** Optional short message for debugging. */
    private String message;
}

