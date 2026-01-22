package iot.mapper;

import iot.domain.TelemetryRecord;
import java.time.LocalDateTime;
import java.util.List;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;

@Mapper
public interface TelemetryMapper {

    void insert(TelemetryRecord record);

    List<TelemetryRecord> findLatestByDevice(@Param("deviceId") String deviceId,
                                             @Param("limit") int limit);

    List<TelemetryRecord> findByDeviceAndRange(@Param("deviceId") String deviceId,
                                               @Param("start") LocalDateTime start,
                                               @Param("end") LocalDateTime end,
                                               @Param("limit") int limit);

    List<TelemetryRecord> findByDevicePaged(@Param("deviceId") String deviceId,
                                            @Param("start") LocalDateTime start,
                                            @Param("end") LocalDateTime end,
                                            @Param("limit") int limit,
                                            @Param("offset") int offset);

    long countByDeviceAndRange(@Param("deviceId") String deviceId,
                               @Param("start") LocalDateTime start,
                               @Param("end") LocalDateTime end);

    List<TelemetryRecord> findByRangePaged(@Param("start") LocalDateTime start,
                                           @Param("end") LocalDateTime end,
                                           @Param("limit") int limit,
                                           @Param("offset") int offset);

    long countByRange(@Param("start") LocalDateTime start, @Param("end") LocalDateTime end);

    TelemetryRecord findLatestWithin(@Param("deviceId") String deviceId, @Param("since") LocalDateTime since);

    List<TelemetryRecord> findAllLatest(@Param("limit") int limit);
}
