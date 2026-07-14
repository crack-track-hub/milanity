#include <arpa/inet.h>             // IP address-a binary-ah mathavum (inet_pton), port network byte order-ku mathavum (htons) use aaguthu.
#include <chrono>                  // System time eduthu API response-la dynamic timestamp podra library.
#include <cstdint>                 // uint8_t, uint16_t maari fixed size integer datatypes use panna ithu venum.
#include <cstring>                 // Memory copy (memcpy) ila memset maari C-style string manipulation panna use aaguthu.
#include <fstream>                 // External disk-la irukra "config.json" file-a open panni read panna use aaguthu.
#include <iomanip>                 // Output-a screen-la format panni (alignment, spacing) kamikka use aaguthu.
#include <iostream>                // std::cout, std::cerr moolama console-la success/error logs print panna.
#include <memory>                  // Smart pointers (std::unique_ptr) use panni memory leak aagama database connection-a manage panna.
#include <string>                  // Normal C++ string handling utilities-kaaga use panrom.
#include <sys/socket.h>            // Core Linux OS-oda internal TCP/IP network socket framework-a access panna.
#include <thread>                  // Background execution ila delay use panna multi-threading support library.
#include <unistd.h>                // Raw socket-a system level-la safe-ah shutdown panna 'close()' function ithula irunthu varuthu.
#include <vector>                  // Telemetry records matrum bytes-a dynamic array block-ah store panna.

#include <nlohmann/json.hpp>       // Configuration text strings-a decode panna external third-party JSON parser.

#define CPPHTTPLIB_OPENSSL_SUPPORT // HTTP Server-la HTTPS security support venumna use panra configuration flag.
#include "httplib.h"               // Light-weight embedded C++ web application server platform header.

#include <mysql_driver.h>          // Backend MySQL server-a connect panna direct Oracle controller connection interface.
#include <mysql_connection.h>      // Database link status session pipelines-a handle panra abstraction.
#include <cppconn/prepared_statement.h> // Prepared statements use panni database operations SQL injection attack illaama secure-ah panna.



struct ReadingConfig {
    std::string name;              // Sensor-oda identity tag mapping name (e.g., "Boiler_Temp").
    uint8_t functionCode = 0;      // Modbus function code code selector (e.g., Read Holding Registers - 0x03).
    uint16_t reg = 0;              // PLC ila instrument-la namma target panra register input memory block number.
    std::string type;              // Value conversion classification tag (e.g., "temperature" ila "discrete").
};

struct DeviceConfig {
    std::string name;              // Mapped physical instrument machine name (e.g., "Chiller_01").
    std::string ip;                // Hardware destination ip configuration target routing string.
    int port = 502;                // Connection communication port (Modbus standard default value: 502).
    uint8_t slaveId = 1;           // Sub-station ID reference tracking location (Default standard is 1).
    std::vector<ReadingConfig> readings; // Intha specific equipment kulla namma track panna vendiya sub-sensors list array.
};

struct DatabaseConfig {
    std::string host;              // Target database url route address tracking variable.
    std::string user;              // Database authentication account login user name.
    std::string password;          // Access verification match target security code.
    std::string schema;            // Specific application database schema partition collection target table.
};

struct AppConfig {
    DatabaseConfig database;       // Unified system structure configuration mapping container database variables.
    std::vector<DeviceConfig> devices; // Vector list template array holding all equipment setups on the gateway.
};




void from_json(const nlohmann::json& j, ReadingConfig& r) {
    j.at("name").get_to(r.name);   // JSON node entries-la irunthu sensor name attribute-a fetch pannuthu.
    j.at("register").get_to(r.reg); // JSON reference token integer data-va reading register variable-ku map pannuthu.
    j.at("type").get_to(r.type);   // Core configuration parsing instruction conversion string-a decode pannuthu.

    if (j.at("function_code").is_string()) {
        // Function code string format-la (hex string "0x03") iruntha integer-ah parse panni uint8_t types-ku cast pannuthu.
        r.functionCode = static_cast<uint8_t>(std::stoi(j.at("function_code").get<std::string>(), nullptr, 0));
    } else {
        r.functionCode = j.at("function_code").get<uint8_t>(); // Raw integer data value field-a direct assign pannuthu.
    }
}

