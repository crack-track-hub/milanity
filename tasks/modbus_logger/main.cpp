// main.cpp
// Single-file OOP Modbus TCP -> MySQL logger.
// Classes: ModbusTcpClient, Device, DatabaseLogger, Config loader.
//
// Design notes:
// - Each Device owns ONE persistent ModbusTcpClient socket. It is opened
//   once at startup and reused every poll cycle. It is only reopened if a
//   transaction actually fails (cable pulled, device rebooted, etc).
// - DatabaseLogger keeps one persistent MySQL connection the same way.
// - Devices + their register maps are loaded from config.json, so adding
//   a 3rd device or changing a register needs no recompiling of logic.

#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include <nlohmann/json.hpp>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>

// ===================================================================
// Config structs (loaded from config.json)
// ===================================================================

struct ReadingConfig {
    std::string name;          // label used when logging, e.g. "supply_temperature"
    uint8_t functionCode = 0;  // Modbus function code (0x02 discrete, 0x03 holding, 0x04 input)
    uint16_t reg = 0;          // starting register/coil address
    std::string type;          // "temperature" | "discrete" | "raw" -> controls conversion
};

struct DeviceConfig {
    std::string name;
    std::string ip;
    int port = 502;
    uint8_t slaveId = 1;
    std::vector<ReadingConfig> readings;
};

struct DatabaseConfig {
    std::string host;      // e.g. "tcp://127.0.0.1:3306"
    std::string user;
    std::string password;
    std::string schema;
};

struct AppConfig {
    int pollIntervalSeconds = 15;
    DatabaseConfig database;
    std::vector<DeviceConfig> devices;
};

// --- nlohmann::json parsing hooks ---

void from_json(const nlohmann::json& j, ReadingConfig& r) {
    j.at("name").get_to(r.name);
    j.at("register").get_to(r.reg);
    j.at("type").get_to(r.type);

    if (j.at("function_code").is_string()) {
        r.functionCode = static_cast<uint8_t>(std::stoi(j.at("function_code").get<std::string>(), nullptr, 0));
    } else {
        r.functionCode = j.at("function_code").get<uint8_t>();
    }
}

void from_json(const nlohmann::json& j, DeviceConfig& d) {
    j.at("name").get_to(d.name);
    j.at("ip").get_to(d.ip);
    j.at("port").get_to(d.port);
    d.slaveId = j.value("slave_id", 1);
    j.at("readings").get_to(d.readings);
}

void from_json(const nlohmann::json& j, DatabaseConfig& db) {
    j.at("host").get_to(db.host);
    j.at("user").get_to(db.user);
    j.at("password").get_to(db.password);
    j.at("schema").get_to(db.schema);
}

void from_json(const nlohmann::json& j, AppConfig& cfg) {
    cfg.pollIntervalSeconds = j.value("poll_interval_seconds", 15);
    j.at("database").get_to(cfg.database);
    j.at("devices").get_to(cfg.devices);
}

AppConfig loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + path);
    }
    nlohmann::json j;
    file >> j;
    return j.get<AppConfig>();
}

// ===================================================================
// ModbusTcpClient - one persistent socket to one Modbus TCP slave
// ===================================================================

class ModbusTcpClient {
public:
    ModbusTcpClient(std::string ip, int port, uint8_t slaveId)
        : ip_(std::move(ip)), port_(port), slaveId_(slaveId) {}

    ~ModbusTcpClient() { disconnect(); }

    // Not copyable: this object owns a raw socket handle. If it were
    // copied, two objects would both believe they own the same socket
    // and both would try to close it.
    ModbusTcpClient(const ModbusTcpClient&) = delete;
    ModbusTcpClient& operator=(const ModbusTcpClient&) = delete;

