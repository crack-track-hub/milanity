#include <iostream>    // Standard input/output streams (std::cout, std::cerr)
#include <vector>      // Dynamic arrays (std::vector) to manage raw bytes
#include <string>      // String manipulation handling (std::string)
#include <chrono>      // Modern C++ timing libraries for accurate 1-minute tracking
#include <thread>      // Threading library to allow the program to sleep/pause
#include <iomanip>     // Stream manipulators to format decimal points and times
#include <cstring>     // Legacy C-string utilities (memset, etc.)
#include <sys/socket.h>// Core Linux/POSIX socket APIs for TCP/IP communication
#include <arpa/inet.h> // Utilities to convert IP addresses from text to binary format
#include <unistd.h>    // Standard symbolic constants and system calls (close())

// MySQL Connector/C++ Headers
#include <mysql_driver.h>           // Core MySQL driver class to initialize the connector interface
#include <mysql_connection.h>       // Core connection handler interface to connect to MySQL server
#include <cppconn/prepared_statement.h> // Interface to execute parameterized safe SQL queries

// Configurations
const std::string SERVER_IP = "192.168.0.136"; // Image-il ulla IP
const int PORT = 503;   // 503
const uint8_t SLAVE_ID = 1;                     // Also known as Unit ID; identifies the specific device

// MySQL Credentials - Change these to match your setup
const std::string DB_HOST = "tcp://127.0.0.1:3306"; // Protocol and address for local MySQL instance
const std::string DB_USER = "root";                 // Database administrative login account username
const std::string DB_PASS = "milanity";// Database administrative password credential
const std::string DB_NAME = "industrial_iot";       // Target schema database container name

// --- Raw Modbus Packet Builders & Parsers ---

// Function to establish a raw TCP connection with the target Modbus device
int connectTCP(const std::string& ip, int port) {
    // Create an IPv4 (AF_INET) stream socket using TCP protocol (SOCK_STREAM)
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1; // Return error code -1 if socket creation fails

    // Configure a struct to set a safety network timeout limit
    struct timeval timeout;
    timeout.tv_sec = 2;       // Set socket receive timeout to 2 seconds max
    timeout.tv_usec = 0;      // 0 microseconds
    // Apply the timeout setting to the socket to prevent the app from freezing indefinitely
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Define the network address configuration structural parameters
    sockaddr_in server_addr{};         // Initialize an empty address structure
    server_addr.sin_family = AF_INET;  // Explicitly designate IPv4 protocol family
    server_addr.sin_port = htons(port);// Convert the port number from host-byte order to network-byte order

    // Convert the human-readable IP string into the binary representation required by sockets
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        close(sock); // Free up the socket resource if conversion fails
        return -1;   // Return error code
    }

    // Attempt to establish a live connection to the remote server address
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock); // Close socket handle if network connection fails
        return -1;   // Return error code
    }
    return sock; // Return the valid network socket descriptor on success
}

