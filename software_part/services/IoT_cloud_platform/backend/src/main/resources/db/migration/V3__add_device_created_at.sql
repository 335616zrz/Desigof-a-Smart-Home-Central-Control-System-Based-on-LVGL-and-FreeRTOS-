-- ============================================================================
-- V3: Add devices.created_at (idempotent)
-- ============================================================================

SET @devices_created_at_exists := (
    SELECT COUNT(*)
    FROM information_schema.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE()
      AND TABLE_NAME = 'devices'
      AND COLUMN_NAME = 'created_at'
);

SET @devices_created_at_sql := IF(
    @devices_created_at_exists = 0,
    'ALTER TABLE devices ADD COLUMN created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP',
    'SELECT 1'
);

PREPARE stmt FROM @devices_created_at_sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

