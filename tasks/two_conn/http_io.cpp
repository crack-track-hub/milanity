

#include <arpa/inet.h>             // Linux networking header for IP conversions (inet_pton) and byte-ordering (htons)
#include <chrono>                  // Standard C++ time/date manipulation library used for timestamps
#include <cstdint>                 // Standard library for fixed-width integer types (uint8_t, uint16_t)
#include <cstring>                 // C-style string manipulation library (e.g., memset, memcpy)
#include <fstream>                 // File stream I/O library to open and read disk-based config files
#include <iomanip>                 // Input/Output stream formatting manipulation utilities
#include <iostream>                // Standard input/output streams for console logs (std::cout, std::cerr)
#include <memory>                  // Smart memory manager utilities providing automated unique_ptr lifecycle handling
#include <string>                  // Standard C++ string object processing wrapper
#include <sys/socket.h>            // Low-level POSIX Linux socket framework for managing TCP/IP pipes
#include <thread>                  // Standard thread library used for application execution delays/concurrency
#include <unistd.h>                // Standard POSIX OS API system header used for socket shutdown routines (close)
#include <vector>                  // Standard dynamic array data block memory tracking container

#include <nlohmann/json.hpp>       // Modern third-party JSON data structure parsing header dependency

// Web Server Library Header Included Here
#define CPPHTTPLIB_OPENSSL_SUPPORT // Enables OpenSSL security wrappers for HTTPS inside the HTTP library engine
#include "httplib.h"               // Light, single-header C++ embedded HTTP web application server platform

#include <mysql_driver.h>          // Official Oracle C++ MySQL native driver management component
#include <mysql_connection.h>      // Session connection pipeline context driver interface for MySQL
#include <cppconn/prepared_statement.h> // Protected PreparedStatement engine used to avoid SQL injection risks






// ===================================================================
// Config structs (loaded from config.json)
// ===================================================================
struct ReadingConfig {
    std::string name;              // Sensor identifier label string (e.g., "Main_Temperature")
    uint8_t functionCode = 0;      // Modbus industrial protocol request action selector identifier 
    uint16_t reg = 0;              // Data tracking target base register address number 
    std::string type;              // Data classification mapping tag (e.g., "temperature", "discrete")
};

struct DeviceConfig {
    std::string name;              // Unique human-readable equipment identifier string (e.g., "Chiller_01")
    std::string ip;                // Hardware device network target destination IP routing string
    int port = 502;                // Connection port sequence handler target (Default standard Modbus port is 502)
    uint8_t slaveId = 1;           // Sub-station asset identification route index (Defaults to Unit ID 1)
    std::vector<ReadingConfig> readings; // Array collection listing all configured register tags for this instrument
};

struct DatabaseConfig {
    std::string host;              // Destination database URL routing location target string
    std::string user;              // Database authentication account access username credential
    std::string password;          // Secure access entry validation password matching credential string
    std::string schema;            // Specific logical database schema table storage name target
};

struct AppConfig {
    DatabaseConfig database;       // Embedded layout containing target database server operational variables
    std::vector<DeviceConfig> devices; // Collection vector holding every instrument profile tracked by the gateway
};






// --- nlohmann::json parsing hooks---
void from_json(const nlohmann::json& j, ReadingConfig& r) {
    j.at("name").get_to(r.name);   // Extract name attribute directly from JSON configuration node
    j.at("register").get_to(r.reg); // Pull the target register mapping reference code number 
    j.at("type").get_to(r.type);   // Pull raw parameter data conversion classification rule word

    // Inspect if function code field configuration was stored as text or primitive integer token
    if (j.at("function_code").is_string()) {
        // Parse incoming text-based integers or hexadecimal syntax parameters into strict uint8_t types
        r.functionCode = static_cast<uint8_t>(std::stoi(j.at("function_code").get<std::string>(), nullptr, 0));
    } else {
        r.functionCode = j.at("function_code").get<uint8_t>(); // Directly assign numerical format value
    }
}

