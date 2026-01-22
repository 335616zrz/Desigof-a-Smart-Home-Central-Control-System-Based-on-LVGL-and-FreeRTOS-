package iot.controller;

import iot.music.IndexEntry;
import iot.music.MusicFileService;
import iot.music.TranscodeService;
import iot.music.dto.AddMusicRequest;
import iot.music.dto.MusicFileDTO;
import iot.music.dto.MusicTrackDTO;
import iot.music.dto.TranscodeStatus;
import iot.music.dto.UpdateMusicRequest;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/music")
public class MusicController {
    private final MusicFileService fileService;
    private final TranscodeService transcodeService;

    public MusicController(MusicFileService fileService, TranscodeService transcodeService) {
        this.fileService = fileService;
        this.transcodeService = transcodeService;
    }

    /**
     * List tracks for both WeChat mini program and web UI.
     * NOTE: Do NOT wrap this response with ApiResponse to keep existing clients compatible.
     */
    @GetMapping
    public List<MusicTrackDTO> list(@RequestParam(required = false) String keyword) {
        List<IndexEntry> entries = fileService.loadIndex();
        if (keyword != null && !keyword.isBlank()) {
            final String kw = keyword.trim().toLowerCase(Locale.ROOT);
            entries = entries.stream()
                    .filter(e -> {
                        String t = e.getTitle() == null ? "" : e.getTitle().toLowerCase(Locale.ROOT);
                        String f = e.getFilename() == null ? "" : e.getFilename().toLowerCase(Locale.ROOT);
                        return t.contains(kw) || f.contains(kw);
                    })
                    .toList();
        }
        return fileService.toTrackDTOs(entries);
    }

    @GetMapping("/{id}")
    public ResponseEntity<MusicTrackDTO> get(@PathVariable String id) {
        String filename = fileService.decodeId(id);
        List<IndexEntry> entries = fileService.loadIndex();
        return entries.stream()
                .filter(e -> filename.equals(e.getFilename()))
                .findFirst()
                .map(e -> ResponseEntity.ok(fileService.toTrackDTOs(List.of(e)).get(0)))
                .orElse(ResponseEntity.notFound().build());
    }

    @PostMapping
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_ADMIN')")
    public ResponseEntity<MusicTrackDTO> add(@RequestBody AddMusicRequest request) {
        String filename = request == null ? null : request.getFilename();
        if (filename == null || filename.isBlank()) {
            // Backward compatibility: some clients send {title, url}.
            String url = request == null ? null : request.getUrl();
            filename = fileService.inferFilenameFromUrl(url);
        }
        if (filename == null || filename.isBlank()) return ResponseEntity.badRequest().build();

        final String finalFilename = filename;

        // Validate file exists under storage root.
        Path filePath = fileService.resolveMusicPath(finalFilename);
        if (!Files.isRegularFile(filePath)) {
            return ResponseEntity.badRequest().build();
        }

        List<IndexEntry> entries = fileService.loadIndex();
        boolean exists = entries.stream().anyMatch(e -> finalFilename.equals(e.getFilename()));
        if (exists) {
            return ResponseEntity.status(HttpStatus.CONFLICT).build();
        }

        IndexEntry entry = fileService.buildNewEntry(finalFilename, request.getTitle());
        entries = fileService.addEntrySorted(entries, entry);
        fileService.saveIndex(entries);

        // Async transcode (no-op for mp3/m4a/aac).
        transcodeService.transcodeAsync(finalFilename);

        return ResponseEntity.status(HttpStatus.CREATED).body(fileService.toTrackDTOs(List.of(entry)).get(0));
    }

    @PutMapping("/{id}")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_ADMIN')")
    public ResponseEntity<MusicTrackDTO> update(@PathVariable String id, @RequestBody UpdateMusicRequest request) {
        String filename = fileService.decodeId(id);
        if (request == null) {
            return ResponseEntity.badRequest().build();
        }

        List<IndexEntry> entries = fileService.loadIndex();
        boolean updated = false;
        IndexEntry found = null;
        for (IndexEntry entry : entries) {
            if (!filename.equals(entry.getFilename())) continue;
            if (request.getTitle() != null) entry.setTitle(request.getTitle());
            if (request.getDuration() != null) entry.setDuration(request.getDuration());
            updated = true;
            found = entry;
            break;
        }
        if (!updated || found == null) {
            return ResponseEntity.notFound().build();
        }
        fileService.saveIndex(entries);
        return ResponseEntity.ok(fileService.toTrackDTOs(List.of(found)).get(0));
    }

    @DeleteMapping("/{id}")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_ADMIN')")
    public ResponseEntity<Void> delete(@PathVariable String id) {
        String filename = fileService.decodeId(id);
        List<IndexEntry> entries = fileService.loadIndex();
        boolean removed = entries.removeIf(e -> filename.equals(e.getFilename()));
        if (!removed) {
            return ResponseEntity.notFound().build();
        }
        fileService.saveIndex(entries);
        return ResponseEntity.noContent().build();
    }

    // ===== Management endpoints =====

    @GetMapping("/files")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_ADMIN')")
    public List<MusicFileDTO> files() {
        return fileService.scanDirectory();
    }

    @GetMapping("/unindexed")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_ADMIN')")
    public List<MusicFileDTO> unindexed() {
        return fileService.getUnindexedFiles();
    }

    @PostMapping("/batch-add")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_ADMIN')")
    public ResponseEntity<Map<String, Object>> batchAdd(@RequestBody List<String> filenames) {
        if (filenames == null || filenames.isEmpty()) {
            return ResponseEntity.badRequest().build();
        }

        List<IndexEntry> entries = fileService.loadIndex();
        int added = 0;
        Map<String, String> errors = new LinkedHashMap<>();

        for (String filename : filenames) {
            if (filename == null || filename.isBlank()) continue;
            try {
                Path filePath = fileService.resolveMusicPath(filename);
                if (!Files.isRegularFile(filePath)) {
                    errors.put(filename, "not_found");
                    continue;
                }
                boolean exists = entries.stream().anyMatch(e -> filename.equals(e.getFilename()));
                if (exists) {
                    continue;
                }
                IndexEntry entry = fileService.buildNewEntry(filename, null);
                entries = fileService.addEntrySorted(entries, entry);
                added++;
                transcodeService.transcodeAsync(filename);
            } catch (IllegalArgumentException ex) {
                errors.put(filename, "invalid_filename");
            } catch (Exception ex) {
                errors.put(filename, "failed");
            }
        }

        if (added > 0) {
            fileService.saveIndex(entries);
        }

        return ResponseEntity.ok(Map.of(
                "added", added,
                "errors", errors
        ));
    }

    @PostMapping("/batch-transcode")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_ADMIN')")
    public ResponseEntity<Map<String, Object>> batchTranscode() {
        int queued = transcodeService.batchTranscodeMissingCache();
        return ResponseEntity.ok(Map.of("queued", queued));
    }

    @GetMapping("/transcode-status")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_ADMIN')")
    public List<TranscodeStatus> transcodeStatus() {
        return transcodeService.getStatus();
    }
}
