import socket
from db_helper import store_message


# Create socket
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind IP and Port
server.bind(("localhost", 5000))

# Listen for incoming connections
server.listen(1)

print("Server is waiting for connection...")

# Accept client connection
client_socket, address = server.accept()

print("Connected to:", address)

while True:
    # Receive message from client
    message = client_socket.recv(1024).decode()

    if message.lower() == "exit":
        print("Client disconnected.")
        store_message("Client", message)  # store the exit message too
        break

    print("Client:", message)
    store_message("Client", message)      # <-- store client's message in MySQL

    # Send reply to client
    reply = input("Server: ")
    client_socket.send(reply.encode())
    store_message("Server", reply)        # <-- store server's reply in MySQL

    if reply.lower() == "exit":
        break

client_socket.close()
server.close()