import socket
server=socket.socket()
print("socket connected")
server.bind(
    ("localhost",5000)
)
server.listen(1)
print("waiting for client")
client_socket, address=server.accept()
print("connected", address)
while True:
    message=client_socket.recv(1024)
    print(message.decode())

    reply=input("enter msg")
    client_socket.send(encode())