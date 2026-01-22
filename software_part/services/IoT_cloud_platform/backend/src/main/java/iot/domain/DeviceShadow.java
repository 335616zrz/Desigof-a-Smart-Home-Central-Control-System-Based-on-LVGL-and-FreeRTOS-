package iot.domain;

import java.time.LocalDateTime;

public class DeviceShadow {

    private Long id;
    private String deviceId;

    // JSON strings stored in MySQL JSON columns.
    private String desired;
    private LocalDateTime desiredUpdatedAt;

    private String reported;
    private LocalDateTime reportedUpdatedAt;

    private LocalDateTime updatedAt;

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

    public String getDesired() {
        return desired;
    }

    public void setDesired(String desired) {
        this.desired = desired;
    }

    public LocalDateTime getDesiredUpdatedAt() {
        return desiredUpdatedAt;
    }

    public void setDesiredUpdatedAt(LocalDateTime desiredUpdatedAt) {
        this.desiredUpdatedAt = desiredUpdatedAt;
    }

    public String getReported() {
        return reported;
    }

    public void setReported(String reported) {
        this.reported = reported;
    }

    public LocalDateTime getReportedUpdatedAt() {
        return reportedUpdatedAt;
    }

    public void setReportedUpdatedAt(LocalDateTime reportedUpdatedAt) {
        this.reportedUpdatedAt = reportedUpdatedAt;
    }

    public LocalDateTime getUpdatedAt() {
        return updatedAt;
    }

    public void setUpdatedAt(LocalDateTime updatedAt) {
        this.updatedAt = updatedAt;
    }
}

