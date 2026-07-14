#include "ModbusTcpClient.h"

#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

ModbusTcpClient::ModbusTcpClient(std::string ip, int port, uint8_t slaveId)
    : ip_(std::move(ip)), port_(port), slaveId_(slaveId) {}

ModbusTcpClient::~ModbusTcpClient() {
    disconnect();
}

void ModbusTcpClient::closeSocket() {
    if (sockFd_ >= 0) {
        close(sockFd_);
        sockFd_ = -1;
    }
}

void ModbusTcpClient::disconnect() {
    closeSocket();
}

bool ModbusTcpClient::connectDevice() {
    // If somehow already open, don't leak a second fd.
    closeSocket();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    // Safety timeout so a dead device can't hang the whole poll loop forever.
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

bool ModbusTcpClient::sendRequest(uint16_t transactionId, uint8_t functionCode,
                                   uint16_t startReg, uint16_t quantity) {
    std::vector<uint8_t> request(12);

    // MBAP header
    request[0] = (transactionId >> 8) & 0xFF;
    request[1] = transactionId & 0xFF;
    request[2] = 0x00;
    request[3] = 0x00;
    request[4] = 0x00;
    request[5] = 0x06;
    request[6] = slaveId_;

    // PDU
    request[7] = functionCode;
    request[8] = (startReg >> 8) & 0xFF;
    request[9] = startReg & 0xFF;
    request[10] = (quantity >> 8) & 0xFF;
    request[11] = quantity & 0xFF;

    ssize_t sent = send(sockFd_, request.data(), request.size(), 0);
    return sent == static_cast<ssize_t>(request.size());
}

bool ModbusTcpClient::receiveResponse(std::vector<uint8_t>& out) {
    uint8_t header_buf[9];
    ssize_t received = recv(sockFd_, header_buf, sizeof(header_buf), MSG_WAITALL);
    if (received < static_cast<ssize_t>(sizeof(header_buf))) return false;

    // High bit of the function code set => device returned an exception response.
    if (header_buf[7] & 0x80) return false;

    uint8_t byteCount = header_buf[8];
    out.resize(byteCount);

    received = recv(sockFd_, out.data(), byteCount, MSG_WAITALL);
    return received == static_cast<ssize_t>(byteCount);
}

bool ModbusTcpClient::readRegisters(uint8_t functionCode, uint16_t startReg,
                                     uint16_t quantity, std::vector<uint8_t>& out) {
    if (!isConnected()) return false;

    bool ok = sendRequest(transactionId_++, functionCode, startReg, quantity) &&
              receiveResponse(out);

    if (!ok) {
        // Any failure on a persistent connection means the link is no longer
        // trustworthy (device rebooted, cable pulled, etc). Close it so the
        // next poll cycle knows to reconnect rather than silently retrying
        // on a half-dead socket.
        closeSocket();
    }
    return ok;
}