void from_json(const nlohmann::json& j, DeviceConfig& d) {
    j.at("name").get_to(d.name);   // Machine name targets assignment data lines transfer mapping.
    j.at("ip").get_to(d.ip);       // Network hardware route criteria field values mapping initialization.
    j.at("port").get_to(d.port);   // Operational connection endpoints configurations parameters assignation.
    d.slaveId = j.value("slave_id", 1); // Slave id value text file block-la missing na code automatically defaults to 1.
    j.at("readings").get_to(d.readings); // Sensor parameters child objects block-a internal storage-ku shift pannuthu.
}

void from_json(const nlohmann::json& j, DatabaseConfig& db) {
    j.at("host").get_to(db.host);  // SQL system endpoint tracking values extraction setup wrapper.
    j.at("user").get_to(db.user);  // User login identification string property parsing allocation data.
    j.at("password").get_to(db.password); // Encryption access key data string configurations setup extraction.
    j.at("schema").get_to(db.schema); // Logical tables workspace mapping parameter extraction.
}

void from_json(const nlohmann::json& j, AppConfig& cfg) {
    j.at("database").get_to(cfg.database); // Global system parsing mapping block database connection definitions.
    j.at("devices").get_to(cfg.devices);   // Industrial equipment list arrays configuration targets structure mapping.
}

AppConfig loadConfig(const std::string& path) {   
    std::ifstream file(path);      // Disk stream path memory canvas reader setup initializing tracking files pipelines.
    if (!file.is_open()) {         // File path location invalid aagi text data pull access fail aanal exception trigger aagum.
        throw std::runtime_error("Could not open config file: " + path);
    }
    nlohmann::json j;              // Temporary JSON canvas memory structure array layout initialization blocks.
    file >> j;                     // Text stream data vectors-a generic structure parsing canvas layers-ku transfer pannuthu.
    return j.get<AppConfig>();     // Object mapping rules hooks trigger panni variables context assignments push pannuthu.
}




class ModbusTcpClient {
public:
    // Class constructor - initial endpoints variables properties values load mapping configuration.
    ModbusTcpClient(std::string ip, int port, uint8_t slaveId)
        : ip_(std::move(ip)), port_(port), slaveId_(slaveId) {}

    ~ModbusTcpClient() { disconnect(); } // Class object scope out ponal standard socket session closure call run aagum.

    ModbusTcpClient(const ModbusTcpClient&) = delete; // Object instances duplicate memory copy tracking values allocation block.
    ModbusTcpClient& operator=(const ModbusTcpClient&) = delete; // Shared instance data reference copy duplication lock wrapper.

    // Move constructor implementation - dynamic resource parameters identity allocation block tracking shifting.
    ModbusTcpClient(ModbusTcpClient&& other) noexcept
        : ip_(std::move(other.ip_)), port_(other.port_), slaveId_(other.slaveId_),
          sockFd_(other.sockFd_), transactionId_(other.transactionId_) {
        other.sockFd_ = -1;        // Origin tracking source socket system identification code reference reset.
    }

    // Move assignment properties operator context transfers processing runtime code sequence steps.
    ModbusTcpClient& operator=(ModbusTcpClient&& other) noexcept {
        if (this != &other) {      // Guard check clause verifying unique memory addresses layout profiles allocations.
            closeSocket();         // Open raw session links already active inside this scope dropping actions execution.
            ip_ = std::move(other.ip_); // Structural configuration property strings shift forward targets.
            port_ = other.port_;   // Network ports criteria parameter attributes extraction assignment values sync.
            slaveId_ = other.slaveId_; // Machine identification address variable parameters setup mapping updates.
            sockFd_ = other.sockFd_;   // Target kernel open file descriptors reference index parameters storage claim.
            transactionId_ = other.transactionId_; // Sequence processing sync continuity track counts context mapping.
            other.sockFd_ = -1;    // Source instance tracking index memory records data cleanup reset values to -1.
        }
        return *this;              // Self mapping instances allocation runtime reference object redirection.
    }

