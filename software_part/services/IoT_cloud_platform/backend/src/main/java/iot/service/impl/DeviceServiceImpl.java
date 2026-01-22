package iot.service.impl;

import iot.domain.Device;
import iot.mapper.DeviceMapper;
import iot.mapper.UserMapper;
import iot.service.DeviceService;
import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@Transactional(readOnly = true)
public class DeviceServiceImpl implements DeviceService {

    private final DeviceMapper deviceMapper;
    private final UserMapper userMapper;

    public DeviceServiceImpl(DeviceMapper deviceMapper, UserMapper userMapper) {
        this.deviceMapper = deviceMapper;
        this.userMapper = userMapper;
    }

    @Override
    public List<Device> listDevices() {
        return deviceMapper.findAll();
    }

    @Override
    public List<Device> listDevicesByOwner(Long ownerId) {
        return deviceMapper.findByOwnerId(ownerId);
    }

    @Override
    public Optional<Device> findByDeviceId(String deviceId) {
        return Optional.ofNullable(deviceMapper.findByDeviceId(deviceId));
    }

    @Override
    @Transactional
    public Device register(Device device) {
        if (device.getStatus() == null) {
            device.setStatus("REGISTERED");
        }
        device.setOwnerId(resolveOwnerId(device.getOwnerId()));
        device.setLastSeenAt(LocalDateTime.now());
        deviceMapper.insert(device);
        return device;
    }

    @Override
    @Transactional
    public Device update(Device device) {
        device.setOwnerId(resolveOwnerId(device.getOwnerId()));
        device.setLastSeenAt(LocalDateTime.now());
        deviceMapper.update(device);
        return deviceMapper.findByDeviceId(device.getDeviceId());
    }

    @Override
    @Transactional
    public void delete(String deviceId) {
        deviceMapper.deleteByDeviceId(deviceId);
    }

    @Override
    @Transactional
    public void updateStatus(String deviceId, String status) {
        Device existing = deviceMapper.findByDeviceId(deviceId);
        if (existing == null) {
            Device device = new Device();
            device.setDeviceId(deviceId);
            device.setName(deviceId);
            device.setStatus(status);
            device.setOwnerId(resolveOwnerId(null));
            device.setLastSeenAt(LocalDateTime.now());
            deviceMapper.insert(device);
        } else {
            deviceMapper.updateStatus(deviceId, status, LocalDateTime.now());
        }
    }

    private Long resolveOwnerId(Long requested) {
        if (requested != null) {
            return requested;
        }
        // 优先使用主管理员，其次管理员
        var primary = userMapper.findFirstByRole("ROLE_PRIMARY_ADMIN");
        if (primary == null) {
            // backward compatible: old DB may store without "ROLE_" prefix
            primary = userMapper.findFirstByRole("PRIMARY_ADMIN");
        }
        if (primary != null) return primary.getId();
        var admin = userMapper.findFirstByRole("ROLE_ADMIN");
        if (admin == null) {
            admin = userMapper.findFirstByRole("ADMIN");
        }
        if (admin != null) return admin.getId();
        throw new IllegalStateException("No admin user found to assign as device owner");
    }
}
