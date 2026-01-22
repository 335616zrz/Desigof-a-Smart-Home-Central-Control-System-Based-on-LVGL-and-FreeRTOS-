package iot.mapper;

import iot.domain.DeviceShadow;
import java.time.LocalDateTime;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;

@Mapper
public interface DeviceShadowMapper {

    DeviceShadow findByDeviceId(@Param("deviceId") String deviceId);

    void upsertDesiredNightLight(@Param("deviceId") String deviceId,
                                 @Param("onJson") String onJson,
                                 @Param("updatedAt") LocalDateTime updatedAt);

    void upsertDesiredAlarm(@Param("deviceId") String deviceId,
                            @Param("activeJson") String activeJson,
                            @Param("updatedAt") LocalDateTime updatedAt);

    void upsertReportedNightLight(@Param("deviceId") String deviceId,
                                  @Param("onJson") String onJson,
                                  @Param("updatedAt") LocalDateTime updatedAt);

    void upsertReportedAlarm(@Param("deviceId") String deviceId,
                             @Param("activeJson") String activeJson,
                             @Param("updatedAt") LocalDateTime updatedAt);
}

