package iot.controller;

import iot.domain.DeviceCredential;
import iot.domain.User;
import iot.dto.DeviceClaimRequest;
import iot.dto.DeviceCredentialResponse;
import iot.dto.DeviceProvisionRequest;
import iot.service.DeviceProvisioningService;
import iot.service.UserService;
import jakarta.validation.Valid;
import java.time.format.DateTimeFormatter;
import java.util.Map;
import java.util.UUID;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.security.core.Authentication;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/devices")
public class DeviceProvisioningController {

    private static final DateTimeFormatter FORMATTER = DateTimeFormatter.ISO_OFFSET_DATE_TIME;

    private final DeviceProvisioningService provisioningService;
    private final UserService userService;

    public DeviceProvisioningController(DeviceProvisioningService provisioningService, UserService userService) {
        this.provisioningService = provisioningService;
        this.userService = userService;
    }

    @PostMapping("/{deviceId}/provision")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_ADMIN','ROLE_OPERATOR')")
    public ResponseEntity<DeviceCredentialResponse> provision(@PathVariable String deviceId,
                                                              @Valid @RequestBody DeviceProvisionRequest request,
                                                              Authentication auth) {
        String resolvedDeviceId = deviceId;
        if ("auto".equalsIgnoreCase(deviceId)) {
            resolvedDeviceId = UUID.randomUUID().toString();
        }
        User currentUser = userService.findByUsername(auth.getName())
                .orElseThrow(() -> new RuntimeException("用户不存在"));
        DeviceCredential credential = provisioningService.issueCredential(resolvedDeviceId, request.getName(), request.getFirmwareVersion(), currentUser.getId());
        return ResponseEntity.status(HttpStatus.CREATED)
                .body(new DeviceCredentialResponse(
                        resolvedDeviceId,
                        credential.getCredentialKey(),
                        credential.getPlaintextSecret(),
                        credential.getExpiresAt() == null ? null : credential.getExpiresAt().atOffset(java.time.ZoneOffset.UTC).format(FORMATTER)));
    }

    @PostMapping("/provision/claim")
    public ResponseEntity<?> claim(@Valid @RequestBody DeviceClaimRequest request) {
        boolean verified = provisioningService.verifyCredential(request.getDeviceId(), request.getCredentialKey(), request.getCredentialSecret());
        if (!verified) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED).body(Map.of("message", "凭证验证失败"));
        }
        return ResponseEntity.ok(Map.of("message", "设备认证成功"));
    }
}
