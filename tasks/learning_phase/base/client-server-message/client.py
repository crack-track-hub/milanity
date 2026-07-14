import socket

# Create socket
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect to server
client.connect(("localhost", 5000))

print("Connected to server")

while True:
    # Send message
    message = input("Client: ")
    client.send(message.encode())

    if message.lower() == "exit":
        break

    # Receive reply
    reply = client.recv(1024).decode()
    print("Server:", reply)

    if reply.lower() == "exit":
        break

client.close()
