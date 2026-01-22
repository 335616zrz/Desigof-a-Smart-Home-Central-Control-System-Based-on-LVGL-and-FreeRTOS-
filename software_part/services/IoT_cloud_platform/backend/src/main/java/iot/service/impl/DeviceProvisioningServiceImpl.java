package iot.service.impl;

import iot.domain.Device;
import iot.domain.DeviceCredential;
import iot.mapper.DeviceCredentialMapper;
import iot.mapper.DeviceMapper;
import iot.service.DeviceProvisioningService;
import java.security.SecureRandom;
import java.time.LocalDateTime;
import java.util.Base64;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@Transactional(readOnly = true)
public class DeviceProvisioningServiceImpl implements DeviceProvisioningService {

    private static final SecureRandom RANDOM = new SecureRandom();

    private final DeviceCredentialMapper credentialMapper;
    private final DeviceMapper deviceMapper;
    private final PasswordEncoder passwordEncoder;

    public DeviceProvisioningServiceImpl(DeviceCredentialMapper credentialMapper,
                                         DeviceMapper deviceMapper,
                                         PasswordEncoder passwordEncoder) {
        this.credentialMapper = credentialMapper;
        this.deviceMapper = deviceMapper;
        this.passwordEncoder = passwordEncoder;
    }

    @Override
    @Transactional
    public DeviceCredential issueCredential(String deviceId, String deviceName, String firmwareVersion, Long ownerId) {
        Device device = deviceMapper.findByDeviceId(deviceId);
        if (device == null) {
            device = new Device();
            device.setDeviceId(deviceId);
            device.setName(deviceName);
            device.setFirmwareVersion(firmwareVersion);
            device.setStatus("PROVISIONING");
            device.setLastSeenAt(LocalDateTime.now());
            device.setOwnerId(ownerId);
            deviceMapper.insert(device);
        }
        credentialMapper.deactivateAll(deviceId);
        String credentialKey = generateToken();
        String credentialSecret = generateToken();
        DeviceCredential credential = new DeviceCredential();
        credential.setDeviceId(deviceId);
        credential.setCredentialKey(credentialKey);
        credential.setCredentialSecretHash(passwordEncoder.encode(credentialSecret));
        credential.setIssuedAt(LocalDateTime.now());
        credential.setExpiresAt(LocalDateTime.now().plusHours(24));
        credential.setActive(true);
        credentialMapper.insert(credential);
        credential.setPlaintextSecret(credentialSecret);
        return credential;
    }

    @Override
    @Transactional
    public boolean verifyCredential(String deviceId, String credentialKey, String credentialSecret) {
        DeviceCredential credential = credentialMapper.findActiveByDeviceAndKey(deviceId, credentialKey);
        if (credential == null) {
            return false;
        }
        boolean match = passwordEncoder.matches(credentialSecret, credential.getCredentialSecretHash());
        if (match) {
            credentialMapper.markUsed(deviceId, credentialKey, false, LocalDateTime.now());
            deviceMapper.updateStatus(deviceId, "AUTHENTICATED", LocalDateTime.now());
        }
        return match;
    }

    private String generateToken() {
        byte[] bytes = new byte[24];
        RANDOM.nextBytes(bytes);
        return Base64.getUrlEncoder().withoutPadding().encodeToString(bytes);
    }
}
