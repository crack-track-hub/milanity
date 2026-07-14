import socket   # Import Python's built-in networking library for handling TCP/IP connections
import struct   # Import Python's binary packing library to convert data into raw bytes

# Simulate the physical holding registers of an industrial device (like a PLC).
# It's an array of 10 slots, storing 16-bit integers. Address 0=100, Address 1=200, etc.
holding_registers = [100, 200, 300, 0, 0, 0, 0, 0, 0, 0]

def handle_request(data):
    # Step 1: Unpack the 7-byte Modbus Application Protocol (MBAP) Header
    # '>' means Big-Endian (Network Byte Order)
    # 'H' is 2 bytes (unsigned short) for Transaction ID
    # 'H' is 2 bytes for Protocol ID
    # 'H' is 2 bytes for Message Length
    # 'B' is 1 byte (unsigned char) for Unit ID
    tid, pid, length, unit_id = struct.unpack('>HHHB', data[:7])
    
    # Step 2: Extract the Function Code (the 8th byte at index 7)
    function_code = data[7]

    # Handle Function Code 0x03: Read Holding Registers
    if function_code == 0x03:
        # Extract the starting register address and the count of registers requested (4 bytes total from index 8 to 12)
        start_addr, count = struct.unpack('>HH', data[8:12])
        
        # Use Python list slicing to pull out the requested register values from our simulated memory list
        regs = holding_registers[start_addr:start_addr+count]
        
        # Calculate how many total data bytes will be sent back (each 16-bit register is 2 bytes)
        byte_count = count * 2
        
        # Start building the Protocol Data Unit (PDU) response header
        # Includes the function code (1 byte) and the byte count (1 byte)
        pdu = struct.pack('>BB', function_code, byte_count)
        
        # Loop through each extracted register value and pack it as a 2-byte Big-Endian short ('>H')
        for r in regs:
            pdu += struct.pack('>H', r)

    # Handle Function Code 0x06: Write Single Register
    elif function_code == 0x06:
        # Extract the specific register address and the new value to write (4 bytes total from index 8 to 12)
        addr, value = struct.unpack('>HH', data[8:12])
        
        # Modify our simulated memory list, overwriting the old value with the client's new value
        holding_registers[addr] = value
        
        # A successful Modbus write echoes back the exact same command received as an acknowledgment
        pdu = struct.pack('>BHH', function_code, addr, value)

    # Handle Unsupported/Invalid Function Codes
    else:
        # Create an Exception response: set the high bit of function code (bitwise OR with 0x80)
        # 0x01 is the Modbus standard exception code for "Illegal Function"
        pdu = struct.pack('>BB', function_code | 0x80, 0x01)

    # Calculate the full response length: size of the PDU + 1 byte for the Unit ID
    resp_length = len(pdu) + 1
    
    # Pack the final MBAP header using the client's original Transaction ID (tid) and Protocol ID (pid)
    header = struct.pack('>HHHB', tid, pid, resp_length, unit_id)
    
    # Concatenate the 7-byte header with the variable-length PDU and return the complete byte packet
    return header + pdu


def start_server(host="localhost",port=5000):
    # Initialize a new IPv4 (AF_INET), TCP (SOCK_STREAM) socket manager
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        # Allow the operating system to immediately reuse the port/address after restarting the script
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # Bind the network socket to the specified IP address and network port
        server.bind((host, port))
        
        # Start listening for incoming network requests (queue size limit of 1 connection)
        server.listen(1)
        print(f"Modbus TCP server listening on {host}:{port}")

        # Infinite loop keeping the network server running continuously
        while True:
            # Block execution and wait here until a client attempts to connect; returns network connection and client address
            conn, addr = server.accept()
            
            # Context manager automatically handles opening and cleaning up the individual client connection
            with conn:
                print("Connected by", addr)
                
                # Dynamic loop to read streaming data over the established connection
                while True:
                    # Receive raw bytes from the network socket buffer up to a limit of 1024 bytes
                    data = conn.recv(1024)
                    
                    # If the data packet is empty, the client has closed the connection; break out of the reading loop
                    if not data:
                        break
                        
                    # Process the incoming Modbus packet and construct the binary reply
                    response = handle_request(data)
                    
                    # Send the finalized raw binary response back across the network to the client
                    conn.sendall(response)


if __name__ == "__main__":
    # Standard entry point to execute the script locally on standard Modbus port 502
    start_server(port=5000)