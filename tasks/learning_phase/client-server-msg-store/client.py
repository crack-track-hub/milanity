import socket
from db_helper import store_message

# NOTE: 'messages' table is already created manually in MySQL
# using SQL (CREATE TABLE ...), so we don't call create_table_if_not_exists() here.

# Create socket
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect to server
client.connect(("localhost", 5000))

print("Connected to server")

while True:
    # Send message
    message = input("Client: ")
    client.send(message.encode())
    store_message("Client", message)      # <-- store client's message in MySQL

    if message.lower() == "exit":
        break

    # Receive reply
    reply = client.recv(1024).decode()
    print("Server:", reply)
    store_message("Server", reply)        # <-- store server's reply in MySQL

    if reply.lower() == "exit":
        break

client.close()