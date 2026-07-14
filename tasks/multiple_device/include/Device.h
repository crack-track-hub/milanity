#pragma once
// Device.h
// Represents one physical Modbus device: its identity/config (name, ip,
// register map) plus the persistent ModbusTcpClient connection used to
// talk to it. main.cpp just loops over a vector<Device> — adding a device
// means adding an entry to config.json, not writing new code.

#include <string>
#include <vector>

#include "Config.h"
#include "ModbusTcpClient.h"

struct ReadingResult {
    std::string name;
    bool valid = false;
    double value = 0.0;
};

class Device {
public:
    explicit Device(DeviceConfig config);

    // Connects if not already connected. Cheap no-op if the persistent
    // connection is already up, so it's safe to call every poll cycle.
    bool ensureConnected();

    bool isConnected() const { return client_.isConnected(); }

    // Reads every configured register over the existing open connection.
    // Does NOT reconnect internally — call ensureConnected() first.
    std::vector<ReadingResult> pollAll();

    const std::string& name() const { return config_.name; }

private:
    DeviceConfig config_;
    ModbusTcpClient client_;

    double convert(const ReadingConfig& cfg, const std::vector<uint8_t>& raw) const;
};