    bool connectDevice() {
        closeSocket();             // Clear previously broken channels histories maps records before establishing fresh connections.
        int sock = socket(AF_INET, SOCK_STREAM, 0); // Request new raw Linux standard IPv4 TCP internet connection socket handle.
        if (sock < 0) return false; // Kernel side tracking descriptor creations fail-aana operation processing failure return.

        struct timeval timeout{};  // Initialize custom network timing management attributes schema layouts block.
        timeout.tv_sec = 2;        // Establish read/write data response lock wait times limit boundaries strictly to 2 seconds.
        timeout.tv_usec = 0;       // Reset microsecond configuration variables baseline down to zero levels.
        // Set standard options block applying connection read timeout boundaries limits mapping.
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        // Set standard options block applying connection write timeout boundaries limits mapping.
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        sockaddr_in server_addr{}; // Low-level interface networking address parameters configuration initialization frameworks.
        server_addr.sin_family = AF_INET; // Limit standard operational configurations properties context back to IPv4 networking scope.
        server_addr.sin_port = htons(port_); // Apply network byte alignment orders shifting conversions to port parameters.

        // Standard human read IP formats string binary mapping representation block evaluations check.
        if (inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr) <= 0) {
            close(sock);           // Terminate unassigned interface tracking records storage channel leaks cleanups.
            return false;          // Failure handler routing response indicators reports.
        }

        // Initialize connection handshake network loop cycles against physical destination industrial equipment.
        if (connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            close(sock);           // Clear active system configuration session resources properties maps tracks on hardware failure.
            return false;          // Return false status indicating failure outcomes checks.
        }

        sockFd_ = sock;            // Save verified working networking session pipeline descriptor pointer index locally.
        return true;               // Return confirmation true flag stating networking interface connection works.
    }

    void disconnect() { closeSocket(); } // Close channels pipeline mappings directly moolama execution hooks exposed pathways.
    bool isConnected() const { return sockFd_ >= 0; } // Active link structural data values check tracking evaluations routines.

    bool readRegisters(uint8_t functionCode, uint16_t startReg, uint16_t quantity, std::vector<uint8_t>& out) {
        if (!isConnected()) return false; // Terminate execution workflows instantly if gateway state tracks offline.
        // Request frame transmission step combined logical evaluation matching response checks tracks.
        bool ok = sendRequest(transactionId_++, functionCode, startReg, quantity) && receiveResponse(out);
        if (!ok) closeSocket();    // Kill network interface structures safely if data frame transmission pipelines fail.
        return ok;                 // Return complete interaction state results flag parameters validation check.
    }

    void setSlaveId(uint8_t slaveId) { slaveId_ = slaveId; } // Dynamic route overrides update methods exposed handler tools.