void from_json(const nlohmann::json& j, DeviceConfig& d) {
    j.at("name").get_to(d.name);   // Transfer equipment name from matching JSON entry context mapping
    j.at("ip").get_to(d.ip);       // Bind physical IP interface targeting parameters string
    j.at("port").get_to(d.port);   // Bind port connectivity sequence configuration value
    d.slaveId = j.value("slave_id", 1); // Extract unit slave id or substitute fallback value 1 if omitted
    j.at("readings").get_to(d.readings); // Map nested sensors register tracking blocks recursively
}

void from_json(const nlohmann::json& j, DatabaseConfig& db) {
    j.at("host").get_to(db.host);  // Pull host location criteria out of specific JSON database config block
    j.at("user").get_to(db.user);  // Pull authentication identity username from JSON block data mapping
    j.at("password").get_to(db.password); // Extract connection security password token parameter
    j.at("schema").get_to(db.schema); // Parse the storage destination workspace collection schema target
}

void from_json(const nlohmann::json& j, AppConfig& cfg) {
    j.at("database").get_to(cfg.database); // Map overall system structural configuration for database fields
    j.at("devices").get_to(cfg.devices);   // Extract list arrays containing industrial sensor instrument targets
}

AppConfig loadConfig(const std::string& path) {   
    std::ifstream file(path);      // Instantiate a file tracking read pipeline utilizing disk target path parameter
    if (!file.is_open()) {         // Verify that configuration file path was accessed and opened correctly
        throw std::runtime_error("Could not open config file: " + path); // Halt pipeline execution with runtime crash log
    }
    nlohmann::json j;              // Prepare temporary memory canvas container layout schema to hold config elements
    file >> j;                     // Stream text contents directly out of file handler data storage into JSON object
    return j.get<AppConfig>();     // Unpack parsed generic parameters directly out to structured AppConfig instances
}




// ===================================================================
// ModbusTcpClient - Connection Handling
// ===================================================================
class ModbusTcpClient {
public:
    // Class constructor capturing networking metrics and target sub-station identification routes
    ModbusTcpClient(std::string ip, int port, uint8_t slaveId)
        : ip_(std::move(ip)), port_(port), slaveId_(slaveId) {}

    ~ModbusTcpClient() { disconnect(); } // Class destructor enforcing safe connection teardowns when exiting context

    ModbusTcpClient(const ModbusTcpClient&) = delete; // Enforce strict copy-prevention rules to avoid shared socket bugs
    ModbusTcpClient& operator=(const ModbusTcpClient&) = delete; // Block duplicating code references to distinct open socket links

    // Move constructor implementation transferring live socket data ownership flags to newer structural entities
    ModbusTcpClient(ModbusTcpClient&& other) noexcept
        : ip_(std::move(other.ip_)), port_(other.port_), slaveId_(other.slaveId_),
          sockFd_(other.sockFd_), transactionId_(other.transactionId_) {
        other.sockFd_ = -1;        // Neutralize source descriptor identity tracking value to prevent duplicate close bugs
    }

    // Move assignment operator definition cleanly taking control of external class resources across code scopes
    ModbusTcpClient& operator=(ModbusTcpClient&& other) noexcept {
        if (this != &other) {      // Guard clause checking if object instances match exactly before processing transfers
            closeSocket();         // Disconnect existing socket pipes currently operational inside this scope
            ip_ = std::move(other.ip_); // Safely transfer source configuration properties forward
            port_ = other.port_;   // Copy active port assignment definitions over to the target context
            slaveId_ = other.slaveId_; // Copy target hardware identification mapping variable
            sockFd_ = other.sockFd_;   // Claim raw kernel file descriptor handle identity tracking index 
            transactionId_ = other.transactionId_; // Maintain continuity synchronization index sequence values
            other.sockFd_ = -1;    // Invalidate ownership markers tracking the origin source component layout
        }
        return *this;              // Return updated self reference pointer mapping address
    }

