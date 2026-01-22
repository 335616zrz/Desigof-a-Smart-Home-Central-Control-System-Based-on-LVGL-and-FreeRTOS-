package iot.controller;

import iot.domain.TelemetryRecord;
import iot.dto.ApiResponse;
import iot.service.TelemetryService;
import java.time.LocalDateTime;
import java.util.List;
import org.springframework.format.annotation.DateTimeFormat;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

@RestController
public class TelemetryController {

    private final TelemetryService telemetryService;

    public TelemetryController(TelemetryService telemetryService) {
        this.telemetryService = telemetryService;
    }

    @GetMapping("/api/devices/{deviceId}/telemetry/latest")
    public List<TelemetryRecord> latest(@PathVariable String deviceId,
                                        @RequestParam(defaultValue = "50") int limit) {
        return telemetryService.latest(deviceId, Math.min(Math.max(limit, 1), 500));
    }

    @GetMapping("/api/devices/{deviceId}/telemetry")
    public ApiResponse<Paginated<TelemetryRecord>> queryRange(@PathVariable String deviceId,
                                            @RequestParam @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime start,
                                            @RequestParam @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime end,
                                            @RequestParam(defaultValue = "1") int page,
                                            @RequestParam(defaultValue = "100") int pageSize) {
        int size = Math.min(Math.max(pageSize, 1), 1000);
        List<TelemetryRecord> items = telemetryService.queryRangePaged(deviceId, start, end, page, size);
        long total = telemetryService.countByDeviceAndRange(deviceId, start, end);
        return ApiResponse.ok(new Paginated<>(items, total, page, size));
    }

    @GetMapping("/api/telemetry")
    public ApiResponse<Paginated<TelemetryRecord>> queryByDevice(@RequestParam(required = false) String deviceId,
                                               @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime from,
                                               @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime to,
                                               @RequestParam(defaultValue = "100") int pageSize,
                                               @RequestParam(defaultValue = "1") int page) {
        int size = Math.min(Math.max(pageSize, 1), 10000);
        List<TelemetryRecord> items;
        long total;

        // If time range specified, query by range (device optional)
        if (from != null && to != null) {
            if (deviceId == null || deviceId.isEmpty()) {
                items = telemetryService.queryRangeAllPaged(from, to, page, size);
                total = telemetryService.countAllInRange(from, to);
            } else {
                items = telemetryService.queryRangePaged(deviceId, from, to, page, size);
                total = telemetryService.countByDeviceAndRange(deviceId, from, to);
            }
        } else if (deviceId == null || deviceId.isEmpty()) {
            // No deviceId specified, return all devices' latest telemetry
            items = telemetryService.latestAll(size);
            total = items.size();
        } else {
            items = telemetryService.latest(deviceId, size);
            total = items.size();
        }
        return ApiResponse.ok(new Paginated<>(items, total, page, size));
    }

    @GetMapping("/api/telemetry/latest")
    public ApiResponse<List<TelemetryRecord>> latestByDevice(@RequestParam String deviceId,
                                                @RequestParam(defaultValue = "50") int limit) {
        return ApiResponse.ok(telemetryService.latest(deviceId, Math.min(Math.max(limit, 1), 500)));
    }

    private record Paginated<T>(List<T> items, long total, int page, int pageSize) {}
}
