package iot.domain;

import java.time.LocalDateTime;

public class DeviceCredential {

    private Long id;
    private String deviceId;
    private String credentialKey;
    private String credentialSecretHash;
    private LocalDateTime issuedAt;
    private LocalDateTime expiresAt;
    private boolean active;
    private transient String plaintextSecret;

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

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

    public String getCredentialSecretHash() {
        return credentialSecretHash;
    }

    public void setCredentialSecretHash(String credentialSecretHash) {
        this.credentialSecretHash = credentialSecretHash;
    }

    public LocalDateTime getIssuedAt() {
        return issuedAt;
    }

    public void setIssuedAt(LocalDateTime issuedAt) {
        this.issuedAt = issuedAt;
    }

    public LocalDateTime getExpiresAt() {
        return expiresAt;
    }

    public void setExpiresAt(LocalDateTime expiresAt) {
        this.expiresAt = expiresAt;
    }

    public boolean isActive() {
        return active;
    }

    public void setActive(boolean active) {
        this.active = active;
    }

    public String getPlaintextSecret() {
        return plaintextSecret;
    }

    public void setPlaintextSecret(String plaintextSecret) {
        this.plaintextSecret = plaintextSecret;
    }
}
