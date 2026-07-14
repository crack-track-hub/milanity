# server side code
import socket
server=socket.socket(AF_INET, socket.SOCK_STREAM)
server.bind(
    ("localhost",5000)
)
print("server connected")
server.listen()
client_socket, address = server.accept()

print("connected", address)

while True:
    message= client_socket.recv(1024)

    print("client:",message.decode())

    reply= input("")
    client_socket.send(
        reply.encode(1024)
    )

# client side
import socket
client=socket.socket()
client.connect(
    ("localhost",5000)
)
while True:
    message= client.recv(1024)

    print("server:",message.decode())

    reply= input("")
    server.send(
        reply.encode(1024)
    )