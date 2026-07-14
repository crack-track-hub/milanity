CREATE DATABASE IF NOT EXISTS industrial_iot;
USE industrial_iot;

CREATE TABLE IF NOT EXISTS device_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    supply_temperature DECIMAL(5,2),
    heating_status TINYINT,
    outlet_temperature DECIMAL(5,2),
    inlet_temperature DECIMAL(5,2),
    firing_rate INT
);