    // Movable, even though it's not copyable: transferring ownership of the
    // socket handle to a new object is safe (only one object owns it at a
    // time); copying it would leave two objects both thinking they own the
    // same socket. This is required so Device (which holds one of these)
    // can live inside a std::vector<Device> — vector needs to relocate
    // elements internally (e.g. during reserve()/push_back()), and it does
    // that by moving, not copying.
    ModbusTcpClient(ModbusTcpClient&& other) noexcept
        : ip_(std::move(other.ip_)),
          port_(other.port_),
          slaveId_(other.slaveId_),
          sockFd_(other.sockFd_),
          transactionId_(other.transactionId_) {
        other.sockFd_ = -1; // so the moved-from object's destructor won't close our socket
    }

    ModbusTcpClient& operator=(ModbusTcpClient&& other) noexcept {
        if (this != &other) {
            closeSocket(); // release whatever socket we currently hold, if any
            ip_ = std::move(other.ip_);
            port_ = other.port_;
            slaveId_ = other.slaveId_;
            sockFd_ = other.sockFd_;
            transactionId_ = other.transactionId_;
            other.sockFd_ = -1;
        }
        return *this;
    }

    // Opens the TCP connection. Safe to call again after a drop.
    bool connectDevice() {
        closeSocket();

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;

        struct timeval timeout{};
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        if (inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr) <= 0) {
            close(sock);
            return false;
        }

        if (connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            close(sock);
            return false;
        }

        sockFd_ = sock;
        return true;
    }

    void disconnect() { closeSocket(); }

    bool isConnected() const { return sockFd_ >= 0; }

    // Reads `quantity` 16-bit registers/coils starting at `startReg`,
    // reusing the already-open connection. On failure, closes the socket
    // so the caller knows to reconnect next cycle.
    bool readRegisters(uint8_t functionCode, uint16_t startReg, uint16_t quantity,
                        std::vector<uint8_t>& out) {
        if (!isConnected()) return false;

        bool ok = sendRequest(transactionId_++, functionCode, startReg, quantity) &&
                  receiveResponse(out);

        if (!ok) closeSocket();
        return ok;
    }

private:
    std::string ip_;
    int port_;
    uint8_t slaveId_;
    int sockFd_ = -1;
    uint16_t transactionId_ = 1;

    void closeSocket() {
        if (sockFd_ >= 0) {
            close(sockFd_);
            sockFd_ = -1;
        }
    }

    bool sendRequest(uint16_t transactionId, uint8_t functionCode, uint16_t startReg, uint16_t quantity) {
        std::vector<uint8_t> request(12);

        request[0] = (transactionId >> 8) & 0xFF;
        request[1] = transactionId & 0xFF;
        request[2] = 0x00;
        request[3] = 0x00;
        request[4] = 0x00;
        request[5] = 0x06;
        request[6] = slaveId_;

        request[7] = functionCode;
        request[8] = (startReg >> 8) & 0xFF;
        request[9] = startReg & 0xFF;
        request[10] = (quantity >> 8) & 0xFF;
        request[11] = quantity & 0xFF;

        ssize_t sent = send(sockFd_, request.data(), request.size(), 0);
        return sent == static_cast<ssize_t>(request.size());
    }

    bool receiveResponse(std::vector<uint8_t>& out) {
        uint8_t header_buf[9];
        ssize_t received = recv(sockFd_, header_buf, sizeof(header_buf), MSG_WAITALL);
        if (received < static_cast<ssize_t>(sizeof(header_buf))) return false;

        if (header_buf[7] & 0x80) return false; // exception response

        uint8_t byteCount = header_buf[8];
        out.resize(byteCount);

        received = recv(sockFd_, out.data(), byteCount, MSG_WAITALL);
        return received == static_cast<ssize_t>(byteCount);
    }
};

// ===================================================================
// Device - one physical unit: its config + persistent client + readings
// ===================================================================

struct ReadingResult {
    std::string name;
    bool valid = false;
    double value = 0.0;
};

class Device {
public:
    explicit Device(DeviceConfig config)
        : config_(std::move(config)),
          client_(config_.ip, config_.port, config_.slaveId) {}

