package iot.service;

import iot.domain.Device;
import java.util.List;
import java.util.Optional;

public interface DeviceService {

    List<Device> listDevices();

    List<Device> listDevicesByOwner(Long ownerId);

    Optional<Device> findByDeviceId(String deviceId);

    Device register(Device device);

    void updateStatus(String deviceId, String status);

    Device update(Device device);

    void delete(String deviceId);
}
