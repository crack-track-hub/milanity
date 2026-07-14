import socket   # Import Python's built-in networking library for handling TCP/IP connections
import struct   # Import Python's binary packing library to convert data into raw bytes

def read_holding_registers(ip, port, unit_id, start_addr, count):
    # Initialize tracking variables required by the Modbus TCP standard
    transaction_id = 1  # Unique identifier for this request-reply exchange
    protocol_id = 0     # Modbus TCP standard mandates that this field must always be 0
    function_code = 0x03  # Standard function code instructing the server to "Read Holding Registers"

    # Pack the request Protocol Data Unit (PDU): Function Code (1 byte), Start Address (2 bytes), Count (2 bytes)
    pdu = struct.pack('>BHH', function_code, start_addr, count)
    
    # Calculate message length: size of the PDU string + 1 byte for the trailing Unit ID
    length = len(pdu) + 1  
    
    # Pack the 7-byte MBAP header matching the required industrial Big-Endian layout
    header = struct.pack('>HHHB', transaction_id, protocol_id, length, unit_id)
    
    # Combine the header and PDU together into a single sequential byte stream ready to send
    request = header + pdu

    # Initialize a clean IPv4, TCP socket using a context manager
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(3)         # Fail safe: abort the connection attempt if the target doesn't answer within 3 seconds
        s.connect(("localhost",5000))#onnection to the destination server
        s.sendall(request)      # Flush the complete binary request message across the network link
        response = s.recv(1024) # Capture the server's binary response block into the local buffer variable

    # Isolate the first 7 bytes representing the incoming MBAP header structure
    resp_header = response[:7]
    
    # Unpack the server header to double check tracking values (tid, pid, length, uid)
    tid, pid, length, uid = struct.unpack('>HHHB', resp_header)
    
    # Read the 8th byte (index 7) which indicates whether the operation was a success or failure
    func_code = response[7]

    # If the response code doesn't match our original request code (0x03), an error occurred
    if func_code != 0x03:
        raise Exception(f"Error response, function code: {func_code:#x}")

    # Extract the 9th byte (index 8), which tells us exactly how many data bytes followed
    byte_count = response[8]
    
    # Slice out the precise data segment from the byte array containing the raw register values
    data = response[9:9+byte_count]
    
    # Dynamically build a format string (e.g., '>3H') to unpack the raw bytes back into 16-bit integer values
    values = struct.unpack(f'>{byte_count//2}H', data)
    
    # Return the parsed data integers to the main program loop
    return values


def write_single_register(ip, port, unit_id, addr, value):
    # Set unique tracking fields for this write operation
    transaction_id = 2  # Unique tracking id distinct from the read command
    protocol_id = 0     # Modbus standard identifier field, always 0
    function_code = 0x06  # Standard function code instructing the server to "Write Single Register"

    # Pack the write request PDU: Function Code (1 byte), Target Address (2 bytes), Target Value (2 bytes)
    pdu = struct.pack('>BHH', function_code, addr, value)
    
    # Calculate length header configuration parameter
    length = len(pdu) + 1
    
    # Convert header fields into a 7-byte network data package
    header = struct.pack('>HHHB', transaction_id, protocol_id, length, unit_id)
    
    # Bind the complete command structure together
    request = header + pdu

    # Initialize a standard safe network channel
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(3)         # Enforce an automatic 3-second network loss timeout
        s.connect(("localhost",5000))#ith server machine
        s.sendall(request)      # Transfer raw command byte block to server processing queue
        response = s.recv(1024) # Await and copy server's raw confirmation byte block

    # Verify the received return code at index pos 7
    func_code = response[7]
    
    # If the echoed function code doesn't match 0x06, it means the server denied the write operation
    if func_code != 0x06:
        raise Exception(f"Write failed, function code: {func_code:#x}")
        
    # Return True indicating successful validation and acknowledgment of the memory change
    return True


if __name__ == "__main__":
    # Setup standard operational network loop parameters
    ip = "127.0.0.1"  # Loopback target address (localhost, your own system running the server)
    port = 502        # Standard official industrial registry port reserved for Modbus TCP
    unit_id = 1       # Identifier designating which internal device instance to address

    # Execute a read command targeting the first 3 consecutive registers starting from index zero
    values = read_holding_registers(ip, port, unit_id, start_addr=0, count=3)
    print("Read values:", values) # Standard output display to verify parsing: expected [100, 200, 300]

    # Execute a write command to change register address 3 over to a value of 999
    write_single_register(ip, port, unit_id, addr=3, value=999)
    print("Write done")

    # Re-run a reading routine targeting register index 3 specifically to verify changes
    values = read_holding_registers(ip, port, unit_id, start_addr=3, count=1)
    print("Confirmed:", values) # Should show [999], demonstrating successful read-after-write confirmation