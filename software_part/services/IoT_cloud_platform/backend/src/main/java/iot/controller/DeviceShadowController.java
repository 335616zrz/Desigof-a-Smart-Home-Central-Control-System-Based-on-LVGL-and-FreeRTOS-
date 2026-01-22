package iot.controller;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import iot.domain.Device;
import iot.domain.DeviceShadow;
import iot.domain.User;
import iot.dto.ApiResponse;
import iot.service.DeviceService;
import iot.service.DeviceShadowService;
import iot.service.UserService;
import java.time.LocalDateTime;
import java.util.Optional;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.Authentication;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import iot.security.RoleUtils;

@RestController
@RequestMapping("/api/devices/{deviceId}/shadow")
public class DeviceShadowController {

    private final DeviceService deviceService;
    private final UserService userService;
    private final DeviceShadowService deviceShadowService;
    private final ObjectMapper objectMapper;

    public DeviceShadowController(DeviceService deviceService,
                                 UserService userService,
                                 DeviceShadowService deviceShadowService,
                                 ObjectMapper objectMapper) {
        this.deviceService = deviceService;
        this.userService = userService;
        this.deviceShadowService = deviceShadowService;
        this.objectMapper = objectMapper;
    }

    private record ShadowResponse(
            String deviceId,
            Boolean nightLightOn,
            String nightLightSource,
            boolean nightLightPending,
            Boolean alarmActive,
            String alarmSource,
            boolean alarmPending,
            Boolean desiredNightLightOn,
            Boolean reportedNightLightOn,
            Boolean desiredAlarmActive,
            Boolean reportedAlarmActive,
            LocalDateTime desiredUpdatedAt,
            LocalDateTime reportedUpdatedAt
    ) {}

    @GetMapping
    public ResponseEntity<ApiResponse<ShadowResponse>> getShadow(@PathVariable String deviceId,
                                                                 Authentication auth) {
        String username = auth.getName();
        User currentUser = userService.findByUsername(username)
                .orElseThrow(() -> new RuntimeException("用户不存在"));

        Optional<Device> existing = deviceService.findByDeviceId(deviceId);
        if (existing.isEmpty()) {
            return ResponseEntity.status(HttpStatus.NOT_FOUND)
                    .body(ApiResponse.error(404, "设备不存在"));
        }
        Device device = existing.get();

        boolean isAdmin = isAdmin(currentUser);
        if (!isAdmin && (device.getOwnerId() == null || !device.getOwnerId().equals(currentUser.getId()))) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN)
                    .body(ApiResponse.error(403, "无权限查看该设备"));
        }

        DeviceShadow shadow = deviceShadowService.findByDeviceId(deviceId).orElse(null);
        Boolean desiredNight = extractBooleanFromJson(shadow == null ? null : shadow.getDesired(), "nightLightOn", "nightLight");
        Boolean desiredAlarm = extractBooleanFromJson(shadow == null ? null : shadow.getDesired(), "alarmActive", "alarm");
        Boolean reportedNight = extractBooleanFromJson(shadow == null ? null : shadow.getReported(), "nightLightOn", "nightLight");
        Boolean reportedAlarm = extractBooleanFromJson(shadow == null ? null : shadow.getReported(), "alarmActive", "alarm");

        // Effective state: prefer reported if present, else desired, else false.
        Boolean nightOn = reportedNight != null ? reportedNight : (desiredNight != null ? desiredNight : Boolean.FALSE);
        String nightSrc = reportedNight != null ? "reported" : (desiredNight != null ? "desired" : "default");
        // Pending means: cloud has a desired value, but device has not yet reported the same value.
        boolean nightPending = desiredNight != null && (reportedNight == null || !desiredNight.equals(reportedNight));

        Boolean alarmOn = reportedAlarm != null ? reportedAlarm : (desiredAlarm != null ? desiredAlarm : Boolean.FALSE);
        String alarmSrc = reportedAlarm != null ? "reported" : (desiredAlarm != null ? "desired" : "default");
        boolean alarmPending = desiredAlarm != null && (reportedAlarm == null || !desiredAlarm.equals(reportedAlarm));

        ShadowResponse resp = new ShadowResponse(
                deviceId,
                nightOn,
                nightSrc,
                nightPending,
                alarmOn,
                alarmSrc,
                alarmPending,
                desiredNight,
                reportedNight,
                desiredAlarm,
                reportedAlarm,
                shadow == null ? null : shadow.getDesiredUpdatedAt(),
                shadow == null ? null : shadow.getReportedUpdatedAt()
        );

        return ResponseEntity.ok(ApiResponse.ok(resp));
    }

    private Boolean extractBooleanFromJson(String json, String... keys) {
        if (json == null || json.isBlank()) return null;
        try {
            JsonNode root = objectMapper.readTree(json);
            for (String k : keys) {
                if (k == null || k.isBlank()) continue;
                if (!root.has(k)) continue;
                JsonNode v = root.get(k);
                if (v == null || v.isNull()) continue;
                if (v.isBoolean()) return v.asBoolean();
                if (v.isNumber()) return v.asInt() != 0;
                if (v.isTextual()) {
                    String s = v.asText("").trim().toLowerCase();
                    if ("true".equals(s) || "1".equals(s) || "on".equals(s) || "yes".equals(s)) return true;
                    if ("false".equals(s) || "0".equals(s) || "off".equals(s) || "no".equals(s)) return false;
                }
            }
        } catch (Exception ignored) {
            // ignore parse errors
        }
        return null;
    }

    private boolean isAdmin(User user) {
        return RoleUtils.isAdminRole(user.getRole());
    }
}
