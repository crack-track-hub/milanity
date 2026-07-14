// main.cpp
// Single-file OOP Modbus TCP -> MySQL logger (Manual Query Version - Entire Device Only).

#include <arpa/inet.h> //used in networking for ip conversion and socket handling String IP → Binary IP.
#include <chrono>    // used for timestamp , time track
#include <cstdint>  // standard integer types modbus accept only 16 and 8 bytes numbers
#include <cstring>  // string and memory function use, low level memory operation use 
#include <fstream>  // used for file read like read json from disk and iimport on program
#include <iomanip>// use to print time and data in design format
#include <iostream>// input and output handling 
#include <memory>// used to free up the unused memory like unused variable
#include <string>// string handling library
#include <sys/socket.h>// used to create socket, send data, receive data, network communication
#include <thread>// used to schedule the processing time on background, multithreading
#include <unistd.h>// used to close the network socket neatly close ()
#include <vector>   // normally arry is fixed sizw, so it is used to increase memory size when it exceed the size

#include <nlohmann/json.hpp> // to read config.json file and acess the varibles on json


#include <mysql_driver.h>
#include <mysql_connection.h> // my sql connector tools
#include <cppconn/prepared_statement.h>

// ===================================================================
// Config structs (loaded from config.json)
// ===================================================================
// explains how the data structre, how the single register form blueprint
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
   
    DatabaseConfig database;  //MySQL database connect panna thevaiyana configuration details-a orae idathula store panna.
    std::vector<DeviceConfig> devices; //A container used to store multiple DeviceConfig objects.
};

// --- nlohmann::json parsing hooks ---
// used to read the json data because cpp cannot directly read json
void from_json(const nlohmann::json& j, ReadingConfig& r) {
    j.at("name").get_to(r.name);// at() na JSON object-la irundhu specific key value-a edukkum function.
    j.at("register").get_to(r.reg); // get_to na JSON value-a C++ variable-kulla copy panna use pannuvom.
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
    
    j.at("database").get_to(cfg.database);
    j.at("devices").get_to(cfg.devices);
}
// automatic load outer json file data when the progrm start
AppConfig loadConfig(const std::string& path) {   //A function named loadConfig that receives a file path as a read-only string reference and returns a fully populated AppConfig object after reading the configuration file

    std::ifstream file(path); //reading phase start
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + path);
    }
    nlohmann::json j;
    file >> j;
    return j.get<AppConfig>(); // appconfig function pass and trigger from_json
}

// ===================================================================
// ModbusTcpClient - one persistent socket to one Modbus TCP slave
// ===================================================================

class ModbusTcpClient {
public:
    ModbusTcpClient(std::string ip, int port, uint8_t slaveId)// is automatically start when function start

        : ip_(std::move(ip)), port_(port), slaveId_(slaveId) {}

    ~ModbusTcpClient() { disconnect(); } // it disconnect when the function close

    ModbusTcpClient(const ModbusTcpClient&) = delete;// object did n't allow copy because socket give error on double enrty, invalid connection
    ModbusTcpClient& operator=(const ModbusTcpClient&) = delete;

    ModbusTcpClient(ModbusTcpClient&& other) noexcept
        : ip_(std::move(other.ip_)),// ownership transfer instead of copying becuse of conflict
          port_(other.port_),
          slaveId_(other.slaveId_),
          sockFd_(other.sockFd_),
          transactionId_(other.transactionId_) {
        other.sockFd_ = -1;
    }

    ModbusTcpClient& operator=(ModbusTcpClient&& other) noexcept {
        if (this != &other) {
            closeSocket();
            ip_ = std::move(other.ip_);
            port_ = other.port_;
            slaveId_ = other.slaveId_; 
            sockFd_ = other.sockFd_;
            transactionId_ = other.transactionId_;
            other.sockFd_ = -1;
        }
        return *this;
    }

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

        if (header_buf[7] & 0x80) return false;

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

    // Polls ALL configured registers for this device
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
            return (combined * 0.18) + 32.0;
        }
        return static_cast<double>(combined);
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
            std::cout << "[DB] logged " << readings.size() << " entry(ies) for " << deviceName << std::endl;

        } catch (sql::SQLException& e) {
            std::cerr << "[DB ERROR] insert failed (" << e.getErrorCode() << "): " << e.what() << std::endl;
            connection_.reset();
        }
    }

private:
    DatabaseConfig config_;
    std::unique_ptr<sql::Connection> connection_;
};

// ===================================================================
// main - Simple Manual Query Loop
// ===================================================================

static void printReadings(const std::string& deviceName, const std::vector<ReadingResult>& readings) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::cout << "\n--- " << deviceName << " @ "
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

    AppConfig config;// object created
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

    std::cout << "Modbus TCP Manual Logger Initialized (" << devices.size() << " device(s) configured)." << std::endl;
    std::cout << "Type 'exit' to quit.\n" << std::endl;

    while (true) {
        std::string targetDevice;

        std::cout << "Enter Device Name to Poll: ";
        std::getline(std::cin, targetDevice);
        
        if (targetDevice == "exit") break;
        if (targetDevice.empty()) continue;

        // Find the device matching the entered name
        Device* selectedDevice = nullptr;
        for (auto& device : devices) {
            if (device.name() == targetDevice) {
                selectedDevice = &device;
                break;
            }
        }

        if (!selectedDevice) {
            std::cerr << "Error: Device '" << targetDevice << "' not found in configuration.\n" << std::endl;
            continue;
        }

        // Try establishing connection explicitly for the chosen device
        if (!selectedDevice->ensureConnected()) {
            std::cerr << "Error: Could not connect to device " << targetDevice << "\n" << std::endl;
            continue;
        }

        // Fetch ALL registers for this device automatically
        std::cout << "[System] Fetching all registers for " << targetDevice << "..." << std::endl;
        std::vector<ReadingResult> finalReadings = selectedDevice->pollAll();
        
        if (!finalReadings.empty()) {
            printReadings(selectedDevice->name(), finalReadings);
            dbLogger.logReadings(selectedDevice->name(), finalReadings);
        } else {
            std::cerr << "Warning: No data was read from device " << targetDevice << std::endl;
        }
        
        std::cout << std::endl;
    }

    std::cout << "Exiting system cleanly." << std::endl;
    return 0;
}