// Function to build a raw Modbus application frame, send it, and receive the raw payload response
bool transactionModbus(int sock, uint16_t transaction_id, uint8_t function_code, uint16_t start_reg, uint16_t quantity, std::vector<uint8_t>& response_out) {
    std::vector<uint8_t> request(12); // Pre-allocate a buffer of 12 bytes for a standard Modbus TCP request
    
    // 1. MBAP Header Construction
    request[0] = (transaction_id >> 8) & 0xFF; // Higher byte of the unique sequential Transaction Identifier
    request[1] = transaction_id & 0xFF;        // Lower byte of the unique sequential Transaction Identifier
    request[2] = 0x00;                         // Protocol Identifier High byte (Always 0x00 for Modbus TCP)
    request[3] = 0x00;                         // Protocol Identifier Low byte (Always 0x00 for Modbus TCP)
    request[4] = 0x00;                         // Length tracker High byte (Always 0x00 since packet size is < 256)
    request[5] = 0x06;                         // Length tracker Low byte (6 remaining bytes follow this position)
    request[6] = SLAVE_ID;                     // Identification code pointing to specific destination device
    
    // 2. PDU (Protocol Data Unit) Construction
    request[7] = function_code;                // Determines action: Read Coils, Input Registers, or Holding Registers
    request[8] = (start_reg >> 8) & 0xFF;      // Target Modbus data offset starting reference Address - High byte
    request[9] = start_reg & 0xFF;             // Target Modbus data offset starting reference Address - Low byte
    request[10] = (quantity >> 8) & 0xFF;      // Amount of continuous register targets to pull - High byte
    request[11] = quantity & 0xFF;             // Amount of continuous register targets to pull - Low byte

    // Transmit the fully constructed 12-byte raw network buffer through our active socket
    if (send(sock, request.data(), request.size(), 0) < 0) return false; // Fail out if connection dropped during transmission

    // Prepare a temporary frame buffer to accept incoming Modbus TCP reply header (9 bytes)
    uint8_t header_buf[9];
    // Block thread execution until precisely 9 bytes are fetched from the socket stream buffer
    ssize_t bytes_received = recv(sock, header_buf, 9, MSG_WAITALL);
    if (bytes_received < 9) return false; // Handle error if connection breaks short of a valid full header

    // Inspect MSB of Function Code field inside header; if set, it signals a device error state
    if (header_buf[7] & 0x80) return false; // Terminate transaction cycle as a failure if exception flag is active

    // Extract the byte payload length counter passed directly at the end of the response header
    uint8_t byte_count = header_buf[8];
    response_out.resize(byte_count); // Size out the output receiver vector exactly to match incoming data size

    // Block thread execution again to retrieve the remaining trailing register bytes
    bytes_received = recv(sock, response_out.data(), byte_count, MSG_WAITALL);
    return (bytes_received == byte_count); // Return true only if all expected registry bytes arrived safely
}

// Extracted method to perform register pulls, process calculations, and convert them to readable temperature outputs
double getTemperature(int sock, uint16_t reg, uint8_t fc, uint16_t &tid) {
    std::vector<uint8_t> reply; // Dynamic array to capture raw response bytes
    // Query register over socket utilizing the custom transaction function (pulling exactly 1 16-bit register)
    if (transactionModbus(sock, tid++, fc, reg, 1, reply) && reply.size() >= 2) {
        // Splice the two distinct 8-bit response indexes together into a single 16-bit integer (Big-Endian handling)
        uint16_t raw = (reply[0] << 8) | reply[1];
        return (raw * 0.18) + 32.0; // Math transformation tracking logic explicitly outlined inside datasheet
    }
    return -999.0; // Out-of-bounds error state output marker used if transaction fails
}

// Extracted function to perform status queries on Discrete binary elements
int getDiscreteInput(int sock, uint16_t reg, uint16_t &tid) {
    std::vector<uint8_t> reply; // Dynamic array to capture raw response bytes
    // Query binary data status block utilizing Function Code 0x02
    if (transactionModbus(sock, tid++, 0x02, reg, 1, reply) && reply.size() >= 1) {
        return (reply[0] & 0x01); // Mask out and read only the lowest-order relevant status bit from payload byte
    }
    return -1; // Return negative identifier as error indicator
}

// Extracted function to query a raw integer parameter value
int getFiringRate(int sock, uint16_t reg, uint16_t &tid) {
    std::vector<uint8_t> reply; // Dynamic array to capture raw response bytes
    // Query register utilizing standard function code 0x04 mapped to Input Registers
    if (transactionModbus(sock, tid++, 0x04, reg, 1, reply) && reply.size() >= 2) {
        return (reply[0] << 8) | reply[1]; // Shift high byte and combine with low byte to output raw 16-bit value
    }
    return -1; // Return negative identifier as error indicator
}