    bool connectDevice() {
        closeSocket();             // Erase existing broken connection records before initiating connection updates
        int sock = socket(AF_INET, SOCK_STREAM, 0); // Request modern IPv4 Standard TCP Streaming socket link handle 
        if (sock < 0) return false; // Immediate failure exit if kernel fails to issue empty system descriptor tracking records

        struct timeval timeout{};  // Construct timeout validation structural component mapping
        timeout.tv_sec = 2;        // Establish connection block limit duration to 2 full execution seconds
        timeout.tv_usec = 0;       // Reset microsecond configuration parameters down to zero
        // Apply read and write block response timeouts to prevent thread execution hangs across bad nodes
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        sockaddr_in server_addr{}; // Instantiate low-level socket interface structural setup configuration 
        server_addr.sin_family = AF_INET; // Restrict system address configurations strictly back to standard IPv4 networks
        server_addr.sin_port = htons(port_); // Transform network parameter values using standard network byte order alignments

        // Convert standard text IP tracking formats into binary representation arrays
        if (inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr) <= 0) {
            close(sock);           // Release open operational system socket context tracking index allocation record
            return false;          // Report structural failure handling back up code tree execution route
        }

        // Initialize handshake execution pipeline against target industrial remote system device address 
        if (connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            close(sock);           // Terminate interface handle record mappings if connection attempts fail
            return false;          // Terminate routine reporting unsuccessful networking connection state
        }

        sockFd_ = sock;            // Cache verified functional operating socket handler descriptor locally
        return true;               // Return successful status acknowledgment flag 
    }

    void disconnect() { closeSocket(); } // Expose clean programmatic entry route to force device connection breaks
    bool isConnected() const { return sockFd_ >= 0; } // Analyze connection health status indicators cleanly

    bool readRegisters(uint8_t functionCode, uint16_t startReg, uint16_t quantity, std::vector<uint8_t>& out) {
        if (!isConnected()) return false; // Deny execution attempts instantly if system link states evaluate down to offline
        // Chain operations: broadcast frame packet out, track response bytes back, and evaluate overall outcome
        bool ok = sendRequest(transactionId_++, functionCode, startReg, quantity) && receiveResponse(out);
        if (!ok) closeSocket();    // Cleanly dump open socket file structures if transmission failures occur
        return ok;                 // Return final transmission task status confirmation
    }

    void setSlaveId(uint8_t slaveId) { slaveId_ = slaveId; } // Expose slave ID overrides to handle dynamic routing requests

private:
    std::string ip_;               // Internal tracker preserving connection target destination IP location
    int port_;                     // Variable storing active target instrument port mapping index
    uint8_t slaveId_;              // Unit address identifier index assigned to active Modbus network nodes
    int sockFd_ = -1;              // Kernel standard file descriptor reference handling index tracking live channels
    uint16_t transactionId_ = 1;   // Sequence index mapping incremented across subsequent command operations

    void closeSocket() {
        if (sockFd_ >= 0) {        // Check if file descriptors map back to functional system entities
            close(sockFd_);        // Terminate pipeline pathways cleanly inside system environment layers
            sockFd_ = -1;          // Return state identification numbers back to unassigned default markers
        }
    }

    bool sendRequest(uint16_t transactionId, uint8_t functionCode, uint16_t startReg, uint16_t quantity) {
        std::vector<uint8_t> request(12); // Construct raw array payload framework initialized across 12 explicit bytes
        request[0] = (transactionId >> 8) & 0xFF;  // Inject upper transaction index sequence data bytes
        request[1] = transactionId & 0xFF;         // Inject lower byte configurations of tracking transaction sequence
        request[2] = 0x00;         // Protocol indicator upper byte parameter assignment location (Modbus standard is 0)
        request[3] = 0x00;         // Protocol identifier lower byte location configuration values
        request[4] = 0x00;         // Command length specification marker high byte assignment tracking
        request[5] = 0x06;         // Remaining command byte lengths declaration index (Station ID + code options = 6 bytes)
        request[6] = slaveId_;     // Set targeted destination slave unit tracking index position parameter
        request[7] = functionCode; // Load explicit hardware polling action request type code value
        request[8] = (startReg >> 8) & 0xFF;       // Split starting register allocation map parameters (High byte)
        request[9] = startReg & 0xFF;              // Inject low byte reference tracking location for register maps
        request[10] = (quantity >> 8) & 0xFF;     // Specify register acquisition volume criteria (Upper byte metrics)
        request[11] = quantity & 0xFF;             // Complete requested byte quantity configuration payload array settings

        // Broadcast binary command package out to target infrastructure via network socket interface structures
        ssize_t sent = send(sockFd_, request.data(), request.size(), 0);
        return sent == static_cast<ssize_t>(request.size()); // Verify total transmitted length matches structure boundaries
    }

    bool receiveResponse(std::vector<uint8_t>& out) {
        uint8_t header_buf[9];     // Allocate data block to intercept inbound Modbus MBAP + functional headers
        // Block processing loop until full 9-byte sequence is received across open communications pipe
        ssize_t received = recv(sockFd_, header_buf, sizeof(header_buf), MSG_WAITALL);
        if (received < static_cast<ssize_t>(sizeof(header_buf))) return false; // Abort processing if returned bytes are truncated
        if (header_buf[7] & 0x80) return false; // Fail request processing if function code contains error flag bits (0x80)

        uint8_t byteCount = header_buf[8]; // Read 9th data byte position containing register payload payload sizing data
        out.resize(byteCount);     // Re-allocate target vector canvas sizes to fit inbound payload parameters
        // Read final remaining string sequences from streaming channels out directly into output buffer array containers
        received = recv(sockFd_, out.data(), byteCount, MSG_WAITALL);
        return received == static_cast<ssize_t>(byteCount); // Report true if final received lengths match expected counts
    }
};