private:
    std::string ip_;               // Destination industrial instrument hardware IP routing parameters storage strings.
    int port_;                     // Variable tracking destination communication interface active connection port parameters.
    uint8_t slaveId_;              // Modbus Unit Station Identification node index code number tracking assignment fields.
    int sockFd_ = -1;              // Operating system network level streaming context file connection pointer value references.
    uint16_t transactionId_ = 1;   // Sequence verification tracking key incremented across consecutive pipeline operations.

    void closeSocket() {
        if (sockFd_ >= 0) {        // Identify if tracking context index parameters maps to active kernel components handles.
            close(sockFd_);        // Terminate channel operations safely inside system background pipeline environments layers.
            sockFd_ = -1;          // Set system tracking reference variables attributes values back to unassigned standard state markers.
        }
    }

    bool sendRequest(uint16_t transactionId, uint8_t functionCode, uint16_t startReg, uint16_t quantity) {
        std::vector<uint8_t> request(12); // Initialize raw Modbus TCP Application Protocol ADU array structure context framework across 12 explicit bytes.
        request[0] = (transactionId >> 8) & 0xFF;  // Transaction key identification sequence assignment metrics upper bytes.
        request[1] = transactionId & 0xFF;         // Sequence tracking index lower byte configuration processing lines.
        request[2] = 0x00;         // Protocol identifier high byte field setup data values (Modbus TCP standard constant is 0).
        request[3] = 0x00;         // Protocol tracking code signature assignment value properties low byte position field.
        request[4] = 0x00;         // Payload byte volume message formatting definitions configuration boundaries upper byte index.
        request[5] = 0x06;         // Remaining tracking characters byte count values (Unit ID block to data end criteria = 6 bytes).
        request[6] = slaveId_;     // Load destination instrument sub-station terminal node routing path variables setup code.
        request[7] = functionCode; // Injects specific core physical register reading process operational logic commands tokens.
        request[8] = (startReg >> 8) & 0xFF;       // Extract starting address reference block parameters layout configurations (Upper byte).
        request[9] = startReg & 0xFF;              // Map starting target index reference layout location variables choices (Lower byte).
        request[10] = (quantity >> 8) & 0xFF;     // Request target acquisition density volume parameters range metrics (High byte data).
        request[11] = quantity & 0xFF;             // Finalize register volume bounds variables attributes configurations settings criteria (Low byte).

        // Send raw payload packet bytes structure data properties over network interfaces streaming socket connection pipelines.
        ssize_t sent = send(sockFd_, request.data(), request.size(), 0);
        return sent == static_cast<ssize_t>(request.size()); // Verify length of transmitted payload matches standard structure constraints.
    }

    bool receiveResponse(std::vector<uint8_t>& out) {
        uint8_t header_buf[9];     // Allocate local byte cache space to intercept inbound streaming response parameters metadata header.
        // Block loop thread tracking till complete 9-byte sequence is read from streaming connection pipeline layers channels.
        ssize_t received = recv(sockFd_, header_buf, sizeof(header_buf), MSG_WAITALL);
        if (received < static_cast<ssize_t>(sizeof(header_buf))) return false; // Terminate execution steps tracking if return frame structure is truncated.
        if (header_buf[7] & 0x80) return false; // Abort operational flow tracking if function code contains error flag bits response signals.

        uint8_t byteCount = header_buf[8]; // Extract payload content length mapping criteria from 9th byte index field location parameters.
        out.resize(byteCount);     // Reallocate canvas target output vector collection sizing properties limits to match incoming byte lengths.
        // Pull final data stream segment values directly out of communication pipe links straight into destination vectors array.
        received = recv(sockFd_, out.data(), byteCount, MSG_WAITALL);
        return received == static_cast<ssize_t>(byteCount); // Confirm complete response message transmission checks parameters verification success.
    }
};




struct ReadingResult {
    std::string name;              // Sensor tracking identification metadata string name label assignments.
    bool valid = false;            // Accuracy checking indicator variable tracking telemetry task results outcomes.
    double value = 0.0;            // Engineering conversion calculated processed float point scalar value tracking parameters.
};

class Device {
public:
    // Device mapping constructor initializing internal parameters link objects patterns references variables models.
    explicit Device(DeviceConfig config)
        : config_(std::move(config)), client_(config_.ip, config_.port, config_.slaveId) {}

    bool ensureConnected() {
        if (client_.isConnected()) return true; // Maintain stream connection channel paths active if state checks evaluate to live.
        return client_.connectDevice(); // Direct dynamic fallback reconnection trigger operation against physical destination targets.
    }

