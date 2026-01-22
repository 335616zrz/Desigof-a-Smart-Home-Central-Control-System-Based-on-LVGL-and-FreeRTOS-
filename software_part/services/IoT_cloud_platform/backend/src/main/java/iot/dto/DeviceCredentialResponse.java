package iot.dto;

public class DeviceCredentialResponse {

    private final String deviceId;
    private final String credentialKey;
    private final String credentialSecret;
    private final String expiresAt;

    public DeviceCredentialResponse(String deviceId, String credentialKey, String credentialSecret, String expiresAt) {
        this.deviceId = deviceId;
        this.credentialKey = credentialKey;
        this.credentialSecret = credentialSecret;
        this.expiresAt = expiresAt;
    }

    public String getDeviceId() {
        return deviceId;
    }

    public String getCredentialKey() {
        return credentialKey;
    }

    public String getCredentialSecret() {
        return credentialSecret;
    }

    public String getExpiresAt() {
        return expiresAt;
    }
}
