package iot.dto;

import jakarta.validation.constraints.NotBlank;

public class DeviceClaimRequest {

    @NotBlank
    private String deviceId;

    @NotBlank
    private String credentialKey;

    @NotBlank
    private String credentialSecret;

    public String getDeviceId() {
        return deviceId;
    }

    public void setDeviceId(String deviceId) {
        this.deviceId = deviceId;
    }

    public String getCredentialKey() {
        return credentialKey;
    }

    public void setCredentialKey(String credentialKey) {
        this.credentialKey = credentialKey;
    }

    public String getCredentialSecret() {
        return credentialSecret;
    }

    public void setCredentialSecret(String credentialSecret) {
        this.credentialSecret = credentialSecret;
    }
}
