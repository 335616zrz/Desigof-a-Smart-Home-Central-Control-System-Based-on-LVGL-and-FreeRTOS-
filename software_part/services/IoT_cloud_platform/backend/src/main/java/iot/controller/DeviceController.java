package iot.controller;

import iot.domain.Device;
import iot.domain.User;
import iot.dto.ApiResponse;
import iot.security.RoleUtils;
import iot.service.DeviceService;
import iot.service.UserService;
import jakarta.validation.Valid;
import java.util.List;
import java.util.Optional;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.security.core.Authentication;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/devices")
public class DeviceController {

    private final DeviceService deviceService;
    private final UserService userService;

    public DeviceController(DeviceService deviceService, UserService userService) {
        this.deviceService = deviceService;
        this.userService = userService;
    }

    @GetMapping
    public ResponseEntity<ApiResponse<Paginated<Device>>> listDevices(Authentication auth) {
        String username = auth.getName();
        User currentUser = userService.findByUsername(username)
                .orElseThrow(() -> new RuntimeException("用户不存在"));

        // 管理员可以查看所有设备，普通用户只能查看自己的设备
        List<Device> devices;
        if (RoleUtils.isAdminRole(currentUser.getRole())) {
            devices = deviceService.listDevices();
        } else {
            devices = deviceService.listDevicesByOwner(currentUser.getId());
        }

        return ResponseEntity.ok(ApiResponse.ok(new Paginated<>(devices, devices.size())));
    }

    private record Paginated<T>(List<T> items, long total) {}

    @GetMapping("/{deviceId}")
    public ResponseEntity<ApiResponse<Device>> getDevice(@PathVariable String deviceId) {
        Optional<Device> device = deviceService.findByDeviceId(deviceId);
        return device.map(value -> ResponseEntity.ok(ApiResponse.ok(value)))
                .orElseGet(() -> ResponseEntity.status(HttpStatus.NOT_FOUND).body(ApiResponse.error(404, "设备不存在")));
    }

    @PostMapping
    public ResponseEntity<ApiResponse<Device>> register(@Valid @RequestBody Device device, Authentication auth) {
        String username = auth.getName();
        User currentUser = userService.findByUsername(username)
                .orElseThrow(() -> new RuntimeException("用户不存在"));

        // 自动设置设备的所有者为当前用户
        device.setOwnerId(currentUser.getId());
        Device saved = deviceService.register(device);
        return ResponseEntity.status(HttpStatus.CREATED).body(ApiResponse.ok(saved));
    }

    @PutMapping("/{deviceId}")
    public ResponseEntity<ApiResponse<Device>> update(@PathVariable String deviceId,
                                                      @Valid @RequestBody Device device,
                                                      Authentication auth) {
        String username = auth.getName();
        User currentUser = userService.findByUsername(username)
                .orElseThrow(() -> new RuntimeException("用户不存在"));
        Optional<Device> existing = deviceService.findByDeviceId(deviceId);
        if (existing.isEmpty()) {
            return ResponseEntity.status(HttpStatus.NOT_FOUND).body(ApiResponse.error(404, "设备不存在"));
        }
        Device target = existing.get();
        boolean isAdmin = isAdmin(currentUser);
        if (!isAdmin && !target.getOwnerId().equals(currentUser.getId())) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN).body(ApiResponse.error(403, "无权限修改该设备"));
        }
        device.setDeviceId(deviceId);
        device.setOwnerId(target.getOwnerId());
        if (device.getName() == null) device.setName(target.getName());
        if (device.getStatus() == null) device.setStatus(target.getStatus());
        if (device.getFirmwareVersion() == null) device.setFirmwareVersion(target.getFirmwareVersion());
        Device updated = deviceService.update(device);
        return ResponseEntity.ok(ApiResponse.ok(updated));
    }

    @DeleteMapping("/{deviceId}")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_ADMIN')")
    public ResponseEntity<Void> delete(@PathVariable String deviceId) {
        deviceService.delete(deviceId);
        return ResponseEntity.noContent().build();
    }

    private boolean isAdmin(User user) {
        return RoleUtils.isAdminRole(user.getRole());
    }
}
