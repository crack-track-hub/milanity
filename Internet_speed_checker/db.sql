-- Create the database with your preferred name
CREATE DATABASE IF NOT EXISTS internet_speed_check;
USE internet_speed_check;

-- Standard table named 'records'
CREATE TABLE IF NOT EXISTS records (
    id INT AUTO_INCREMENT PRIMARY KEY,
    time_recorded DATETIME NOT NULL,
    download DOUBLE NOT NULL,
    upload DOUBLE NOT NULL,
    test_ping DOUBLE NOT NULL,
    connection_status VARCHAR(20) NOT NULL,
    system_ping DOUBLE NOT NULL
);