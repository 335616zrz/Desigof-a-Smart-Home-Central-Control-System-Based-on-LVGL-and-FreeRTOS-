package iot.music;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import iot.music.dto.MusicFileDTO;
import iot.music.dto.MusicTrackDTO;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.function.UnaryOperator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.web.util.UriUtils;

@Service
@RequiredArgsConstructor
public class MusicFileService {
    private static final TypeReference<List<IndexEntry>> INDEX_LIST_TYPE = new TypeReference<>() {};

    // Strip leading numeric prefix like "028 " for better titles.
    private static final Pattern TITLE_PREFIX = Pattern.compile("^(\\d{1,4})\\s+(.+)$");

    private final MusicProperties props;
    private final ObjectMapper objectMapper;

    private final ReadWriteLock indexLock = new ReentrantReadWriteLock();

    public Path storageRoot() {
        return Path.of(props.getStoragePath()).toAbsolutePath().normalize();
    }

    public Path cacheRoot() {
        return Path.of(props.getCachePath()).toAbsolutePath().normalize();
    }

    public Path indexPath() {
        return storageRoot().resolve("index.json");
    }

    public Path resolveMusicPath(String filename) {
        if (filename == null) throw new IllegalArgumentException("filename is null");
        // Disallow path traversal; we only accept direct filenames.
        if (filename.contains("/") || filename.contains("\\") || filename.contains("\0")) {
            throw new IllegalArgumentException("invalid filename");
        }
        Path root = storageRoot();
        Path p = root.resolve(filename).normalize();
        if (!p.startsWith(root)) {
            throw new IllegalArgumentException("path traversal");
        }
        return p;
    }

    public Path resolveCachePath(String cacheFilename) {
        if (cacheFilename == null) throw new IllegalArgumentException("cacheFilename is null");
        if (cacheFilename.contains("/") || cacheFilename.contains("\\") || cacheFilename.contains("\0")) {
            throw new IllegalArgumentException("invalid cache filename");
        }
        Path root = cacheRoot();
        Path p = root.resolve(cacheFilename).normalize();
        if (!p.startsWith(root)) {
            throw new IllegalArgumentException("path traversal");
        }
        return p;
    }

    public List<IndexEntry> loadIndex() {
        indexLock.readLock().lock();
        try {
            Path p = indexPath();
            if (!Files.exists(p)) {
                return new ArrayList<>();
            }
            byte[] raw = Files.readAllBytes(p);
            if (raw.length == 0) {
                return new ArrayList<>();
            }
            List<IndexEntry> entries = objectMapper.readValue(raw, INDEX_LIST_TYPE);
            if (entries == null) return new ArrayList<>();
            entries.forEach(this::normalizeEntryInPlace);
            return new ArrayList<>(entries);
        } catch (IOException e) {
            throw new RuntimeException("Failed to read index.json", e);
        } finally {
            indexLock.readLock().unlock();
        }
    }

    public void saveIndex(List<IndexEntry> entries) {
        if (entries == null) {
            throw new IllegalArgumentException("entries is null");
        }

        indexLock.writeLock().lock();
        try {
            writeIndexFileNoLock(entries);
        } catch (IOException e) {
            throw new RuntimeException("Failed to write index.json", e);
        } finally {
            indexLock.writeLock().unlock();
        }
    }

    /**
     * Read-modify-write index.json under a single write lock to avoid lost updates between concurrent operations
     * (e.g. batch-add vs async transcode completion).
     */
    public void updateIndex(UnaryOperator<List<IndexEntry>> updater) {
        if (updater == null) throw new IllegalArgumentException("updater is null");
        indexLock.writeLock().lock();
        try {
            List<IndexEntry> current = readIndexFileNoLock();
            List<IndexEntry> next = updater.apply(new ArrayList<>(current));
            if (next == null) next = current;
            writeIndexFileNoLock(next);
        } catch (IOException e) {
            throw new RuntimeException("Failed to update index.json", e);
        } finally {
            indexLock.writeLock().unlock();
        }
    }

    public List<MusicFileDTO> scanDirectory() {
        List<IndexEntry> index = loadIndex();
        Set<String> indexed = new HashSet<>();
        for (IndexEntry e : index) {
            if (e.getFilename() != null) indexed.add(e.getFilename());
        }

        Path root = storageRoot();
        List<MusicFileDTO> out = new ArrayList<>();
        if (!Files.isDirectory(root)) {
            return out;
        }
        try {
            try (var stream = Files.list(root)) {
                stream.filter(Files::isRegularFile)
                        .forEach(path -> {
                            String filename = path.getFileName().toString();
                            String fmt = getFormat(filename);
                            if (!isSupportedFormat(fmt)) return;
                            MusicFileDTO dto = new MusicFileDTO();
                            dto.setFilename(filename);
                            dto.setFormat(fmt);
                            dto.setId(encodeId(filename));
                            try {
                                dto.setSize(Files.size(path));
                            } catch (IOException e) {
                                dto.setSize(0L);
                            }
                            dto.setIndexed(indexed.contains(filename));
                            out.add(dto);
                        });
            }
        } catch (IOException e) {
            throw new RuntimeException("Failed to scan music directory", e);
        }

        out.sort(Comparator.comparing(MusicFileDTO::getFilename, String::compareTo));
        return out;
    }