    std::vector<ReadingResult> pollAll(uint8_t overrideSlaveId = 0) {
        if (overrideSlaveId != 0) {
            client_.setSlaveId(overrideSlaveId); // Injects parameter level transient routing node identification modifications.
        } else {
            client_.setSlaveId(config_.slaveId); // Fallback configuration profile reset tracking down back to configuration file specs.
        }

        std::vector<ReadingResult> results; // Initialize dynamic canvas lists array to hold parsed physical tracking outcomes records.
        results.reserve(config_.readings.size()); // Preallocate internal tracking memory arrays density properties to optimize process loop steps.

        for (const auto& readingCfg : config_.readings) { // Execute sequentially systematic parsing loops across every register task profile.
            ReadingResult result;  // Instantiate step level storage layout schema mapping frameworks properties.
            result.name = readingCfg.name; // Apply sensor naming designation property metrics string identifiers labels.

            std::vector<uint8_t> raw; // Prepare empty variable bucket sequence byte vectors to catch incoming digital telemetry lines.
            // Dispatch target acquisition register request commands sequences out across live communication interface channels.
            bool ok = client_.readRegisters(readingCfg.functionCode, readingCfg.reg, 1, raw);

            if (ok && !raw.empty()) { // Verify successful hardware interface responses tracking properties data arrays match.
                result.valid = true; // Turn accuracy status flag properties parameters indicators checking over to true configurations.
                result.value = convert(readingCfg, raw); // Parse binary stream data packages inputs out into standard metric values.
            } else {
                result.valid = false; // Assign error state status markers indicators across failure operational tracking pipelines.
                result.value = 0.0;  // Force telemetry results parameter numeric metrics baseline data down back to absolute zero.
            }
            results.push_back(result); // Enqueue processed record objects structural definitions block into unified application storage collection arrays.
        }
        return results;            // Return collected dataset back up to high level runtime interaction orchestration layers.
    }

    const std::string& name() const { return config_.name; } // Exposure utility method returning string label identity maps.

private:
    DeviceConfig config_;          // Local configuration variables model preserving hardware destination targeting requirements properties data maps.
    ModbusTcpClient client_;       // Network driver interface software communication mechanism attached to specific device layout structures.

    double convert(const ReadingConfig& cfg, const std::vector<uint8_t>& raw) const {
        if (cfg.type == "discrete") return raw.empty() ? 0.0 : (raw[0] & 0x01); // Read precise discrete indicator signal single bit.
        uint16_t combined = (raw[0] << 8) | raw[1]; // Merge split 8-bit tracking characters pairs back into unified 16-bit registers values.
        if (cfg.type == "temperature") return (combined * 0.18) + 32.0; // Apply custom data calibration formulas transformations computations.
        return static_cast<double>(combined); // Cast standard binary tracking metrics out directly into standard float numbers profiles.
    }
};



class DatabaseLogger {
public:
    explicit DatabaseLogger(DatabaseConfig config) : config_(std::move(config)) {} // Cache access authentication credentials properties maps.

    bool ensureConnected() {
        if (connection_ && !connection_->isClosed()) return true; // Bypass connection initialization protocols if active sessions check matches safe states.
        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance(); // Pull active runtime driver instance layout pointers.
            // Open communication links paths connecting host relational database locations parameters.
            connection_.reset(driver->connect(config_.host, config_.user, config_.password));
            connection_->setSchema(config_.schema); // Target destination table logical schema context tracking selections workspace names.
            return true;           // Return true tracking confirmations stating database server is ready for tasks.
        } catch (sql::SQLException&) {
            connection_.reset();   // Safely erase invalid memory reference tracking link pointers to purge corrupt data layouts.
            return false;          // Inform upper processing execution routines database connectivity operations failed.
        }
    }

    void logReadings(const std::string& deviceName, const std::vector<ReadingResult>& readings) {
        if (!ensureConnected()) return; // Kill data persistence pipelines operations immediately if database engines links crash.
        try {
            // Build secure pre-compiled parameters template interface blueprint models to isolate SQL processing logic from data data inputs injection risks.
            std::unique_ptr<sql::PreparedStatement> pstmt(connection_->prepareStatement(
                "INSERT INTO device_readings (device_name, reading_name, value, is_valid, reading_time) VALUES (?, ?, ?, ?, NOW())"));

            for (const auto& r : readings) { // Read sequentially through database entries parameters sheets records map lists.
                pstmt->setString(1, deviceName); // Bind target instrument system profile identity string definitions into execution index position 1.
                pstmt->setString(2, r.name);     // Bind sensor tracking target label name properties variables into placeholder position 2.
                if (r.valid) pstmt->setDouble(3, r.value); // Bind verified functional telemetry data measurement digits value targets into argument index position 3.
                else pstmt->setNull(3, sql::DataType::DOUBLE); // Set query column tracking fields down to Null if hardware data faults occur.
                pstmt->setBoolean(4, r.valid);   // Bind operation data accuracy validity results state checks evaluation markers into index argument position 4.
                pstmt->executeUpdate();         // Execute transactional relational database persistence insertion queries records commands across active frameworks tables.
            }
        } catch (sql::SQLException&) {
            connection_.reset();   // Clear corrupted memory data link references upon facing transactional communication anomalies.
        }
    }

