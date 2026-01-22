-- ============================================================================
-- IoT Cloud Platform - Complete Database Schema
-- ============================================================================
-- This migration creates the complete database schema with all features:
-- - User management with hierarchical roles (PRIMARY_ADMIN, SECONDARY_ADMIN, OPERATOR)
-- - Device management with ownership tracking
-- - Telemetry data collection
-- - Alert rules and alerts
-- - User deletion request workflow
-- - Protection mechanisms for admin accounts
-- ============================================================================

-- ============================================================================
-- STEP 1: Create users table with all columns
-- ============================================================================
CREATE TABLE IF NOT EXISTS users (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(64) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(128),
    role VARCHAR(32) NOT NULL,
    is_protected BOOLEAN NOT NULL DEFAULT FALSE COMMENT 'Protected users cannot be deleted',
    manager_id BIGINT NULL COMMENT 'Secondary admin managing this user',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_role (role),
    INDEX idx_manager (manager_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- STEP 2: Add foreign key for manager_id (self-reference)
-- ============================================================================
ALTER TABLE users
ADD CONSTRAINT fk_user_manager
FOREIGN KEY (manager_id) REFERENCES users(id) ON DELETE SET NULL;

-- ============================================================================
-- STEP 3: Create devices table
-- ============================================================================
CREATE TABLE IF NOT EXISTS devices (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    device_id VARCHAR(64) NOT NULL UNIQUE,
    name VARCHAR(128) NOT NULL,
    status VARCHAR(32),
    firmware_version VARCHAR(64),
    owner_id BIGINT NOT NULL,
    last_seen_at TIMESTAMP NULL DEFAULT NULL,
    FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_owner (owner_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- STEP 4: Create device_credentials table
-- ============================================================================
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

-- ============================================================================
-- STEP 5: Create device_telemetry table
-- ============================================================================
CREATE TABLE IF NOT EXISTS device_telemetry (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    device_id VARCHAR(64) NOT NULL,
    payload JSON NOT NULL,
    reported_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_telemetry_device_time (device_id, reported_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- STEP 6: Create alert_rules table
-- ============================================================================
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
    INDEX idx_device_enabled (device_id, enabled)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- STEP 7: Create alerts table
-- ============================================================================
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

-- ============================================================================
-- STEP 8: Create user_deletion_requests table
-- ============================================================================
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

-- ============================================================================
-- STEP 9: Create trigger to protect admin accounts
-- ============================================================================
-- Note: For initial deployment, we create the trigger without dropping first
-- This avoids Flyway warnings on fresh databases

DELIMITER $$

CREATE TRIGGER prevent_protected_user_deletion
BEFORE DELETE ON users
FOR EACH ROW
BEGIN
    DECLARE primary_admin_count INT;

    -- Prevent deletion of protected users
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

-- ============================================================================
-- STEP 10: Seed initial data
-- ============================================================================
-- NOTE:
-- For security, we DO NOT ship a default PRIMARY_ADMIN user or demo data.
-- Bootstrap an admin user at runtime via env:
--   APP_ADMIN_USERNAME / APP_ADMIN_PASSWORD (/ APP_ADMIN_EMAIL)
-- (see backend: iot.config.AdminInitializer), or insert demo data manually in a dev-only environment.

-- ============================================================================
-- Migration completed successfully!
-- ============================================================================
