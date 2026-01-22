package iot.service;

import iot.domain.DeviceShadow;
import java.time.LocalDateTime;
import java.util.Optional;

public interface DeviceShadowService {

    Optional<DeviceShadow> findByDeviceId(String deviceId);

    void setDesiredNightLightOn(String deviceId, boolean on);

    void setDesiredAlarmActive(String deviceId, boolean active);

    void applyReportedFromTelemetry(String deviceId, String payload, LocalDateTime reportedAt);
}

