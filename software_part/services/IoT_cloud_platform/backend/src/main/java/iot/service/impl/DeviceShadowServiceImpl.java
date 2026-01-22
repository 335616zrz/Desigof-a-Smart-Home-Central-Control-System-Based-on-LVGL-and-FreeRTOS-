package iot.service.impl;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import iot.domain.DeviceShadow;
import iot.mapper.DeviceShadowMapper;
import iot.service.DeviceShadowService;
import java.time.LocalDateTime;
import java.util.Optional;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@Transactional(readOnly = true)
public class DeviceShadowServiceImpl implements DeviceShadowService {

    private static final Logger log = LoggerFactory.getLogger(DeviceShadowServiceImpl.class);

    private final DeviceShadowMapper deviceShadowMapper;
    private final ObjectMapper objectMapper;

    public DeviceShadowServiceImpl(DeviceShadowMapper deviceShadowMapper, ObjectMapper objectMapper) {
        this.deviceShadowMapper = deviceShadowMapper;
        this.objectMapper = objectMapper;
    }

    @Override
    public Optional<DeviceShadow> findByDeviceId(String deviceId) {
        return Optional.ofNullable(deviceShadowMapper.findByDeviceId(deviceId));
    }

    @Override
    @Transactional
    public void setDesiredNightLightOn(String deviceId, boolean on) {
        deviceShadowMapper.upsertDesiredNightLight(deviceId, on ? "true" : "false", LocalDateTime.now());
    }

    @Override
    @Transactional
    public void setDesiredAlarmActive(String deviceId, boolean active) {
        deviceShadowMapper.upsertDesiredAlarm(deviceId, active ? "true" : "false", LocalDateTime.now());
    }

    @Override
    @Transactional
    public void applyReportedFromTelemetry(String deviceId, String payload, LocalDateTime reportedAt) {
        if (deviceId == null || deviceId.isBlank()) return;
        if (payload == null || payload.isBlank()) return;

        try {
            JsonNode root = objectMapper.readTree(payload);
            JsonNode data = root.has("data") ? root.get("data") : root;

            Boolean night = extractBoolean(data, "nightLightOn", "nightLight");
            Boolean alarm = extractBoolean(data, "alarmActive", "alarm");

            if (night == null && alarm == null) return;

            LocalDateTime ts = reportedAt != null ? reportedAt : LocalDateTime.now();
            if (night != null) {
                deviceShadowMapper.upsertReportedNightLight(deviceId, night ? "true" : "false", ts);
            }
            if (alarm != null) {
                deviceShadowMapper.upsertReportedAlarm(deviceId, alarm ? "true" : "false", ts);
            }
        } catch (Exception e) {
            log.debug("Failed to parse telemetry payload for shadow update. deviceId={}", deviceId, e);
        }
    }

    private static Boolean extractBoolean(JsonNode node, String... keys) {
        if (node == null || keys == null) return null;
        for (String key : keys) {
            if (key == null || key.isBlank()) continue;
            if (!node.has(key)) continue;
            JsonNode v = node.get(key);
            if (v == null || v.isNull()) continue;
            if (v.isBoolean()) return v.asBoolean();
            if (v.isNumber()) return v.asInt() != 0;
            if (v.isTextual()) {
                String s = v.asText("").trim().toLowerCase();
                if (s.isEmpty()) continue;
                if ("true".equals(s) || "1".equals(s) || "on".equals(s) || "yes".equals(s)) return true;
                if ("false".equals(s) || "0".equals(s) || "off".equals(s) || "no".equals(s)) return false;
            }
        }
        return null;
    }
}