    public List<MusicFileDTO> getUnindexedFiles() {
        return scanDirectory().stream().filter(f -> !f.isIndexed()).toList();
    }

    public IndexEntry buildNewEntry(String filename, String overrideTitle) {
        String fmt = getFormat(filename);
        if (!isSupportedFormat(fmt)) {
            throw new IllegalArgumentException("unsupported format: " + fmt);
        }

        IndexEntry entry = new IndexEntry();
        entry.setFilename(filename);
        entry.setFormat(fmt);
        entry.setTitle((overrideTitle == null || overrideTitle.isBlank()) ? deriveTitleFromFilename(filename) : overrideTitle);
        entry.setUrl(generateEsp32Url(filename));
        entry.setDuration(getDuration(filename));

        // Not ready until transcode done.
        if (!isWechatPlayableFormat(fmt)) {
            String cache = expectedCacheFilename(filename);
            entry.setCachedMp3(Files.exists(resolveCachePath(cache)) ? cache : null);
        }

        normalizeEntryInPlace(entry);
        return entry;
    }

    public List<IndexEntry> addEntrySorted(List<IndexEntry> entries, IndexEntry entry) {
        List<IndexEntry> out = new ArrayList<>(entries == null ? List.of() : entries);
        out.add(entry);
        // Keep a stable ordering by filename (matches "000/001/..." prefixes).
        out.sort(Comparator.comparing(e -> e.getFilename() == null ? "" : e.getFilename(), String::compareTo));
        return out;
    }

    public String encodeId(String filename) {
        return encodePathSegment(filename);
    }

    public String decodeId(String id) {
        if (id == null) throw new IllegalArgumentException("id is null");
        return UriUtils.decode(id, StandardCharsets.UTF_8);
    }

    public String encodePathSegment(String raw) {
        if (raw == null) return null;
        return UriUtils.encodePathSegment(raw, StandardCharsets.UTF_8);
    }

    public String generateEsp32Url(String filename) {
        String base = props.getEsp32BaseUrl() == null ? "" : props.getEsp32BaseUrl().trim();
        if (base.endsWith("/")) base = base.substring(0, base.length() - 1);
        return base + "/" + encodePathSegment(filename);
    }

    public String expectedCacheFilename(String filename) {
        int dot = filename.lastIndexOf('.');
        String base = (dot > 0 ? filename.substring(0, dot) : filename)
                .replace(' ', '_')
                .replace('/', '_')
                .replace('\\', '_');
        String ext = getFormat(filename);
        if (ext.isEmpty()) ext = "audio";
        return base + "__" + ext + ".mp3";
    }

    public String generatePlayableUrl(IndexEntry entry) {
        if (entry == null) return null;
        String fmt = (entry.getFormat() == null ? "" : entry.getFormat()).toLowerCase(Locale.ROOT);
        if (isWechatPlayableFormat(fmt)) {
            return entry.getUrl();
        }
        String cache = entry.getCachedMp3();
        if (cache == null || cache.isBlank()) {
            // Try auto-detect cache if present.
            if (entry.getFilename() != null) {
                String expected = expectedCacheFilename(entry.getFilename());
                if (Files.exists(resolveCachePath(expected))) {
                    entry.setCachedMp3(expected);
                    cache = expected;
                }
            }
        }
        if (cache == null || cache.isBlank()) return null;
        if (!Files.exists(resolveCachePath(cache))) return null;

        String base = props.getEsp32BaseUrl() == null ? "" : props.getEsp32BaseUrl().trim();
        if (base.endsWith("/")) base = base.substring(0, base.length() - 1);
        return base + "/cache/" + encodePathSegment(cache);
    }

    public List<MusicTrackDTO> toTrackDTOs(List<IndexEntry> entries) {
        if (entries == null) return List.of();
        return entries.stream().map(e -> {
            normalizeEntryInPlace(e);
            MusicTrackDTO dto = new MusicTrackDTO();
            String filename = e.getFilename();
            dto.setId(filename == null ? null : encodeId(filename));
            dto.setTitle(e.getTitle());
            dto.setDuration(e.getDuration());
            dto.setFormat(e.getFormat());
            dto.setFilename(filename);
            dto.setSourceUrl(e.getUrl());
            dto.setUrl(generatePlayableUrl(e));
            dto.setCacheReady(dto.getUrl() != null && !dto.getUrl().isBlank());
            return dto;
        }).toList();
    }

