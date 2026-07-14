#pragma once
// DatabaseLogger.h
// Wraps the MySQL Connector/C++ handle. Like the Modbus side, the DB
// connection is kept persistent and only re-established if a query fails,
// rather than opening/closing a connection every log cycle.

#include <memory>
#include <string>
#include <vector>

#include "Config.h"
#include "Device.h" // ReadingResult

namespace sql {
    class Connection;
}

class DatabaseLogger {
public:
    explicit DatabaseLogger(DatabaseConfig config);
    ~DatabaseLogger();

    DatabaseLogger(const DatabaseLogger&) = delete;
    DatabaseLogger& operator=(const DatabaseLogger&) = delete;

    bool ensureConnected();

    // Inserts one row per reading into device_readings.
    // See schema.sql for the CREATE TABLE statement.
    void logReadings(const std::string& deviceName, const std::vector<ReadingResult>& readings);

private:
    DatabaseConfig config_;
    std::unique_ptr<sql::Connection> connection_;
};
