#include "Device.h"

#include <iostream>

Device::Device(DeviceConfig config)
    : config_(std::move(config)),
      client_(config_.ip, config_.port, config_.slaveId) {}

bool Device::ensureConnected() {
    if (client_.isConnected()) return true; // already up, reuse it — no reconnect

    if (!client_.connectDevice()) {
        std::cerr << "[" << config_.name << "] connect failed to "
                  << config_.ip << ":" << config_.port << std::endl;
        return false;
    }
    std::cout << "[" << config_.name << "] connected to "
              << config_.ip << ":" << config_.port << std::endl;
    return true;
}

double Device::convert(const ReadingConfig& cfg, const std::vector<uint8_t>& raw) const {
    if (cfg.type == "discrete") {
        return raw.empty() ? 0.0 : (raw[0] & 0x01);
    }

    // temperature / raw both need 2 bytes combined big-endian into 16 bits
    uint16_t combined = (raw[0] << 8) | raw[1];

    if (cfg.type == "temperature") {
        return (combined * 0.18) + 32.0; // per device datasheet scaling
    }
    // "raw" (e.g. firing rate %) - value as-is
    return static_cast<double>(combined);
}

std::vector<ReadingResult> Device::pollAll() {
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
