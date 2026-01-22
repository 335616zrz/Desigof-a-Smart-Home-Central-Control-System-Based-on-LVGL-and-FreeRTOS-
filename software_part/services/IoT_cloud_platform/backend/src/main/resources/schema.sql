CREATE TABLE IF NOT EXISTS devices (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    device_id VARCHAR(64) NOT NULL UNIQUE,
    name VARCHAR(128) NOT NULL,
    status VARCHAR(32),
    firmware_version VARCHAR(64),
    owner_id BIGINT NOT NULL,
    last_seen_at TIMESTAMP NULL DEFAULT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_owner (owner_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS users (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(64) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(128),
    role VARCHAR(32) NOT NULL,
    is_protected BOOLEAN NOT NULL DEFAULT FALSE,
    manager_id BIGINT NULL COMMENT 'Secondary admin managing this user',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (manager_id) REFERENCES users(id) ON DELETE SET NULL,
    INDEX idx_manager (manager_id),
    INDEX idx_role (role)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS device_credentials (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    device_id VARCHAR(64) NOT NULL,
    credential_key VARCHAR(128) NOT NULL,
    credential_secret_hash VARCHAR(255) NOT NULL,
    issued_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NULL DEFAULT NULL,
    active BOOLEAN NOT NULL DEFAULT TRUE,
    UNIQUE KEY uk_device_credential (device_id, credential_key),
    INDEX idx_device_active (device_id, active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS device_telemetry (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    device_id VARCHAR(64) NOT NULL,
    payload JSON NOT NULL,
    reported_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_telemetry_device_time (device_id, reported_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS alert_rules (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(128) NOT NULL,
    description TEXT,
    device_id VARCHAR(64),
    metric VARCHAR(64) NOT NULL,
    operator VARCHAR(16) NOT NULL,
    threshold DOUBLE NOT NULL,
    duration_seconds INT DEFAULT 60,
    level VARCHAR(16) NOT NULL DEFAULT 'warning',
    enabled BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    season ENUM('summer', 'winter', 'transition') NULL COMMENT 'Season (NULL means active all year)',
    is_system_template BOOLEAN NOT NULL DEFAULT FALSE COMMENT 'System template rule (read-only except enabled)',
    condition_type ENUM('above', 'below') NULL COMMENT 'Condition type (above/below)',
    INDEX idx_device_enabled (device_id, enabled),
    INDEX idx_season_enabled (season, enabled)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS alerts (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    device_id VARCHAR(64) NOT NULL,
    rule_id BIGINT,
    level VARCHAR(16) NOT NULL,
    message TEXT NOT NULL,
    status VARCHAR(16) NOT NULL DEFAULT 'open',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    acknowledged_at TIMESTAMP NULL,
    closed_at TIMESTAMP NULL,
    FOREIGN KEY (rule_id) REFERENCES alert_rules(id) ON DELETE SET NULL,
    INDEX idx_device_status (device_id, status),
    INDEX idx_created_at (created_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS user_deletion_requests (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    target_user_id BIGINT NOT NULL COMMENT 'User to be deleted',
    requester_id BIGINT NOT NULL COMMENT 'Secondary admin requesting deletion',
    status VARCHAR(16) NOT NULL DEFAULT 'pending' COMMENT 'pending, approved, rejected',
    reason TEXT COMMENT 'Reason for deletion request',
    reviewed_by BIGINT NULL COMMENT 'Primary admin who reviewed',
    reviewed_at TIMESTAMP NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (target_user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (requester_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (reviewed_by) REFERENCES users(id) ON DELETE SET NULL,
    INDEX idx_status (status),
    INDEX idx_requester (requester_id),
    INDEX idx_created_at (created_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Trigger to prevent deletion of protected users and primary admin
DELIMITER $$
CREATE TRIGGER IF NOT EXISTS prevent_protected_user_deletion
BEFORE DELETE ON users
FOR EACH ROW
BEGIN
    DECLARE primary_admin_count INT;

    -- Prevent deletion of protected users (primary admin)
    IF OLD.is_protected = TRUE THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Cannot delete protected user account';
    END IF;

    -- Prevent deletion of the last primary admin
    IF OLD.role = 'ROLE_PRIMARY_ADMIN' THEN
        SELECT COUNT(*) INTO primary_admin_count FROM users WHERE role = 'ROLE_PRIMARY_ADMIN';

        IF primary_admin_count <= 1 THEN
            SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'Cannot delete the last primary administrator account';
        END IF;
    END IF;
END$$
DELIMITER ;

-- ==========================================
-- System preset seasonal alert rule templates
-- Based on GB/T 18883-2022
-- ==========================================

-- Summer templates (6-8)
INSERT INTO alert_rules (name, description, device_id, metric, operator, threshold, duration_seconds, level, enabled, season, is_system_template, condition_type) VALUES
('夏季高温警告', '夏季室内温度超过30°C（国标舒适区上限）', NULL, 'temperature', '>', 30.0, 180, 'warning', TRUE, 'summer', TRUE, 'above'),
('夏季超高温危险', '夏季室内温度超过32°C（明显不适）', NULL, 'temperature', '>', 32.0, 180, 'critical', TRUE, 'summer', TRUE, 'above'),
('夏季低温提示', '夏季室内温度低于22°C（空调过度）', NULL, 'temperature', '<', 22.0, 180, 'info', TRUE, 'summer', TRUE, 'below'),
('夏季高湿度警告', '夏季室内湿度超过70%（开始闷热）', NULL, 'humidity', '>', 70.0, 180, 'warning', TRUE, 'summer', TRUE, 'above'),
('夏季超高湿度危险', '夏季室内湿度超过80%（明显不适，易发霉）', NULL, 'humidity', '>', 80.0, 180, 'critical', TRUE, 'summer', TRUE, 'above'),
('夏季低湿度提示', '夏季室内湿度低于35%（需要加湿）', NULL, 'humidity', '<', 35.0, 180, 'info', TRUE, 'summer', TRUE, 'below');

-- Winter templates (12-2)
INSERT INTO alert_rules (name, description, device_id, metric, operator, threshold, duration_seconds, level, enabled, season, is_system_template, condition_type) VALUES
('冬季高温警告', '冬季室内温度超过26°C（过度供暖）', NULL, 'temperature', '>', 26.0, 180, 'warning', TRUE, 'winter', TRUE, 'above'),
('冬季低温警告', '冬季室内温度低于18°C（国标采暖下限）', NULL, 'temperature', '<', 18.0, 180, 'warning', TRUE, 'winter', TRUE, 'below'),
('冬季超低温危险', '冬季室内温度低于16°C（健康风险）', NULL, 'temperature', '<', 16.0, 180, 'critical', TRUE, 'winter', TRUE, 'below'),
('冬季高湿度警告', '冬季室内湿度超过70%（易结露）', NULL, 'humidity', '>', 70.0, 180, 'warning', TRUE, 'winter', TRUE, 'above'),
('冬季低湿度警告', '冬季室内湿度低于35%（明显干燥）', NULL, 'humidity', '<', 35.0, 180, 'warning', TRUE, 'winter', TRUE, 'below'),
('冬季超低湿度危险', '冬季室内湿度低于25%（严重干燥，北方常见）', NULL, 'humidity', '<', 25.0, 180, 'critical', TRUE, 'winter', TRUE, 'below');

-- Transition templates (3-5, 9-11)
INSERT INTO alert_rules (name, description, device_id, metric, operator, threshold, duration_seconds, level, enabled, season, is_system_template, condition_type) VALUES
('过渡季高温警告', '过渡季室内温度超过26°C（偏热）', NULL, 'temperature', '>', 26.0, 180, 'warning', TRUE, 'transition', TRUE, 'above'),
('过渡季超高温危险', '过渡季室内温度超过28°C（明显过热）', NULL, 'temperature', '>', 28.0, 180, 'critical', TRUE, 'transition', TRUE, 'above'),
('过渡季低温警告', '过渡季室内温度低于18°C（略感偏凉）', NULL, 'temperature', '<', 18.0, 180, 'warning', TRUE, 'transition', TRUE, 'below'),
('过渡季超低温危险', '过渡季室内温度低于16°C（明显偏冷）', NULL, 'temperature', '<', 16.0, 180, 'critical', TRUE, 'transition', TRUE, 'below'),
('过渡季高湿度警告', '过渡季室内湿度超过70%（偏闷）', NULL, 'humidity', '>', 70.0, 180, 'warning', TRUE, 'transition', TRUE, 'above'),
('过渡季超高湿度危险', '过渡季室内湿度超过80%（明显不适）', NULL, 'humidity', '>', 80.0, 180, 'critical', TRUE, 'transition', TRUE, 'above'),
('过渡季低湿度警告', '过渡季室内湿度低于40%（略感干燥）', NULL, 'humidity', '<', 40.0, 180, 'warning', TRUE, 'transition', TRUE, 'below'),
('过渡季超低湿度危险', '过渡季室内湿度低于30%（明显干燥）', NULL, 'humidity', '<', 30.0, 180, 'critical', TRUE, 'transition', TRUE, 'below');