    // No-op if already connected — this is what makes it persistent
    // instead of reconnect-per-cycle.
    bool ensureConnected() {
        if (client_.isConnected()) return true;

        if (!client_.connectDevice()) {
            std::cerr << "[" << config_.name << "] connect failed to "
                      << config_.ip << ":" << config_.port << std::endl;
            return false;
        }
        std::cout << "[" << config_.name << "] connected to "
                  << config_.ip << ":" << config_.port << std::endl;
        return true;
    }

    // Reads every configured register over the existing open connection.
    std::vector<ReadingResult> pollAll() {
        std::vector<ReadingResult> results;
        results.reserve(config_.readings.size());

        for (const auto& readingCfg : config_.readings) {
            ReadingResult result;
            result.name = readingCfg.name;

            std::vector<uint8_t> raw;
            bool ok = client_.readRegisters(readingCfg.functionCode, readingCfg.reg, 1, raw);

            if (ok && !raw.empty()) {
                result.valid = true;
                result.value = convert(readingCfg, raw);
            } else {
                result.valid = false;
                result.value = 0.0;
            }
            results.push_back(result);
        }
        return results;
    }

    const std::string& name() const { return config_.name; }

private:
    DeviceConfig config_;
    ModbusTcpClient client_;

    double convert(const ReadingConfig& cfg, const std::vector<uint8_t>& raw) const {
        if (cfg.type == "discrete") {
            return raw.empty() ? 0.0 : (raw[0] & 0x01);
        }

        uint16_t combined = (raw[0] << 8) | raw[1];

        if (cfg.type == "temperature") {
            return (combined * 0.18) + 32.0; // per device datasheet scaling
        }
        return static_cast<double>(combined); // "raw"
    }
};

// ===================================================================
// DatabaseLogger - persistent MySQL connection
// ===================================================================

class DatabaseLogger {
public:
    explicit DatabaseLogger(DatabaseConfig config) : config_(std::move(config)) {}

    DatabaseLogger(const DatabaseLogger&) = delete;
    DatabaseLogger& operator=(const DatabaseLogger&) = delete;

    bool ensureConnected() {
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

    void logReadings(const std::string& deviceName, const std::vector<ReadingResult>& readings) {
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
            connection_.reset(); // force reconnect next cycle instead of hammering a broken handle
        }
    }

private:
    DatabaseConfig config_;
    std::unique_ptr<sql::Connection> connection_;
};

// ===================================================================
// main - wiring + poll loop
// ===================================================================

static void printReadings(const std::string& deviceName, const std::vector<ReadingResult>& readings) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::cout << "--- " << deviceName << " @ "
              << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << " ---" << std::endl;
    for (const auto& r : readings) {
        std::cout << "  " << r.name << ": ";
        if (r.valid) std::cout << r.value;
        else std::cout << "N/A";
        std::cout << std::endl;
    }
}

int main(int argc, char** argv) {
    std::string configPath = (argc > 1) ? argv[1] : "config.json";

    AppConfig config;
    try {
        config = loadConfig(configPath);
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] Failed to load config: " << e.what() << std::endl;
        return 1;
    }

    DatabaseLogger dbLogger(config.database);

    std::vector<Device> devices;
    devices.reserve(config.devices.size());
    for (const auto& deviceConfig : config.devices) {
        devices.emplace_back(deviceConfig);
    }

    std::cout << "Starting Modbus TCP Logger (" << devices.size() << " device(s), poll every "
              << config.pollIntervalSeconds << "s)..." << std::endl;

    // One-time initial connection attempt per device.
    for (auto& device : devices) {
        device.ensureConnected();
    }

    while (true) {
        auto next_run = std::chrono::steady_clock::now() + std::chrono::seconds(config.pollIntervalSeconds);

        for (auto& device : devices) {
            if (!device.ensureConnected()) continue; // still down, retry next cycle

            auto readings = device.pollAll();          // reuses the open socket
            printReadings(device.name(), readings);
            dbLogger.logReadings(device.name(), readings);
        }

        std::cout << std::endl;
        std::this_thread::sleep_until(next_run);
    }

    return 0;
}