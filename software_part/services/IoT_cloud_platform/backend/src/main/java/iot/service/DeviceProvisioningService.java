package iot.service;

import iot.domain.DeviceCredential;

public interface DeviceProvisioningService {

    DeviceCredential issueCredential(String deviceId, String deviceName, String firmwareVersion, Long ownerId);

    boolean verifyCredential(String deviceId, String credentialKey, String credentialSecret);
}