    public String getDuration(String filename) {
        Path input = resolveMusicPath(filename);
        if (!Files.isRegularFile(input)) return null;

        // Prefer ffprobe (covers mp3/flac/m4a/aac...). If missing, just return null.
        ProcessBuilder pb = new ProcessBuilder(
                "ffprobe",
                "-v", "error",
                "-show_entries", "format=duration",
                "-of", "default=noprint_wrappers=1:nokey=1",
                input.toString()
        );
        pb.redirectErrorStream(true);

        try {
            Process p = pb.start();
            boolean ok = p.waitFor(30, TimeUnit.SECONDS);
            if (!ok) {
                p.destroyForcibly();
                return null;
            }
            if (p.exitValue() != 0) return null;
            String out = new String(p.getInputStream().readAllBytes(), StandardCharsets.UTF_8).trim();
            if (out.isEmpty()) return null;
            String first = out.split("\\R", 2)[0].trim();
            double seconds = Double.parseDouble(first);
            if (seconds < 0) return null;

            long total = Math.round(seconds);
            Duration d = Duration.ofSeconds(total);
            long mm = d.toMinutes();
            long ss = d.minusMinutes(mm).getSeconds();
            return mm + ":" + String.format("%02d", ss);
        } catch (IOException | InterruptedException | NumberFormatException e) {
            if (e instanceof InterruptedException) {
                Thread.currentThread().interrupt();
            }
            return null;
        }
    }

    public boolean isWechatPlayableFormat(String fmt) {
        if (fmt == null) return false;
        return props.getWechatFormats().stream().anyMatch(f -> f.equalsIgnoreCase(fmt));
    }

    public boolean isSupportedFormat(String fmt) {
        if (fmt == null) return false;
        return props.getSupportedFormats().stream().anyMatch(f -> f.equalsIgnoreCase(fmt));
    }

    public String getFormat(String filename) {
        if (filename == null) return "";
        int dot = filename.lastIndexOf('.');
        if (dot < 0 || dot == filename.length() - 1) return "";
        return filename.substring(dot + 1).toLowerCase(Locale.ROOT);
    }

    public String deriveTitleFromFilename(String filename) {
        if (filename == null) return "";
        String base = filename;
        int dot = base.lastIndexOf('.');
        if (dot > 0) base = base.substring(0, dot);
        base = base.trim();
        Matcher m = TITLE_PREFIX.matcher(base);
        if (m.matches()) {
            return m.group(2).trim();
        }
        return base;
    }

    public String inferFilenameFromUrl(String url) {
        if (url == null || url.isBlank()) return null;
        String u = url.trim();
        int q = u.indexOf('?');
        if (q >= 0) u = u.substring(0, q);
        int slash = u.lastIndexOf('/');
        if (slash >= 0 && slash < u.length() - 1) {
            String segment = u.substring(slash + 1);
            // url is already percent-encoded.
            return UriUtils.decode(segment, StandardCharsets.UTF_8);
        }
        if (u.startsWith("/")) {
            // "/xxx.mp3"
            return UriUtils.decode(u.substring(1), StandardCharsets.UTF_8);
        }
        return null;
    }

    private void normalizeEntryInPlace(IndexEntry e) {
        if (e == null) return;
        if (e.getFilename() == null || e.getFilename().isBlank()) {
            String inferred = inferFilenameFromUrl(e.getUrl());
            if (inferred != null && !inferred.isBlank()) {
                e.setFilename(inferred);
            }
        }
        if (e.getFormat() == null || e.getFormat().isBlank()) {
            if (e.getFilename() != null) {
                e.setFormat(getFormat(e.getFilename()));
            }
        }
        if (e.getTitle() == null || e.getTitle().isBlank()) {
            if (e.getFilename() != null) {
                e.setTitle(deriveTitleFromFilename(e.getFilename()));
            }
        }
        if ((e.getUrl() == null || e.getUrl().isBlank()) && e.getFilename() != null) {
            e.setUrl(generateEsp32Url(e.getFilename()));
        }
    }

    private List<IndexEntry> readIndexFileNoLock() throws IOException {
        Path p = indexPath();
        if (!Files.exists(p)) {
            return new ArrayList<>();
        }
        byte[] raw = Files.readAllBytes(p);
        if (raw.length == 0) {
            return new ArrayList<>();
        }
        List<IndexEntry> entries = objectMapper.readValue(raw, INDEX_LIST_TYPE);
        if (entries == null) return new ArrayList<>();
        entries.forEach(this::normalizeEntryInPlace);
        return new ArrayList<>(entries);
    }

    private void writeIndexFileNoLock(List<IndexEntry> entries) throws IOException {
        Path root = storageRoot();
        Files.createDirectories(root);

        Path index = indexPath();
        Path tmp = root.resolve("index.json.tmp");

        entries.forEach(this::normalizeEntryInPlace);
        objectMapper.writerWithDefaultPrettyPrinter().writeValue(tmp.toFile(), entries);

        try {
            Files.move(tmp, index, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE);
        } catch (AtomicMoveNotSupportedException ex) {
            Files.move(tmp, index, StandardCopyOption.REPLACE_EXISTING);
        }
    }
}