// --- MySQL Data Insertion Handler ---
void saveToDatabase(double supply_t, int heat_status, double outlet_t, double inlet_t, int fire_rate) {
    try {
        // Initialize the MySQL Driver instance singleton object
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
        
        // Establish network connection socket pipeline to the running database instance
        std::unique_ptr<sql::Connection> con(driver->connect(DB_HOST, DB_USER, DB_PASS));
        
        // Connect to and activate the specific schema target namespace
        con->setSchema(DB_NAME);

        // Define a parameterized prepared statement blueprint to manage clean value formatting safely
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "INSERT INTO device_logs (supply_temperature, heating_status, outlet_temperature, inlet_temperature, firing_rate) VALUES (?, ?, ?, ?, ?)"
        ));

        // Bind data parameters (Handle potential sensor errors gracefully by inserting NULL data records)
        if (supply_t != -999.0) pstmt->setDouble(1, supply_t); else pstmt->setNull(1, sql::DataType::DECIMAL); // Index 1: Supply Temp
        if (heat_status != -1)  pstmt->setInt(2, heat_status); else pstmt->setNull(2, sql::DataType::TINYINT); // Index 2: Heating Status
        if (outlet_t != -999.0) pstmt->setDouble(3, outlet_t); else pstmt->setNull(3, sql::DataType::DECIMAL); // Index 3: Outlet Temp
        if (inlet_t != -999.0)  pstmt->setDouble(4, inlet_t);  else pstmt->setNull(4, sql::DataType::DECIMAL); // Index 4: Inlet Temp
        if (fire_rate != -1)    pstmt->setInt(5, fire_rate);   else pstmt->setNull(5, sql::DataType::INTEGER); // Index 5: Firing Rate

        // Execute finalized SQL query transaction payload onto the remote server host database engines
        pstmt->executeUpdate();
        std::cout << "[SUCCESS] Telemetry stored to MySQL successfully." << std::endl; // Confirm completion visually via console stream

    } catch (sql::SQLException &e) {
        // Catch and isolate any processing library runtime query execution failure exceptions
        std::cerr << "[DATABASE ERROR] MySQL Error Code " << e.getErrorCode() << ": " << e.what() << std::endl;
    }
}

// --- Main Execution Loop ---

int main() {
    uint16_t transaction_counter = 1; // Tracking counter initializing transaction packets
    std::cout << "Starting Modbus TCP Logger to MySQL Application..." << std::endl; // Welcome bootup flag message text

    // Enter permanent cyclic execution loop tracking and capturing system logs continuously
    while (true) {
        // Calculate timestamp for the precise start boundary of the next tracking cycle (current time + 60s)
        auto next_run = std::chrono::steady_clock::now() + std::chrono::seconds(15);

        // Attempt a live socket connection to the target endpoint device
        int sock = connectTCP(SERVER_IP, PORT);
        if (sock < 0) {
            // Print error stream alert notice details if network endpoint cannot be bound or contacted
            std::cerr << "[ERROR] Could not connect to Modbus Device. Retrying in 1 minute..." << std::endl;
        } else {
            // Read target records sequentially processing them by Function Code and targeted configuration addresses
            double supply_temp  = getTemperature(sock, 6, 0x03, transaction_counter); // Pull Reg 6 (Holding Register)
            int heating_status  = getDiscreteInput(sock, 32, transaction_counter);    // Pull Reg 32 (Discrete Input Bit)
            double outlet_temp  = getTemperature(sock, 8, 0x04, transaction_counter); // Pull Reg 8 (Input Register)
            double inlet_temp   = getTemperature(sock, 9, 0x04, transaction_counter); // Pull Reg 9 (Input Register)
            int firing_rate     = getFiringRate(sock, 11, transaction_counter);       // Pull Reg 11 (Input Register)

            close(sock); // Close socket connection channel context immediately when finished gathering data parameters

            // Fetch a precise current clock time snapshot to tag the printed logs
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now); // Convert standard system time object to legacy arithmetic C type
            std::cout << "--- Log entry: " << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << " ---" << std::endl; // Output formatted localized time stamp to terminal screen
            std::cout << "  Supply: " << supply_temp << " F | Heat: " << heating_status << " | Outlet: " << outlet_temp << " F | Inlet: " << inlet_temp << " F | Firing: " << firing_rate << " %" << std::endl; // Visual stdout feedback print log entry string summary

            // Commit all gathered sensor readings directly down into the database backend repository fields
            saveToDatabase(supply_temp, heating_status, outlet_temp, inlet_temp, firing_rate);
            std::cout << std::endl; // Append empty carriage string space split line for clarity reading structures
        }

        std::this_thread::sleep_until(next_run); // Lock execution thread pacing to precise 1 minute intervals securely
    }
    return 0; // Standard main execution return structure closure (unreachable code due to infinite loop structure pattern)
}