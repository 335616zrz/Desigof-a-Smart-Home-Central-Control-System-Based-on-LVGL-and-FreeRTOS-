package iot.music;

import iot.music.dto.TranscodeStatus;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

@Service
@RequiredArgsConstructor
@Slf4j
public class TranscodeService {
    private final MusicProperties props;
    private final MusicFileService fileService;

    private final ExecutorService executor = Executors.newFixedThreadPool(2);
    private final Map<String, TranscodeStatus> statusMap = new ConcurrentHashMap<>();

    public void transcodeAsync(String filename) {
        if (filename == null || filename.isBlank()) return;

        String fmt = fileService.getFormat(filename);
        if (fileService.isWechatPlayableFormat(fmt)) {
            statusMap.put(filename, new TranscodeStatus(filename, "skipped", "wechat-playable"));
            return;
        }
        if (!fileService.isSupportedFormat(fmt)) {
            statusMap.put(filename, new TranscodeStatus(filename, "skipped", "unsupported"));
            return;
        }

        String cacheFilename = fileService.expectedCacheFilename(filename);
        Path inputPath = fileService.resolveMusicPath(filename);
        Path outputPath = fileService.resolveCachePath(cacheFilename);

        if (!Files.isRegularFile(inputPath)) {
            statusMap.put(filename, new TranscodeStatus(filename, "failed", "source_not_found"));
            return;
        }

        // If cache already exists, just mark ready and ensure index.json has cachedMp3.
        if (Files.isRegularFile(outputPath)) {
            statusMap.put(filename, new TranscodeStatus(filename, "completed", "cache_exists"));
            ensureIndexCachedMp3(filename, cacheFilename);
            return;
        }

        statusMap.put(filename, new TranscodeStatus(filename, "pending", null));

        executor.submit(() -> {
            statusMap.put(filename, new TranscodeStatus(filename, "transcoding", null));
            try {
                Files.createDirectories(outputPath.getParent());

                // Use ffmpeg for broad format support. Produce a WeChat-friendly mp3.
                ProcessBuilder pb = new ProcessBuilder(
                        "ffmpeg",
                        "-nostdin",
                        "-y",
                        "-i", inputPath.toString(),
                        "-vn",
                        "-b:a", "192k",
                        "-map_metadata", "0",
                        outputPath.toString()
                );
                pb.redirectErrorStream(true);

                Process p = pb.start();
                boolean ok = p.waitFor(10, TimeUnit.MINUTES);
                if (!ok) {
                    p.destroyForcibly();
                    statusMap.put(filename, new TranscodeStatus(filename, "failed", "timeout"));
                    return;
                }
                if (p.exitValue() != 0) {
                    String out = new String(p.getInputStream().readAllBytes(), StandardCharsets.UTF_8);
                    log.warn("ffmpeg failed for {} (exit={}): {}", filename, p.exitValue(), truncate(out, 1800));
                    statusMap.put(filename, new TranscodeStatus(filename, "failed", "ffmpeg_exit_" + p.exitValue()));
                    return;
                }

                ensureIndexCachedMp3(filename, cacheFilename);
                statusMap.put(filename, new TranscodeStatus(filename, "completed", null));
            } catch (IOException e) {
                log.error("Transcode failed for {}: {}", filename, e.getMessage(), e);
                statusMap.put(filename, new TranscodeStatus(filename, "failed", "io_error"));
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                statusMap.put(filename, new TranscodeStatus(filename, "failed", "interrupted"));
            } catch (Exception e) {
                log.error("Transcode failed for {}: {}", filename, e.getMessage(), e);
                statusMap.put(filename, new TranscodeStatus(filename, "failed", "error"));
            }
        });
    }

    public int batchTranscodeMissingCache() {
        List<IndexEntry> entries = fileService.loadIndex();
        int queued = 0;
        for (IndexEntry e : entries) {
            if (e == null) continue;
            String filename = e.getFilename();
            if (filename == null || filename.isBlank()) continue;
            String fmt = (e.getFormat() == null ? "" : e.getFormat()).toLowerCase(Locale.ROOT);
            if (fileService.isWechatPlayableFormat(fmt)) continue;

            // If cache already exists or entry already has cachedMp3, no need to queue.
            String expected = fileService.expectedCacheFilename(filename);
            Path output = fileService.resolveCachePath(expected);
            if (Files.isRegularFile(output) || (e.getCachedMp3() != null && !e.getCachedMp3().isBlank())) {
                continue;
            }

            transcodeAsync(filename);
            queued++;
        }
        return queued;
    }

    public List<TranscodeStatus> getStatus() {
        return new ArrayList<>(statusMap.values());
    }

    private void ensureIndexCachedMp3(String filename, String cacheFilename) {
        fileService.updateIndex(entries -> {
            boolean changed = false;
            for (IndexEntry e : entries) {
                if (e == null) continue;
                if (!filename.equals(e.getFilename())) continue;
                if (e.getCachedMp3() == null || e.getCachedMp3().isBlank()) {
                    e.setCachedMp3(cacheFilename);
                    changed = true;
                }
                if (e.getFormat() == null || e.getFormat().isBlank()) {
                    e.setFormat(fileService.getFormat(filename));
                    changed = true;
                }
                break;
            }
            return changed ? entries : entries;
        });
    }

    private static String truncate(String s, int max) {
        if (s == null) return "";
        String t = s.trim();
        if (t.length() <= max) return t;
        return t.substring(0, max) + "...";
    }
}

