package iot.controller;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Duration;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

@RestController
@RequestMapping("/api/events")
public class EventStreamController {

    private static final ScheduledExecutorService SCHEDULER = Executors.newScheduledThreadPool(2);
    private static final Path LOG_PATH = Paths.get("logs/app.log");
    private static final Duration INTERVAL = Duration.ofSeconds(2);

    @GetMapping(path = "/stream", produces = MediaType.TEXT_EVENT_STREAM_VALUE)
    public SseEmitter stream() throws IOException {
        ensureLogFile();
        SseEmitter emitter = new SseEmitter(0L); // no timeout
        long[] pointer = {Files.size(LOG_PATH)};

        ScheduledFuture<?> future = SCHEDULER.scheduleAtFixedRate(() -> {
            try {
                if (!Files.exists(LOG_PATH)) {
                    return;
                }
                long size = Files.size(LOG_PATH);
                if (size < pointer[0]) {
                    // log rotated, reset pointer
                    pointer[0] = 0;
                }
                if (size > pointer[0]) {
                    try (RandomAccessFile raf = new RandomAccessFile(LOG_PATH.toFile(), "r")) {
                        raf.seek(pointer[0]);
                        String line;
                        while ((line = raf.readLine()) != null) {
                            emitter.send(SseEmitter.event()
                                    .name("log")
                                    .data(line));
                        }
                        pointer[0] = raf.getFilePointer();
                    }
                }
            } catch (Exception e) {
                emitter.completeWithError(e);
            }
        }, 0, INTERVAL.toMillis(), TimeUnit.MILLISECONDS);

        emitter.onCompletion(() -> future.cancel(true));
        emitter.onTimeout(() -> future.cancel(true));
        emitter.onError(e -> future.cancel(true));
        return emitter;
    }

    private void ensureLogFile() throws IOException {
        if (!Files.exists(LOG_PATH.getParent())) {
            Files.createDirectories(LOG_PATH.getParent());
        }
        if (!Files.exists(LOG_PATH)) {
            Files.createFile(LOG_PATH);
        }
    }
}
