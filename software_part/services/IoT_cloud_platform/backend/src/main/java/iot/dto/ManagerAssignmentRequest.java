package iot.dto;

import jakarta.validation.constraints.NotNull;

/**
 * Request DTO for assigning a manager to a user
 */
public class ManagerAssignmentRequest {

    @NotNull(message = "Manager ID is required")
    private Long managerId;

    public Long getManagerId() {
        return managerId;
    }

    public void setManagerId(Long managerId) {
        this.managerId = managerId;
    }
}