// ===================================================================
// Device Class
// ===================================================================
struct ReadingResult {
    std::string name;              // Sensor tracking identification string value label
    bool valid = false;            // Telemetry accuracy status flag tracking query operations outcomes
    double value = 0.0;            // Processed data point floating point scalar output parameter
};

class Device {
public:
    // Constructor associating configuration fields directly into dedicated Modbus client execution modules
    explicit Device(DeviceConfig config)
        : config_(std::move(config)), client_(config_.ip, config_.port, config_.slaveId) {}

    bool ensureConnected() {
        if (client_.isConnected()) return true; // Keep current configurations active if pipeline states evaluate to online
        return client_.connectDevice(); // Trigger fresh hardware reconnection sequence against destination target
    }

    std::vector<ReadingResult> pollAll(uint8_t overrideSlaveId = 0) {
        if (overrideSlaveId != 0) {
            client_.setSlaveId(overrideSlaveId); // Enforce transient station destination dynamic override variables
        } else {
            client_.setSlaveId(config_.slaveId); // Revert settings back to static properties saved in base configuration
        }

        std::vector<ReadingResult> results; // Create temporary tracking list to hold parsed operational read values
        results.reserve(config_.readings.size()); // Optimize memory handling performance layout before iterating loops

        for (const auto& readingCfg : config_.readings) { // Step sequentially through every configured register task
            ReadingResult result;  // Instantiate localized container framework to evaluate current step outcomes
            result.name = readingCfg.name; // Apply sensor naming convention metadata mappings

            std::vector<uint8_t> raw; // Prepare array structures to capture downstream telemetry data lines
            // Request single registry data point over active communication pipeline layers
            bool ok = client_.readRegisters(readingCfg.functionCode, readingCfg.reg, 1, raw);

            if (ok && !raw.empty()) { // Verify successful hardware response parameters were collected
                result.valid = true; // Set validation status parameter tracking rules to true
                result.value = convert(readingCfg, raw); // Decouple raw byte streams into floating point values
            } else {
                result.valid = false; // Flag error outcome mappings across system operation pipelines
                result.value = 0.0;  // Force telemetry result readings to zero out on operational faults
            }
            results.push_back(result); // Enqueue updated record structures back into global metrics output arrays
        }
        return results;            // Return collected results back to upper level query tracking managers
    }

    const std::string& name() const { return config_.name; } // Read-only access exposing mapped instrument identities

private:
    DeviceConfig config_;          // Local configuration dataset containing targeting and sensor requirements parameters
    ModbusTcpClient client_;       // Network driver communication engine linked to specific device configurations

