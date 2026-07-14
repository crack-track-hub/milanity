#pragma once
// Config.h
// Plain data structures describing everything loaded from config.json:
// database credentials, poll interval, and the list of Modbus devices with
// their register maps. Keeping this separate from the classes that *use*
// the data (Device, DatabaseLogger) is what lets you add/remove devices or
// registers purely by editing JSON.

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// One register/coil to read off a device, and how to interpret the raw bytes.
struct ReadingConfig {
    std::string name;          // column/label used when logging, e.g. "supply_temperature"
    uint8_t functionCode = 0;  // Modbus function code (0x02 discrete, 0x03 holding, 0x04 input)
    uint16_t reg = 0;          // starting register/coil address
    std::string type;          // "temperature" | "discrete" | "raw" -> controls conversion
};

// Everything needed to talk to one physical Modbus TCP device.
struct DeviceConfig {
    std::string name;                       // friendly identifier, e.g. "Boiler1"
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

// --- nlohmann::json (de)serialization hooks ---
// These let us write `json.get<AppConfig>()` instead of hand-parsing fields.

inline void from_json(const nlohmann::json& j, ReadingConfig& r) {
    j.at("name").get_to(r.name);
    j.at("register").get_to(r.reg);
    j.at("type").get_to(r.type);

    // function_code may be given as either an integer (3) or a string ("0x03")
    if (j.at("function_code").is_string()) {
        r.functionCode = static_cast<uint8_t>(std::stoi(j.at("function_code").get<std::string>(), nullptr, 0));
    } else {
        r.functionCode = j.at("function_code").get<uint8_t>();
    }
}

inline void from_json(const nlohmann::json& j, DeviceConfig& d) {
    j.at("name").get_to(d.name);
    j.at("ip").get_to(d.ip);
    j.at("port").get_to(d.port);
    d.slaveId = j.value("slave_id", 1);
    j.at("readings").get_to(d.readings);
}

inline void from_json(const nlohmann::json& j, DatabaseConfig& db) {
    j.at("host").get_to(db.host);
    j.at("user").get_to(db.user);
    j.at("password").get_to(db.password);
    j.at("schema").get_to(db.schema);
}

inline void from_json(const nlohmann::json& j, AppConfig& cfg) {
    cfg.pollIntervalSeconds = j.value("poll_interval_seconds", 15);
    j.at("database").get_to(cfg.database);
    j.at("devices").get_to(cfg.devices);
}
