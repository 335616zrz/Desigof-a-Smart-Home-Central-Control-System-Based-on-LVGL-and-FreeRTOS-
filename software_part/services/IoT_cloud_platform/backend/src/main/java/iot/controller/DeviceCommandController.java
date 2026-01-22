package iot.controller;

import iot.domain.Device;
import iot.domain.User;
import iot.dto.ApiResponse;
import iot.service.DeviceService;
import iot.service.DeviceShadowService;
import iot.service.UserService;
import iot.service.impl.MqttCommandPublisher;
import iot.security.RoleUtils;
import java.util.Map;
import java.util.Optional;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.Authentication;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/devices/{deviceId}/commands")
public class DeviceCommandController {

    private final DeviceService deviceService;
    private final UserService userService;
    private final DeviceShadowService deviceShadowService;
    private final MqttCommandPublisher mqttCommandPublisher;

    public DeviceCommandController(DeviceService deviceService,
                                   UserService userService,
                                   DeviceShadowService deviceShadowService,
                                   MqttCommandPublisher mqttCommandPublisher) {
        this.deviceService = deviceService;
        this.userService = userService;
        this.deviceShadowService = deviceShadowService;
        this.mqttCommandPublisher = mqttCommandPublisher;
    }

    private record NightLightCommandRequest(Boolean on) {}

    @PostMapping("/night-light")
    public ResponseEntity<ApiResponse<Map<String, Object>>> setNightLight(@PathVariable String deviceId,
                                                                          @RequestBody(required = false) NightLightCommandRequest req,
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
                    .body(ApiResponse.error(403, "无权限控制该设备"));
        }

        boolean on = req != null && Boolean.TRUE.equals(req.on());
        String topic = "devices/" + deviceId + "/command/night_light";
        String payload = "{\"cmd\":\"night_light\",\"on\":" + (on ? "true" : "false") + ",\"deviceId\":\"" + deviceId + "\"}";
        try {
            mqttCommandPublisher.publish(topic, payload, true);
            deviceShadowService.setDesiredNightLightOn(deviceId, on);
        } catch (Exception ex) {
            return ResponseEntity.status(HttpStatus.SERVICE_UNAVAILABLE)
                    .body(ApiResponse.error(503, "MQTT 下发失败，请稍后重试"));
        }

        return ResponseEntity.ok(ApiResponse.ok(Map.of("deviceId", deviceId, "on", on)));
    }

    private boolean isAdmin(User user) {
        return RoleUtils.isAdminRole(user.getRole());
    }
}
