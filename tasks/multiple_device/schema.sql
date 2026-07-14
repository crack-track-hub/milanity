CREATE DATABASE IF NOT EXISTS industrial_iot;
USE industrial_iot;

-- Generic table: one row per reading per poll, rather than one fixed-width
-- row per device. This is what lets config.json add/remove readings or
-- whole devices without ever touching the schema or the C++ code.
CREATE TABLE IF NOT EXISTS device_readings (
    id            BIGINT AUTO_INCREMENT PRIMARY KEY,
    device_name   VARCHAR(64)  NOT NULL,
    reading_name  VARCHAR(64)  NOT NULL,
    value         DOUBLE       NULL,      -- NULL when the read failed (is_valid = 0)
    is_valid      TINYINT(1)   NOT NULL,
    reading_time  DATETIME     NOT NULL,
    INDEX idx_device_time (device_name, reading_time),
    INDEX idx_reading_name (reading_name)
);
