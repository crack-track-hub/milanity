#pragma once
// ModbusTcpClient.h
// Owns exactly one persistent TCP socket to a single Modbus TCP slave.
// The socket is opened once via connectDevice() and then reused for every
// subsequent transaction — there is no reconnect-per-request like the
// original script did. If a send/recv fails, the socket is closed and
// isConnected() goes false, which is the caller's cue to call
// connectDevice() again on the next cycle (see Device::ensureConnected).

#include <cstdint>
#include <string>
#include <vector>

class ModbusTcpClient {
public:
    ModbusTcpClient(std::string ip, int port, uint8_t slaveId);
    ~ModbusTcpClient();

    // Non-copyable (owns a raw socket fd); movable if you ever need it.
    ModbusTcpClient(const ModbusTcpClient&) = delete;
    ModbusTcpClient& operator=(const ModbusTcpClient&) = delete;

    // Opens the TCP connection. Safe to call again after a drop; it will
    // just re-dial. Returns false (and leaves isConnected() == false) on
    // any failure so the caller can retry later without crashing.
    bool connectDevice();

    void disconnect();

    bool isConnected() const { return sockFd_ >= 0; }

    // Reads `quantity` 16-bit registers (or coil bits, for fc 0x01/0x02)
    // starting at `startReg` using the already-open connection.
    // Returns false on any I/O error and closes the socket internally.
    bool readRegisters(uint8_t functionCode, uint16_t startReg, uint16_t quantity,
                        std::vector<uint8_t>& out);

    const std::string& ip() const { return ip_; }
    int port() const { return port_; }

private:
    std::string ip_;
    int port_;
    uint8_t slaveId_;
    int sockFd_ = -1;
    uint16_t transactionId_ = 1;

    bool sendRequest(uint16_t transactionId, uint8_t functionCode, uint16_t startReg, uint16_t quantity);
    bool receiveResponse(std::vector<uint8_t>& out);
    void closeSocket();
};
