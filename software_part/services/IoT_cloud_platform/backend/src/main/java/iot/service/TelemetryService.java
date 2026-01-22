package iot.service;

import iot.domain.TelemetryRecord;
import java.time.LocalDateTime;
import java.util.List;

public interface TelemetryService {

    void recordTelemetry(String deviceId, String payload);

    List<TelemetryRecord> latest(String deviceId, int limit);

    List<TelemetryRecord> queryRange(String deviceId, LocalDateTime start, LocalDateTime end, int limit);

    List<TelemetryRecord> queryRangePaged(String deviceId, LocalDateTime start, LocalDateTime end, int page, int pageSize);

    long countByDeviceAndRange(String deviceId, LocalDateTime start, LocalDateTime end);

    List<TelemetryRecord> queryRangeAllPaged(LocalDateTime start, LocalDateTime end, int page, int pageSize);

    long countAllInRange(LocalDateTime start, LocalDateTime end);

    TelemetryRecord findLatestWithin(String deviceId, LocalDateTime since);

    List<TelemetryRecord> latestAll(int limit);
}
