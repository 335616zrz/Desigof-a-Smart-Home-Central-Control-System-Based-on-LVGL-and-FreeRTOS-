-- ============================================================================
-- V2: Seasonal alert rule support
-- - Add season/system-template fields to alert_rules
-- - Insert system preset seasonal templates (20 rules)
-- ============================================================================

ALTER TABLE alert_rules
    ADD COLUMN season ENUM('summer', 'winter', 'transition') NULL
        COMMENT 'Season (NULL means active all year)' AFTER updated_at,
    ADD COLUMN is_system_template BOOLEAN NOT NULL DEFAULT FALSE
        COMMENT 'System template rule (read-only except enabled)' AFTER season,
    ADD COLUMN condition_type ENUM('above', 'below') NULL
        COMMENT 'Condition type (above/below)' AFTER is_system_template;

ALTER TABLE alert_rules
    ADD INDEX idx_season_enabled (season, enabled);

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

