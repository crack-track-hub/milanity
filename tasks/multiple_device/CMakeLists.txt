cmake_minimum_required(VERSION 3.10)
project(modbus_logger CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(modbus_logger
    src/main.cpp
    src/ModbusTcpClient.cpp
    src/Device.cpp
    src/DatabaseLogger.cpp
)

target_include_directories(modbus_logger PRIVATE include)

target_link_libraries(modbus_logger
    PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    mysqlcppconn
)
