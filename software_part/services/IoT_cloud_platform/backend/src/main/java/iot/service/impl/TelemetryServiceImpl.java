package iot.service.impl;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import iot.domain.TelemetryRecord;
import iot.event.TelemetryRecordedEvent;
import iot.mapper.TelemetryMapper;
import iot.service.TelemetryService;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.List;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.ApplicationEventPublisher;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@Transactional(readOnly = true)
public class TelemetryServiceImpl implements TelemetryService {

    private static final Logger log = LoggerFactory.getLogger(TelemetryServiceImpl.class);

    private final TelemetryMapper telemetryMapper;
    private final ObjectMapper objectMapper;
    private final ApplicationEventPublisher eventPublisher;

    public TelemetryServiceImpl(
            TelemetryMapper telemetryMapper,
            ObjectMapper objectMapper,
            ApplicationEventPublisher eventPublisher) {
        this.telemetryMapper = telemetryMapper;
        this.objectMapper = objectMapper;
        this.eventPublisher = eventPublisher;
    }

    @Override
    @Transactional
    public void recordTelemetry(String deviceId, String payload) {
        TelemetryRecord record = new TelemetryRecord();
        record.setDeviceId(deviceId);
        record.setPayload(normalizePayload(payload));

        // Parse timestamp from payload if available, otherwise use server time
        LocalDateTime reportedAt = parseTimestampFromPayload(payload);
        record.setReportedAt(reportedAt != null ? reportedAt : LocalDateTime.now());

        telemetryMapper.insert(record);

        // Evaluate alert rules after this transaction commits
        eventPublisher.publishEvent(new TelemetryRecordedEvent(record));
    }

    @Override
    public List<TelemetryRecord> latest(String deviceId, int limit) {
        return telemetryMapper.findLatestByDevice(deviceId, limit);
    }

    @Override
    public List<TelemetryRecord> queryRange(String deviceId, LocalDateTime start, LocalDateTime end, int limit) {
        return telemetryMapper.findByDeviceAndRange(deviceId, start, end, limit);
    }

    @Override
    public List<TelemetryRecord> queryRangePaged(String deviceId, LocalDateTime start, LocalDateTime end, int page, int pageSize) {
        int limit = Math.max(pageSize, 1);
        int offset = Math.max(page - 1, 0) * limit;
        return telemetryMapper.findByDevicePaged(deviceId, start, end, limit, offset);
    }

    @Override
    public List<TelemetryRecord> queryRangeAllPaged(LocalDateTime start, LocalDateTime end, int page, int pageSize) {
        int limit = Math.max(pageSize, 1);
        int offset = Math.max(page - 1, 0) * limit;
        return telemetryMapper.findByRangePaged(start, end, limit, offset);
    }

    @Override
    public long countByDeviceAndRange(String deviceId, LocalDateTime start, LocalDateTime end) {
        return telemetryMapper.countByDeviceAndRange(deviceId, start, end);
    }

    @Override
    public long countAllInRange(LocalDateTime start, LocalDateTime end) {
        return telemetryMapper.countByRange(start, end);
    }

    @Override
    public TelemetryRecord findLatestWithin(String deviceId, LocalDateTime since) {
        return telemetryMapper.findLatestWithin(deviceId, since);
    }

    @Override
    public List<TelemetryRecord> latestAll(int limit) {
        return telemetryMapper.findAllLatest(limit);
    }

    private String normalizePayload(String payload) {
        try {
            JsonNode node = objectMapper.readTree(payload);
            return objectMapper.writeValueAsString(node);
        } catch (Exception ex) {
            log.warn("Failed to parse telemetry payload, storing raw string. payload={}", payload, ex);
            return payload;
        }
    }

    /**
     * Parse timestamp from MQTT payload (ESP32 sends ISO 8601 format with Z timezone)
     * Example: {"deviceId":"esp32-001","timestamp":"2025-12-10T12:26:02.000Z",...}
     */
    private LocalDateTime parseTimestampFromPayload(String payload) {
        try {
            JsonNode node = objectMapper.readTree(payload);
            if (node.has("timestamp")) {
                String timestampStr = node.get("timestamp").asText();
                // Parse ISO 8601 format with UTC timezone (e.g., 2025-12-10T12:26:02.000Z)
                ZonedDateTime utcTime = ZonedDateTime.parse(timestampStr, DateTimeFormatter.ISO_DATE_TIME);
                // Convert UTC to Asia/Shanghai timezone (UTC+8)
                ZonedDateTime localTime = utcTime.withZoneSameInstant(ZoneId.of("Asia/Shanghai"));
                return localTime.toLocalDateTime();
            }
        } catch (Exception ex) {
            log.warn("Failed to parse timestamp from payload, will use server time. payload={}", payload, ex);
        }
        return null;
    }
}
