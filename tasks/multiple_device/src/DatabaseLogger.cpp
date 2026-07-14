#include "DatabaseLogger.h"

#include <iostream>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>

DatabaseLogger::DatabaseLogger(DatabaseConfig config) : config_(std::move(config)) {}

DatabaseLogger::~DatabaseLogger() = default;

bool DatabaseLogger::ensureConnected() {
    if (connection_ && !connection_->isClosed()) return true;

    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        connection_.reset(driver->connect(config_.host, config_.user, config_.password));
        connection_->setSchema(config_.schema);
        std::cout << "[DB] connected to " << config_.host << std::endl;
        return true;
    } catch (sql::SQLException& e) {
        std::cerr << "[DB ERROR] connect failed (" << e.getErrorCode() << "): " << e.what() << std::endl;
        connection_.reset();
        return false;
    }
}

void DatabaseLogger::logReadings(const std::string& deviceName,
                                  const std::vector<ReadingResult>& readings) {
    if (!ensureConnected()) return;

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(connection_->prepareStatement(
            "INSERT INTO device_readings (device_name, reading_name, value, is_valid, reading_time) "
            "VALUES (?, ?, ?, ?, NOW())"));

        for (const auto& r : readings) {
            pstmt->setString(1, deviceName);
            pstmt->setString(2, r.name);
            if (r.valid) {
                pstmt->setDouble(3, r.value);
            } else {
                pstmt->setNull(3, sql::DataType::DOUBLE);
            }
            pstmt->setBoolean(4, r.valid);
            pstmt->executeUpdate();
        }
        std::cout << "[DB] logged " << readings.size() << " reading(s) for " << deviceName << std::endl;

    } catch (sql::SQLException& e) {
        std::cerr << "[DB ERROR] insert failed (" << e.getErrorCode() << "): " << e.what() << std::endl;
        // Drop the handle so the next cycle re-establishes the connection
        // instead of repeatedly hammering a broken one.
        connection_.reset();
    }
}
