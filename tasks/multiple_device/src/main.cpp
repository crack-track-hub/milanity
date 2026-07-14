// main.cpp
// Loads config.json, builds one Device per entry (each opening ONE
// persistent Modbus TCP connection), and then loops forever polling every
// device over its already-open socket and logging to MySQL over one
// persistent DB connection. Nothing reconnects unless a link actually drops.

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "Config.h"
#include "DatabaseLogger.h"
#include "Device.h"

namespace {

AppConfig loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + path);
    }
    nlohmann::json j;
    file >> j;
    return j.get<AppConfig>();
}

void printReadings(const std::string& deviceName, const std::vector<ReadingResult>& readings) {
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

} // namespace

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

    // One-time initial connection attempt per device. If a device is down
    // at startup that's fine — ensureConnected() will keep retrying it
    // inside the loop below without affecting the other devices.
    for (auto& device : devices) {
        device.ensureConnected();
    }

    while (true) {
        auto next_run = std::chrono::steady_clock::now() + std::chrono::seconds(config.pollIntervalSeconds);

        for (auto& device : devices) {
            // No-op if already connected — this is what makes the
            // connection persistent instead of reconnect-per-cycle.
            if (!device.ensureConnected()) {
                continue; // still down; try again next cycle
            }

            auto readings = device.pollAll();
            printReadings(device.name(), readings);
            dbLogger.logReadings(device.name(), readings);
        }

        std::cout << std::endl;
        std::this_thread::sleep_until(next_run);
    }

    return 0;
}
