-- Device shadow (desired/reported state) for remote controls like WS2812 night light.

CREATE TABLE IF NOT EXISTS device_shadow (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    device_id VARCHAR(64) NOT NULL UNIQUE,
    desired JSON NULL,
    desired_updated_at TIMESTAMP NULL DEFAULT NULL,
    reported JSON NULL,
    reported_updated_at TIMESTAMP NULL DEFAULT NULL,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_device_shadow_device (device_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