    double convert(const ReadingConfig& cfg, const std::vector<uint8_t>& raw) const {
        if (cfg.type == "discrete") return raw.empty() ? 0.0 : (raw[0] & 0x01); // Extract singular state bit flags
        uint16_t combined = (raw[0] << 8) | raw[1]; // Merge individual byte sequences back to standard 16-bit parameters
        if (cfg.type == "temperature") return (combined * 0.18) + 32.0; // Apply custom engineering calculations
        return static_cast<double>(combined); // Cast standard registry data points directly out to floating numbers
    }
};





// ===================================================================
// DatabaseLogger Class
// ===================================================================
class DatabaseLogger {
public:
    explicit DatabaseLogger(DatabaseConfig config) : config_(std::move(config)) {} // Cache access details upon initialization

    bool ensureConnected() {
        if (connection_ && !connection_->isClosed()) return true; // Skip reconnection steps if context states evaluate to healthy
        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance(); // Fetch active runtime driver pointer
            // Initialize physical session pipeline linking target database server address locations
            connection_.reset(driver->connect(config_.host, config_.user, config_.password));
            connection_->setSchema(config_.schema); // Target data workspace schema maps for insertion operations
            return true;           // Return positive initialization outcome indicators
        } catch (sql::SQLException&) {
            connection_.reset();   // Safely erase faulty runtime session pointers to clear cache layouts
            return false;          // Report operational transaction failures back up execution frameworks
        }
    }

    void logReadings(const std::string& deviceName, const std::vector<ReadingResult>& readings) {
        if (!ensureConnected()) return; // Abort storage tasks immediately if database availability links collapse
        try {
            // Compile secure parameters statement layout to mitigate data contamination risks
            std::unique_ptr<sql::PreparedStatement> pstmt(connection_->prepareStatement(
                "INSERT INTO device_readings (device_name, reading_name, value, is_valid, reading_time) VALUES (?, ?, ?, ?, NOW())"));

            for (const auto& r : readings) { // Iterate sequentially over every element logged across sensor data sheets
                pstmt->setString(1, deviceName); // Bind target instrument name parameters into execution index 1
                pstmt->setString(2, r.name);     // Bind unique sensor tracking tag identity properties into position 2
                if (r.valid) pstmt->setDouble(3, r.value); // Load validated telemetry numbers into input mapping parameter 3
                else pstmt->setNull(3, sql::DataType::DOUBLE); // Set database value fields down to Null during device faults
                pstmt->setBoolean(4, r.valid);   // Bind operation success indicator state tracking flags to argument index 4
                pstmt->executeUpdate();         // Execute transactional storage routines onto target tables
            }

        } catch (sql::SQLException&) {
            connection_.reset();   // Clear corrupted memory connection pointers upon structural communication crashes
        }
    }

private:
    DatabaseConfig config_;        // Preserves targeted credentials data maps for handling database authentications
    std::unique_ptr<sql::Connection> connection_; // Intelligent auto-managed system memory reference mapping database sessions
};