private:
    DatabaseConfig config_;        // Retains persistent workspace parameter definitions sets tracking database authentication.
    std::unique_ptr<sql::Connection> connection_; // Dynamic unique smart manager pointer managing database network access lifecycle contexts fields.
};



int main(int argc, char** argv) {
    // Capture user execution input string vectors from shell arguments or default values setup path routes mappings.
    std::string configPath = (argc > 1) ? argv[1] : "config.json";

    AppConfig config;              // Instantiate global blueprint container layout model mapping properties configuration.
    try {
        config = loadConfig(configPath); // Open and translate structural JSON properties configurations out of text based configuration file.
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] Failed to load config: " << e.what() << std::endl; // Broadcast critical startup execution fault alerts onto system terminals.
        return 1;                  // Direct termination routing back to core operating system environments layer exiting loop contexts with error flag 1.
    }

    DatabaseLogger dbLogger(config.database); // Build and instantiate live database background transaction logger pipeline entity.

    std::vector<Device> devices;   // Initialize array allocation vector tracks to handle ongoing monitoring loop cycles variables profiles.
    devices.reserve(config.devices.size()); // Adjust interior tracking map allocations bounds properties ahead to raise step data processing efficiency loops.
    for (const auto& deviceConfig : config.devices) {
        devices.emplace_back(deviceConfig); // Construct and instantiate physical machine automation instrumentation monitoring entities arrays blocks inline.
    }

    httplib::Server svr;           // Instantiate core web api server connection endpoint management router service instance.

    std::cout << "Web Gateway initialized. Server running on http://0.0.0.0:8080" << std::endl; // Log network accessibility status indicators parameters.

    // Map HTTP GET connection request routing logic against explicit gateway path paths selectors.
    svr.Get("/get_reading", [&](const httplib::Request& req, httplib::Response& res) {
        
        // 1. Extract query strings from Chrome Browser URL
        std::string targetDevice = req.get_param_value("device"); // Extract device identification target strings properties from parameter query blocks.
        std::string slaveIdStr = req.get_param_value("slave_id"); // Extract temporary sub-station endpoint path routing manipulation parameter characters strings.

        if (targetDevice.empty()) { // Verify inbound connection routing signals contain required query targets parameters filters.
            res.status = 400;      // Route network error tracking outputs codes back to Bad Request configuration status code 400.
            res.set_content("{\"error\":\"Missing query parameter 'device'\"}", "application/json"); // Pack descriptive error feedback objects strings payload.
            return;                // Exit active processing execution route loops scopes instantly.
        }

        uint8_t reqSlaveId = 0;    // Standard baseline override parameter reset tracking definitions assignment initialization variables.
        if (!slaveIdStr.empty()) {
            reqSlaveId = static_cast<uint8_t>(std::stoi(slaveIdStr)); // Convert user requested parameter numeric strings safely over to basic integers bytes data.
        }

        // 2. Locate targeted device object matrix inside the vector array
        Device* selectedDevice = nullptr; // Initialize target machine interface destination reference mapping layout pointers back to null values trackers.
        for (auto& device : devices) { // Traverse systematically across every pre-instantiated instrument tracking instance saved inside config arrays.
            if (device.name() == targetDevice) { // Compare if structural configuration machine label matches URL query target parameters properties string.
                selectedDevice = &device; // Transfer target destination memory allocation index address pointers tracking matching hardware setups.
                break;             // Break parsing sequence operations immediately upon matching appropriate instrumentation criteria targets.
            }
        }

        if (!selectedDevice) {     // Confirm valid equipment data sheets layout objects references were successfully derived out of selection loops pools.
            res.status = 404;      // Set target network response parameter metrics codes back to Not Found error status code 404.
            res.set_content("{\"error\":\"Device not found in configuration\"}", "application/json"); // Return JSON validation details error notifications payload.
            return;                // Terminate logic tracking route execution steps.
        }

        // 3. Drive hardware execution pipelines safe trigger
        if (!selectedDevice->ensureConnected()) { // Inspect if physical interface network link pathways evaluating parameters check tracks to active healthy connections.
            res.status = 503;      // Force target response configurations codes back to Service Unavailable server failure error status code 503.
            res.set_content("{\"error\":\"Could not connect to target device hardware pipe\"}", "application/json"); // Dispatch channel disruption messages logs.
            return;                // Terminate logic tracking execution steps immediately.
        }

        // 4. Fetch readings and translate hex data parameters
        std::vector<ReadingResult> finalReadings = selectedDevice->pollAll(reqSlaveId); // Execute register data acquisition routines pulling telemetry strings across network fields.

        // 5. Build dynamic text output data block format for Browser display
        nlohmann::json json_response; // Instantiate empty target JSON structural data dictionary schema to frame metric telemetry outputs records parameters.
        json_response["device_name"] = selectedDevice->name(); // Inject instrument identity definition string criteria straight into target JSON dictionary entries tracks.
        // Record precise execution date times processing points metrics into target frames schemas via standard system clock routines.
        json_response["timestamp"] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        
        nlohmann::json readings_arr = nlohmann::json::array(); // Create core array block structures inside destination JSON environment layers arrays configurations.
        for (const auto& r : finalReadings) { // Parse processed industrial data updates lines details sequentially record per record.
            nlohmann::json single_r; // Establish local micro schema block structures to hold individual sensor properties records metrics values.
            single_r["sensor_name"] = r.name; // Assign specific physical sensor node parameter designations label strings keys.
            single_r["is_valid"] = r.valid;  // Embed precision evaluation checking accuracy status configurations flag bits parameters.
            if (r.valid) {
                single_r["value"] = r.value; // Store calculated real world values updates directly onto matching json response array node.
            } else {
                single_r["value"] = nullptr; // Force JSON null tracking tokens values entries into sensor parameters keys facing transmission data errors.
            }
            readings_arr.push_back(single_r); // Append processed minor structural data rows back up into root system collection array blocks templates.
        }
        json_response["readings"] = readings_arr; // Embed completed structured multi sensor telemetry datasets back onto master dictionary tracking output frames.

        // Auto background logging execution trigger to database
        if (!finalReadings.empty()) {
            dbLogger.logReadings(selectedDevice->name(), finalReadings); // Transfer collected data points changes straight over to backend storage tables workflows.
        }

        // 6. Return response to Chrome Client Engine interface wrapper
        res.status = 200;          // Set system operation interaction success confirmation indicators metrics up to success standard status code 200.
        res.set_content(json_response.dump(4), "application/json"); // Process target JSON configuration dictionary maps directly out into standard indented string parameters payload streams.
    });

    // Server keeps listening on port 8080 continuously online
    svr.listen("0.0.0.0", 8080);   // Bind web gateway server connection handling execution loops across all physical network cards targeting interface port 8080.
    return 0;                      // Gracefully close main execution framework workflows returning absolute success index status code 0 back up to system shell layers.
}