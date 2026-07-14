# Modbus TCP Logger (OOP, multi-device, JSON-configured)

## What changed vs. the original script

| Old | New |
|---|---|
| One giant `main()`, free functions | `ModbusTcpClient`, `Device`, `DatabaseLogger` classes |
| One hardcoded device (IP/port in a constant) | Any number of devices, defined in `config.json` |
| Opens a fresh TCP socket **every 15s cycle** | Socket opened **once** per device, reused every cycle; reconnects only if a transaction actually fails |
| Registers hardcoded (`getTemperature(sock, 6, 0x03, ...)`) | Registers described declaratively in `config.json` (name, function code, address, type) |
| Fixed 5-column MySQL row | Generic `device_readings` table (`device_name`, `reading_name`, `value`) so devices/readings can differ in count |

## Files

```
modbus_logger/
├── config.json              # devices + registers + DB creds + poll interval
├── schema.sql                # MySQL table (run once)
├── CMakeLists.txt
├── include/
│   ├── Config.h              # AppConfig/DeviceConfig/ReadingConfig + JSON parsing
│   ├── ModbusTcpClient.h      # persistent raw Modbus TCP socket
│   ├── Device.h               # one physical device + its register map
│   └── DatabaseLogger.h       # persistent MySQL connection wrapper
└── src/
    ├── ModbusTcpClient.cpp
    ├── Device.cpp
    ├── DatabaseLogger.cpp
    └── main.cpp               # wiring + poll loop
```

## How the "connect once, keep listening" part works

`ModbusTcpClient::connectDevice()` is only called:
1. Once at startup for each device, and
2. Again later **only if** `isConnected()` is false — which only happens after
   a `send()`/`recv()` failure closes the socket internally.

`Device::pollAll()` never reconnects — it just reuses whatever socket is
already open. So the loop in `main.cpp` is really:

```cpp
while (true) {
    for (auto& device : devices) {
        if (!device.ensureConnected()) continue;  // no-op if already up
        auto readings = device.pollAll();          // reuses open socket
        dbLogger.logReadings(device.name(), readings);
    }
    sleep_until(next_run);
}
```

Note: real Modbus TCP is still request/response (there's no server-push
"subscribe" mode in the protocol itself), so "continuous listening" here
means *keep the same connection open and poll it repeatedly* rather than
opening/closing a new TCP handshake every single time — which is what was
eating time and file descriptors in the original version.

## Adding a 3rd device (or changing registers)

Just edit `config.json` — no rebuild of logic needed, only recompilation
(config is read at runtime, so not even that, if you don't rebuild):

```json
{
  "name": "Boiler3",
  "ip": "192.168.0.140",
  "port": 503,
  "slave_id": 1,
  "readings": [
    { "name": "supply_temperature", "function_code": 3, "register": 6, "type": "temperature" }
  ]
}
```

`type` controls how raw bytes are interpreted:
- `"temperature"` → 2 bytes, big-endian, `(raw * 0.18) + 32`
- `"discrete"` → 1 byte, bit 0 only (0/1)
- `"raw"` → 2 bytes, big-endian, no scaling

## Build

Dependencies (Ubuntu/Debian example):

```bash
sudo apt install cmake g++ nlohmann-json3-dev libmysqlcppconn-dev
```

```bash
mkdir build && cd build
cmake ..
make
```

## Run

```bash
# one-time schema setup
mysql -u root -p < ../schema.sql

# run (looks for config.json in the current dir by default)
./modbus_logger ../config.json
```

## Notes / things to double check before production use

- `config.json` currently has the DB password in plaintext, same as the
  original — fine for a lab/test box, but for anything internet-facing
  consider environment variables or a secrets file with restricted
  permissions instead.
- Both devices in `config.json` share the same register map for
  demonstration; give `Boiler2` its real IP and its actual register
  addresses if they differ from `Boiler1`.
- The 2-second socket timeout (`SO_RCVTIMEO`) in `ModbusTcpClient` means a
  genuinely slow (not dead) device could get treated as failed under heavy
  network latency — bump it up if you see spurious reconnects.