// ===================================================================
// Main Engine Web API Server Orchestrator
// ===================================================================
int main(int argc, char** argv) {
    // Read command-line parameter vectors or assign default config file matching paths
    std::string configPath = (argc > 1) ? argv[1] : "config.json";

    AppConfig config;              // Instantiate main structural blueprint architecture tracking data layout parameters
    try {
        config = loadConfig(configPath); // Extract and process layout configuration rules out of disk configuration files
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] Failed to load config: " << e.what() << std::endl; // Log structural crashes onto system interfaces
        return 1;                  // Halt OS processing executions terminating loop sequences with failure index codes
    }

    DatabaseLogger dbLogger(config.database); // Construct active runtime relational database transaction processing node

    std::vector<Device> devices;   // Initialize array allocation vectors to manage live instrument class structures
    devices.reserve(config.devices.size()); // Pre-allocate internal memory structures to increase tracking step performance
    for (const auto& deviceConfig : config.devices) {
        devices.emplace_back(deviceConfig); // Build and instantiate active physical instrumentation modules inline
    }

    // Creating HTTP Server running on core loop pipeline
    httplib::Server svr;           // Instantiate core network application server pipeline framework logic instance

    std::cout << "Web Gateway initialized. Server running on http://0.0.0.0:8080" << std::endl; // Broadcast initialization status

    // Browser URL trigger interception mapping endpoint
    svr.Get("/get_reading", [&](const httplib::Request& req, httplib::Response& res) {
        
        // 1. Extract query strings from Chrome Browser URL
        std::string targetDevice = req.get_param_value("device"); // Extract device identification target string parameters
        std::string slaveIdStr = req.get_param_value("slave_id"); // Extract sub-station address routing override variables

        if (targetDevice.empty()) { // Verify operational request data contains minimal necessary target parameters
            res.status = 400;      // Set return error tracking indicators back to bad request status 400
            res.set_content("{\"error\":\"Missing query parameter 'device'\"}", "application/json"); // Return descriptive error payload
            return;                // Exit routing logic context immediately
        }

        uint8_t reqSlaveId = 0;    // Standard local override initialization baseline variable
        if (!slaveIdStr.empty()) {
            reqSlaveId = static_cast<uint8_t>(std::stoi(slaveIdStr)); // Safe conversion of parameter data strings to bytes
        }

        // 2. Locate targeted device object matrix inside the vector array
        Device* selectedDevice = nullptr; // Clear selection pointer record assignments down to zero states
        for (auto& device : devices) { // Iterate systematically through every available instantiated device resource
            if (device.name() == targetDevice) { // Evaluate if hardware profile tags match URL request criteria
                selectedDevice = &device; // Extract memory link destination pointers tracking matching profiles
                break;             // Break tracking operation loops upon securing proper target parameters
            }
        }

        if (!selectedDevice) {     // Confirm matching equipment profiles were obtained successfully out of config pools
            res.status = 404;      // Set target return responses back to Not Found state status codes 404
            res.set_content("{\"error\":\"Device not found in configuration\"}", "application/json"); // Dispatch error details
            return;                // Terminate handler code execution steps
        }

        // 3. Drive hardware execution pipelines safe trigger
        if (!selectedDevice->ensureConnected()) { // Verify communication paths to hardware nodes evaluate to live status
            res.status = 503;      // Assign server error responses down to Service Unavailable status code 503
            res.set_content("{\"error\":\"Could not connect to target device hardware pipe\"}", "application/json"); // Log fault messages
            return;                // Exit tracking routines immediately
        }

        // 4. Fetch readings and translate hex data parameters
        std::vector<ReadingResult> finalReadings = selectedDevice->pollAll(reqSlaveId); // Collect live system metrics from network

        // 5. Build dynamic text output data block format for Browser display
        nlohmann::json json_response; // Instantiate empty JSON object schema arrays to wrap telemetry records
        json_response["device_name"] = selectedDevice->name(); // Assign instrument identification parameters directly into JSON nodes
        // Log generation time parameters inside response frames via system clock routines
        json_response["timestamp"] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        
        nlohmann::json readings_arr = nlohmann::json::array(); // Create standardized array structure layers inside JSON environments
        for (const auto& r : finalReadings) { // Parse collected real-world performance updates line per line
            nlohmann::json single_r; // Create mini JSON sub-structures to preserve individual sensor values
            single_r["sensor_name"] = r.name; // Map specific physical sensor system parameter names
            single_r["is_valid"] = r.valid;  // Embed hardware operational status tracking evaluations flags
            if (r.valid) {
                single_r["value"] = r.value; // Map translated metric data into final output tracks
            } else {
                single_r["value"] = nullptr; // Append JSON null definitions into fields experiencing data faults
            }
            readings_arr.push_back(single_r); // Load sub-records back up into unified master data array lines
        }
        json_response["readings"] = readings_arr; // Append fully built sensor datasets back into root response frames

        // Auto background logging execution trigger to database
        if (!finalReadings.empty()) {
            dbLogger.logReadings(selectedDevice->name(), finalReadings); // Write telemetry points safely to database engines
        }

        // 6. Return response to Chrome Client Engine interface wrapper
        res.status = 200;          // Finalize operational route success confirmations with status code 200
        res.set_content(json_response.dump(4), "application/json"); // Dump fully structured and indented JSON out to network stream
    });

    // Server keeps listening on port 8080 continuously online
    svr.listen("0.0.0.0", 8080);   // Bind communications server context across all network adapters targeting port 8080
    return 0;                      // Terminate core program workflows safely return success codes back to host systems
}